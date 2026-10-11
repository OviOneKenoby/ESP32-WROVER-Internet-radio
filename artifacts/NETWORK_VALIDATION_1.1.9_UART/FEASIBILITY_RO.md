# Observație software fără JTAG — incident OPEN

Echipamentul disponibil este CH340/UART COM5. Protocolul 1.1.8-JTAG nu este
executabil aici. Dovezile 1.1.7 rămân baza investigației, inclusiv intervalul
208720–293034 ms (84,314 s): accepted124, refs/frees124/120 și aceleași patru
pbuf; NO_MEM31→193, failures4 constante, RX701→1041, largest DMA ulterior3700.
Acestea demonstrează persistența lipsei progresului la granițele instrumentate,
nu inventarul poolului, un leak sau blocajul unei cozi private.

## Fezabilitate verificabilă

ELF-ul de referință este cel descărcat pentru9cf48e9022c0, SHA256
22D30785E7C25F3587A04ECFBE73610A438D7D78AB843D1773DE167EBC75D999.
Frameworkul fixat: Arduino2.0.17/IDF4.4.7, package
3.20017.241212+sha.dcc1105b, Xtensa8.4.0, espressif32@7.0.1.

`wifi_os_adapter.h` declară `wifi_osi_funcs_t`, versiunea8/magicDEADBEAF,
și semnăturile `_mutex_unlock`, `_malloc_internal`, `_wifi_malloc`.
`wifi_init_config_t.osi_funcs` este declarat în `esp_wifi.h`.
Nu se ghicește ABI-ul funcțiilor private. În ELF-ul exact:
`esp_wifi_init_internal` transmite pointerul cfg.osi_funcs către
`wifi_osi_funcs_register`; acesta verifică versiunea/magic și stochează
pointerul. `esf_buf_alloc` și allocatorul dinamic folosesc același pointer
înregistrat, nu o copie ascunsă a callbackurilor originale.

Proba copiază cfg și tabela în DRAM cu durată de viață statică, înainte de
inițializare. Înlocuiește numai aceste trei intrări cu funcții de aceeași
semnătură, care apelează originalul exact o dată, cu aceleași argumente,
și păstrează rezultatul. Tabela globală SDK nu este modificată. Configurația
efectivă rămâne dynamic32/cache4/static0/type1/features bit0x2.

`_mutex_unlock` este la offset88, nu `_wifi_int_restore` (offset44).
Sursa esp_adapter.c:389–397 folosește mutexul recursiv FreeRTOS.
Instrumentarea rulează după apelul original de deblocare; niciun lock propriu
nu este ținut peste vreun apel original. Nu există logging, heap queries,
alocări diagnostice, apeluri TX noi sau acces la structuri/cozi private în probe.
Callbackurile și memoria lor sunt IRAM/DRAM; unwind-ul este limitat la12 cadre,
numai în task/cache-on. Contextul nesigur este contorizat ca skipped.

Adresele de retur se compară cu simbol+offset din ELF-ul propriu; nu se
reutilizează adrese absolute1.1.7. Normalizarea echivalentă helperului IDF scade3 pentru
instrucțiunea call; proba adaugă3 pentru comparația adreselor de retur.
Operațiile sunt forțate always_inline: -Os poate emite helperul SDK în flash,
inadecvat înaintea verificării cache-ului. ELF-ul final trebuie să confirme
absența acelui apel din probe. Urmele SW_PC păstrează instrucțiunile call
normalizate, gata pentru addr2line.

| k | Adresa de retur1.1.7 | Punct observabil prin callback declarat |
|---|---|---|
|0|esf_buf_alloc+0x7a,402413ba|Ramura headNULL a listei statice, după incrementarea refuzului|
|1|ieee80211_output_process+0x4f,4023d43b|Intrare scoasă din lista pending|
|2|ieee80211_output_process+0x108,4023d4f4|Conversie NULL reintrodusă la cap, înainte de return0x101|
|3|ieee80211_output_process+0x226,4023d612|Lista pending găsită goală|
|4|esf_buf_alloc_dynamic+0x78,40240c2c|Verificarea cotei a fost trecută|
|5|esf_buf_alloc_dynamic+0x1a8,40240d5c|Ieșire NULL: cotă refuzată sau eșec după allocator|

Callbackurile malloc se observă la esf_buf_alloc_dynamic+0xa8 (40240c5c).
Cadrele exacte output_process+0xb7/+0xa9 identifică traseul conversiei.
Starea pe core/task, resetată la k1, separă k5 fără vizită malloc de k5
după malloc. Unwind incomplet, identitate ambiguă sau schimbare de core produc
`unpaired` ori împiedică identificarea contextului; nu justifică o concluzie
negativă. Pentru traseul tip1 obișnuit, k5 fără malloc este refuz înainte de
heap; atribuirea cotei necesită urmele valide și traseul static verificat.
Ramura default-handler/WAPI sau alt context nu trebuie presupusă tip1 normal.

Pentru NO_MEM, patru sloturi urmăresc invocații TX simultane după taskHandle.
`paired` cere STA, buffer și pbuf nonNULL, un singur k0 în aceeași invocație,
și NO_MEM la întoarcere. Configurația și fluxul verificat alloc_tx_buf selectează
tip9 pentru acest caz. Urma trebuie să confirme alloc_tx_buf/output_do;
overflow/nesting, mai multe hituri sau alte rezultate sunt semnalate și invalidează
atribuirea automată. Asocierea este pe invocație, nu după egalitatea contoarelor.

## Interceptabil / neobservat

Interceptabil sigur în această versiune: TX/ref/free/RX/init deja instrumentate,
plus callbackurile OSI declarate de mai sus. Observăm trasee private prin
punctele lor de apel, fără a le apela ori a le înlocui direct.

Neobservat direct: head/count/total al poolului, descriptorii individuali,
cozile private, toate timer-ele/gate-urile și mecanismele de notificare.
Nu sunt folosite `--wrap` pe funcții private fără prototip, patchuri de cod,
trampoline, modificări de registre sau replay de evenimente interne.
Intrarea recycle nu este interceptată separat: callbackul free deja declarat
măsoară progresul eliberării pbuf, fără a presupune că reprezintă ACK radio.

Nicio vizită k1/k3 după incident, cu control pozitiv și fără skipped, susține
lipsa progresului la aceste puncte; nu exclude blocarea înaintea lor ori alte
retry-uri private. k2 repetat demonstrează conversii NULL repetate, chiar dacă
failed-allocator hook nu mai crește. k5 și malloc pot separa refuzul preheap
de NULL primit efectiv de la allocator. Recuperarea largest DMA nu declanșează
prin ea însăși un retry. Notificarea event5 după admitere și retry-ul selectiv
prin TX-done rămân dependențe statice identificate, nu cauzalitate demonstrată.

## Verificare și perturbări

Build guard verifică SHA256 libpp.a/libnet80211.a. Static assertions verifică
offseturile88/348/364 și mărimea468 a ABI-ului OSI. Verificatorul final compară
instrucțiunile private normalizate cu ELF-ul exact1.1.7 și verifică punctele
de callback, pointerul înregistrat, IRAM și lipsa loggerului/heap queries.
Fragmentele Serial.printf sunt verificate sub64 bytes chiar la UINT32_MAX,
pentru a evita fallback-ul malloc al Arduino Print.printf la afișarea x.
Validarea host nu dovedește acoperirea runtime: controlul pozitiv din protocol
este obligatoriu. Hardware NOT RUN de agent.

Perturbări: DRAM suplimentar pentru tabela/config/contori/opt urme; cost CPU
pentru callbackuri și unwind limitat; relink și layout diferit. Mutexul original
este eliberat înainte de observație, dar poate fi recursiv/alte lockuri pot
exista; niciun lock de diagnostic nu este ținut peste apeluri SDK originale.
UART x ocupă temporar main și poate schimba timingul. Contoarele sunt cumulative,
urmele păstrează primele4/ultimele4 contexte rare; nu există jurnal complet al
fiecărei conversii. k4 primește unwind pentru clasificare, fără stocare de urme.
Timpii/heapul baseline trebuie raportați; non-reproducerea nu este fix.
Encoderul rămâne activ. src/main.cpp și branch main rămân nemodificate.

Incident OPEN. Candidat de diagnostic, fără validare de fix, merge, flash
automat, schimbare TX/cache, gzip sau recuperare automată.

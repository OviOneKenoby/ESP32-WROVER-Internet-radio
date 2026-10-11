# Control pozitiv 1.1.9: discrepanță confirmată în ELF

Incident OPEN. Controlul original NU este PASS. Nu declanșa HTTP cu acest
protocol. Nu s-a executat flash, merge sau test hardware de către agent.

Proveniență: două snapshoturi furnizate de proprietar, uptime 156049 și
201550 ms, la 14:34:53.986 și 14:35:39.486. Intervalul este 45,501 s.
Proprietarul confirmă ping/audio funcționale, fără HTTP. Aceste fragmente nu
înlocuiesc un log serial integral de la boot.

|Contor|B0|B1|Diferență|
|---|---:|---:|---:|
|quota_passed|0|0|0|
|k1 dequeue|67|115|48|
|k3 pending empty|134|230|96|
|k4 quota unlock, toate contextele|870|1302|432|
|SW_MALLOC calls/ok|67/67|115/115|48/48|
|TX accepted/refs/frees|65/65/65|113/113/113|48/48/48|
|RX total|489|649|160|

ready=1; skipped/scope_overflow/unexpected/unpaired=0; HTTP, allocation failures
și NO_MEM=0. Largest DMA 5620 apoi 5108 B. Snapshoturile sunt afișate în timp
ce taskurile continuă; grupurile de contori nu sunt un snapshot atomic comun.

## Verificarea sursei și ELF-ului exact

ELF: firmware-1.1.9-uart-fe228ca737b9.elf, SHA256
2495FC047B66DE639B25440744A0920F5B7FB91984441D1DDDDE5E1EB4A51406.
Sursa la commit fe228ca737b9100cd220da3facb314de192eb060 compară, în ambele
clasificatoare, PC-ul normalizat cu output_process+0xb7 sau +0xa9.

În ELF output_process începe la 0x4023d640. Apelul normal ic_ebuf_alloc este
la 0x4023d6f7 (+0xb7), iar apelul WAPI este la 0x4023d6e9 (+0xa9).
Acestea sunt adrese de instrucțiuni call, după normalizarea care scade 3,
nu adresele de revenire.

În unlock, la 0x40081e5a se încarcă literalul de la 0x400804d0;
valoarea lui este **0x4023d6e2**. Comparația de la 0x40081e5f folosește
această valoare, cu 21 B înaintea apelului normal real. Comparația alternativă
calculează corect baza+0xa9. În observeMalloc, la 0x4008206c–0x40082074,
se calculează baza+183, adică **0x4023d6f7**, iar comparația este la
0x400820a7. Cele două predicate executabile NU sunt echivalente, deși
expresia sursă este aceeași. Nu atribuim încă eroarea unei anumite faze de
compilare/link/relaxare sau unui obiect incremental fără o comparație de build.

k4 identifică corect callbackul _mutex_unlock de la 0x40240e7d, revenire
0x40240e80. Dar filtrul său de context normal output_process este greșit în
acest ELF. De aceea creșterea malloc-urilor recunoscute nu implică creșterea
quota_passed. k4 include și alte contexte, care nu sunt păstrate în urmele
rare; cei 432 de hituri nu pot fi distribuiți retrospectiv pe consumatori.

Există și un bypass static: 0x40240e14 sare la 0x40240e80 dacă limita locală
este zero, fără să apeleze unlock la 0x40240e7d. Nu folosim această posibilitate
ca explicație demonstrată a capturilor: esf_buf_setup, la 0x4020c7e0, setează
limita tip1 din dynamic_tx_buf_num, iar configurația observată raportează 32.
Nu s-a citit valoarea privată runtime. Cache-enabled nu elimină această
inițializare în codul examinat.

## Impact și pas următor

Controlul malloc confirmă observația unor conversii normale care ajung la
allocator și obțin nonNULL. Contorii dequeue/pending-empty și TX/ref/free
arată progres sănătos în acest interval. Aceste observații pozitive rămân utile.
Ele nu validează clasificarea quota/null din unlock și nici ramurile încă
neexecutate de refuz/requeue.

Același filtru din unlock clasifică DynamicNull: preheap_null,
after_malloc_null și unpaired nu au acoperire negativă validată pentru
traseul normal. unpaired=0 nu detectează acest fals negativ: ramura care ar
incrementa contorul este condiționată de recunoașterea contextului. paired
NO_MEM/k0 au alt mecanism (scope pe task), dar nu au fost exercitate aici.
Nu deducem lipsa retry-ului, lipsa refuzului de cotă sau inventarul cache din
zerourile acestui baseline.

Verificatorul anterior controla adresele callbackurilor și instrucțiunile
private, nu operandul final al ambelor predicate de context. Aceasta este o
lacună a verificării agentului. Următoarea etapă trebuie să corecteze generarea
predicatului și să verifice ambele comparații în ELF final, inclusiv literalul
rezolvat, înaintea unui nou protocol/candidat. Nu relaxăm controlul pentru a
declara acest build PASS și nu cerem HTTP. Binarul/ELF-ul istoric rămân intacte.

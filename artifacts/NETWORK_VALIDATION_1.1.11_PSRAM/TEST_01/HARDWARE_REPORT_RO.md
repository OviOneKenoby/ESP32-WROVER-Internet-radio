# 1.1.11 TEST_01 — recuperare TX observată, validare finală încă deschisă

Firmware **1.1.11-psram-validation / 124e84801b06**, sursă executabilă
`124e84801b06f9e07764dabc3985a573dffaff3b`. ELF-ul exact are SHA-256
`c1bc29571b7ac59d12bab006ec0761464cebb4c76e7f14686606c73866f7c209`,
identic cu manifestul candidatului. Analiza folosește fișierele brute inventariate
în `EVIDENCE_SHA256SUMS.txt`. Incidentul rămâne OPEN.

## Verdict

TEST_01 este un **PASS funcțional și de recuperare în fereastra observată**, dar
nu un PASS al criteriului strict „zero eșecuri DMA” și nu validarea finală a
fixului. Politica PSRAM a fost exercitată. Trei conversii Wi-Fi au ajuns la
allocatorul intern și au primit NULL; toate trei au intrat pe ramura de requeue.
Ulterior traseul de conversie, alocarea, eliberarea referințelor și TX au continuat să
progreseze aproximativ123,010s, fără cache `headNULL`, NO_MEM, pbuf reținut,
disconnect sau pierdere observată a serviciului.

Nu există identificator de pbuf în urmele conversiei, deci nu putem demonstra
individual că exact fiecare dintre cele trei obiecte reintroduse a fost cel
convertit ulterior. Putem demonstra recuperarea traseului și golirea ledgerului
la nivelul sistemului, cu refs/frees echilibrate.

## Cronologie verificată

Boot power-on la19:33:09, firmware/ID corecte, PSRAM4192107B și GOT_IP la
uptime12428ms. B0 uptime162949 și B1 uptime212742 au HTTP0, failures0, NO_MEM0,
traseu normal valid și TX/ref/free echilibrate. P0 uptime251388 păstrează aceleași
condiții. Bluetooth/SBC apare playing; proprietarul confirmă audio audibil.

Cererea curl începe la19:38:02.861 și se termină la19:38:03.650. Curl raportează
exit0, HTTP200, 3617B primiți și transfer0,104710s. Fișierul are exact3617B,
egal cu Content-Length3617 și cu expected/sent3617 din WEB_RESPONSE count1.
Snapshotul uptime294812 arată failures0/NO_MEM0 și primele4 alocări PSRAM,
6144B cumulativ. La uptime359564, la64,752s după, TX/ref/free sunt237/237/237,
iar pagina controlată nu a produs incidentul.

Browserul a fost deschis/reîmprospătat intenționat separat. Între19:39:36.221
și19:39:38.882 apar root/status/other/stations, apoi root/status/stations;
`other` corespunde handlerului absent și răspunsului21B. Toate cele8 răspunsuri
cumulative raportează expected=sent, socket_error0, failed0 și overflow0.
Confirmarea proprietarului stabilește recepția/utilizarea paginii și diagnostics;
contoarele firmware singure nu ar fi suficiente pentru recepția browserului.

Cele trei eșecuri au ticks388337,388337,388338, în taskul `wifi`, core0,
size1622, caps0x80c INTERNAL|DMA|8BIT. Apar în fereastra celui de-al doilea root:
WEB_RESPONSE count6 se încheie la19:39:38.589 și trece alloc_fail0/0/3.
Captura uptime405795 este la17,457s după ultimul tick; captura finală
uptime511348 la19:41:41 este la123,010s după el.

Pingul conține377 răspunsuri între19:35:41.088 și19:41:59.549, zero timeout și
zero unreachable. Pauza maximă între răspunsuri este1,126398s și apare înainte
de traficul browserului. În jurul eșecurilor există răspunsuri la19:39:37.518,
38.540,39.563 și continuu după aceea. Proprietarul confirmă că root, diagnostics,
ping și sunetul au rămas funcționale.

## Urme și mecanism observat

ELF-ul exact decodează ALLOC_FAILURE astfel:

`heap_caps_alloc_failed -> heap_caps_malloc -> malloc_internal_wrapper ->
SoftwareTxProbe::internalMalloc -> esf_buf_alloc_dynamic -> esf_buf_alloc ->
ic_ebuf_alloc -> ieee80211_output_process -> ppTask`.

Pentru fiecare eșec există o urmă kind5 (`DynamicNull`) prin
`esf_buf_alloc_dynamic/esf_buf_alloc/ic_ebuf_alloc/ieee80211_output_process/ppTask`
și imediat o urmă kind2 (`RequeueNull`) în `ieee80211_output_process/ppTask`, la
același tick și task. `quota_passed=330`, `preheap_null=0`,
`after_malloc_null=3`, iar SW_MALLOC330/327/3 confirmă trecerea gate-ului,
apelul allocatorului și NULL-ul real, urmate de trei requeue.

De la captura uptime405795 la cea finală:

- k1/conversii și SW_MALLOC cresc330→443; toate cele113 apeluri noi reușesc;
- k2 și k5 rămân3, deci nu apar NULL/requeue noi;
- k3 crește632→858, TX accepted325→438 și refs/frees325/325→438/438;
- RX crește1262→1613; NO_MEM/other_error/paired/unpaired rămân zero;
- nu apare TX_RETAINED_REF, iar DMA free/largest ajunge8204/4852.

Aceasta demonstrează progres TX și recuperare observată după eșecuri. Nu
demonstrează că driverul ar recupera în orice sarcină sau că starea poolului
privat a fost inventariată.

## Politica PSRAM și consumatorii rămași

Prima cerere mută4 alocări/6144B. Browserul ridică totalul la22/33792B, fără
fallback; diferența este18 alocări/27648B. Acesta este efectul direct demonstrat
al politicii. La final valoarea rămâne22 deoarece contorul este cumulativ, nu
un inventar al blocurilor încă alocate.

Rămân active în memoria internă: apelurile `mem_malloc` sub512B (contorul
`original` continuă), `mem_calloc` neinterceptat, alocările globale ale
WebServer/Arduino și ale aplicației, plus Wi-Fi/Bluetooth/display și conversiile
DMA obligatoriu interne. În frameworkul fixat, MEMP_MEM_MALLOC=1 face multe
obiecte lwIP să treacă tot prin mem_malloc, dar pragul și mărimile individuale
nu sunt înregistrate. Logurile nu atribuie cele trei lipsuri tranzitorii unuia
dintre acești consumatori și nu demonstrează că pragul512 este cauza rămasă.

Prin urmare nu coborâm pragul și nu interceptăm `mem_calloc` acum. Cele trei
NULL arată că 1.1.11 nu elimină complet presiunea internă, dar rezultatul nou și
discriminatoriu este recuperarea fără tranziția distructivă din1.1.10.

## Comparație și pas următor

În1.1.10, cinci eșecuri au lăsat patru pbuf reținute; headNULL/paired și NO_MEM
au crescut, iar accepted/refs/frees și conversiile s-au oprit persistent. În
1.1.11, trei eșecuri au produs trei requeue, apoi conversiile și frees au reluat
progresul; headNULL/NO_MEM/retained au rămas zero și serviciile au continuat.

Recomandarea este validarea susținută a **aceluiași binar**, fără reflash dacă
identitatea și bootul sunt încă verificabile. O schimbare suplimentară înainte
de această probă ar pierde comparația directă și nu are încă un consumator
atribuit. Protocolul complet este `../PROTOCOL_SUSTAINED_RO.md`. Un firmware nou
devine justificat dacă testul reproduce indisponibilitatea sau arată eșecuri
repetate fără recuperare; următoarea instrumentare va măsura distribuția
dimensiunilor/contextelor lwIP originale înainte de schimbarea unui singur prag.

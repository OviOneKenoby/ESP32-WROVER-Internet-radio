# 1.1.11 SUSTAINED_01 — PASS funcțional în fereastra executată

Firmware **1.1.11-psram-validation / 124e84801b06**, sursă executabilă
`124e84801b06f9e07764dabc3985a573dffaff3b`. Binarul și ELF-ul sunt cele din
manifestul candidatului; fișierele brute sunt inventariate în
`EVIDENCE_SHA256SUMS.txt`.

## Verdict

Runda este un **PASS funcțional** pentru secvența executată. Pagina principală,
diagnostics, pingul și audio Bluetooth au rămas disponibile, iar TX a continuat
după trei eșecuri tranzitorii de alocare DMA. Nu s-a reprodus tranziția din
1.1.10 către `headNULL`, NO_MEM, patru referințe persistente și oprirea TX.

Protocolul susținut de 30 de minute nu a fost parcurs integral: fișierul ping
acoperă aproximativ 3 min 44,566 s și există o singură rundă browser consemnată.
Prin urmare rezultatul nu este o validare de anduranță de 30 de minute. Criteriul
strict „zero eșecuri DMA” nu a trecut. Proprietarul a evaluat comportamentul
funcțional observat drept PASS și a autorizat integrarea, cu aceste limite
menționate explicit.

## Integritatea și identitatea rundei

Monitorul a fost deschis în două fișiere, iar al doilea conține toate cele șapte
snapshoturi utile. `session-notes.log` declară `NEW_BOOT`; snapshoturile au
`reset_reason=power_on`, aceeași identitate firmware/build și uptime monoton:
87967, 248689, 319401, 332448, 354559, 418052 și 427470 ms. Nu se compară
contoare peste booturi diferite.

Configurația rămâne TX dynamic32/cache4, cu instrumentarea software activă.
Baseline-ul are Wi-Fi conectat, Bluetooth SBC playing, zero HTTP, NO_MEM,
allocation failures și erori de instrumentare. La uptime87967 TX/ref/free sunt
43/43/43; la uptime248689 sunt206/206/206.

## Traficul browser și disponibilitatea

Logul conține11 răspunsuri web: trei `root`, trei `status`, patru `stations` și
un `other`. Toate raportează `expected=sent`, `failed=0`, `socket_error=0` și
`json_overflow=0`. Traficul este browser, nu cererea curl din TEST_01.

Până la uptime418052, după primul grup browser, allocation failures rămân zero,
conversiile/SW_MALLOC sunt438/438 reușite, k0/k2/k5 sunt zero,
TX/ref/free sunt436/436/436, iar politica lwIP a direcționat20 alocări/30720 B
în PSRAM fără fallback.

Fișierul ping conține224 răspunsuri între19:56:00.662 și19:59:45.228, fără
timeout sau unreachable. Pauza maximă este1,144310 s și apare înaintea ultimelor
eșecuri. Pingul continuă aproximativ22,5 s după acestea. Logurile seriale arată
în continuare Wi-Fi connected, fără disconnect/lost-IP, și Bluetooth SBC playing.

## Eșecurile tranzitorii și recuperarea TX

Ultimul `root`, la19:59:22, delimitează trei eșecuri de1622 B, caps0x80c, în
taskul `wifi`, core0, la ticks422740–422742. Traseul este același decodat cu
ELF-ul exact în TEST_01:

`heap_caps_alloc_failed -> heap_caps_malloc -> malloc_internal_wrapper ->
SoftwareTxProbe::internalMalloc -> esf_buf_alloc_dynamic -> esf_buf_alloc ->
ic_ebuf_alloc -> ieee80211_output_process -> ppTask`.

Fiecare NULL are clasificare `DynamicNull` (k2), urmată de `RequeueNull` (k5).
Captura finală, uptime427470, este la4,728 s după ultimul eșec și arată:

- `after_malloc_null=3`, SW_MALLOC468/465/3 și k2=k5=3;
- k0/headNULL=0, paired/unpaired=0 și nicio referință reținută;
- k1 ajunge468, cu ultimul tick426758, deci conversiile reușite reapar în cel
  mult4,016 s după incident;
- k3 ajunge902, iar TX accepted/ref/free sunt463/463/463;
- NO_MEM=0, other_error=0, RX1429;
- lwIP PSRAM28 alocări/43008 B, fallback0;
- DMA free/largest5140/1396 B.

Aceste date demonstrează recuperarea traseului și progres TX în fereastra
observată. Nu demonstrează că fiecare obiect reintrodus este urmărit individual,
că presiunea DMA a fost eliminată sau că testul ar rezista 30 de minute.

## Concluzie pentru integrare

TEST_01 și SUSTAINED_01 sunt două booturi independente cu același binar exact.
Ambele produc trei NULL tranzitorii în conversia Wi-Fi, dar continuă conversia,
eliberarea și TX cu refs/frees echilibrate, fără NO_MEM sau pierdere de serviciu.
Rezultatul susține integrarea schimbării testate ca remediere funcțională pentru
incidentul observat. Profilul corectiv rămâne mediul explicit
`esp32-lwip-psram`; nu se afirmă eliminarea tuturor eșecurilor DMA și nici
validarea completă a protocolului susținut.

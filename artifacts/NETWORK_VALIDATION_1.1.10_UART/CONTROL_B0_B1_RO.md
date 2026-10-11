# Control pozitiv normal 1.1.10 — confirmat pe hardware de proprietar

Incident OPEN. Analiza agentului este read-only; hardware executat de
proprietar, fără HTTP. Firmware1.1.10-uart-validation / a7a31ef9afc7.
Serial integral: logs/device-monitor-261011-151012.log. Ping: ping-B0-B1.log.
Fișierele sunt păstrate ca bytes originale, fără normalizare Git a newline-urilor.

|Captură|Ora serială a identității|Uptime|
|---|---|---:|
|B0|2026-10-11 15:11:51.468 +03:00|98381ms|
|B1|2026-10-11 15:12:24.172 +03:00|131087ms|

Interval uptime **32,706s**, nu cele10s planificate. Intervalul timestampurilor
seriale este32,704s; diferența2ms este compatibilă cu afișarea UART.
Înainte deB0 există aproximativ17,3s de ping în log, nu30s. Această abatere
este consemnată: nu afirmăm respectarea integrală a timpilor protocolului.
Recunoașterea contextului normal și progresul sunt confirmate în intervalul
B0/B1 efectiv, suficient pentru controlul discriminatoriu al clasificatorului.

|Contor|B0|B1|Diferență|
|---|---:|---:|---:|
|SW_CONVERT quota_passed|31|65|34|
|SW_MALLOC calls / ok / null|31/31/0|65/65/0|34/34/0|
|k1 dequeue|31|65|34|
|k3 pending empty|62|130|68|
|k4 quota unlock global|424|733|309|
|TX calls / accepted|29/29|63/63|34/34|
|refs / frees|29/29|63/63|34/34|
|RX calls / accepted|270/270|387/387|117/117|
|rx_echo_request|18|50|32|

HTTP0, NO_MEM0, other_error0, allocation failures0; SW ready1;
skipped/scope_overflow/paired/unpaired/unexpected0; conversion unpaired0,
preheap_null0 și after_malloc_null0; k0/k2/k5/trace0;
ledger_overflow/unmatched_free0, fără referințe reținute raportate.
TX_CONFIG captured1/init_calls1/result0/type1/dynamic32/cache4/static0,
features3 (cache bit0x2). Wi-Fi connected, aceleașiBSSID/canal și trei evenimente
de boot, fără disconnect/reset nou. Audio Bluetooth/SBC playing; proprietarul
confirmă explicit că era audibil corect (snapshotul singur nu certifică sunetul).

DMA: free7396/largest4852 laB0, free7500/largest5620 laB1. Main stack HWM
4404→3944; audio HWM7644 constant. Aceste valori sunt măsurători ale candidatului
instrumentat, nu consumul firmware stock. Grupurile de contori sunt copiate
în momente diferite în timpul afișării; nu sunt un snapshot atomic comun.

## Continuitatea pingului

**92 răspunsuri**, toate de la192.168.1.129, bytes32/TTL64, din
15:11:34.1675869 până la15:13:05.7052157 +03:00. Niciun timeout,
unreachable sau alt rezultat de eșec în fișier. Pauza maximă între răspunsuri
este**1,3436401s**; RTT19–406ms, medie93,511ms. Logul acoperă întregul
intervalB0/B1 și continuă dupăB1. Nu conține sumarul final Windows al pachetelor
trimise; formularea justificată este continuitate fără eșecuri observate în log,
nu o statistică de pierderi completă dintr-un sumar absent.

## Ce este validat și ce rămâne deschis

**PASS pentru controlul pozitiv al traseului normal**, coroborat cu verificarea
operanzilor ELF: unlock recunoaște conversiile care trec cota, malloc recunoaște
conversii care obțin nonNULL, dequeue și pending-empty sunt vizitate, TX/ref/free
progresează echilibrat, cu ping/audio sănătoase. Discrepanța1.1.9 nu apare aici.

Nu au fost exercitate NO_MEM, lista statică goală, requeue NULL, refuzul preheap,
NULL după malloc sau WAPI. Nu declarăm aceste ramuri validate de controlul normal.
Nici inventarul cache, leak-ul sau cauza persistenței1.1.7 nu sunt demonstrate.
Zero erori de instrumentare nu certifică stivele unor ramuri neexecutate.

Nu este necesară modificarea firmwareului sau reflash. Următoarea etapă este
[protocolul HTTP pe aceeași imagine](PROTOCOL_HTTP_RO.md), după verificarea
bootului curent și a unui baseline proaspăt. Redeschiderea CH340/monitorului
poate reseta placa; protocolul include ambele ramuri și nu presupune continuitatea.

## Hashuri de proveniență SHA256

- ping-B0-B1.log:
  F188639E618B8C5EDBCACE9036CB3BC3F897189EBEAC97B2EA17147F77C26F1A.
- logs/device-monitor-261011-151012.log:
  04B451AB2AB8F75153EF7E3D01D272B3F93CD7BA0A97CD07F90474E0A9FD4831.

Binarul/ELF-ul și SHA256SUMS rămân cele ale candidatuluia7a31ef9afc7;
documentarea și includerea logurilor nu schimbă build-ul executabil.

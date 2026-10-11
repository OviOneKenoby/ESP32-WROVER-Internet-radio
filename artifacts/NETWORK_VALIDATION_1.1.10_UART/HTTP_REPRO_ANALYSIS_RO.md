# HTTP_REPRO_01: dovadă nouă privind refuzul și retry-ul

Incident OPEN. Analiză read-only a fișierelor locale HTTP_REPRO_01.
ELF exact a7a31ef9afc7, SHA256 verificat:
1E6E9EDCBBA406A370E4F86108F6A3B14BB6EA1FC301A2FF0A8FAF80662E909A.
Nu s-a modificat firmwareul, făcut flash, merge sau recovery.

Baseline HTTP0/fail0/NO_MEM0, quota/malloc/dequeue și TX/ref/free progresează.
Uptime5060921 la16:34:33.815 și același GOT_IP12518ms sunt compatibile cu
bootul B0/B1, fără reset observat în acest log. BT/SBC playing în snapshoturi;
audibilitatea acestei sesiuni nu este demonstrată doar de log.

O singură cerere root la16:37:41.6324081. Firmwareul raportează header100B și
body3617B acceptate de API, fără socket error. Clientul primește antetul
HTTP200/Content-Length3617, dar **zero octeți de corp**, timeout10,003s,
curl_exit28; nu există root-1.html în director la analiză. Este dovadă de
recepție parțială la client, nu doar observația unei pagini inaccesibile.

Ping:145 răspunsuri valide de la192.168.1.129 înainte de cădere; ultimul la
16:37:41.7163715. Primul timeout16:37:46.5714680, apoi cinci timeouturi în total
și54 mesaje Destination host unreachable de la192.168.1.135, primul la
16:38:04.0596215. Niciun răspuns valid ulterior în fișierul analizat.
Host unreachable este emis de gazda.135, nu răspuns de succes al tunerului.

## Eșecuri de conversie și retry direct observat

quota_passed354, malloc calls354/ok349/null5, after_malloc_null5,
preheap_null0, k2requeue5, k5NULL5, alloc_failures5. Toate eșecurile:
1622B/caps0x80c, taskwifi/core0, ticks5248914–5248916.

PC-uri decodate cu ELF-ul exact:
- 0x40240ead: esf_buf_alloc_dynamic, apelul allocatorului;
- 0x40241678: esf_buf_alloc;
- 0x4020c9f1: ic_ebuf_alloc;
- 0x4023d6f7: ieee80211_output_process, conversie normală;
- 0x4023f059: ppTask, pentru primele patru eșecuri;
- **al cincilea** include0x4023df45 pp_process_hmac_waiting_txq,
  0x402100e9 ppProcTxDone, 0x4023f062 ppTask.

Urmele SW confirmă și requeue la0x4023d745. Astfel, în această sesiune,
cinci conversii au primit efectiv NULL de la allocator și s-au reînscris,
inclusiv **cel puțin o încercare declanșată prin TX-done**. Nu este un refuz
preheap observat pentru aceste cinci eșecuri. Nu deducem că fiecare eșec
corespunde unui pbuf distinct: ledgerul păstrează patru referințe distincte.

## Persistență și ramura NO_MEM

|Contor|Uptime5256069|Uptime5382087|
|---|---:|---:|
|accepted / refs / frees|351/351/347|351/351/347|
|NO_MEM|2|131|
|paired / k0|2/2|130/130|
|k1 / k2 / k3|354/5/697|354/5/697|
|quota / malloc calls / malloc null|354/354/5|354/354/5|
|after_malloc_null / alloc_failures|5/5|5/5|
|RX|15421|15912|
|DMA free / largest|5540/1972|4444/980|

Interval **126,018s**. Aceleași patru pbuf și first_tick:
3ffff2d8@5248913,3ffff8e8@5248914,3f8041d8@5248914,3fffe708@5248915.
accepted/ref/free, dequeue și conversiile nu mai progresează între capturi,
deși RX crește491 și k4 global crește559. k0/paired cresc fără alocări noi eșuate.
Diferența finală131NO_MEM față de130paired nu demonstrează un refuz neasociat:
SW este copiat înaintea NET, traficul continuă în timpul afișării; unpaired0.

Paired demonstrează observarea unui singur headNULL în scope-ul apelului TX
STA by-ref care returnează NO_MEM, pentru apelurile asociate. Traseul static
verificat și configdynamic32/cache4 selectează poolul tip9; **nu am citit direct
inventarul poolului**. Nu este doar coincidența numerică dintre două contoare.
Urmele finale k0 merg prin ieee80211_alloc_tx_buf→ieee80211_output_do→
esp_wifi_internal_tx_by_ref. Frame-urile lwIP includ **etharp_raw și etharp_input**;
ultimul TX_NO_MEM kind1/len42 este ARP. Driverul refuză deci și traficul ARP,
compatibil cu trecerea PC-ului de la timeout ICMP la host unreachable.
Nu echivalăm această compatibilitate cu demonstrația completă a stării ARP
a PC-ului: nu avem captura pachetelor sau tabela ARP a gazdei.

## Limite și implicație

Capturile post-incident sunt la16:37:48.957 și16:39:54.972; nu există în
fișiere toate punctele+5/+30/+60/+90 solicitate, iar notele nu definescT0.
Raportăm intervalul efectiv, fără a inventa capturi sau timpi.

Nu este demonstrată recuperarea durabilă a disponibilității DMA: largest este
1972 la prima captură și980 la ultima. Nu transferăm aici afirmația despre
heapul recuperat din1.1.7. Datele demonstrează lipsa vizitelor la siturile
identificate de dequeue/conversie în interval, nu absența fiecărui mecanism
privat de retry/TX-done și nici blocarea unei cozi sau un leak.

Explicația susținută acum: conversiile DMA eșuate sunt reînscrise; există un
retry prin TX-done la început, apoi admiterea refuză pe headNULL și traseul
conversiei observat nu mai este revizitat, cu patru referințe persistente.
Rămâne de separat de ce nu se mai declanșează/progresează acel traseu și ce
resurse private sunt reținute. Nicio corecție TX nu este demonstrată încă.

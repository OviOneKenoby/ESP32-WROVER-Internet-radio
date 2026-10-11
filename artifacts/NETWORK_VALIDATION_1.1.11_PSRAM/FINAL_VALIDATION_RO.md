# Verdict de integrare 1.1.11 PSRAM

Proprietarul a acceptat rezultatul drept **PASS funcțional** la 2026-10-11 și a
autorizat merge în `main` cu limitele consemnate.

Două booturi hardware independente ale binarului exact
**1.1.11-psram-validation / 124e84801b06** au păstrat pagina principală,
diagnostics, pingul și audio Bluetooth funcționale. În fiecare rundă au existat
trei eșecuri tranzitorii de1622 B în conversia Wi-Fi. În ambele cazuri ramura de
requeue a fost observată, conversiile și TX au continuat, refs/frees au revenit
echilibrate, iar `headNULL`, NO_MEM și referințele persistente au rămas zero.

- [TEST_01](TEST_01/HARDWARE_REPORT_RO.md) observă recuperarea timp de aproximativ
  123,010 s după eșecuri, cu377 răspunsuri ping fără pierderi.
- [SUSTAINED_01](SUSTAINED_01/HARDWARE_REPORT_RO.md) repetă recuperarea pe un
  boot nou; pingul are224 răspunsuri fără pierderi și continuă după eșecuri.

Acesta este un PASS al disponibilității și recuperării în ferestrele executate.
Nu este un PASS al criteriului strict „zero eșecuri DMA”, iar SUSTAINED_01 nu a
parcurs protocolul integral de30 minute. Politica mută în PSRAM alocările lwIP
de minimum512 B și păstrează fallbackul original; TX dynamic32/cache4,
driverul, retry-ul și payloadul HTTP nu sunt schimbate.

Integrarea păstrează profilul corectiv în mediul explicit PlatformIO
`esp32-lwip-psram`. Mediul implicit rămâne `esp32-dev`; activarea profilului
corectiv trebuie făcută explicit la build. Nu se adaugă reconnect sau reboot
automat și nu se pretinde că toate sursele de presiune DMA au fost eliminate.

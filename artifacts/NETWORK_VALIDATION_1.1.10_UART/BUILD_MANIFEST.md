# 1.1.10 UART — clasificator corectat, incident OPEN

- Firmware: **1.1.10-uart-validation**, build ID **a7a31ef9afc7**.
- Executable source commit: a7a31ef9afc73cf33cee979800f230a7a0fb5aac.
  Commiturile ulterioare de verificare/documente/artefacte nu schimbă sursa executabilă.
- Environment explicit: esp32-uart-probe; default esp32-dev nemodificat.
  espressif32@7.0.1, Arduino2.0.17/IDF4.4.7,
  framework3.20017.241212+sha.dcc1105b, Xtensa8.4.0+2021r2-patch5,
  aceleași biblioteci fixate și patch AAC. Fără upgrade SDK/toolchain.
- Build SUCCESS în232,87s. RAM85912B, flash2021649B.
  DRAM data31184B, BSS54728B, IRAM text128923B.
  Față de1.1.9: **+72B DRAM data, −40B IRAM text, +32B flash total**.
  Față de1.1.7: +1472B DRAM data și +1608B IRAM text, BSS neschimbat.
  Mărimile nu măsoară costul CPU al apelurilor/încărcărilor volatile.
- Binary: firmware-1.1.10-uart-a7a31ef9afc7.bin, 2028224bytes.
  SHA256 B568574452345659522FC86AC53DD1825E2E2C8E556879C3201757AA8DB3A579.
- Exact ELF: firmware-1.1.10-uart-a7a31ef9afc7.elf, 29557620bytes.
  SHA256 1E6E9EDCBBA406A370E4F86108F6A3B14BB6EA1FC301A2FF0A8FAF80662E909A.
- esptool4.11.0 image_info: cinci segmente, checksum/hash de validare corecte.
  app0 rămâne offset0x10000/size0x300000. Nu s-a făcut flash sau readback.

## Corecția și verificarea operanzilor finali

Noua tabelă DRAM conține baze de simbol fără addend și offseturi separate.
Încărcările volatile obligă adunarea pe target; classificationAddress,
kindFor, conversionPc și mallocPc sunt funcții IRAM noinline.
unlock și observeMalloc apelează aceeași conversionPc. Callbackurile originale
sunt primele apelate în forwarding, o singură dată, cu aceleași argumente și
rezultate. Traseele TX private și configurația dynamic32/cache4 nu sunt schimbate.

verify_tx_classification_elf.py citește ELF32 little-endian și disassemblyul
final, verifică fiecare bază/offset DRAM și interpretează numai subsetul mic
de instrucțiuni emis pentru predicate, inclusiv literal loads, load/add,
comparații, ramuri și call8 cu ABI windowed. Operanzii necunoscuți/instrucțiunile
nesuportate/read fără date în ELF/control-flow gap depășit sunt erori, nu PASS.
Verifică rezultate pozitive, adrese vecine negative și clasificarea încrucișată
a tuturor siturilor; verifică și apelurile ambelor observatoare la predicatul comun.
Nu este emulator complet ESP32 și nu dovedește unwind-ul real pe hardware.

|Sit|Operand final efectiv|Convenție|
|---|---|---|
|k0 static empty|0x4024160e|retur callback|
|k1 dequeue|0x4023d68f|retur callback|
|k2 requeue NULL|0x4023d748|retur callback|
|k3 pending empty|0x4023d866|retur callback|
|k4 quota passed|0x40240e80|retur callback|
|k5 dynamic NULL|0x40240fb0|retur callback|
|Conversie normală, comun unlock/malloc|0x4023d6f7|PC call ic_ebuf_alloc|
|Conversie WAPI, comun unlock/malloc|0x4023d6e9|PC call WAPI|
|malloc dinamic|0x40240eb0|retur callback|

Verificatorul întărit respinge explicit ELF-ul exact fe228ca737b9:
operand real0x4023d6e2, așteptat0x4023d6f7. Noul predicat respinge și
offsetul relativ eronat+0xa2. Testele mută independent toate cele18 câmpuri
base/offset ale tabelei, inclusiv recrearea+0xa2, și cer refuzul fiecăruia.
Detaliile rezultatelor sunt în ELF_VERIFICATION.txt și VERIFIER_TESTS.txt.
Verificatorul software principal invocă obligatoriu acest control suplimentar.

Verificarea existentă compară cele șase corpuri private normalizate cu ELF-ul
exact1.1.7 (hash verificat), callbackurile OSI, tabela înregistrată, placement
IRAM, original-first și fragmentele printf sub64bytes. Verificarea NET_PROGRESS
controlează cele cinci wrapperuri și rutarea lor. Aceste verificări pe host
nu validează livrarea TCP sau cazurile NULL/NO_MEM pe hardware.

Sursa src/main.cpp este neschimbată față de06011be; branch main rămâne
112d68a49a053d302a87fc5658d497617eedda88. Encoderul rămâne activ; nu există
gzip, recovery automat, schimbare de buffer sizing sau dezactivarea webului.
Documentarea1.1.7/1.1.9 și artefactele istorice rămân intacte.

## Prima etapă hardware

Urmează exclusiv [protocolul B0/B1](PROTOCOL_RO.md), fără HTTP. Succesul host
nu este baseline PASS. Logurile B0/B1 trebuie analizate înainte de orice
declanșare a incidentului într-o etapă separată. Hardware NOT RUN de agent.
Incident OPEN, fără merge/flash automat și fără fix demonstrat.

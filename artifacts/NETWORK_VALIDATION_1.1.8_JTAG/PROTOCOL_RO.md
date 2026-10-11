# Protocol condițional complet: ramura NO_MEM și retry/recycling

**Neexecutabil pe echipamentul disponibil: proprietarul are numai CH340/UART,
fără adaptor JTAG.** Alternativa software și protocolul UART sunt în
[NETWORK_VALIDATION_1.1.10_UART](../NETWORK_VALIDATION_1.1.10_UART/PROTOCOL_RO.md).
Prima etapă este numai B0/B1, fără HTTP; protocolul1.1.9 este suspendat.
Protocolul de mai jos rămâne arhivat, nu este următorul test cerut aici.

**Candidat în repository, nu release. Incident OPEN. Fără merge sau flash executat de
agent.** 1.1.8-jtag-validation / e2c5667dd9a1 păstrează instrumentarea1.1.7 și
TX dynamic32/cache4; dezactivează encoderul (inițializare, IRQ, polling) pentru
a păstra muxul JTAG GPIO13/14. Butoanele Play/Next/Prev și comanda serială l
rămân disponibile. Nu modifică driverul, retry, buffer sizing sau politica TX.

Întrebări: (A) NO_MEM al unui apel STA provine efectiv din headNULL al poolului
tip9? (B) după incident mai există încercări de conversie și intrări în recycling,
în ce context de retry și cu ce rezultat? Este o observație separată cu debugger,
nu validarea unui fix și nici inventar complet al poolului.

## Condiții de fezabilitate: STOP dacă una lipsește

- Adaptor JTAG ESP-Prog/FT2232 compatibil și modul confirmat ESP32-WROVER-E/B
  cu flash3.3V pentru comenzile de mai jos. Pe laptop a fost enumerat doar
  CH340/COM5 (UART, nu adaptor JTAG). Alt adaptor/tensiune necesită configurația
  sa verificată; nu încerca profilul FTDI sau3.3V prin presupunere.
- Înainte de conectare, păstrează logul ultimului boot eșuat. Cu placa oprită,
  izolează fizic encoderul de GPIO13/14; fără încărcare electrică pe JTAG.
  Conectează TDI12/TCK13/TMS14/TDO15/GND și Vref3.3V potrivit adaptorului,
  fără alimentări concurente. GPIO12 este strap pentru tensiunea flash.
- Imaginea JTAG și ELF-ul corespunzător, hashuri verificate. Nu folosi adresele
 1.1.7 după relink. Nu atașa acest protocol bootului stock1.1.7: pinMode
  selectează GPIO pe13/14. Deconectarea encoderului singură nu rezolvă muxul.
- OpenOCD Espressif, SMP cu ambele nuclee; **ESP_FLASH_SIZE0 înainte de target
  config** dezactivează fallback-ul software flash. Două sloturi hardware pe
  fiecare core, fără alte breakpoint-uri. Confirmă inserarea hardware în logd3;
  simplul mesaj GDB „hardware breakpoint” nu este suficient.
- Orice eroare de adaptor, tensiune, JTAG, SMP, breakpoint, ELF, simbol/register,
  stack unwind sau baseline: STOP; nu modifica registre/driver/flash ca fallback.

## Comenzi de pregătire și instalare MANUALĂ

PowerShell: inițializează variabilele în FIECARE terminal nou. Aceste comenzi sunt oferite utilizatorului;
flashul și conectarea debuggerului nu au fost executate de agent.

```powershell
Set-Location 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.8_JTAG'
$candidateDir = (Get-Location).Path
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$py = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
$espTool = 'C:\Users\Dell\.platformio\packages\tool-esptoolpy\esptool.py'
$ocd = 'C:\Users\Dell\.platformio\packages\tool-openocd-esp32\bin\openocd.exe'
$gdb = 'C:\Users\Dell\.platformio\packages\tool-xtensa-esp-elf-gdb\bin\xtensa-esp-elf-gdb-3.11.exe'
$scripts = 'C:\Users\Dell\.platformio\packages\tool-openocd-esp32\share\openocd\scripts'
$env:PATH = 'C:\Users\Dell\.platformio\python3;' + $env:PATH
Get-Content "$candidateDir\SHA256SUMS.txt"
Get-FileHash "$candidateDir\firmware-1.1.8-jtag-e2c5667dd9a1.bin" -Algorithm SHA256
Get-FileHash "$candidateDir\firmware-1.1.8-jtag-e2c5667dd9a1.elf" -Algorithm SHA256
& $pio device list
```

După confirmarea partiției app0/offset0x10000 și salvarea bootului anterior,
închide terminalul care ocupă COM5. Instalează MANUAL numai binarul candidat;
comanda de mai jos face reset pentru instalarea noului experiment, nu recuperare
automată a incidentului. Nu șterge NVS sau flashul întreg.

```powershell
& $py $espTool --chip esp32 --port COM5 --baud 460800 write_flash 0x10000 "$candidateDir\firmware-1.1.8-jtag-e2c5667dd9a1.bin"
```

Terminal UART, păstrat deschis, logging automat în fișier:

```powershell
& $pio device monitor --port COM5 --baud 115200 --filter time --filter log2file
```

În terminal tastează l pentru Bluetooth, împerechează/redă audio, apoi x pentru
snapshot. Confirmă versiunea/ID, mesajul encoder disabled, TX_CONFIG
captured1/result0/type1/dynamic32/cache4/static0 și cache feature bit0x2.

Terminal separat pentru ping (oprire cu Ctrl+C doar la final):

```powershell
ping.exe -t 192.168.1.129 | Tee-Object -FilePath "$candidateDir\ping.log"
```

Terminal OpenOCD, DOAR profilul hardware confirmat de mai sus:

```powershell
& $ocd -s $scripts -d3 -l "$candidateDir\openocd.log" -c 'set ESP_FLASH_SIZE 0' -c 'set ESP32_FLASH_VOLTAGE 3.3' -f interface/ftdi/esp32_devkitj_v1.cfg -f target/esp32.cfg -c 'adapter speed 1000' -c init
```

Terminal GDB, din directorul candidat:

```powershell
Set-Location $candidateDir
& $gdb -nx firmware-1.1.8-jtag-e2c5667dd9a1.elf
```

În GDB:

```gdb
set pagination off
set confirm off
set logging file gdb.log
set logging overwrite on
set logging on
target remote localhost:3333
monitor targets
info threads
info breakpoints
set remote hardware-breakpoint-limit 2
set $tuner_jtag_ready = 1
continue
```

Setarea ready=1 este permisă numai după verificarea condițiilor/logurilor.
Nu folosi load, reset, breakpoint software, next cu sloturile ocupate, call
sau scrieri în memoria targetului. Conectarea GDB poate opri temporar ambele
core-uri; exclude perioada de attach din baseline, apoi așteaptă30s cu ping/audio
funcționale. x la sfârșit: response_count0/failures0, ledger flags0.

## Ordine și timpi, într-un singur boot de test

1. **Control pozitiv, fără HTTP:** Ctrl+C în GDB, apoi comenzile de mai jos.
   Lasă pingul să ruleze până la5s. Trebuie să existe atât CONVERSION_RESULT
   nonNULL cât și CACHE_RECYCLE_ENTRY; verifică logurile hardware. Ctrl+C,
   delete, continue; apoi x și încă5s baseline. Lipsa controlului pozitiv sau
   pierderea pingului înainte de HTTP înseamnă test invalid; STOP.

```gdb
source TX_RETRY_PROBE.gdb
info breakpoints
continue
# Ctrl+C după cel mult5s, apoi:
delete
continue
```

2. **Faza A, atribuirea refuzului:** Ctrl+C, apoi:

```gdb
source TX_BRANCH_PROBE.gdb
info breakpoints
continue
```

   Execută o singură cerere root controlată într-un PowerShell din directorul
   candidat; loghează ora și rezultatul. Nu există polling JavaScript/browser:

```powershell
Get-Date -Format o | Tee-Object -FilePath root-trigger.log -Append
curl.exe --http1.1 --max-time 10 --dump-header root.headers --output root.html http://192.168.1.129/ 2>&1 | Tee-Object -FilePath root-trigger.log -Append
```

   Dacă nu cade, maximum încă două cereri identice, la cel puțin5s distanță,
   cu x între ele. **Oprește cererile imediat** la prima pierdere ping sau primul
   NO_MEM/fail allocator. După trei cereri fără reproducere, STOP și raportează
   non-reproducerea; curl/debug/input/layout diferă de browserul original.
   Nu repeta1.1.6 și nu folosi alte rute HTTP pentru snapshot.

3. **T0** este primul incident observat. x imediat, +5s; fazaA se oprește la
   maximum8 returnări sau16 opriri de breakpoint (scriptul dezactivează probele
   și reia execuția), ori manual la10s dupăT0. Ctrl+C, delete, continue.
   Dacă există hitCACHE9_EMPTY a3=9/a2=0 și un NO_MEM în același apel, păstrează
   backtrace/thread/SP/contextul pentru asociere. Numărul egal de hituri nu este
   asociere de apeluri. Backtrace invalid => atribuirea nu este concludentă.

4. **Faza B** după dezactivareaA, cel târziuT0+15s: Ctrl+C și:

```gdb
source TX_RETRY_PROBE.gdb
info breakpoints
continue
```

   Observă30s, fără alte cereri HTTP. x laT0+30s șiT0+60s (dacă debuggerul e
   oprit pe target, reia mai întâi; snapshotul nu trebuie așteptat sub halt).
   Scriptul se dezactivează după16 hituri totale. La30s de fazăB: Ctrl+C,
   apoi elimină probele și colectează stările taskurilor:

```gdb
delete
monitor targets
info threads
thread apply all bt 8
continue
```

   Un taskwifi observat în așteptarea cozii este o stare la acel instant, nu
   dovada întregului interval. Capturile să includă și uptime/timpii reali;
   dacă fazele depășesc timpii prevăzuți, raportează timpii efectivi.

5. După ultimul x, fără alte HTTP sau restart:

```gdb
detach
quit
```

   Oprește ping/OpenOCD manual după salvarea logurilor. O eroare SMP/JTAG,
   watchdog/reset, audio schimbat de debugger, fallback software, identitate
   greșită, pierdere ping în controlul pozitiv sau capul de eșantion atins înainte
   de incident oprește testul și invalidează afirmațiile despre absența hiturilor.

## Loguri obligatorii și interpretare

Păstrează: serial integral de la boot (inclusiv toate x și ALLOC_FAILURE), ping,
root-trigger.log/root.headers, OpenOCDd3, GDB cu toate backtrace-urile/controlul
pozitiv/SAMPLE_LIMIT, manifest/hashuri și oreleT0/fazelor. Notează adaptorul,
modulul/tensiunea, cablajul și orice oprire/eroare. Packet capture PC este util,
dar absența sa nu anulează probele directe de ramură.

| Rezultat | Ce susține / ce nu demonstrează |
|---|---|
| Tip9/headNULL în apelul care returnează NO_MEM | Refuz prin lipsă de intrare liberă în acel pool la acel moment; nu leak sau cauza persistenței |
| NO_MEM fără hit asociabil tip9 | Altă ramură sau acoperire/context insuficient; inspectează stackul, nu ghici ramura |
| Conversii NULL repetate după recuperarea heapului | Retry există dar nu reușește; quota/gate/heap instantaneu rămân de separat; lipsa hookului nu înseamnă lipsa apelului |
| Conversii nonNULL, recycling și frees cresc | Progres revine sau problema e în altă etapă; debuggerul poate fi cauza schimbării |
| Intrări recycle fără frees | Callback absent/condiție/pbufNULL sau eroare de asociere; intrarea nu dovedește completarea recycling-ului |
| Zero conversii/recycle în fazaB, control pozitiv valid, probe armate și NO_MEM continuă | Nicio vizită la punctele selectate în fereastră; susține lipsa declanșării/progresului acestei conversii, nu lipsa tuturor mecanismelor private de retry |
| Funcționează/non-reproducere sau își revine numai cu debugger | Rezultat perturbat/inconcludent pentru firmware stock, nu fix demonstrat |
| Config/tool/SMP/control pozitiv invalid, SAMPLE_LIMIT timpuriu sau reset | Test invalid pentru concluzii negative; nu cere alte refreshuri neinstrumentate |

Pregătire verificată pe host: OpenOCD0.11.0-esp32-20220706; configurația
FTDI/ESP32/SMP a fost PARSATĂ fără init/adaptor/target și confirmă flash support
disabled + force hard breakpoints. GDBesp12.1_20221002/Python3.11 a încărcat
ELF-ul și a verificat guardul ready0: ambele scripturi refuză armarea, fără
breakpoint-uri create. Asta nu validează conectarea/SMP pe hardware sau stivele.
Comenzile PowerShell sunt inline: politica locală blochează fișiereps1;
nu o dezactiva și nu folosi bypass. Repetă blocul de variabile în fiecare
terminal, fără a executa un fișier de inițializare.

Limite: debuggerul oprește core-uri și poate modifica watchdog/coex/timing;
dezactivarea encoderului și relinkul schimbă overhead/layout/heap baseline.
Probe host/stack/logging adaugă timp de halt. Limitele8/16 controlează volumul,
nu garantează absența efectelor. Nu citește liste private, nu schimbă TX, nu
reapelează funcții driver și nu injectează recovery. Firmware/hardware/JTAG
NOT RUN de agent. Incident OPEN indiferent de rezultat până la dovada cauzală.

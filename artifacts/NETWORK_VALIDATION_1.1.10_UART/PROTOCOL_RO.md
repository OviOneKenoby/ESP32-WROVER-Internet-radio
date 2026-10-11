# 1.1.10 UART — numai controlul pozitiv B0/B1

**Incident OPEN. Fără HTTP în această etapă.** Candidat
1.1.10-uart-validation / a7a31ef9afc7, pentru CH340/UART COM5.
1.1.9 rămâne suspendat; nu folosi protocolul său pentru declanșare.
Agentul nu execută flash/reset/merge. Encoderul rămâne activ.

Întrebarea: clasificatorul unlock corectat recunoaște conversiile normale
recunoscute și de malloc, în timpul ping/audio sănătoase, fără HTTP?
Hostul verifică operanzii din ELF; B0/B1 verifică separat rutarea callbackurilor
și recunoașterea stivei reale. Acest control nu exercită NO_MEM sau conversii
NULL și nu demonstrează cauza incidentului.

## Pregătire și instalare manuală

Salvează logurile existente și ultimul snapshot înainte de instalare. Nu
repeta refreshuri pe 1.1.9. Închide terminalul care ocupă COM5 după salvare.
Execută blocul de variabile în FIECARE terminal nou; nu este necesar un ps1
și nu se dezactivează politica PowerShell.

```powershell
Set-Location 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.10_UART'
$candidateDir = (Get-Location).Path
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$py = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
$espTool = 'C:\Users\Dell\.platformio\packages\tool-esptoolpy\esptool.py'
$bin = Join-Path $candidateDir 'firmware-1.1.10-uart-a7a31ef9afc7.bin'
$elf = Join-Path $candidateDir 'firmware-1.1.10-uart-a7a31ef9afc7.elf'
Get-Content .\BUILD_MANIFEST.md
foreach ($line in Get-Content .\SHA256SUMS.txt) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'STOP: invalid manifest' }
    $expectedHash = $Matches[1]
    $artifactPath = Join-Path $candidateDir $Matches[2]
    if ((Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash -ne $expectedHash) {
        throw "STOP: hash mismatch $artifactPath"
    }
}
& $pio device list
```

STOP dacă hashurile/identitatea nu corespund, COM5 lipsește sau partiția app0
nu este cea existentă (huge_app.csv: offset0x10000, dimensiune0x300000).
Nu schimba partițiile, NVS, SDK, TX/cache ori configurația pentru control.
Numai proprietarul execută instalarea, după verificările de mai sus:

```powershell
& $py $espTool --chip esp32 --port COM5 --baud 460800 write_flash 0x10000 $bin
```

Aceasta începe un boot nou pentru experiment; nu este recovery automat.
Nu executa erase_flash sau upload implicit și nu conecta un debugger.

## Ordine și timpi — un singur boot, fără HTTP

1. Deschide terminalul serial, păstrat deschis până după B1:

```powershell
& $pio device monitor --port COM5 --baud 115200 --filter time --filter log2file
```

   Păstrează logul integral de la boot (logs/device-monitor-*.log). Dacă bootul
   a precedat monitorul, apasă manual EN o singură dată acum, înaintea controlului.
   Confirmă 1.1.10-uart-validation / a7a31ef9afc7. Tastează `l`, împerechează
   Bluetooth și pornește audio SBC. Nu schimba sursa și nu deschide browserul.

2. În terminal separat, după blocul de variabile, pornește ping continuu:

```powershell
ping.exe -t 192.168.1.129 | ForEach-Object { '{0:o} {1}' -f (Get-Date), $_ } | Tee-Object -FilePath .\ping-B0-B1.log
```

   Folosește IP-ul efectiv raportat serial dacă diferă. După 30 s de ping și
   audio funcționale, tastează `x` în serial: B0. Notează ora și uptime-ul.
   După încă 10 s de ping/audio, tastează `x`: B1. Notează timpii efectivi dacă
   nu sunt exact 10 s. Nu face alte acțiuni în acest interval.

3. După B1, oprește ping cu Ctrl+C și salvează serialul integral. Testul este
   încheiat aici, inclusiv dacă toate criteriile par satisfăcute.
   Trimite B0/B1 și logurile pentru validarea clasificării înainte de orice HTTP.
   Nu există comandă HTTP autorizată de acest protocol.

## Criterii, oprire și rezultate

În ambele capturi: firmware/ID corecte; TX_CONFIG captured1/result0/type1/
dynamic32/cache4/static0/features bit0x2; HTTP count0, allocation failures0,
NO_MEM0; SW_TX ready1 și skipped/scope_overflow/paired/unpaired/unexpected0;
SW_CONVERT unpaired0, preheap_null0, after_malloc_null0; SW_MALLOC null0;
ledger_overflow/unmatched_free0 și fără referințe reținute raportate.

Între B0 și B1 trebuie să crească k1, k3, **quota_passed** și SW_MALLOC calls/ok,
precum și TX accepted/refs/frees, cu ping/audio funcționale. k4 este global,
nu ține loc de quota_passed. Cota tip1 este configurată la32; bypass-ul limitei
zero există static, dar nu justifică ignorarea unui quota_passed nul aici.
Grupurile SW și NET sunt copiate la momente diferite în timpul afișării:
nu cerem egalitate exactă între diferențele lor. Dacă apare o diferență de
refs/frees sau un ledger nenul, nu declara PASS automat; păstrează contextul
pentru analiză, fără refresh/restart pentru a-l curăța.

|Rezultat|Interpretare și acțiune|
|---|---|
|Toate criteriile îndeplinite|Control pozitiv observat pentru traseul normal; trimite logurile. Clasificarea trebuie validată înaintea etapei HTTP separate. Nu validează ramura WAPI, refuzul cache sau NULL/retry.|
|malloc ok și k1 cresc, quota_passed rămâne fix|Discrepanța persistă: STOP, fără HTTP. Nu relaxa criteriul; trebuie inspectate stivele/rutarea, nu declarată lipsa conversiei.|
|k4 crește fără clasificare normală validă|Activitate în alte contexte sau acoperire incompletă; nu demonstrează controlul conversiei. STOP și păstrează logurile.|
|Contorii nu cresc, ping lipsește sau audio nu este corect|Control invalid/incident înainte de trigger: STOP. Păstrează ultimul x și logul; nu injecta recuperare.|
|HTTP>0, fail/NO_MEM>0, skipped/overflow/unpaired/unexpected, reset/watchdog sau identitate greșită|STOP; baseline-ul nu este controlul solicitat. Nu continua cu HTTP.|

Loguri necesare: serial integral de la boot cu B0/B1 și toate liniile SW,
NET_PROGRESS, TX_CONFIG, ledger și ALLOC_FAILURE; ping-B0-B1.log; manifest/
hashuri; orele și uptime B0/B1; confirmare audio și orice anomalie. Nu sunt
suficiente doar liniile quota_passed și malloc. Nu se cere repetarea 1.1.6.

Perturbări: față de1.1.9 se adaugă o tabelă DRAM și apeluri IRAM pentru
adunare/clasificare; comparațiile sunt comune unlock/malloc, cu încărcări
volatile. Callbackurile originale sunt forwardate o singură dată, cu aceleași
argumente și rezultate; mutexul original se eliberează înainte de observație.
Niciun logger, allocator de diagnostic sau heap query în callback; unwind
limitat la12 frame-uri, urme rare primele4/ultimele4. Relinkul, memoria,
costul callbackurilor și afișarea UART pot modifica timingul și reproducerea.
Valorile măsurate pe host sunt în manifest. Hardware NOT RUN de agent.

Limite: nu se citește inventarul poolului privat, nu se schimbă retry/driver,
TX dynamic32/cache4, root sau encoderul; fără gzip, reconnect/reboot automat,
dezactivarea webului în Bluetooth, merge ori flash automat. Dovezile1.1.7
rămân valabile separat. Incident OPEN, indiferent de rezultatul acestui control.

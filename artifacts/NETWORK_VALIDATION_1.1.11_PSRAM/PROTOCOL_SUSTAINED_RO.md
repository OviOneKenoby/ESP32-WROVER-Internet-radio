# 1.1.11 — validare susținută pe același binar

Incident OPEN. Folosește **același 1.1.11-psram-validation / 124e84801b06**,
TX dynamic32/cache4. Fără reflash dacă imaginea este deja instalată; fără merge,
reconnect/reboot automat, browser polling sau schimbări de firmware/configurație.
Scopul este repetabilitatea recuperării și disponibilitatea susținută, nu
demonstrarea inventarului poolului privat.

## 1. Director și identitate

În PowerShell:

```powershell
$candidateDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM'
$runDir = Join-Path $candidateDir 'SUSTAINED_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
if (Test-Path -LiteralPath $runDir) { throw 'STOP: choose SUSTAINED_02 everywhere' }
foreach ($line in Get-Content (Join-Path $candidateDir 'SHA256SUMS.txt')) {
  if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'STOP: bad manifest' }
  $p = Join-Path $candidateDir $Matches[2]
  if ((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne $Matches[1]) { throw "STOP: hash mismatch $p" }
}
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Copy-Item (Join-Path $candidateDir 'SHA256SUMS.txt'),(Join-Path $candidateDir 'BUILD_MANIFEST.md') -Destination $runDir
"start=$((Get-Date).ToString('o')); same 1.1.11; no automatic recovery" | Tee-Object -FilePath (Join-Path $runDir 'session-notes.log')
& $pio device list
```

STOP dacă lipsește COM5 sau hashul nu corespunde. Nu executa esptool/upload.

## 2. UART, reset și baseline

Terminal UART separat:

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\SUSTAINED_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
Set-Location $runDir
& $pio device monitor --port COM5 --baud 115200 --dtr 0 --rts 0 --no-reconnect --filter time --filter log2file
```

Deschiderea CH340 poate reseta placa. Tastează x imediat și verifică exact
versiunea/ID, reset_reason, uptime, TX_CONFIG dynamic32/cache4, SW ready1 și
contoarele. Dacă uptime indică boot nou sau apare banner, notează `NEW_BOOT` și
continuă numai după 30s Bluetooth SBC audibil + ping stabil; acesta devine
baseline-ul unei runde noi. Dacă bootul vechi continuă, notează `SAME_BOOT` și
valorile inițiale. Nu compara contoare peste două booturi.

Terminal ping, păstrat pe toată durata:

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\SUSTAINED_01'
ping.exe -t 192.168.1.129 | ForEach-Object { '{0} {1}' -f (Get-Date).ToString('o'), $_ } | Tee-Object -FilePath (Join-Path $runDir 'ping.log')
```

Cu audio redat, x=S0, așteaptă30s, x=S1. Baseline valid: Wi-Fi connected fără
eveniment nou, ping/audio corecte, NO_MEM0, refs=frees, fără retained și fără
erori de instrumentare. Eșecurile istorice pot rămâne în SAME_BOOT; verifică
că nu cresc și că k1/k3/TX/ref/free progresează. STOP dacă baseline-ul nu trece.

## 3. Runda browser, 30 minute

Folosește un singur browser/tab, fără auto-refresh. În fiecare dintre minutele
0,5,10,15,20 și25 execută intenționat **două refreshuri ale paginii principale
la 2s distanță**. Nu deschide alte pagini simultan. Notează înainte de pereche:

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\SUSTAINED_01'
"browser_pair start=$((Get-Date).ToString('o')) minute=REPLACE" | Tee-Object -FilePath (Join-Path $runDir 'session-notes.log') -Append
```

Înlocuiește REPLACE cu0/5/10/15/20/25. După fiecare pereche, tastează x imediat,
apoi x la+5s și+60s. Nu lansa următoarea pereche înainte de captura+60s. Pagina,
diagnostics, pingul și audio trebuie verificate și consemnate la fiecare rundă.

Dacă apar allocation failures dar pagina/ping/audio continuă și NO_MEM rămâne0,
oprește temporar refreshurile și capturează x imediat,+5,+30,+90s. Continuă cu
următoarea pereche numai dacă k1/SW_MALLOC ok/k3/TX/frees cresc, k2/null nu mai
cresc în fereastră, refs=frees și nu există retained/headNULL. Notează intervalul
real de la ultimul tick de eșec. Aceasta este recuperare observată, nu zero-fail.

După minutul25, nu mai genera HTTP. Menține ping/audio până la minutul30;
x final și încă x după30s de repaus.

## 4. Oprire imediată și ramură de incident

T0 este primul timeout/unreachable, pagină/diagnostics inaccesibilă, NO_MEM,
k0/headNULL, retained ref, refs/frees care rămân dezechilibrate, Wi-Fi event,
reset/watchdog sau audio afectat. La T0 nu mai face HTTP și nu aplica recovery.
Notează ora/simptomul, apoi x imediat,+5,+30,+60,+90s:

```powershell
"T0=$((Get-Date).ToString('o')); symptom=REPLACE" | Tee-Object -FilePath (Join-Path $runDir 'session-notes.log') -Append
```

Oprește doar după captura+90s, exceptând riscul electric/termic. Păstrează
monitorul deschis; redeschiderea poate reseta placa și invalida persistența.

## 5. Criterii și fișiere

Validare susținută funcțională: toate răspunsurile browserului sunt recepționate,
ping fără timeout/unreachable, audio corect, fără reset/disconnect/NO_MEM/headNULL
sau retained; după orice NULL, k1/alocările reușite/k3/TX/frees reiau progresul
și refs=frees în maximum90s. Criteriul separat de prevenție strictă trece numai
dacă alloc_failures nu crește.

Predă întregul SUSTAINED_01: serial complet, ping.log, session-notes cu etichetele
și confirmările audio/web, toate snapshoturile și orice captură/export browser
disponibil. Nu șterge o rundă eșuată și nu începe alta înainte de analiză.

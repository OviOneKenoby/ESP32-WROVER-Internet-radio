# Candidat corectiv 1.1.11: buffere lwIP mari în PSRAM

**TEST_01 executat:** curl a trecut complet; browserul real a produs trei
eșecuri DMA, urmate de recuperare TX observată timp de aproximativ123s, fără
NO_MEM/headNULL/referințe reținute. Criteriul strict zero-fail nu a trecut și
incidentul rămâne OPEN. Vezi [raportul hardware](TEST_01/HARDWARE_REPORT_RO.md).
Pasul următor este [validarea susținută a aceluiași binar](PROTOCOL_SUSTAINED_RO.md),
nu repetarea protocolului inițial și nu un firmware nou în acest moment.

Incident OPEN. Test manual CH340/COM5; fără JTAG, merge, recovery sau flash
executat de agent. TX dynamic32/cache4, driverul și payloadul root rămân aceleași.

## Ce schimbă și ce poate demonstra

În SDK-ul fixat, mem_malloc folosește malloc obișnuit, cu preferință pentru
memoria internă sub 4096 B. PBUF_RAM/TCP concurează astfel cu conversiile DMA
Wi-Fi de 1622 B. Candidatul preferă PSRAM numai pentru apelurile lwIP mem_malloc
de minimum 512 B, cu fallback la allocatorul original. PSRAM nu devine memorie
DMA: mutăm un consumator concurent, nu conversia Wi-Fi. Alocările mici,
mem_calloc și alocările din alte componente păstrează politica originală.

Instrumentarea UART din 1.1.10 rămâne. LWIP_MEMORY numără apelurile, alocările
PSRAM și fallbackurile; bytes este cumulativ, NU memoria încă reținută.
Wrapperul adaugă apeluri, acces PSRAM și o secțiune critică scurtă pentru
contoare. Schimbă locul alocării/layoutul/timingul; nu schimbă retry/cache/TX.
Nu poate demonstra că toate sursele de presiune DMA sunt eliminate sau că
driverul se recuperează dintr-un eșec viitor. Evitarea reproducerii este validare
a candidatului în condițiile testate, nu demonstrația completă a root cause.

## 1. Pregătire și instalare manuală

Păstrează toate logurile bootului vechi înainte de instalare. Închide monitorul
care ocupă COM5. Rulează în PowerShell; nu reutiliza un director de test existent.

```powershell
$candidateDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM'
$runDir = Join-Path $candidateDir 'TEST_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$py = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
$espTool = 'C:\Users\Dell\.platformio\packages\tool-esptoolpy\esptool.py'
if (Test-Path -LiteralPath $runDir) { throw 'STOP: choose TEST_02 everywhere' }
foreach ($line in Get-Content (Join-Path $candidateDir 'SHA256SUMS.txt')) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'STOP: invalid manifest' }
    $expectedHash = $Matches[1]
    $artifactPath = Join-Path $candidateDir $Matches[2]
    if ((Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash -ne $expectedHash) { throw 'STOP: hash mismatch' }
}
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Copy-Item -LiteralPath (Join-Path $candidateDir 'SHA256SUMS.txt'), (Join-Path $candidateDir 'BUILD_MANIFEST.md') -Destination $runDir
& $pio device list
$bin = Join-Path $candidateDir 'firmware-1.1.11-psram-124e84801b06.bin'
```

Confirmă COM5 și partiția app0 la 0x10000 din configurația păstrată. Instalarea
de mai jos este MANUALĂ și resetează placa pentru noul test; nu este recovery.
Nu șterge NVS/flash și nu folosi upload automat.

```powershell
& $py $espTool --chip esp32 --port COM5 --baud 460800 write_flash 0x10000 $bin
if ($LASTEXITCODE -ne 0) { throw 'STOP: flash failed' }
```

## 2. Logging, boot și control pozitiv

Terminal UART nou; inițializează variabilele și păstrează terminalul deschis.

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\TEST_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
Set-Location $runDir
& $pio device monitor --port COM5 --baud 115200 --dtr 0 --rts 0 --no-reconnect --filter time --filter log2file
```

Deschiderea CH340 poate reseta chiar cu dtr/rts0. Verifică bannerul, versiunea
1.1.11-psram-validation, ID124e84801b06, reset_reason și uptime prin x. Dacă
monitorul este redeschis ulterior, verifică din nou uptime și contoarele; un
boot nou cere reluarea întregului baseline. Nu continua peste un reset neobservat.
Tastează l numai dacă Bluetooth nu este activ, apoi împerechează/redă SBC.

Terminal ping nou:

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\TEST_01'
ping.exe -t 192.168.1.129 | ForEach-Object { '{0} {1}' -f (Get-Date).ToString('o'), $_ } | Tee-Object -FilePath (Join-Path $runDir 'ping.log')
```

Așteaptă 30 s cu audio/ping corecte. x=B0, încă 30 s, x=B1. Fără HTTP.
Confirmă TX_CONFIG captured1/result0/type1/dynamic32/cache4/static0, PSRAM
disponibilă, SW ready1 și zero skipped/overflow/unpaired/unexpected, zero
HTTP/NO_MEM/failures. quota_passed, k1 și SW_MALLOC calls/ok cresc împreună;
k3 crește, accepted/refs/frees progresează și sunt echilibrate după repaus.
LWIP_MEMORY poate rămâne psram0 în baseline: pingul mic nu este controlul
pozitiv al alocărilor mari. Normal PASS nu validează ramurile NULL/NO_MEM.
Orice abatere: STOP înainte de HTTP și predă logurile.

## 3. Prima cerere root și capturi

Terminal HTTP nou. Fără browser, polling sau alte rute. Nu repeta blocul pentru
a reseta numărul cererii. La prima abatere încetează cererile, dar păstrează
loggingul și capturile fără recovery.

```powershell
$runDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.11_PSRAM\TEST_01'
Set-Location $runDir
$rootNumber = 0
"session_start=$((Get-Date).ToString('o')); 1.1.11; manual install" | Tee-Object -FilePath session-notes.log
```

x=P0 imediat înainte. Pentru fiecare cerere autorizată mai jos:

```powershell
$rootNumber++
"root=$rootNumber start=$((Get-Date).ToString('o'))" | Tee-Object -FilePath root-trigger.log -Append
curl.exe --http1.1 --max-time 10 --dump-header "root-$rootNumber.headers" --output "root-$rootNumber.html" --write-out 'http=%{http_code} bytes=%{size_download} time=%{time_total}\n' http://192.168.1.129/ 2>&1 | Tee-Object -FilePath root-trigger.log -Append
$curlExit = $LASTEXITCODE
"exit=$curlExit end=$((Get-Date).ToString('o'))" | Tee-Object -FilePath root-trigger.log -Append
if (Test-Path "root-$rootNumber.html") { Get-Item "root-$rootNumber.html" | Select-Object Name,Length | Out-File root-trigger.log -Append }
```

După prima cerere: x imediat, +5/+30/+60/+90 s de la cerere, fără alte HTTP.
Notează timpii reali și audio; nu aștepta terminarea curl pentru primul x dacă
incidentul apare mai devreme. Succesul cere curl exit0/HTTP200 și corp3617 B,
ping continuu, audio corect, zero NO_MEM/eșecuri DMA și instrumentare validă.
Confirmă creșterea LWIP_MEMORY psram/bytes și fallback0: altfel nu este dovedită
exercitarea politicii noi. Bytes sent din firmware nu demonstrează recepția.

**Incident:** T0 este prima pierdere ping, NO_MEM, allocation failure sau curl
incomplet. Notează ora/T0 în session-notes.log. x imediat, apoi T0+5/+30/+60/+90 s
(fără repetarea unei capturi deja făcute la acel timp). Nu mai trimite HTTP,
nu reconnect/reboot și nu închide/redeschide UART. La reset/watchdog, audio
afectat sau erori de instrumentare: STOP testul și păstrează bootul integral.

Pentru T0 sau orice observație, în terminalul HTTP:

```powershell
$note = 'T0: descrie primul semn; noteaza ora reala daca deja a trecut'
"$((Get-Date).ToString('o')) $note" | Tee-Object -FilePath session-notes.log -Append
```

La fiecare x notează eticheta/timpul efectiv în același mod. Comanda serială x
se tastează în monitor; nu deschide o a doua conexiune COM5 pentru capturi.

## 4. Numai dacă prima cerere și fereastra de 90 s trec

Repetă blocul curl pentru încă 9 cereri root, la cel puțin 5 s distanță;
x după fiecare. Oprește imediat la orice abatere și urmează ramura incident.
Apoi 10 minute Bluetooth/ping, o cerere root la fiecare minut și x după ea.
La final x, așteaptă30s fără HTTP, încă un x. Nu schimba volumul/modul/audio,
cache-ul sau rețeaua în aceeași probă. Testul controlat precedă folosirea normală
a browserului; browserul poate fi validat ulterior într-o sesiune separată.

## 5. Interpretare și fișiere de predat

| Rezultat | Interpretare |
|---|---|
| PSRAM crește, corp primit, ping/audio/TX continuă, zero DMAfail/NO_MEM | Susține evitarea presiunii declanșatoare în test; validare hardware a candidatului, nu recuperare dovedită după DMAfail |
| PSRAM nu crește | Politica nu a fost exercitată; nu atribui succesul mutării bufferelor |
| fallback crește | PSRAM/politica nu a susținut toate alocările; separă caps externe de eșecurile interne |
| DMAfail/NO_MEM persistă cu PSRAM utilizată | Corecție insuficientă; păstrează probele pentru ceilalți consumatori/contextul DMA |
| paired crește împreună cu k0 | Ramura statică headNULL observată în același apel refuzat; nu inventar complet sau leak demonstrat |
| preheap_null crește | Conversie refuzată înainte de allocatorul observat, compatibilă cu gate/quota; nu lipsă heap demonstrată |
| after_malloc_null și SW_MALLOC null cresc | NULL efectiv al allocatorului conversiei; corelează caps/task/trace |
| k1/k2/conversii cresc după eșec | Există noi încercări/requeue; stackul stabilește dacă vin prin TX-done, nu doar numărul |
| NO_MEM crește, accepted/refs/frees/k1 rămân fixe | Persistență a lipsei progresului observat; patru referințe nu dovedesc singure cauza privată |
| frees cresc și referințele dispar | Eliberare/progres observat; intrarea k3 singură nu dovedește finalizarea |

Contoarele din snapshot se copiază în momente diferite: diferențele de1 între
paired și NO_MEM nu sunt automat unpaired. Urmele sunt limitate la primele4 și
ultimele4; lipsa unei urme nu demonstrează lipsa tuturor retry-urilor.

Predă întregul TEST_01: serial din boot cu toate x/WEB_RESPONSE/SW_TRACE și
ALLOC_FAILURE, ping.log, root-trigger.log, toate root-N.headers/html existente,
manifest/hashuri, session-notes.log cu T0/timpi, audio și orice reset/abatere.
Nu elimina logurile eșecului. Nu putem declara rezolvarea durabilă înainte de
aceste rezultate și validarea ulterioară a utilizării normale.

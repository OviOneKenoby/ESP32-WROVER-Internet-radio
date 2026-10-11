# 1.1.10 UART — reproducere HTTP după controlul normal confirmat

**Incident OPEN. Aceeași imagine1.1.10-uart-validation / a7a31ef9afc7 și același
ELF. Nu recompila și nu reflasha.** CH340/COM5, fără JTAG/browser/polling.
[Controlul B0/B1](CONTROL_B0_B1_RO.md) validează numai traseul normal.
Ramurile NO_MEM/NULL vor fi evaluate separat prin această probă.

Întrebări: (A) refuzul NO_MEM este asociat în același apel TX cu ramura statică
headNULL, selectată tip9 în configurația efectivă? (B) după primele eșecuri DMA
mai există dequeue/conversii/requeue, și refuzul conversiei apare înainte de
allocator ori după un NULL efectiv? (C) ce progresează după recuperarea heapului?
Nu citim inventarul privat și nu tratăm ameliorarea simptomului drept root cause.

## 1. Fișiere noi și identificarea imaginii — înainte de monitor

Terminal pregătire/HTTP, PowerShell. Acest director de sesiune se creează
**o singură dată**. Dacă există deja, STOP și alege02 în loc de01 în TOATE
blocurile; nu șterge și nu reutiliza directorul. Logurile B0/B1 rămân intacte.

```powershell
$candidateDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.10_UART'
$runDir = Join-Path $candidateDir 'HTTP_REPRO_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$elf = Join-Path $candidateDir 'firmware-1.1.10-uart-a7a31ef9afc7.elf'
$addr2line = 'C:\Users\Dell\.platformio\packages\toolchain-xtensa-esp32\bin\xtensa-esp32-elf-addr2line.exe'
if (Test-Path -LiteralPath $runDir) { throw 'STOP: run directory exists; choose a new suffix everywhere' }
foreach ($line in Get-Content (Join-Path $candidateDir 'SHA256SUMS.txt')) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'STOP: invalid manifest' }
    $expectedHash = $Matches[1]
    $artifactPath = Join-Path $candidateDir $Matches[2]
    if ((Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash -ne $expectedHash) {
        throw "STOP: hash mismatch $artifactPath"
    }
}
New-Item -ItemType Directory -Path $runDir -ErrorAction Stop | Out-Null
Set-Location $runDir
Copy-Item -LiteralPath (Join-Path $candidateDir 'SHA256SUMS.txt'), (Join-Path $candidateDir 'BUILD_MANIFEST.md') -Destination $runDir
"session_start=$((Get-Date).ToString('o')); CH340 COM5; no reflash" | Tee-Object -FilePath .\session-notes.log
& $pio device list
$rootNumber = 0
```

STOP pentru hash greșit/lipsă COM5. În terminalul HTTP păstrează rootNumber;
nu îl reinițializa între cereri. Niciuna din comenzile de mai jos nu face flash.

## 2. Redeschidere UART și verificarea bootului

Terminal UART separat; definește propriile variabile, apoi monitor:

```powershell
$candidateDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.10_UART'
$runDir = Join-Path $candidateDir 'HTTP_REPRO_01'
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
if (-not (Test-Path -LiteralPath $runDir)) { throw 'STOP: preparation directory missing' }
Set-Location $runDir
& $pio device monitor --port COM5 --baud 115200 --dtr 0 --rts 0 --no-reconnect --filter time --filter log2file
```

Opțiunile dtr/rts0 și no-reconnect există în PIO6.2.0; reduc anumite tranziții,
**nu garantează absența resetului la deschiderea CH340**. log2file scrie în
HTTP_REPRO_01/logs/device-monitor-*.log, separat de logul B0/B1.
Nu apăsa EN și nu trimite l dacă Bluetooth este deja activ.

Tastează imediat `x`: S0. Păstrează bannerul/mesajele de boot dacă apar.
Verifică versiunea/ID, reset_reason, uptime, evenimentele Wi-Fi și contoarele.

- **Același boot plauzibil:** uptime continuă, contorii nu scad față deB1
  (quota/malloc/k1≥65, k3≥130, accepted/refs/frees≥63), last GOT_IP12518ms
  și evenimentele de boot rămân coerente, fără banner/reset nou.
  Compară și timpul scurs de laB1, nu numai uptime>131087.
  B1 era la2026-10-11T15:12:24.172+03:00, uptime131087ms; originea estimată
  a millis este2026-10-11T15:10:13.085+03:00. Din ora liniei identitățiiS0
  scade uptimeS0: originea trebuie să corespundă aproximativ (toleranță5s).
  Un reset vechi de câteva minute poate avea uptime mai mare decâtB1 și tot
  este alt boot. reset_reason=power_on singur nu demonstrează continuitatea.
- **Boot nou după redeschidere sau continuitate incertă:** banner nou,
  uptime/counter restart, altă origine estimată, evenimente resetate ori
  ceas PC ajustat care împiedică asocierea. Consemnează în session-notes.log
  „new boot” sau „uncertain”, ora și reset reason. Nu reflasha/reporni încă o dată.
  Pe boot nou lasă setup/Wi-Fi să termine; dacă nu este în Bluetooth, trimite
  `l` numai din lista principală, împerechează și pornește SBC. Acesta este
  un baseline nou, nu continuarea stării B1. Dacă logul începe după reset,
  marchează că începutul bootului nu a fost capturat.
- Dacă S0 arată deja HTTP/failures/NO_MEM sau ping indisponibil înainte de
  trigger, STOP pentru reproducerea controlată. Păstrează x/logurile; nu face
  restart/reconnect pentru a obține un baseline curat.

Notează în terminalul HTTP, înlocuind textul cu observația reală:

```powershell
"$((Get-Date).ToString('o')) boot_assessment: same/new/uncertain; S0 uptime and reset_reason: see serial" | Tee-Object -FilePath .\session-notes.log -Append
```

## 3. Ping continuu și baseline proaspăt înainte de HTTP

Terminal ping separat (schimbă IP-ul dacă S0 raportează altul):

```powershell
$candidateDir = 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.10_UART'
$runDir = Join-Path $candidateDir 'HTTP_REPRO_01'
Set-Location $runDir
if (Test-Path -LiteralPath .\ping-http.log) { throw 'STOP: ping log exists; do not overwrite' }
ping.exe -t 192.168.1.129 | ForEach-Object { '{0:o} {1}' -f (Get-Date), $_ } | Tee-Object -FilePath .\ping-http.log
```

După30s de ping/audio funcționale: `x`=P0. După încă10s: `x`=P1.
Aceste capturi sunt obligatorii atât pe același boot, cât și pe boot nou,
pentru a verifica starea actuală după întreruperea loggingului.
Notează orele reale/uptime P0/P1; nu deduce continuitatea din logurile vechi.

În ambele: versiune/ID corecte; Wi-Fi/IP disponibile; BT active/SBC playing,
audio audibil corect; TX_CONFIG captured1/result0/type1/dynamic32/cache4/static0/
features cachebit0x2; HTTP0, allocation failures0, NO_MEM0, other_error0;
ready1, skipped/scope_overflow/paired/unpaired/unexpected0;
preheap_null/after_malloc_null/conversion unpaired/malloc null0;
ledger_overflow/unmatched_free0, fără retained refs raportate.
P0→P1: quota_passed, malloc calls/ok, k1/k3 și accepted/ref/free cresc,
refs/frees echilibrate; ping fără timeout și audio corect.
Nu substitui k4 global lui quota_passed; grupurile nu sunt copiate atomic.
Un delta neașteptat sau un ledger nenul nu se declară PASS automat.

**Dacă oricare condiție lipsește: STOP fără HTTP**, păstrează logurile și x.
Pe boot nou, P0/P1 validează din nou numai traseul normal. Dacă toate trec,
etapa HTTP de mai jos este permisă de acest protocol; nu modifică imaginea.

## 4. Cererea root și T0

Terminalul HTTP inițial, același rootNumber, IP-ul verificat dinS0/P1.
Execută blocul o singură dată pentru prima cerere:

```powershell
if ($rootNumber -ge 3) { throw 'STOP: maximum three requests reached' }
$rootNumber++
$headers = Join-Path $runDir "root-$rootNumber.headers"
$body = Join-Path $runDir "root-$rootNumber.html"
if ((Test-Path -LiteralPath $headers) -or (Test-Path -LiteralPath $body)) { throw 'STOP: request files already exist' }
"$((Get-Date).ToString('o')) request=$rootNumber start" | Tee-Object -FilePath .\root-trigger.log -Append
curl.exe --noproxy '*' --http1.1 --retry 0 --connect-timeout 3 --max-time 10 --max-redirs 0 --dump-header $headers --output $body --write-out 'http=%{http_code} bytes=%{size_download} time=%{time_total}\n' http://192.168.1.129/ 2>&1 | Tee-Object -FilePath .\root-trigger.log -Append
$rootExit = $LASTEXITCODE
"$((Get-Date).ToString('o')) request=$rootNumber curl_exit=$rootExit end" | Tee-Object -FilePath .\root-trigger.log -Append
```

Fără -L, browser, JS, polling sau alte rute (nici /api/diagnostics).
Nu repeta blocul automat. Păstrează UART și ping vizibile concomitent;
nu aștepta terminarea celor10s de curl pentru snapshotul imediat.

**T0 = ora primei observații a incidentului:** primul timeout/unreachable de
ping, primul NO_MEM/allocation failure observat (inclusiv WEB_RESPONSE alloc_fail),
sau eșecul curl/incompletitudinea răspunsului dacă nu exista un semnal anterior.
Notează exact ce semnal a definitT0. Timeoutul Windows și afișarea UART întârzie
observarea; tickul unei ALLOC_FAILURE poate fi anterior. Nu pretinde căT0 este
momentul fizic exact al defecțiunii; păstrează și tickurile pentru cronologie.
Un eșec curl singur nu dovedește mecanismul TX și poate fi un incident de alt tip.

- LaT0: STOP imediat cererilor suplimentare; `x` imediat în UART. Cererea în
  curs se termină natural/max10s, fără retry. Ping/audio continuă.
- Dacă cererea a reușit (curl_exit0, HTTP200, fără răspuns incomplet): `x`
  post-cerere; verifică ping/audio și contorii. Alt status sau răspuns incomplet
  oprește cererile, chiar dacă exitul curl este0; consemnează-l separat.
  Așteaptă minimum5s, apoi `x` înaintea eventualei cereri următoare. Nu continua
  dacă există fail/NO_MEM, erori de instrumentare, retained refs persistente ori
  lipsă de progres. Poți repeta același bloc cel mult încă de două ori, numai
  cât timp nu apare incidentul și starea rămâne sănătoasă.
- După trei cereri fără incident: `x` final, salvează logurile și STOP.
  Non-reproducerea cu curl în candidatul instrumentat nu înseamnă fix sau
  infirmarea incidentului browser/stock.

## 5. Capturi după T0 — fără HTTP și fără recovery

`x` imediat, apoi **T0+5s, +30s, +60s, +90s**. Timpii sunt relativi laT0,
nu pauze cumulate între capturi. Notează ora reală și uptime din fiecare x.
Fereastra90s depășește intervalul84,314s de persistență1.1.7.

În terminalul HTTP poți consemna T0 și momentele reale fără a trimite date
la placă; editează eticheta fiecărei note, nu executa HTTP:

```powershell
"$((Get-Date).ToString('o')) T0/capture label and observed signal: fill from UART/ping" | Tee-Object -FilePath .\session-notes.log -Append
```

Fără l, schimbarea sursei/volumului/playbackului, reconnect, reboot, EN,
alte cereri sau curățarea contoarelor. Dacă pingul revine singur, continuă
capturile până la+90s și notează momentul revenirii, fără cerere de verificare.
Un snapshot nu este freeze global; costul afișării și evoluția între copiile
contorilor trebuie luate în calcul.

Reset/watchdog/brownout, UART pierdut, identitate/config schimbată, audio
defect înainte de HTTP ori erori de instrumentare invalidează concluziile
negative dependente de acoperire. Oprește cererile, salvează ultimul x dacă
placa răspunde serial și încheie; nu redeschide monitorul repetat ca recovery.
Pierderea pingului după HTTP este semnalul de capturat, nu motiv de reboot.
Un reset în fereastră întrerupe proba de persistență; nu combina contoare din
booturi diferite. Nu continua experimentul după reset cu alte root-uri.

După ultimul x, salvează logurile; oprește monitorul/ping cu Ctrl+C. Închiderea
monitorului poate și ea afecta liniile de control: orice reset la închidere este
după fereastra observată și trebuie notat, nu folosit drept recuperare a testului.

## 6. Fișiere de predat și decodare exactă

Predă întregul HTTP_REPRO_01, incluzând:

- logs/device-monitor-*.log de la redeschidere până la final, toate x S0/P0/P1/
  post-cerere/T0/+5/+30/+60/+90, toate SW_TX/SW_SITE/SW_CONVERT/SW_MALLOC/
  SW_TRACE/SW_PC, ALLOC_FAILURE, TX_CONFIG/TX_NO_MEM/TX_RETAINED_REF/NET_PROGRESS;
- ping-http.log cu ore; session-notes.log cu verdict boot, ora/reset/uptime,
  audio, T0 și semnalul său, timpii efectivi și orice anomalie;
- root-trigger.log, fiecare root-N.headers/html existent (inclusiv parțial);
- SHA256SUMS.txt și BUILD_MANIFEST.md, decoded-pcs.txt dacă există PC-uri.

Logurile B0/B1 originale rămân separat, nu se suprascriu. Dacă s-a păstrat
același boot, combinația dintre serialul original și noul serial are un interval
neobservat: descrie-l, nu pretinde log continuu. Dacă s-a resetat, separă booturile.
Absența packet capture nu anulează probele directe; RX total nu înseamnă că
toate pachetele sunt noi echo requests sau au fost răspunse.

După închiderea monitorului, în terminalul HTTP inițial:

```powershell
$serialFiles = @(Get-ChildItem -LiteralPath (Join-Path $runDir 'logs') -Filter 'device-monitor-*.log')
if ($serialFiles.Count -ne 1) { throw 'STOP: expected one new serial log; select and explain multiple logs manually' }
$serialLog = $serialFiles[0].FullName
if (-not (Select-String -LiteralPath $serialLog -SimpleMatch 'build_git_id=a7a31ef9afc7')) { throw 'STOP: wrong or missing serial identity' }
$pcMatches = Select-String -LiteralPath $serialLog -Pattern '0x[0-9a-fA-F]{8}' -AllMatches
$pcs = @($pcMatches | ForEach-Object { $_.Matches.Value } | Where-Object { $_ -match '^0x40' } | Sort-Object -Unique)
if ($pcs.Count -gt 0) {
    if (Test-Path -LiteralPath .\decoded-pcs.txt) { throw 'STOP: decode file exists; do not overwrite' }
    & $addr2line -pfiaC -e $elf @pcs | Tee-Object -FilePath .\decoded-pcs.txt
}
```

Folosește exclusiv ELF1.1.10a7a31ef9afc7 cu hash1E6E9EDCBBA406A370E4F86108F6A3B14BB6EA1FC301A2FF0A8FAF80662E909A.
SW_TRACE site este adresă de revenire; SW_PC și ALLOC_FAILURE sunt PC-uri
normalizate de call. Păstrează raw chiar dacă bibliotecile private nu au linii
sursă. Clasificatorul folosește PCcall normal0x4023d6f7 și WAPI0x4023d6e9;
nu refolosi adresele/interpretările clasificatorului defect1.1.9.

## 7. Interpretarea discriminatorie

k0=lista statică headNULL; k1=dequeue pending; k2=requeue după conversieNULL;
k3=pending empty; k4=quota-unlock trecut, toate contextele; k5=ieșire dinamică
NULL observabilă la unlock. quota_passed/malloc clasifică contextul output_process.
Un bypass cu limită localăzero poate evita anumite unlock-uri; nu interpreta
absența lor drept absența tuturor funcțiilor. Urmele rare păstrează primele4/
ultimele4, nu fiecare eveniment; contorii nu au limita8.

|Rezultat|Ce susține și limite|
|---|---|
|NO_MEM și paired cresc, k0 observat în același scope, config/urme valide|Un singur headNULL a fost observat în apelul TX STA by-ref care returnează NO_MEM. Traseul static/config selectează tip9; acesta este refuz la lipsă de intrare liberă în acel moment, nu inventar direct, leak sau cauza persistenței. Numărul egal de hituri k0/NO_MEM singur nu este asociere.|
|NO_MEM fără paired ori unpaired/unexpected/overflow/skipped|Nu atribui automat poolului; altă ramură/context/acoperire insuficientă. Decodează urmele și păstrează observațiile pozitive locale.|
|k1/k2 și preheap_null cresc, malloc/failures nu cresc, urme normale complete|Există retry/conversii care returnează NULL înaintea allocatorului observat; traseul verificat susține gate-ul de cotă. Nu ghici valoarea cotei sau inventarul. Verifică pairing/contextul înaintea concluziei.|
|k1/k2 și malloc null/after_malloc_null cresc|Retry ajunge la allocator și primește NULL; corelează mărimea/caps/tickul din ALLOC_FAILURE. Heapul recuperat ulterior nu este heapul la acel apel. Hook absent impune investigarea discrepanței, nu presupunerea unei epuizări private.|
|k1/malloc ok/frees cresc și accepted revine|Progresul conversiei/eliberării/TX revine. Dacă pingul nu revine, problema poate fi în aval; nu considera recuperarea demonstrație de fix.|
|NO_MEM/paired cresc, aceleași retained pbuf/tickuri, accepted/refs/frees și k1/k2/k3/malloc fixe, RX crește|Lipsă persistentă de progres la frontierele observate; compatibilă cu încetarea triggerului de retry identificat static. Nu exclude așteptarea înaintea siturilor sau alt retry privat și nu dovedește blocarea unei cozi/leak.|
|k3 crește, dar retained rămân și k1 nu crește|Pending-empty este revizitat: lipsa globală a dispatchului nu explică singură retenția. Lista/inventarul/asocierea rămân de investigat.|
|k1 crește fără malloc/k2, retained persistă|Dequeue este atins; traseul ulterior nu progresează sau nu este acoperit. Nu declara absența retry-ului; inspectează urmele.|
|refs−frees și aceleași pbuf persistă|Referințe netstack reținute la limita instrumentată; nu echivalează automat cu toate descriptorii cache sau un leak. ledger_overflow/unmatched_free invalidează inventarul derivat.|
|body_sent=expected, fără socket error|Octeți acceptați de API; nu demonstrează ACK/livrarea la client. Coroborează cu exit/status/bytes curl și fișierul primit.|
|Flaguri zero, dar stive lipsă/incomplete ori ramură neexercitată|Controlul normal nu validează cazurile rare. Nu trage concluzii negative preheap/pool din absența unei urme valide.|

Perturbări neschimbate: +72B DRAM față de1.1.9, cost de încărcări volatile/
apeluri IRAM/unwind12frame, relink și UART; mutexul original se eliberează
înainte de observație, fără logging/alocări de diagnostic în callback.
Nu citim liste private și nu schimbăm driver/retry/TX dynamic32/cache4/root,
nu dezactivăm webul în Bluetooth și nu adăugăm gzip sau reconnect/reboot.
Fără merge/flash automat. Incident OPEN până la demonstrația cauzală și un
fix validat separat; acest test colectează dovezi, nu aplică un fix.

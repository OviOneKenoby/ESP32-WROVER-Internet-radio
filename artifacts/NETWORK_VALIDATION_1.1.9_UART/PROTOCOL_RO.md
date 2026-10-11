# Protocol unic complet — UART, refuz TX și retry/conversie

**SUSPENDAT după controlul pozitiv al proprietarului. Nu declanșa HTTP.**
ELF-ul exact conține un operand incorect în clasificatorul de context din
unlock; controlul original nu este PASS. Vezi
[analiza discrepanței](BASELINE_DISCREPANCY_RO.md). Instrucțiunile de mai jos
rămân istorice până la corectarea și verificarea predicatului final.

Candidatul corectat și prima etapă **numai B0/B1, fără HTTP** sunt în
[NETWORK_VALIDATION_1.1.10_UART](../NETWORK_VALIDATION_1.1.10_UART/PROTOCOL_RO.md).

**1.1.9-uart-validation / fe228ca737b9. Candidat de diagnostic, incident OPEN.**
Necesită numai CH340/COM5. Fără JTAG, recablare sau dezactivarea encoderului.
Nu este un fix: TX dynamic32/cache4, retry, payload root și comportamentul
driverului rămân aceleași. Agentul nu execută flash, reset sau merge.

Întrebarea discriminatorie: NO_MEM STA provine din ramura de listă statică
goală selectată ca tip9? După conversiile inițiale eșuate mai ajunge execuția
la dequeue/conversie/requeue, și NULL apare înainte sau după un apel malloc?

Metoda și limitele sunt în FEASIBILITY_RO.md. Probele folosesc numai trei
callbackuri OSI cu ABI declarat; codul privat este observat după punctele sale
de apel, fără patchuri, citirea listelor private sau invocarea funcțiilor private.

## 1. Pregătire, înainte de orice instalare

Salvează logul bootului1.1.7/incidentului anterior și ultimul x, fără alte HTTP.
Nu repeta1.1.6/1.1.7. Instalarea manuală de mai jos începe un experiment nou.
Închide terminalul care ocupă COM5 numai după salvarea logului.

În FIECARE terminal PowerShell folosit mai jos, inițializează astfel:

```powershell
Set-Location 'C:\Users\Dell\Downloads\TUNER01_Arduino_builder\TUNER01_Arduino\investigation\artifacts\NETWORK_VALIDATION_1.1.9_UART'
$candidateDir = (Get-Location).Path
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$py = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
$espTool = 'C:\Users\Dell\.platformio\packages\tool-esptoolpy\esptool.py'
$addr2line = 'C:\Users\Dell\.platformio\packages\toolchain-xtensa-esp32\bin\xtensa-esp32-elf-addr2line.exe'
$bin = Join-Path $candidateDir 'firmware-1.1.9-uart-fe228ca737b9.bin'
$elf = Join-Path $candidateDir 'firmware-1.1.9-uart-fe228ca737b9.elf'
$rootNumber = 0
Get-Content .\BUILD_MANIFEST.md
Get-Content .\SHA256SUMS.txt
foreach ($line in Get-Content .\SHA256SUMS.txt) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'STOP: invalid manifest' }
    $expectedHash = $Matches[1]
    $artifactPath = Join-Path $candidateDir $Matches[2]
    $actualHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash
    if ($actualHash -ne $expectedHash) { throw "STOP: hash mismatch $artifactPath" }
}
& $pio device list
```

STOP pentru hash/identitate greșită, lipsa COM5 sau partiție app0 diferită
de offset0x10000 din configurația verificată. Nu schimba frameworkul, NVS,
cache-ul ori partițiile pentru acest test. Nu se dezactivează politica PowerShell;
comenzile sunt inline, nu fișierps1. Nu folosi upload implicit/default env.

Numai instalare MANUALĂ, la alegerea proprietarului, după verificările de mai sus:

```powershell
& $py $espTool --chip esp32 --port COM5 --baud 460800 write_flash 0x10000 $bin
```

Această comandă resetează pentru instalarea noului experiment; nu este
recuperare automată a incidentului. Nu executa erase_flash sau flash complet.

## 2. Loguri și control pozitiv, fără HTTP

Terminal UART, din directorul candidat (rămâne deschis până la final):

```powershell
& $pio device monitor --port COM5 --baud 115200 --filter time --filter log2file
```

Păstrează serialul integral de la boot. Dacă monitorul a fost deschis după boot,
apasă manual EN/Reset o singură dată acum, înainte de baseline, pentru captură
completă. Nu reporni după T0. Tastează `l`, împerechează Bluetooth și
pornește audio SBC; nu schimba sursa pe durata experimentului. Verifică firmware
1.1.9-uart-validation /fe228ca737b9. Comanda `x` rămâne snapshot serial read-only;
nu folosi /api/diagnostics sau alt HTTP pentru capturi.

Terminal ping separat, cu blocul de variabile de mai sus executat:

```powershell
ping.exe -t 192.168.1.129 | ForEach-Object { '{0:o} {1}' -f (Get-Date), $_ } | Tee-Object -FilePath .\ping.log
```

Dacă IP-ul serial este diferit, folosește IP-ul serial atât la ping cât și la
curl. Așteaptă30s cu ping/audio funcționale. `x` = baselineB0. După10s de ping,
încă un `x` = controlB1. Ambele trebuie să aibă:

- web_response_count0, alloc_failures0, TX_CONFIG captured1/result0/type1/
  dynamic32/cache4/static0/cache feature0x2;
- SW_TX ready1, skipped0, scope_overflow0, unexpected0;
- ledger_overflow0/unmatched_free0, pbuf ledger gol;
- SW_CONVERT unpaired0; TX accepted și refs/frees cresc, ping/audio funcționale;
- întreB0/B1 cresc k1 (dequeue), k3 (pending empty), quota_passed și
  SW_MALLOC calls/ok. Acesta este controlul pozitiv pentru rutarea reală,
  adresele normalizate și unwind-ul conversiei.

Lipsa oricărei condiții: STOP fără HTTP; salvează logurile. Un build verificat
static nu substituie acest control. Raportează heap/DMA/stack HWM baseline,
deoarece instrumentarea consumă DRAM și timp CPU suplimentar.

## 3. Declanșare controlată și condiții de oprire

În al treilea terminal PowerShell, cu blocul inițial executat:

```powershell
if ($rootNumber -ge 3) { throw 'STOP: maximum three root requests' }
$rootNumber++
"request=$rootNumber" | Tee-Object -FilePath .\root-trigger.log -Append
Get-Date -Format o | Tee-Object -FilePath .\root-trigger.log -Append
curl.exe --http1.1 --max-time 10 --dump-header ".\root-$rootNumber.headers" --output ".\root-$rootNumber.html" http://192.168.1.129/ 2>&1 | Tee-Object -FilePath .\root-trigger.log -Append
$rootExit = $LASTEXITCODE
"curl_exit=$rootExit" | Tee-Object -FilePath .\root-trigger.log -Append
Get-Date -Format o | Tee-Object -FilePath .\root-trigger.log -Append
```

O singură cerere root, fără browser/polling JavaScript. Observă UART și ping
în paralel; nu aștepta finalul timeoutului curl ca să captezi incidentul.

- T0 = prima pierdere ping, primul NO_MEM sau allocator failure. Oprește
  imediat toate cererile suplimentare. Nu trimite încă o cerere pentru verificare.
- Dacă prima cerere rămâne sănătoasă, `x`, așteaptă minimum5s, apoi maximum
  încă două cereri identice, cu `x` între ele, repetând blocul curl în același
  terminal. Fișierele root-1/2/3.headers/html păstrează rezultatele separat.
  Nu reinițializa rootNumber între cereri și nu depăși trei cereri.
- După trei cereri sănătoase: `x` final și STOP. Non-reproducerea cu curl și
  instrumentare este inconcludentă pentru incidentul stock/browser; nu este fix.
- Reset/watchdog/brownout, audio defect înainte de HTTP, control invalid,
  identitate/config greșită, skipped/scope_overflow/unpaired sau stivă nevalidă:
  STOP pentru concluziile care depind de acea acoperire. Păstrează observațiile
  pozitive valide, fără a interpreta absența drept dovadă.

## 4. Capturi după T0, fără cereri noi sau recovery

`x` imediat laT0, apoi laT0+5s, +30s, +60s, **+90s**. Pingul și audio continuă.
Nu trimite `l`, nu reconecta Wi-Fi/Bluetooth, nu reporni și nu schimba playback.
Fereastra90s depășește persistența84,314s deja demonstrată în1.1.7.
Notează ora reală și uptime pentru fiecare captură; întârzierile sunt raportate,
nu ascunse. Notează dacă pingul își revine singur și momentul revenirii.

După ultimul x, oprește monitorul și pingul cu Ctrl+C și păstrează toate
fișierele. Nu se cere restartul stării eșuate. La STOP invalid salvează ultimul
x dacă targetul funcționează, apoi încheie; nu încerca reparații în același boot.

## 5. Loguri și decodare cu ELF-ul EXACT

Necesare: serial complet boot→B0/B1→trigger→toate x până la+90s; ping.log cu ore;
root-trigger.log și headers/html existente pentru fiecare cerere; manifest,
SHA256SUMS, adaptor CH340/COM5, oreleT0, audio și orice eroare/non-reproducere.
Include toate SW_TX/SW_SITE/SW_CONVERT/SW_MALLOC/SW_TRACE/SW_PC, ALLOC_FAILURE,
TX_CONFIG, TX_RETAINED_REF, TX_NO_MEM și NET_PROGRESS, nu numai totalurile.

După închiderea monitorului, decodare completă a tuturor PC-urilor existente
din cel mai recent log serial generat în logs de log2file:

```powershell
$serialFile = Get-ChildItem -LiteralPath (Join-Path $candidateDir 'logs') -Filter 'device-monitor-*.log' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $serialFile) { throw 'STOP: serial log missing' }
$serialLog = $serialFile.FullName
if (-not (Select-String -LiteralPath $serialLog -SimpleMatch 'build_git_id=fe228ca737b9')) { throw 'STOP: wrong serial build identity' }
$serialLog
$pcMatches = Select-String -LiteralPath $serialLog -Pattern '0x[0-9a-fA-F]{8}' -AllMatches
$pcs = @($pcMatches | ForEach-Object { $_.Matches.Value } | Where-Object { $_ -match '^0x40' } | Sort-Object -Unique)
& $addr2line -pfiaC -e $elf @pcs | Tee-Object -FilePath .\decoded-pcs.txt
```

Păstrează logul brut chiar dacă unele adrese nu au linii sursă în bibliotecile
private. Nu substitui ELF1.1.7/1.1.8 sau un ELF recompilat cu alt build ID.
`site` SW_TRACE este adresă de retur; SW_PC și ALLOC_FAILURE sunt PC de call.

## 6. Interpretare pentru fiecare ramură de rezultat

SW_SITE: k0=listă statică goală; k1=dequeue pending; k2=requeue după conversie
NULL; k3=pending gol; k4=cotă trecută (toate contextele); k5=ieșire dinamică
NULL (toate contextele). `n` este numărul cumulativ, `tick` ultima vizită.
SW_CONVERT și SW_MALLOC clasifică numai urme identificate output_process.
SW_TRACE reține primele4/ultimele4 contexte rare; contorii nu au limita8.
Fragmentele unui x nu sunt atomice împreună: Wi-Fi continuă în timpul afișării,
deci SW_TX și NET_PROGRESS pot diferi puțin între momentele copierii.

| Rezultat | Interpretare justificată / limită |
|---|---|
|NO_MEM și paired crește, k0 și stack valid alloc_tx_buf/output_do, config valid|Ramura listei statice goale a fost traversată în aceeași invocație STA/by_ref; fluxul static selectează tip9. Nu dovedește inventarul complet sau cauza nereciclării.|
|NO_MEM dar unpaired crește, mai multe hituri sau scope_overflow|Alt context/ramură sau asociere insuficientă. Nu atribui automat poolului cache; inspectează urma exactă.|
|După incident k1 și k2 cresc, preheap_null crește, malloc/failures nu cresc, urme normale tip1 valide|Există retry/conversii NULL refuzate înainte de allocator; traseul static verificat susține gate-ul de cotă. Nu este o lipsă pur temporală de heap. Nu ghici valoarea cotei/inventarul.|
|k1/k2 cresc și malloc null/after_malloc_null cresc|Retry ajunge efectiv la allocator și primește NULL. Corelează exact cu hookul eșuat/caps/timpi; largest DMA ulterior nu reprezintă heapul la apel. Dacă hookul nu crește, raportează discrepanța și inspectează callbackul decodat.|
|k1 și malloc ok cresc, frees/accepted revin|Conversia/alocatorul/eliberarea progresează. Dacă rețeaua tot nu revine, următoarea etapă este în aval. Dacă revine, este recuperare observată în candidatul perturbat, nu fix demonstrat.|
|NO_MEM/paired cresc, retained rămân aceleași, k1/k2/k3 și malloc rămân fixe, RX crește, control pozitiv valid|Lipsă de vizite/progres la punctele conversiei selectate în fereastră, compatibilă cu lipsa declanșării retry-ului după încetarea admiterilor/TX-done calificat. Nu exclude o așteptare înaintea lor sau un alt mecanism privat; nu demonstrează coadă blocată/leak.|
|k3 crește dar aceleași pbuf rămân reținute și k1 nu crește|Dispatcherul revizitează punctul pending-empty; lipsa declanșării nu explică singură retenția. Inventarul/starea/lista ori asocierea sunt de investigat; nu eticheta leak.|
|k1 crește, k2/malloc nu cresc și pbuf rămân|Execuția atinge dequeue, dar traseul urmărit ulterior nu progresează/nu este acoperit. Nu declara absența retry-ului; inspectează toate urmele/contextul.|
|skipped/unpaired/stivă fără context sau control pozitiv absent|Nu se justifică concluzii negative/preheap/pool din acoperirea incompletă. Un hit pozitiv de ramură rămâne o observație locală, în limitele contextului valid.|
|Ping revine singur ori nu reproduce|Rezultat temporal diferit/perturbat; raportăm timpi și contori. Nu închidem incidentul și nu afirmăm că proba a reparat driverul.|

Instrumentarea adaugă DRAM, cost de callback/unwind și timp UART, schimbă layoutul
la relink; valorile exacte sunt în BUILD_MANIFEST. Nu se schimbă intenționat
comportamentul TX. Nicio ameliorare nu demonstrează root cause. Hardware NOT RUN
de agent; incidentul rămâne OPEN până la dovada cauzală și validarea unui fix.

# Variables only: no flash, reset, hardware connection or packet transmission.
$candidateDir = $PSScriptRoot
$pio = 'C:\Users\Dell\.platformio\penv\Scripts\platformio.exe'
$py = 'C:\Users\Dell\.platformio\penv\Scripts\python.exe'
$ocd = 'C:\Users\Dell\.platformio\packages\tool-openocd-esp32\bin\openocd.exe'
$gdb = 'C:\Users\Dell\.platformio\packages\tool-xtensa-esp-elf-gdb\bin\xtensa-esp-elf-gdb-3.11.exe'
$scripts = 'C:\Users\Dell\.platformio\packages\tool-openocd-esp32\share\openocd\scripts'
$env:PATH = 'C:\Users\Dell\.platformio\python3;' + $env:PATH

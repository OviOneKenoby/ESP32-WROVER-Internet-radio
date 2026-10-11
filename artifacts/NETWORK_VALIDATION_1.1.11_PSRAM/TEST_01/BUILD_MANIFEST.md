# 1.1.11 PSRAM — candidat corectiv, incident OPEN

Firmware **1.1.11-psram-validation / 124e84801b06**.
Sursă executabilă: 124e84801b06f9e07764dabc3985a573dffaff3b.
Mediu explicit esp32-lwip-psram; default esp32-dev și main nemodificate.
Arduino2.0.17/IDF4.4.7, espressif32@7.0.1,
framework3.20017.241212+sha.dcc1105b, Xtensa8.4.0+2021r2-patch5.

Build SUCCESS242.32s, RAM85936B, flash2022097B. DRAMdata31208B/BSS54728B,
IRAMtext128947B: față de1.1.10 +24B DRAM, +24B IRAM, +448B flash.
Nu este măsurat costul CPU/PSRAM sau heapul runtime pe hardware.
app0 offset0x10000/size0x300000; esptool4.11.0 image_info checksum/hash valid.

- BIN2028672B: firmware-1.1.11-psram-124e84801b06.bin.
  SHA256796221B3521800EB5D6E41F6932C81F600EA9F6E57E9BBC240640C9E8522DBF2.
- ELF29599964B: firmware-1.1.11-psram-124e84801b06.elf.
  SHA256C1BC29571B7AC59D12BAB006EC0761464CEBB4C76E7F14686606C73866F7C209.

## Justificare și verificări

O singură schimbare de politică: apelurile lwIP mem_malloc>=512B preferă
SPIRAM|8BIT caps0x404, fallback original. malloc global/mem_calloc, conversia
Wi-Fi DMA, TXdynamic32/cache4, retry și payloadul root nu sunt modificate.
Wrapperul este o funcție normală flash, ca mem_malloc original; nu pretinde
siguranță cache-off/ISR mai largă decât ABI-ul original. Preferința PSRAM este
activă numai cache-on și non-ISR. Contoarele se copiază/loghează în snapshot.

Sursa IDF4.4.7 mem.c are deja o opțiune oficială PSRAM-first pentru lwIP;
SDK-ul binar fixat o are dezactivată. Nu schimbăm macro-ul unei biblioteci
deja compilate: interceptăm simbolul public mem_malloc la link.
Verificarea ELF confirmă originalul malloc/free fără prefix de stats și
argument neschimbat; ESP-IDF free acceptă pointeri din heapurile capabilităților.
Guardurile compilate MEM_LIBC_MALLOC1/MEM_STATS0/MEM_ALIGNMENT<=4 limitează ABI.

- Testul actualului wrapper pe host:7 cazuri PASS (511/512, caps/mărime,
  fallback o singură dată, NULL, cache-off, ISR, zero).
- ELF: pbuf_alloc→wrapper, caps0x404 și predicat efectiv size>511,
  allocator PSRAM/fallback original câte un call-site, fără logging/heap query.
- Clasificator: operanzi finali și acord unlock/malloc normal/WAPI verificați;
  cazul eronat fe228ca737b9 și mutațiile independente sunt respinse.
- OSI/public TX: rutare/IRAM/original-first și cele6 funcții private normalizate
  neschimbate față de ELF exact1.1.7.

Nu citim inventarul privat și nu injectăm recovery. Noua politică are limite:
fallbackul sau alți consumatori pot recrea presiunea DMA; succesul nu demonstrează
repararea retry-ului după un eșec. Hardware/flash NOT RUN de agent.
[Protocol complet CH340/COM5](PROTOCOL_RO.md). Publicare numai pe ramura
investigației; PR rămâne nemerguit și incidentul OPEN.

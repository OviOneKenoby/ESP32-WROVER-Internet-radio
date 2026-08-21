# Web manager security review

## Current authentication state

The web manager has no application-layer authentication, authorization or CSRF
protection. Any client that can reach TCP port 80 can call its API. During
fallback setup, access is partially limited by the WPA2 setup-AP password, but
normal LAN operation relies entirely on the trustworthiness of the local
network. No OTA route exists today.

No route returns a saved Wi-Fi password. `/api/status` does return the connected
SSID and IP address. `/api/stations` returns saved station and Favorite URLs,
and `/api/diagnostics` returns runtime health and the current stream URL.

## State-changing endpoints

| Endpoint | Effect | Persistent storage | Authentication |
|---|---|---|---|
| `POST /api/wifi` | Tests and saves Wi-Fi SSID/password after connection | EEPROM | None |
| `POST /api/timezone` | Changes the POSIX timezone rule | NVS | None |
| `POST /api/stations` | Adds a saved station | NVS | None |
| `DELETE /api/station/{index}` | Deletes a saved station | NVS | None |
| `DELETE /api/favorite/{index}` | Removes a Favorite | NVS | None |

The browser UI calls these routes directly over HTTP. An attacker on the same
reachable network could therefore change configuration without possessing the
radio or knowing its setup-AP password.

## Minimum design required before web OTA

Before an OTA upload or download endpoint is added:

1. Provision a unique random secret per radio. Store it once in NVS and reveal
   it only through a deliberate physical/serial setup flow; never compile a
   shared password into public firmware.
2. Protect every state-changing route with request authentication, not only the
   OTA route. A practical minimum is HMAC-SHA256 over the HTTP method, path,
   request-body hash and a device-issued single-use nonce. Expire and consume
   each nonce to prevent replay. Do not use an automatically attached browser
   cookie as the only credential.
3. Rate-limit failed authentication and do not disclose whether a guessed
   credential was close or valid.
4. Independently verify firmware integrity and origin. Sign the SHA-256 digest
   of every release with an offline ECDSA P-256 private key and embed only the
   public verification key in the radio. Authentication alone is not firmware
   signing.
5. Write only to the inactive OTA slot, reject oversized images before writing,
   verify the signature before selecting the new slot, and retain rollback.
6. Mark a new image valid only after a hardware-tested health check completes.
   Test power loss, corrupt images, wrong signatures and boot-loop rollback.
7. Apply strict request-size and timeout limits. Never accept a URL or image
   size that can exceed the inactive partition.

HTTPS is still recommended when feasible because HMAC does not hide local
traffic. Signed images remain mandatory even if HTTPS is later added.

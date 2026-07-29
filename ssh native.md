# Refactor Blueprint Native C (libssh2 + Channel Engine)

**Role**: Senior Systems Engineer (C, libssh2, mbedTLS, Android NDK)

## Purpose

Perform a TOTAL refactor on the native backend architecture into a modular Native C architecture.

### Target:
- Bukan VPN TUN.
- Tidak menggunakan hev-socks5-tunnel.
- Menggunakan Channel Engine libssh2.
- Semua backend berada di Native C.
- Kotlin hanya UI + JNI.

> **Catatan Change Control**: JANGAN mengubah logika aplikasi, urutan koneksi, payload, reconnect, atau keepalive. Hanya refactor struktur source.

---

## Struktur Direktori (Arsitektur Baru)

```text
native/
├── core/
│   ├── engine.c
│   ├── engine.h
│   ├── session.c
│   ├── session.h
│   ├── reconnect.c
│   ├── reconnect.h
│   ├── keepalive.c
│   ├── keepalive.h
│   ├── state.c
│   └── state.h
│
├── transport/
│   ├── tcp_transport.c
│   ├── tcp_transport.h
│   ├── tls_transport.c
│   ├── tls_transport.h
│   ├── http_transport.c
│   ├── http_transport.h
│   ├── payload_transport.c
│   └── payload_transport.h
│
├── proxy/
│   ├── proxy_http.c
│   ├── proxy_http.h
│   ├── proxy_https.c
│   ├── proxy_https.h
│   ├── proxy_direct.c
│   └── proxy_direct.h
│
├── ssh/
│   ├── ssh_client.c
│   ├── ssh_client.h
│   ├── ssh_auth.c
│   ├── ssh_auth.h
│   ├── ssh_channel.c
│   ├── ssh_channel.h
│   ├── ssh_forward.c
│   ├── ssh_forward.h
│   ├── ssh_keepalive.c
│   ├── ssh_keepalive.h
│   ├── ssh_hostkey.c
│   └── ssh_hostkey.h
│
├── payload/
│   ├── payload_builder.c
│   ├── payload_builder.h
│   ├── payload_parser.c
│   ├── payload_parser.h
│   ├── payload_split.c
│   ├── payload_split.h
│   ├── payload_replace.c
│   └── payload_replace.h
│
├── config/
│   ├── config.c
│   ├── config.h
│   ├── parser.c
│   ├── parser.h
│   └── mmkv_config.c
│
├── logger/
│   ├── logger.c
│   └── logger.h
│
├── utils/
│   ├── buffer.c
│   ├── buffer.h
│   ├── socket_util.c
│   ├── socket_util.h
│   ├── string_util.c
│   ├── string_util.h
│   ├── dns.c
│   ├── dns.h
│   └── timer.c
│
└── jni/
    ├── bridge.c
    └── bridge.h
```

---

## Modul & Rule Tanggung Jawab (Single Responsibility Principle)

### 1. `core/engine.c`
Hanya mengatur flow utama:
`connect TCP` → `optional TLS` → `payload builder` → `payload injection` → `HTTP CONNECT response` → `SSH Session` → `Authentication` → `Open Channel Engine` → `Channel Loop` → `Reconnect`
- **Batasan**: Tidak boleh ada implementasi detail di `engine.c`.

### 2. `payload/payload_builder.c`
Hanya membangun payload.
- **Batasan**: Tidak boleh membuka socket, tidak boleh kirim data, tidak boleh membaca response.

### 3. `transport/payload_transport.c`
Hanya mengirim payload: `send()`, `split`, `delay`, `flush`.

### 4. `transport/http_transport.c`
Hanya mengelola HTTP Tunneling/Proxy: `CONNECT`, membaca HTTP Response, validasi `200 OK`.

### 5. `transport/tls_transport.c`
Hanya mengelola TLS/SSL: TLS, SNI, Handshake, Encrypt, Decrypt.

### 6. `ssh/ssh_client.c`
`libssh2_session_init`, Handshake, Disconnect, Session Free.

### 7. `ssh/ssh_auth.c`
Password, Public Key, Keyboard Interactive.

### 8. `ssh/ssh_channel.c`
Open Session Channel, Open Direct TCPIP, Read, Write, Polling, Close.

### 9. `ssh/ssh_forward.c`
Semua forwarding memakai Channel Engine.
- **Batasan**: Tidak menggunakan DynamicPortForwarder, tidak menggunakan TUN, tidak menggunakan SOCKS server Java.

### 10. `ssh/ssh_keepalive.c`
`libssh2_keepalive_send()`, Timer, Reconnect Trigger.

### 11. `core/reconnect.c`
Semua reconnect logic. Tidak ada reconnect di `engine.c`.

### 12. `core/state.c`
Semua state: `CONNECTING`, `CONNECTED`, `AUTH`, `CHANNEL`, `DISCONNECTED`, `STOPPING`.

### 13. `proxy/proxy_http.c`
HTTP Proxy, CONNECT.

### 14. `proxy/proxy_https.c`
HTTPS Proxy, CONNECT, TLS.

### 15. `proxy/proxy_direct.c`
Direct TCP.

### 16. Kotlin Layer
Kotlin hanya memiliki:
`MainActivity`, `ViewModel`, `Config Builder`, `JNI Wrapper`, `MMKV`.
Semua networking dipindahkan ke Native C.

---

## Hasil Akhir

- Tidak ada kode duplikat.
- Setiap file hanya memiliki satu tanggung jawab (Single Responsibility Principle).
- Engine menjadi modular.
- Mudah menambah HTTP, HTTPS, SSL, WS, atau transport lain tanpa mengubah engine.
- Semua komunikasi SSH menggunakan Channel Engine libssh2.
- Tidak menggunakan TUN maupun DynamicPortForwarder.
- Tidak mengubah perilaku aplikasi, hanya merapikan arsitektur dan memisahkan tanggung jawab setiap modul.

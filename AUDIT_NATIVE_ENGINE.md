# NATIVE CHANNEL ENGINE AUDIT REPORT & ARCHITECTURAL GAP ANALYSIS

**Architect:** Senior Network Engineer & Android NDK Architect  
**Date:** 2026-07-28  
**Target Standard:** C99 Android NDK Native Channel Engine Standard  
**Status:** Audit Completed — Awaiting User Approval before Execution  

---

## 1. Executive Summary

Hasil audit komparatif antara implementasi *Native C Engine* yang ada di `/app/src/main/cpp` dengan target **Modular Channel Engine Architecture** menunjukkan bahwa kode native saat ini masih memiliki ketergantungan monolitik, pengelompokan file yang belum modular, serta beberapa ketidaksesuaian dengan prinsip *Single Responsibility Principle (SRP)*.

### Ringkasan Temuan Utama:
1. **Struktur Folder Belum Modular**: Seluruh file C native terkumpul di satu directory tunggal `/native/channel/` alih-alih terbagi secara eksplisit ke dalam sub-folder domain (`common/`, `transport/`, `proxy/`, `payload/`, `ssh/`, `keepalive/`, `config/`, `session/`, `connection/`, `monitor/`, `crypto/`, `bridge/`).
2. **Absensi `ChannelContext` Terpusat**: State koneksi saat ini tersebar di beberapa struct terpisah (`ChannelNode`, `TunnelConfig`, `EventContext`, variabel global `keepalive.c`), belum dipusatkan pada satu struct `ChannelContext` yang dilewatkan ke seluruh modul layer.
3. **Akses Socket I/O Langsung (Bypass Transport Layer)**: Beberapa modul masih memanggil `read()`, `write()`, `recv()`, `send()`, `libssh2_channel_read()`, dan `libssh2_channel_write()` secara langsung tanpa melalui abstraksi `transport_*` (`transport_connect`, `transport_read`, `transport_write`, `transport_write_all`, `transport_read_exact`).
4. **Penggunaan C++ pada Module Config**: File `/app/src/main/cpp/config/config_manager.cpp` menggunakan C++ (MMKV & `std::string`), melanggar aturan murni C99 untuk Native Engine.
5. **State & Timer Global**: `keepalive.c` dan `channel_manager.c` masih mengandalkan variabel global alih-alih menyimpan state timer dan statistik pada `ChannelContext` berbasis `CLOCK_MONOTONIC`.
6. **Sistem Error Status Belum Terstandarisasi**: Return value fungsi native belum menggunakan `enum EngineError` internal (`ENGINE_OK`, `ENGINE_TIMEOUT`, `ENGINE_IO_ERROR`, dll.).

---

## 2. Tabel Analisis Kesenjangan (Gap Analysis)

| Target Module & File | Status Saat Ini | Masalah / Kebutuhan Refactor |
| :--- | :--- | :--- |
| **`common/`** | | |
| `logger.c / logger.h` | Ada di `native/channel/` | Perlu dipindah ke `common/`, disesuaikan agar menutup direct `__android_log_print` dengan macro `LOGD/LOGI/LOGW/LOGE`. |
| `buffer.c / buffer.h` | Ada di `native/channel/` | Perlu dipindah ke `common/` dan dioptimalkan tanpa alokasi heap berlebih pada fast path. |
| `timer.c / timer.h` | **BELUM ADA** | Harus dibuat untuk mengelola pewaktu non-blocking berbasis `CLOCK_MONOTONIC`. |
| `event_loop.c / event_loop.h` | Tergabung di `channel.c` | Perlu dipisahkan dari `channel.c` menjadi modul `event_loop` mandiri. |
| `dns_resolver.c / dns_resolver.h` | **BELUM ADA** | Harus dibuat untuk resolving domain non-blocking / helper. |
| `url_parser.c / url_parser.h` | Tergabung di `utils.c` | Perlu diekstrak dari `utils.c` menjadi modul parser URL khusus. |
| `base64.c / base64.h` | Tergabung di `utils.c` | Perlu diekstrak dari `utils.c` menjadi modul pengkodean base64 dedicated. |
| **`transport/`** | | |
| `tcp_transport.c / tcp_transport.h` | Ada di `native/channel/` | Perlu dipindah ke `transport/`, menyediakan wrapper `transport_read/write/connect`. |
| `tls_transport.c / tls_transport.h` | Sebagian di `ssl_client.c` | Perlu dipindah ke `transport/` dan menyatukan wrapper SSL/TLS MbedTLS. |
| `websocket_transport.c / websocket_transport.h` | Sebagian di `websocket.c` | Perlu dipindah ke `transport/` dan disesuaikan dengan interface `transport_*`. |
| **`proxy/`** | | |
| `proxy_connect.c / proxy_connect.h` | Ada di `native/channel/` | Perlu dipindah ke `proxy/` dan disesuaikan menerima `ChannelContext*`. |
| `http_proxy.c / http_proxy.h` | Sebagian di `http_request.c` | Perlu dipisahkan dari HTTP payload parser dan dijadikan modul HTTP Proxy Handshake. |
| **`payload/`** | | |
| `payload_parser.c / payload_parser.h` | Ada di `native/channel/` | Perlu dipindah ke `payload/`. |
| `payload_builder.c / payload_builder.h` | Ada di `native/channel/` | Perlu dipindah ke `payload/`. |
| `payload_executor.c / payload_executor.h` | **BELUM ADA** | Harus dibuat untuk mengeksekusi payload sequence pada `ChannelContext`. |
| `payload_variable.c / payload_variable.h` | Ada di `native/channel/` | Perlu dipindah ke `payload/`. |
| **`ssh/`** | | |
| `ssh_client.c / ssh_client.h` | Tergabung di `channel.c` | Harus dibuat khusus mengisolasi inisialisasi & handshake `LIBSSH2_SESSION`. |
| `ssh_channel.c / ssh_channel.h` | Tergabung di `channel_manager.c` | Harus dibuat khusus mengelola pembukaan & I/O `LIBSSH2_CHANNEL`. |
| `ssh_forward.c / ssh_forward.h` | **BELUM ADA** | Harus dibuat khusus mengelola port forwarding (direct-tcpip). |
| `ssh_keepalive.c / ssh_keepalive.h` | Tergabung di `keepalive.c` | Harus dipisahkan dari keepalive umum untuk penanganan khusus SSH ignore paket. |
| `ssh_auth.c / ssh_auth.h` | Tergabung di `channel.c` | Harus dibuat khusus menangani autentikasi password & public key SSH. |
| **`keepalive/`** | | |
| `keepalive.c / keepalive.h` | Ada di `native/channel/` | Perlu di-refactor tanpa variabel global, menggunakan timer di `ChannelContext`. |
| `keepalive_tcp.c / keepalive_tcp.h` | **BELUM ADA** | Harus dibuat untuk TCP keep-alive socket option. |
| `keepalive_ws.c / keepalive_ws.h` | **BELUM ADA** | Harus dibuat untuk WebSocket Ping/Pong frame interval. |
| `keepalive_http.c / keepalive_http.h` | **BELUM ADA** | Harus dibuat untuk HTTP HEAD keepalive request. |
| `keepalive_ssh.c / keepalive_ssh.h` | **BELUM ADA** | Harus dibuat khusus interval SSH ping. |
| **`config/`** | | |
| `config_loader.c / config_loader.h` | **BELUM ADA** | Dibuat dalam C99 murni untuk membaca data konfigurasi. |
| `config_parser.c / config_parser.h` | Ada (`channel_config.c`) | Perlu dipindah ke `config/config_parser.c`. |
| `config_validator.c / config_validator.h` | **BELUM ADA** | Dibuat untuk validasi format IP, port, dan payload. |
| `config_serializer.c / config_serializer.h` | **BELUM ADA** | Dibuat untuk serialisasi struct `TunnelConfig` / context. |
| **`session/`** | | |
| `session.c / session.h` | **BELUM ADA** | Dibuat sebagai pengelola utama siklus hidup `ChannelContext` (init, reset, destroy). |
| **`connection/`** | | |
| `connection_manager.c` | Sebagian di `channel_manager.c` | Dipisahkan menjadi pengelola koneksi utama non-blocking. |
| `reconnect.c / reconnect.h` | Ada di `native/channel/` | Dipindah ke `connection/` dan menggunakan `ChannelContext`. |
| `timeout.c / timeout.h` | **BELUM ADA** | Dibuat untuk kalkulasi timeout koneksi dan I/O deadline. |
| **`monitor/`** | | |
| `statistics.c / latency.c / traffic.c` | **BELUM ADA** | Modul monitoring traffic (tx/rx byte counter) dan pengukuran latensi ping. |
| **`crypto/`** | | |
| `certificate.c / certificate_verify.c` | **BELUM ADA** | Pengelolaan verifikasi sertifikat SSL/TLS & fingerprint host SSH. |
| `secure_memory.c / secure_memory.h` | **BELUM ADA** | Utilitas wiping memori sensitif (password, key). |
| **`bridge/`** | | |
| `jni_bridge.c / jni_bridge.h` | Ada di `/app/src/main/cpp/` | Dipindah ke `native/bridge/` agar sesuai arsitektur modular. |

---

## 3. Desain `ChannelContext` (Pusat State Engine)

Rancangan `ChannelContext` C99 murni yang akan digunakan oleh seluruh modul:

```c
typedef enum {
    CONN_STATE_DISCONNECTED = 0,
    CONN_STATE_CONNECTING,
    CONN_STATE_PAYLOAD_HANDSHAKE,
    CONN_STATE_PROXY_HANDSHAKE,
    CONN_STATE_TLS_HANDSHAKE,
    CONN_STATE_SSH_HANDSHAKE,
    CONN_STATE_CONNECTED,
    CONN_STATE_RECONNECTING,
    CONN_STATE_ERROR
} ConnectionState;

typedef struct ChannelContext {
    /* Socket & Layer FDs */
    int socket_fd;
    void *ssl_ctx;            /* MbedTLS Context Pointer */
    LIBSSH2_SESSION *ssh_session;
    LIBSSH2_CHANNEL *ssh_channel;

    /* Host & Routing Config */
    char hostname[256];
    int port;
    char proxy_host[256];
    int proxy_port;
    char sni[256];

    /* Protocols & Modes */
    int protocol;             /* DIRECT, HTTP_PROXY, SOCKS4, SOCKS5, WEBSOCKET, SSL */
    int keepalive_type;       /* NONE, TCP, SSH_IGNORE, WS_PING, HTTP_HEAD */
    
    /* State & Status */
    ConnectionState state;
    int last_error;           /* enum EngineError */

    /* Timing & KeepAlive (CLOCK_MONOTONIC) */
    uint64_t timeout_ms;
    uint64_t last_read_time;
    uint64_t last_write_time;
    uint64_t last_ping_time;
    int reconnect_counter;

    /* Monitoring & Statistics */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t latency_ms;

    /* Buffers & Data Stream */
    void *rx_buffer;          /* DynamicBuffer* */
    void *tx_buffer;          /* DynamicBuffer* */
} ChannelContext;
```

---

## 4. Rencana Tahapan Refactoring (Execution Roadmap)

Apabila disetujui oleh User, eksekusi refactoring akan dilaksanakan secara bertahap tanpa merusak build:

1. **Fase 1: Restrukturisasi Direktori & Common Layer**
   - Membuat direktori: `common/`, `transport/`, `proxy/`, `payload/`, `ssh/`, `keepalive/`, `config/`, `session/`, `connection/`, `monitor/`, `crypto/`, `bridge/`.
   - Memindahkan `logger`, `buffer`, `utils` ke `common/`. Membuat `timer.c/h` dan `event_loop.c/h`.
2. **Fase 2: Transport Abstraction Layer**
   - Menyatukan TCP, TLS, dan WebSocket ke dalam wrapper `transport_*` (`transport_connect`, `transport_read`, `transport_write`, dll.).
3. **Fase 3: Core Context & Session Engine**
   - Membuat `ChannelContext` di `session/session.h` dan migrasi fungsi `proxy_connect_*`, `keepalive_*`, `ssh_*` untuk menerima `ChannelContext*`.
4. **Fase 4: Payload, Proxy & SSH Decoupling**
   - Memisahkan modul payload executor, SSH client/channel/auth, dan HTTP proxy handshake secara terisolasi.
5. **Fase 5: Monitoring, Config C99 & JNI Bridge**
   - Menghapus C++ pada config manager, memindahkan `jni_bridge.c` ke `native/bridge/`.
6. **Fase 6: Verifikasi & Build Testing**
   - Memastikan kompilasi Gradle `assembleDebug` berhasil 100% dan memperbarui `PROGRESS.md`.

---

*Status: Audit selesai. Menunggu instruksi dari User sebelum memulai perubahan kode.*

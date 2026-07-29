# AUDIT & IMPLEMENTASI CHANNEL ENGINE (C++)
Dokumen ini mencatat daftar komponen C++ dalam arsitektur `Channel Engine` yang masih berupa stub (belum diimplementasikan penuh) atau belum sesuai spesifikasi arsitektur final, dan harus dikerjakan secara bertahap.

## DAFTAR KOMPONEN YANG BELUM SELESAI / STUB

### 1. Channel API (`channel.c` / `channel.h`)
- **Status Saat Ini**: Belum diimplementasikan secara terstruktur.
- **Tugas**: 
  - Mengimplementasikan `channel_create()`, `channel_destroy()`, `channel_connect()`, `channel_disconnect()`.
  - Mengimplementasikan `channel_send()`, `channel_recv()`, `channel_flush()`, `channel_close()`, `channel_is_connected()`.
  - Mengintegrasikan pipeline: `tcp_transport` -> `ssl_client` -> `websocket` -> `proxy_connect` -> `payload_builder` -> `ssh_connect`.

### 2. Payload Builder & Parser (`payload_builder.c`, `payload_parser.c`, `payload_variable.c`)
- **Status Saat Ini**: Hanya stub (misalnya dalam `payload_builder.c` saat ini masih campur aduk dengan proxy I/O).
- **Tugas**: 
  - `payload_builder`: Implementasi dukungan `GET`, `POST`, `CONNECT`, `HEAD`, `OPTIONS`, `PUT`, `DELETE`, `PATCH`.
  - `payload_parser`: Implementasi fungsi parsing custom header (Host, Connection, Proxy-Connection, Keep-Alive, dll).
  - `payload_variable`: Implementasi pencarian dan penggantian tag dinamis (`[host]`, `[port]`, `[host_port]`, `[protocol]`, `[method]`, `[crlf]`, `[cr]`, `[lf]`, `[lfcr]`, `[ua]`, `[random]`, `[rotate]`, `[auth]`, `[proxy]`).

### 3. Split Engine (`split_engine.c`)
- **Status Saat Ini**: Berupa stub yang selalu me-return 0.
- **Tugas**:
  - Implementasi metode injeksi multi-stage: `NORMAL_SPLIT`, `INSTANT_SPLIT`, `DELAY_SPLIT`, `REVERSE_SPLIT`, `FRONT_SPLIT`, `BACK_SPLIT`, `CUSTOM_SPLIT`.

### 4. WebSocket (`websocket.c`, `websocket_frame.c`)
- **Status Saat Ini**: Berupa stub sederhana (`websocket_handshake`).
- **Tugas**:
  - `websocket.c`: Implementasi handshake lengkap (Upgrade, Path, Host Override, Origin, Cookie, Sec-WebSocket-Key, dll).
  - `websocket_frame.c`: Implementasi pembentukan dan pemecahan frame (Masking, Binary Frame, Text Frame, Ping, Pong, Close).

### 5. HTTP Request & Response (`http_request.c`, `http_response.c`)
- **Status Saat Ini**: Perlu diimplementasikan terpisah.
- **Tugas**: 
  - Modul khusus untuk membaca header respon (status 200/101) secara clean.

### 6. SSL / TLS (`ssl_client.c`)
- **Status Saat Ini**: Terimplementasi sebagian (dasar `tls_transport`).
- **Tugas**:
  - Peningkatan pada validasi dan verifikasi (TLS 1.2, TLS 1.3, ALPN, SNI, Certificate Verify, Hostname Verify).

### 7. Proxy Connect (`proxy_connect.c`)
- **Status Saat Ini**: Belum dienkapsulasi dengan rapi.
- **Tugas**:
  - Implementasi mode proxy dinamis: Direct, HTTP Proxy, HTTPS Proxy, SOCKS4, SOCKS5, CONNECT.

### 8. KeepAlive (`keepalive.c`)
- **Status Saat Ini**: Belum ada mekanisme ping di level channel.
- **Tugas**:
  - Implementasi TCP KeepAlive, SSH Ignore, WebSocket Ping, HTTP HEAD, HTTP GET.

### 9. Reconnect (`reconnect.c`)
- **Status Saat Ini**: Belum ada logika auto-reconnect di sisi native.
- **Tugas**:
  - Implementasi Auto Retry, Retry Delay, Retry Count, Exponential Backoff, dan pemantauan Network Change.

### 10. Buffer (`buffer.c`)
- **Status Saat Ini**: Buffer bersifat statis/hardcoded di beberapa file.
- **Tugas**:
  - Implementasi Dynamic Buffer, Ring Buffer untuk Send/Receive buffer guna mendukung throughput tinggi (VpnService/SOCKS5 traffic).

### 11. Transport (`tcp_transport.c`)
- **Status Saat Ini**: Ada beberapa kode jaringan I/O.
- **Tugas**:
  - Konsolidasi API jaringan menjadi `tcp_connect()`, `tcp_send()`, `tcp_recv()`, `tcp_close()` yang andal.

### 12. Utility & Config (`utils.c`, `channel_config.c`, `logger.c`)
- **Status Saat Ini**: Logging standar.
- **Tugas**: 
  - Standarisasi logger dan parsing config ke struct C.

## HASIL ANALISIS DARI KODE JAVA LEGACY (Trilead SSH2)
Berdasarkan tinjauan pada arsitektur legacy (`TransportManager.java`, `KexManager.java`, dll), berikut adalah fitur-fitur yang perlu diadaptasi ke dalam Native C Channel Engine namun belum terimplementasi secara penuh:

### 13. Manajemen Timeout & Tuning TCP (`tcp_transport.c`)
- **Dari Legacy**: Terdapat konfigurasi `setTcpNoDelay()`, `setSoTimeout()`, `connectTimeout`, dan `readTimeout`.
- **Status Saat Ini**: `tcp_transport.c` belum menangani timeout I/O secara granular.
- **Tugas**: 
  - Mengimplementasikan `non-blocking connect` dengan fallback timeout (melalui `select` / `poll`).
  - Menerapkan parameter socket `TCP_NODELAY`, `SO_RCVTIMEO`, dan `SO_SNDTIMEO` yang dapat diatur melalui profil konfigurasi.

### 14. Asynchronous Write Queue / Buffering (`buffer.c` & `channel.c`)
- **Dari Legacy**: Menggunakan `asynchronousQueue` dan thread `AsynchronousWorker` agar penulisan soket tidak memblokir antrean (misal pada traffic VPN yang padat).
- **Status Saat Ini**: Penulisan ke file descriptor (FD) di channel C masih bisa mengalami blocking atau packet-drop saat status socket `EAGAIN`/`EWOULDBLOCK`.
- **Tugas**:
  - Mengimplementasikan Send Buffer (Ring Buffer) di layer `buffer.c`.
  - Jika socket belum siap (EAGAIN), data ditampung dalam buffer dan ditrigger kembali melalui event loop `EVENT_WRITE`.

### 15. Connection Monitor & Disconnect Callback (`channel.c` & JNI)
- **Dari Legacy**: Mekanisme `connectionMonitors.connectionLost(reasonClosedCause)` memberitahu layer service bila transport putus, memicu fitur auto-reconnect.
- **Status Saat Ini**: Belum ada sistem callback rapi dari Channel Engine C ke Kotlin (`SiVpnService.kt`) saat socket terputus secara tidak wajar.
- **Tugas**:
  - Mengimplementasikan fungsi JNI Callback atau sistem notifikasi event yang dapat di-hook oleh Kotlin, sehingga Engine Reconnect dapat bekerja responsif.

*(Catatan: Fitur kriptografi SSH seperti negosiasi MAC, Key Exchange, Cipher Enkripsi yang sangat panjang di file `TransportConnection.java` dan `KexManager.java` **tidak perlu ditulis ulang**, karena backend native ini sudah dirancang menggunakan library terintegrasi `libssh2` dan `mbedtls` yang menangani semua itu di level bawah secara otomatis).*

## 16. Kebutuhan Spesifik Payload Parser & Formatter (`payload_variable.c` & `split_engine.c`)
Berdasarkan `TunnelUtils.java` dan `HttpProxyCustom.java`:
- **Tag Tambahan**: Perlu mendukung `[ssh]` (alias untuk host:port).
- **Injeksi Delay Split (`[delay_split]`)**: Di legacy Java, ini menggunakan `Thread.sleep(1000)` (blokir selama 1 detik). Dalam arsitektur C++ event loop (`split_engine.c`), kita **TIDAK BOLEH** menggunakan `sleep()` karena akan memblokir I/O lain. Harus diimplementasikan menggunakan non-blocking timer (misal event timeout pada `select`/`poll` atau `timerfd`).
- **Rotasi & Acak (`[rotate=...]` & `[random=...]`)**: Diperlukan state memory statis/heap (`static int last_rotate_idx`) di `payload_variable.c` untuk melacak elemen array terakhir yang digunakan.
- **Header Autentikasi Proxy**: Harus dapat menghasilkan header `Proxy-Authorization: Basic <base64(user:pass)>` menggunakan utilitas Base64 (bisa menggunakan dari mbedtls/libssh2).
- **Bypass Respons (Modo Dropbear)**: Jika fitur dropbear aktif, pastikan mesin pembacaan respons HTTP dilewati (langsung masuk ke mode tunneling setelah injeksi).

## 17. Implementasi SSH Pinger (`keepalive.c`)
Berdasarkan `TunnelManagerThread.java` (`startPinger`):
- Legacy mengirimkan paket SSH request kustom (`trilead-ping`) secara berkala di thread terpisah.
- **Tugas C++**: Menggunakan `libssh2_keepalive_config` dan mengirimkan paket keepalive reguler melalui event loop menggunakan `libssh2_keepalive_send`, agar koneksi tidak diputus firewall (idle timeout).

## 18. Konfigurasi SSL/TLS & SNI (`ssl_client.c`)
Berdasarkan `SSLTunnelProxy.java`:
- Menggunakan TrustAllCerts (menolak verifikasi sertifikat karena custom SNI).
- **Tugas C++**: Pastikan `mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE)` diterapkan ketika terhubung ke SSL/Stunnel payload custom. 

## 19. Auto Reconnect & Network State
Berdasarkan `TunnelManagerThread.java` (`reconnectSSH`):
- Legacy melakukan retry connect hingga 5 kali dengan jeda jika terjadi putus koneksi atau jaringan berganti.
- **Tugas C++**: Di layer native `reconnect.c`, sediakan logika *Exponential Backoff* atau batasan jumlah *retry* ketika socket PING gagal atau `EPIPE`, serta menunda pengiriman hingga antarmuka jaringan hidup (memberitahu/mendapat notifikasi dari Kotlin JNI).

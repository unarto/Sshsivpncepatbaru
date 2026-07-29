# BACKEND ARCHITECTURE (STRICT)

Mulai saat ini lakukan migrasi arsitektur backend aplikasi ke desain berikut tanpa mengubah UI maupun perilaku aplikasi.

## Target Arsitektur

- UI → Kotlin
- Android VpnService → Kotlin
- JNI Bridge → Kotlin
- Native Engine → C
- Config Builder → Native C
- Config Parser → Native C
- SSH Engine (libssh2) → Native C
- SOCKS5 Server → Native C
- Channel Engine → Native C
- Session Manager → Native C
- Payload Engine → Native C
- Reconnect Manager → Native C
- KeepAlive Manager → Native C
- Buffer Manager → Native C
- Event Loop → Native C
- Statistics → Native C
- Logger → Native C

Konfigurasi aplikasi menggunakan MMKV.

Native Engine wajib membaca konfigurasi langsung dari MMKV menggunakan MMKV Native API (C++).

Jangan mengambil konfigurasi dari Kotlin lalu mengirimkannya satu per satu melalui JNI.

Targetnya adalah Native Engine membaca sendiri seluruh konfigurasi dari MMKV, sehingga parameter JNI menjadi sangat sedikit dan seluruh konfigurasi terpusat.

---

## PEMBAGIAN TUGAS

### Kotlin

Kotlin hanya bertanggung jawab terhadap:

- UI
- Activity
- Fragment
- Compose
- ViewModel
- Android Lifecycle
- Android VpnService
- Notification
- Permission
- MMKV Initialization
- JNI Bridge
- Log Viewer

Kotlin tidak boleh menangani:

- SSH
- SOCKS5
- Payload
- Parser
- Routing
- Multiplexing
- Session
- KeepAlive
- Reconnect
- Buffer
- Config Builder
- Config Parser

---

### MMKV

MMKV menjadi satu-satunya sumber konfigurasi aplikasi.

Seluruh konfigurasi disimpan di MMKV, misalnya:

- Host
- Port
- Username
- Password
- Payload
- Proxy
- DNS
- MTU
- IPv4
- IPv6
- UDP
- TCP
- KeepAlive
- Compression
- Timeout
- Retry
- Fingerprint
- Konfigurasi Tunnel
- Konfigurasi SSH
- Konfigurasi SOCKS5
- Pengaturan lainnya

Native Engine wajib membaca data tersebut secara langsung melalui MMKV Native API (C++).

Jangan membuat salinan konfigurasi yang sama di Kotlin.

---

### JNI

JNI hanya menjadi jembatan sederhana.

Contoh fungsi JNI yang diperbolehkan:

- nativeStart()
- nativeStop()
- nativePause()
- nativeResume()
- nativeReloadConfig()
- nativeGetStatistics()
- nativeGetLog()
- nativeIsRunning()

JNI tidak boleh menerima puluhan parameter konfigurasi.

Semua konfigurasi dibaca langsung oleh Native Engine dari MMKV.

---

### CONFIG BUILDER

Pindahkan seluruh Config Builder dari Kotlin ke Native C.

Builder bertanggung jawab terhadap:

- Validasi konfigurasi
- Parsing konfigurasi
- Default value
- Konversi konfigurasi
- Penyusunan struktur konfigurasi internal

Builder tidak boleh berada di Kotlin.

---

### CONFIG PARSER

Pindahkan seluruh parser ke Native C.

Contoh:

- SSH Config
- Payload
- HTTP
- CONNECT
- SOCKS5
- JSON
- URI
- Dynamic Payload

---

### SSH ENGINE

Seluruh implementasi SSH wajib berada di Native C menggunakan libssh2.

Meliputi:

- Session
- Authentication
- Host Key Verification
- Known Hosts
- Channel
- Window Adjust
- KeepAlive
- Reconnect

Tidak boleh ada implementasi SSH di Kotlin.

---

### SOCKS5

Seluruh implementasi SOCKS5 berada di Native C.

hev-socks5-tunnel hanya bertugas mengubah:

IP Packet ⇄ SOCKS5

Native Engine bertugas sebagai SOCKS5 Server dan meneruskan trafik melalui SSH Channel.

---

### CHANNEL ENGINE

Gunakan satu SSH Session.

Semua koneksi aplikasi wajib menggunakan SSH Channel melalui "libssh2_channel_direct_tcpip_ex()".

Dilarang membuat SSH Session baru untuk setiap koneksi.

---

### EVENT LOOP

Gunakan Event Loop Native.

Prioritas:

1. epoll
2. poll

Hindari penggunaan "select()" sebagai mekanisme utama.

---

### THREAD MODEL

Gunakan:

- Single IO Thread untuk seluruh operasi libssh2
- Worker Thread hanya untuk pekerjaan non-libssh2

Jangan mengakses satu "LIBSSH2_SESSION" dari banyak thread secara bersamaan.

---

### MEMORY

Gunakan Buffer Manager Native.

Kurangi alokasi malloc/free berulang.

Gunakan object reuse atau buffer pool bila memungkinkan.

---

### TUJUAN MIGRASI

Target akhir adalah:

UI (Kotlin)
↓
VpnService (Kotlin)
↓
JNI
↓
Native Engine (C)
↓
MMKV Native API
↓
Config Builder
↓
Config Parser
↓
SOCKS5 Server
↓
Channel Engine
↓
libssh2
↓
mbedTLS
↓
TCP Socket
↓
SSH Server

Dengan arsitektur tersebut, hampir seluruh backend aplikasi berjalan di Native C, sedangkan Kotlin hanya menjadi pengendali Android dan antarmuka pengguna. Seluruh konfigurasi tersimpan terpusat di MMKV dan dibaca langsung oleh Native Engine, sehingga komunikasi JNI menjadi minimal, arsitektur lebih bersih, modular, dan mudah dipelihara.

---

### SETELAH SETIAP PERUBAHAN

Wajib:

1. Audit seluruh perubahan.
2. Jelaskan alasan teknis perubahan.
3. Jelaskan dampaknya terhadap performa, stabilitas, keamanan, dan penggunaan memori.
4. Laporkan potensi risiko bila ada.
5. Perbarui "PROGRESS.md" setelah pekerjaan selesai agar progres implementasi selalu terdokumentasi.

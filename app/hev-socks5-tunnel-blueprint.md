==================================================
ATURAN KERAS HEV-SOCKS5-TUNNEL
==================================================

JANGAN:

- memindahkan source hev-socks5-tunnel ke folder lain
- mengubah struktur source upstream
- memecah file upstream menjadi module baru
- mengubah API internal upstream
- membuat clone engine sendiri
- membuat implementasi SOCKS5 baru
- membuat implementasi TUN baru
- membuat parser konfigurasi baru di luar kebutuhan wrapper
- mengubah hev-jni.c
- mengubah hev-main.c
- mengubah Android.mk/Meson upstream kecuali flag build

hev-socks5-tunnel HARUS tetap menjadi Git Submodule resmi.

==================================================
TANGGUNG JAWAB HEV-SOCKS5-TUNNEL
==================================================

Engine hev-socks5-tunnel adalah satu-satunya yang bertanggung jawab atas:

- membaca konfigurasi YAML
- membuat event loop
- menjalankan SOCKS5 client
- menangani UDP relay
- mengelola TUN FD
- membaca paket dari TUN
- menulis paket ke TUN
- DNS forwarding sesuai konfigurasi
- routing internal engine
- statistik packet
- lifecycle engine
- thread engine
- task scheduler internal
- signal quit
- socket polling
- network forwarding
- packet encapsulation
- packet decapsulation

Android tidak boleh mengimplementasikan ulang fungsi-fungsi tersebut.

==================================================
ANDROID HANYA BERTUGAS
==================================================

Android/Kotlin hanya boleh:

UI
↓

ViewModel
↓

Service Controller
↓

VpnService
↓

NativeBridge
↓

TProxyService (JNI Wrapper)

↓

libhev-socks5-tunnel.so

Tidak boleh ada logika networking di Kotlin.

==================================================
NATIVE BRIDGE
==================================================

NativeBridge hanya menyediakan abstraksi:

startEngine(configPath, tunFd)

stopEngine()

restartEngine()

isRunning()

getStatistics()

NativeBridge tidak boleh memiliki implementasi SOCKS5, TUN, DNS, ataupun routing.

==================================================
VPN SERVICE
==================================================

VpnService hanya bertugas:

- establish TUN
- membuat Notification
- memperoleh File Descriptor
- menyerahkan FD ke NativeBridge
- mengontrol lifecycle Android Service

Tidak boleh menjalankan engine.

Tidak boleh membaca paket.

Tidak boleh membuat socket.

==================================================
CONFIG
==================================================

ConfigBuilder hanya menghasilkan konfigurasi YAML yang dibutuhkan upstream.

ConfigBuilder tidak boleh mengimplementasikan logika engine.

==================================================
TARGET AKHIR
==================================================

Arsitektur yang dihasilkan harus seperti berikut:

Compose UI
        │
        ▼
ViewModel
        │
        ▼
Service Controller
        │
        ▼
HevVpnService
        │
        ▼
NativeBridge
        │
        ▼
TProxyService (JNI Wrapper)
        │
        ▼
libhev-socks5-tunnel.so
        │
        ▼
hev-jni.c (upstream)
        │
        ▼
hev_socks5_tunnel_main()

==================================================
PRINSIP
==================================================

hev-socks5-tunnel adalah ENGINE.

Android adalah WRAPPER.

JNI adalah BRIDGE.

UI adalah CONTROLLER.

Jangan membalik tanggung jawab tersebut.

Seluruh refactor harus menjaga agar update upstream tetap mudah, tanpa perlu mengedit source resmi hev-socks5-tunnel.

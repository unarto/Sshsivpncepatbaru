# Peta Pembagian Tugas Library C (C/C++ Native)

Dokumen ini mendefinisikan batasan tanggung jawab (Single Responsibility Principle) untuk setiap komponen/library C/C++ dalam proyek ini guna mencegah overlapping dan tumpang tindih alur kerja (tight coupling).

========================================
HEV-SOCKS5-TUNNEL
========================================

Tanggung jawab:

✓ TUN Engine
✓ Membaca File Descriptor TUN
✓ Routing Packet
✓ SOCKS5 Client
✓ UDP Relay
✓ DNS Forward
✓ NAT Internal (sesuai upstream)
✓ Packet Loop
✓ Statistics
✓ Traffic Counter
✓ Interface MTU
✓ IPv4
✓ IPv6
✓ Event Loop

Tidak boleh mengetahui:

✗ SSH
✗ libssh2
✗ Payload
✗ WebSocket
✗ HTTP Proxy
✗ HTTPS Proxy
✗ Cloudflare
✗ CloudFront
✗ Squid
✗ TLS Socket
✗ Android UI
✗ VpnService

========================================
LIBSSH2
========================================

Tanggung jawab:

✓ TCP Connection
✓ SSH Handshake
✓ HostKey Verification
✓ Authentication
✓ Password Auth
✓ Public Key Auth
✓ Keyboard Interactive
✓ Session Management
✓ Channel Engine
✓ Direct TCPIP Channel
✓ Local Forward
✓ Dynamic Forward
✓ KeepAlive
✓ Rekey
✓ Disconnect
✓ Window Management

Tidak boleh mengetahui:

✗ TUN
✗ SOCKS5 Engine
✗ Android UI
✗ VpnService
✗ MMKV
✗ Compose
✗ Statistics UI

========================================
PAYLOAD MODULE
========================================

Tanggung jawab:

✓ Payload Builder
✓ Variable Resolver
✓ Header Builder
✓ Split
✓ Delay
✓ Repeat
✓ Rotate
✓ Random
✓ Replace

Tidak boleh:

✗ Membuka Socket
✗ SSH
✗ SOCKS5
✗ Routing

========================================
TRANSPORT MODULE
========================================

Tanggung jawab:

✓ TCP Socket
✓ TLS Socket
✓ HTTP CONNECT
✓ HTTPS CONNECT
✓ Proxy Negotiation
✓ WebSocket Upgrade
✓ Cloudflare
✓ CloudFront
✓ Squid

Tidak boleh:

✗ SSH Authentication
✗ SOCKS5 Engine
✗ TUN

========================================
ENGINE CHANNEL
========================================

Tanggung jawab:

✓ Menghubungkan semua module
✓ Mengatur urutan koneksi
✓ Memilih transport
✓ Memilih engine aktif
✓ Mengirim socket ke engine berikutnya
✓ Mengelola lifecycle engine

Tidak boleh:

✗ Implementasi SSH
✗ Implementasi SOCKS5
✗ Implementasi TLS
✗ Implementasi Payload

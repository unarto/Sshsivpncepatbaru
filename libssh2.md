# Panduan Integrasi & Arsitektur libssh2 (SiVPN Cepat)

Dokumen ini disusun dari perspektif **Senior Android NDK, C++ Systems Engineer, dan Network Tunnel Engineer** untuk mengaudit serta mendokumentasikan integrasi `libssh2` sebagai native SSH transport engine menggunakan C++ wrapper di aplikasi **SiVPN Cepat**.

---

## 1. Konsep Arsitektur Utama & Pemisahan Tanggung Jawab

Pemisahan tanggung jawab secara ketat (*Separation of Concerns*) antara engine VPN (`hev-socks5-tunnel`) dan engine SSH (`libssh2`):

```
+-------------------------------------------------------------+
|                      Kotlin Layer                           |
|      (com.sivpn.cepat.ssh.LibSsh2Client / NativeSshTunnel)  |
+-------------------------------------------------------------+
                              |
                              | JNI Bridge
                              v
+-------------------------------------------------------------+
|                     C++ SSH Wrapper                         |
|   (native/ssh/ -> ssh_client.cpp, socks5_server.cpp, dll)   |
+-------------------------------------------------------------+
                              |
                              | Direct C API
                              v
+-------------------------------------------------------------+
|                       libssh2.so                            |
|             (Native SSH Protocol Engine)                    |
+-------------------------------------------------------------+
                              |
                              | Encrypted TCP Tunnel
                              v
+-------------------------------------------------------------+
|                       SSH Server                            |
+-------------------------------------------------------------+
```

### Pembagian Tugas:
1. **`hev-socks5-tunnel`**:
   - Sumber kode C resmi tetap utuh tanpa modifikasi (Zero modification).
   - Tidak dimasukkan logika SSH atau autentikasi SSH sama sekali.
   - Murni bertindak sebagai TUN -> SOCKS5 tunnel engine (menangkap IP traffic dari TUN fd dan meneruskannya ke Local SOCKS5 Proxy).

2. **`libssh2` & C++ Wrapper**:
   - Digunakan sebagai backend transport SSH yang menyediakan Local SOCKS5 Server / Dynamic Port Forwarding.
   - Menggunakan C++ wrapper/adapter layer di atas `libssh2` tanpa mengubah kode pustaka `libssh2` itu sendiri.
   - Menyediakan interface JNI terisolasi untuk mengontrol koneksi SSH.

---

## 2. Kontrak Config & Structure C++ Native Module

### A. Data Configuration Struct (`ssh_config.h`)
Konfigurasi SSH terpisah sepenuhnya dari konfigurasi HEV SOCKS5:

```cpp
#ifndef SSH_CONFIG_H
#define SSH_CONFIG_H

#include <string>

struct SshConfig {
    std::string host;
    int port = 22;
    std::string username;
    std::string password;
    std::string payload;      // Payload HTTP/Proxy injeksi jika ada
    std::string proxyHost;    // IP/Host Proxy HTTP
    int proxyPort = 8080;     // Port Proxy HTTP
    int localSocksPort = 1080;// Local SOCKS5 listener port untuk HEV
    int timeoutMs = 15000;    // Connection timeout
    bool autoReconnect = true;
};

#endif // SSH_CONFIG_H
```

### B. Struktur File Module Native (`app/src/main/cpp/ssh/` atau `native/ssh/`)

```
app/src/main/cpp/
 ├── CMakeLists.txt
 ├── native-ssh-jni.c (atau cpp/ssh/jni_bridge.cpp)
 └── ssh/
      ├── ssh_client.h
      ├── ssh_client.cpp
      ├── ssh_config.h
      ├── socks5_server.h
      ├── socks5_server.cpp
      └── jni_bridge.cpp
```

---

## 3. Alur Lifecycle libssh2 Native Wrapper

C++ Wrapper wajib menangani urutan lifecycle koneksi SSH sebagai berikut:

1. **Inisialisasi (`libssh2_init`)**:
   - Memanggil `libssh2_init(0)` saat module dimuat atau sebelum membuat sesi pertama.
2. **TCP Socket Creation & Connect**:
   - Membuka POSIX socket (`socket(AF_INET, SOCK_STREAM, 0)`).
   - Melakukan koneksi ke SSH Host / Proxy Server dengan non-blocking socket dan timeout handling.
3. **HTTP Payload Injection (jika menggunakan HTTP Proxy)**:
   - Jika payload/proxy diaktifkan, mengirimkan HTTP Request Payload via socket TCP sebelum handshake SSH.
4. **SSH Session Init (`libssh2_session_init_ex`)**:
   - Mengalokasikan `LIBSSH2_SESSION*`.
   - Mengatur socket mode (blocking/non-blocking).
5. **SSH Handshake (`libssh2_session_handshake`)**:
   - Melakukan negosiasi algoritma enkripsi/kunci dengan SSH server.
6. **Authentication (`libssh2_userauth_password`)**:
   - Autentikasi pengguna menggunakan username & password (atau public key).
7. **Dynamic Port Forwarding / SOCKS5 Server**:
   - Membuka listener SOCKS5 lokal (misal port 1080) menggunakan thread native.
   - Mengubah incoming connection dari `hev-socks5-tunnel` menjadi channel SSH (`libssh2_channel_direct_tcpip_ex`).
8. **Disconnect & Cleanup Resource**:
   - Menutup seluruh channel active (`libssh2_channel_free`).
   - Mengakhiri sesi SSH (`libssh2_session_disconnect`, `libssh2_session_free`).
   - Menutup socket TCP (`close(sock)`).
   - De-inisialisasi global (`libssh2_exit()`).

---

## 4. Spesifikasi Kontrak JNI Kotlin (`LibSsh2Client`)

### Package & Symbol Standard
Package Kotlin Target: `com.sivpn.cepat.ssh`
Nama Kelas: `LibSsh2Client`

Symbol Native C/C++ wajib mengikuti konvensi JNI:
```cpp
Java_com_sivpn_cepat_ssh_LibSsh2Client_startTunnel
Java_com_sivpn_cepat_ssh_LibSsh2Client_stopTunnel
Java_com_sivpn_cepat_ssh_LibSsh2Client_isTunnelRunning
Java_com_sivpn_cepat_ssh_LibSsh2Client_getStats
```

### Deklarasi Kotlin Interface:

```kotlin
package com.sivpn.cepat.ssh

object LibSsh2Client {
    init {
        System.loadLibrary("ssh")
    }

    external fun startTunnel(
        host: String,
        port: Int,
        user: String,
        pass: String,
        payload: String,
        proxyHost: String,
        proxyPort: Int,
        localSocksPort: Int
    ): Boolean

    external fun stopTunnel(): Boolean
    external fun isTunnelRunning(): Boolean
    external fun getStats(): LongArray?
}
```

---

## 5. Audit Temuan Code Existing & Mitigasi Bug

Berdasarkan audit terhadap implementasi JNI & C++ native saat ini, ditemukan beberapa potensi masalah kritis yang wajib dimitigasi:

| No | Kategori Bug / Masalah | Detail Risiko | Solusi / Mitigasi |
| :--- | :--- | :--- | :--- |
| 1 | **Socket Leak** | Socket TCP tidak ditutup dengan `close(fd)` jika `libssh2_session_handshake` gagal di tengah jalan. | Tambahkan RAII wrapper atau block `cleanup:` terpusat untuk selalu memanggil `close(sock)` jika gagal. |
| 2 | **String JNI Leak** | Pemanggilan `GetStringUTFChars` di JNI tidak selalu dipasangkan dengan `ReleaseStringUTFChars`. | Gunakan helper RAII `ScopedUtfString` agar String JNI otomatis dilepas saat keluar dari scope function. |
| 3 | **Thread Leak & Race Condition** | Pemanggilan `stopTunnel()` secara beruntun dengan `startTunnel()` berpotensi memicu race condition pada event loop thread C++. | Gunakan `std::mutex` dan `std::atomic<bool>` untuk status running dan pastikan thread SSH di-join (`thread.join()`) secara synchronous sebelum start baru. |
| 4 | **Session Cleanup Incomplete** | Channel SSH terbuka yang tidak di-`libssh2_channel_free` sebelum `libssh2_session_free` memicu memory leak di `mbedtls`. | Selalu iterasi dan tutup semua channel aktif sebelum menghancurkan session `libssh2`. |
| 5 | **Hardcoded Settings** | Sebagian port lokal atau timeout terdistribusi langsung di variabel global C. | Masukkan seluruh parameter ke struct `SshConfig` yang dikirim dari Kotlin. |

---

## 6. Hubungan Sistem antara `libssh2` dan `hev-socks5-tunnel`

```
  [ Android Application ]
            |
            v
  [ SiVpnService (VPN Controller) ]
      |                      |
      | 1. Start SSH          | 2. Start TUN Engine
      v                      v
[ LibSsh2Client ]     [ TProxyService ]
      |                      |
      v (Start Local SOCKS5) v (Read TUN & Forward to SOCKS5)
[ SOCKS5 Server ] <======= [ hev-socks5-tunnel.so ]
 (127.0.0.1:1080)
      |
      v (Encrypted SSH Tunnel)
  [ Internet / Remote SSH Server ]
```

1. **Urutan Start**: `SiVpnService` memanggil `LibSsh2Client.startTunnel()` terlebih dahulu untuk membuka Local SOCKS5 Proxy pada `127.0.0.1:1080`.
2. **Koneksi Engine**: Setelah Local SOCKS5 Proxy aktif, `SiVpnService` membuat TUN File Descriptor dan memanggil `TProxyService.TProxyStartService(configPath, fd)`.
3. **Rute Traffic**: Traffic IP dari sistem Android ditangkap oleh TUN fd -> diproses oleh `hev-socks5-tunnel.so` -> diteruskan ke Local SOCKS5 Proxy (`127.0.0.1:1080`) -> dienkripsi oleh `libssh2.so` -> dikirim ke SSH Server.
4. **Urutan Stop**: `TProxyService.TProxyStopService()` dipanggil terlebih dahulu untuk menghentikan TUN engine, disusul oleh `LibSsh2Client.stopTunnel()` untuk membersihkan sesi SSH.

---

## 7. Rangkuman Perubahan Minimal (Minimal Patch Strategy)

- **Source C HEV**: 0% perubahan (100% upstream original).
- **Source C/C++ libssh2**: 0% perubahan pada library core `libssh2`.
- **C++ JNI Wrapper**: Mengisolasikan fungsi JNI ke package `com.sivpn.cepat.ssh.LibSsh2Client`.
- **Target Stabilitas**: Bebas leak socket, thread-safe, dan teratur dalam lifecycle teardown.

---
*Status: Panduan `libssh2.md` telah berhasil dibuat.*

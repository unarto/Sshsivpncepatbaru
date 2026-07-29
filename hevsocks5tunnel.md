# Panduan Integrasi hev-socks5-tunnel (SiVPN Cepat)

Dokumen ini berisi panduan teknis dan arsitektur integrasi native engine `hev-socks5-tunnel` ke dalam aplikasi **SiVPN Cepat** tanpa mengubah source code C asli `hev-socks5-tunnel`.

---

## 1. Prinsip & Aturan Utama (Hard Rules)

1. **Zero C Modification**: Dilarang mengubah, mengedit, atau meremark source code C / submodule GitHub upstream `hev-socks5-tunnel`.
2. **Upstream Compatibility**: Struktur C/Native dipertahankan 100% agar pembaruan (upstream sync) di masa mendatang dapat dilakukan dengan mudah tanpa conflict.
3. **Strict Separation of Concerns**:
   - `libhev-socks5-tunnel.so`: Murni menangani tunnel SOCKS5/TUN di layer native C.
   - `com.sivpn.cepat.TProxyService`: Murni bertindak sebagai Adapter/Wrapper JNI ringan.
   - `com.sivpn.cepat.vpn.SiVpnService`: Bertindak sebagai Controller utama lifecycle Android VPN (VpnService, TUN establish, Coroutines, Reconnect, Service Notification, State Management).

---

## 2. Kontrak JNI & Kompilasi Native

Kompilasi NDK/Build script menggunakan flag:

```
-DPKGNAME=com/sivpn/cepat
-DCLSNAME=TProxyService
```

Dengan konfigurasi kompilasi di atas, nama fungsi JNI C yang dihasilkan oleh NDK secara otomatis dipetakan ke target kelas Kotlin:

```
com.sivpn.cepat.TProxyService
```

### Pemetaan Simbol Native JNI:

| Fungsi Native C (`hev-socks5-tunnel`) | Target Method Kotlin (`com.sivpn.cepat.TProxyService`) |
| :--- | :--- |
| `Java_com_sivpn_cepat_TProxyService_TProxyStartService` | `external fun TProxyStartService(configPath: String, fd: Int): Boolean` |
| `Java_com_sivpn_cepat_TProxyService_TProxyStopService` | `external fun TProxyStopService(): Boolean` |
| `Java_com_sivpn_cepat_TProxyService_TProxyIsRunning` | `external fun TProxyIsRunning(): Boolean` |
| `Java_com_sivpn_cepat_TProxyService_TProxyGetStats` | `external fun TProxyGetStats(): LongArray?` |

---

## 3. Arsitektur Komponen & Flow Data

```
+-------------------------------------------------------------+
|                libhev-socks5-tunnel.so                      |
|                     (Native C)                              |
+-------------------------------------------------------------+
                              ^
                              | (JNI Bridge)
                              v
+-------------------------------------------------------------+
|               com.sivpn.cepat.TProxyService                 |
|               (JNI Adapter / Pure Wrapper)                  |
+-------------------------------------------------------------+
                              ^
                              | (Direct Method Calls)
                              v
+-------------------------------------------------------------+
|             com.sivpn.cepat.vpn.SiVpnService                |
|           (Android VpnService & Orchestrator)               |
+-------------------------------------------------------------+
```

### Tugas Masing-Masing Layer:

1. **`TProxyService` (JNI Adapter)**:
   - Memuat library native `libhev-socks5-tunnel.so` via `System.loadLibrary("hev-socks5-tunnel")`.
   - Mendaftarkan fungsi `external native`.
   - Meneruskan panggilan `StartService`, `StopService`, `IsRunning`, `GetStats` langsung ke native.
   - **TIDAK BOHLEH** memiliki logika VPN, Coroutine, Notification, Reconnect, atau Setting.

2. **`SiVpnService` (VPN Controller)**:
   - Mengelola Android `VpnService` lifecycle (`onCreate`, `onStartCommand`, `onDestroy`).
   - Membuat FD (File Descriptor) TUN melalui `Builder().establish()`.
   - Mengambil setting dari `SettingsManager` (MMKV) dan membuat file konfigurasi `hev-socks5-tunnel.ini`.
   - Memanggil `TProxyService.startServiceSafe(configPath, fd)` untuk menyalakan engine native.
   - Menjalankan background monitoring (ping, speedometer, reconnect logic) dan memunculkan Foreground Notification.

---

## 4. Rencana Implementasi & Persiapan

### A. JNI Wrapper (`TProxyService.kt`)
Lokasi: `app/src/main/java/com/sivpn/cepat/TProxyService.kt`
Package: `com.sivpn.cepat`

```kotlin
package com.sivpn.cepat

import android.util.Log

object TProxyService {
    private const val TAG = "TProxyService"

    @JvmStatic
    var isLibraryLoaded: Boolean = false
        private set

    init {
        try {
            System.loadLibrary("hev-socks5-tunnel")
            isLibraryLoaded = true
            Log.i(TAG, "Berhasil memuat libhev-socks5-tunnel.so")
        } catch (e: Throwable) {
            Log.e(TAG, "Gagal memuat libhev-socks5-tunnel.so", e)
            isLibraryLoaded = false
        }
    }

    @JvmStatic external fun TProxyStartService(configPath: String, fd: Int): Boolean
    @JvmStatic external fun TProxyStopService(): Boolean
    @JvmStatic external fun TProxyIsRunning(): Boolean
    @JvmStatic external fun TProxyGetStats(): LongArray?
}
```

### B. Konfigurasi `AndroidManifest.xml`
Memastikan deklarasi Service dan izin sesuai dengan standar Android VpnService:

```xml
<service
    android:name=".vpn.SiVpnService"
    android:permission="android.permission.BIND_VPN_SERVICE"
    android:foregroundServiceType="specialUse"
    android:exported="false">
    <property
        android:name="android.app.PROPERTY_SPECIAL_USE_FGS_SUBTYPE"
        android:value="VPN service" />
    <intent-filter>
        <action android:name="android.net.VpnService" />
    </intent-filter>
</service>
```

---

## 5. Ringkasan Audit Kompatibilitas

1. **Paket JNI**: Paket `com.sivpn.cepat` cocok dengan flag `-DPKGNAME=com/sivpn/cepat`.
2. **Nama Kelas Target**: `TProxyService` cocok dengan flag `-DCLSNAME=TProxyService`.
3. **Nama Library Native**: `hev-socks5-tunnel` cocok dengan file `libhev-socks5-tunnel.so`.
4. **Isolasi Code**: Native C upstream tetap utuh tanpa modifikasi sama sekali.

---
*Status: Panduan telah dibuat. Siap melakukan langkah integrasi setelah menerima instruksi pengguna.*

# RANGKUMAN AUDIT & RENCANA PEMBERSIHAN KODE (CLEANUP PLAN)

**Project:** SiVPN Cepat Android  
**Tanggal:** 29 Juli 2026  
**Status:** Perencanaan Pembersihan Post-Refactoring  

---

## 1. MASALAH UTAMA: FILE & DOKUMEN BAPAK / AUDIT LAMA YANG TERSISA
Pasca-refactoring, beberapa file audit markdown (`AUDIT_PAYLOAD.md`, `AUDIT_NATIVE_ENGINE.md`, `AUDIT_CLEANUP.md`, `audit.md`) tidak langsung dihapus atau diperbarui statusnya, sehingga membingungkan mana pekerjaan yang sudah 100% selesai dan mana yang belum.

---

## 2. STATUS DOKUMEN AUDIT CURRENT

| File Audit MD | Status | Tindakan Rekomendasi |
|---|---|---|
| **`AUDIT_PAYLOAD.md`** | ✅ **SELESAI 100%** | **Sudah Dihapus** dari root project pada langkah sebelumnya. |
| **`AUDIT_CLEANUP.md`** | ⚠️ **SEBAGIAN SELESAI** | Berisi daftar *Unused Code*, *Dead Function*, dan *Unused Class* hasil audit awal. Perlu dieksekusi pembersihannya lalu dihapus. |
| **`AUDIT_NATIVE_ENGINE.md`** | ⚠️ **SEDANG BERJALAN** | Berisi 6 Fase Refactoring Native Engine C99 (Fase 1-4 sentralisasi utils & SSL client sudah dikerjakan). |
| **`audit.md`** | ⚠️ **SEDANG BERJALAN** | Berisi daftar komponen C++ stub / channel engine yang perlu dirapikan. |

---

## 3. DAFTAR KODE/FILE MATI (DEAD CODE & STUBS) YANG HARUS DIHAPUS / DIRAPIKAN

Berdasarkan `AUDIT_CLEANUP.md` dan sisa refactoring:

### A. Kode / File Kotlin Yang Tidak Digunakan (Unused / Dead Code)
1. **`SiVpnConfigParser.kt`** (`app/src/main/java/com/sivpn/cepat/config/SiVpnConfigParser.kt`):  
   - *Keterangan:* Class parser konfig lama yang sudah digantikan penuh oleh `SettingsManager.kt` & native config loader. 0 referensi di luar file.
   - *Rekomendasi:* Hapus file `SiVpnConfigParser.kt`.

### B. Fungsi Native C/C++ Matinya Tidak Dipanggil (Dead C Functions)
1. **`channel_create`**, **`channel_destroy`**, **`channel_connect`** di `app/src/main/cpp/native/ssh/channel.c`:  
   - *Keterangan:* Fungsi-fungsi stub lama yang 0 referensi karena pemanggilan SSH disalurkan melalui `channel_manager.c` dan `session.c`.
   - *Rekomendasi:* Hapus fungsi mati ini agar file `channel.c` bersih.
2. **`stopSshTunnel` JNI Bridge** (`app/src/main/cpp/bridge/jni_bridge.c`):  
   - *Keterangan:* Fungsi JNI bridge mati yang tidak pernah dipanggil dari Kotlin/Service.
   - *Rekomendasi:* Hapus deklarasi JNI mati.

### C. Script Shell Sisa / Utility Root
1. **`patch_proxy.sh`**, **`patch_loader.sh`**, **`patch_variable.sh`**:  
   - *Keterangan:* Script utility sementara di root directory yang tidak dipakai dalam proses Gradle build.
   - *Rekomendasi:* Pindahkan ke folder `scripts/` atau hapus jika sudah tidak digunakan.

---

## 4. RENCANA EKSEKUSI PEMBERSIHAN (ACTION PLAN)

1. **Langkah 1 (Selesai):** Hapus `AUDIT_PAYLOAD.md` (Sudah dilakukan).
2. **Langkah 2 (Rekomendasi Berikutnya):**
   - Hapus class mati `SiVpnConfigParser.kt`.
   - Hapus fungsi mati `channel_create`, `channel_destroy`, `channel_connect` dari `channel.c`.
   - Hapus JNI bridge mati `stopSshTunnel`.
3. **Langkah 3:** Hapus file `AUDIT_CLEANUP.md` setelah seluruh item di atas dibersihkan.
4. **Langkah 4:** Update `PROGRESS.md` secara terstruktur setiap kali pembersihan selesai.

---

## 5. CATATAN PERLINDUNGAN MODUL (PROTECTED MODULE COMPLIANCE)
Pembersihan ini **HANYA** menyentuh file Kotlin (`.kt`), C (`.c`), dan JNI Bridge (`.c`), serta file dokumentasi MD. **SAMA SEKALI TIDAK** menyentuh modul terlindungi seperti `CMakeLists.txt`, `libssh2`, `hev-socks5-tunnel`, `mbedtls`, atau skrip build NDK.

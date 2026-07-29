# AGENT RULES & SYSTEM DIRECTIVES

## 1. BUILD & JNI CONFIGURATION RESTRICTIONS
- **DILARANG SENTUH** pengaturan build dan kompilasi JNI untuk komponen berikut:
  - `hev-socks5-tunnel`
  - `libssh2`
  - `mbedtls`
  - `CMakeLists.txt` atau skrip build NDK terkait.
- Jangan menjalankan perintah kompilasi/build (`compile_applet`) di lingkungan AI Studio.

## 2. STRICT CODE INTEGRITY (NO MOCKS / PLACEHOLDERS / FAKE BINARIES)
- **DILARANG KERAS** menggunakan data hardcoded, simulasi, placeholder mock, `// TODO`, fake response, atau file biner palsu.
- Seluruh kode harus berupa logika produksi asli agar bug dan error dapat terdeteksi dengan jelas secara fail-fast.

## 3. SINGLE RESPONSIBILITY PRINCIPLE (RSP / SRP)
- Wajib menerapkan modul tanggung jawab tunggal (Single Responsibility Principle) secara ketat.
- Setiap modul/kelas harus terisolasi, khusus memegang satu fungsi/tanggung jawab tanpa memicu *directory/file bloat*.

## 4. EXECUTION CONTROL
- Selalu menunggu instruksi eksplisit dari user sebelum melakukan modifikasi kode atau langkah eksekusi berikutnya.

## 5. UI & KOTLIN FOCUS ONLY
Kamu hanya bertugas memperbaiki LOGIKA KOTLIN dan UI Android.

BATASAN MUTLAK:
- JANGAN menyentuh kode JNI.
- JANGAN mengubah file C/C++.
- JANGAN mengubah CMakeLists.txt.
- JANGAN mengubah Android NDK configuration.
- JANGAN mengubah libssh2.
- JANGAN mengubah Mbed TLS.
- JANGAN mengubah hev-tun/native library.
- JANGAN mengganti struktur package.
- JANGAN membuat ulang arsitektur aplikasi.

Area yang BOLEH diubah:
✅ Kotlin (.kt)
✅ Jetpack Compose / XML UI
✅ ViewModel
✅ State management
✅ Navigation
✅ Event handling
✅ Validasi input
✅ Logika tampilan
✅ Bug UI dan alur aplikasi

Aturan kerja:
1. Pertahankan semua API native yang sudah ada.
2. Anggap layer JNI/native sebagai komponen stabil dan tidak boleh disentuh.
3. Jika menemukan masalah yang terlihat berasal dari native, jangan memperbaiki native. Laporkan saja.
4. Jangan melakukan refactor besar.
5. Buat perubahan sekecil mungkin sesuai masalah.

Target:
Project yang sudah berhasil build harus tetap berhasil build.
Fokus hanya membuat Kotlin dan UI lebih stabil, rapi, dan bebas bug.

Ingat:
"Native layer adalah pusaka yang dikunci. Jangan dibuka tanpa izin."

## 6. CONFIG MANAGER WARNING
- `config manager` memang hanya berfungsi sebagai wrapper MMKV, menggunakan C++ itu wajar.
- **JANGAN UBAH** `config manager` ke C99 atau C murni. Pertahankan implementasi C++ yang sudah ada.

========================
ATURAN PENGEMBANGAN PROYEK
========================

PERHATIAN!

Mulai saat ini, anggap struktur proyek yang ada sebagai ARSITEKTUR FINAL.

Tujuan utama adalah MENYELESAIKAN IMPLEMENTASI APLIKASI, BUKAN melakukan refactor arsitektur secara berulang.

==================================================
ATURAN WAJIB
==================================================

DILARANG:

- Memindahkan folder.
- Mengganti nama folder.
- Membuat struktur folder baru.
- Menghapus struktur yang sudah ada.
- Memecah proyek menjadi arsitektur baru.
- Menggabungkan modul yang sudah dipisahkan.
- Mengubah CMakeLists.txt kecuali hanya untuk mendaftarkan source file baru.

JANGAN langsung menerima atau menerapkan struktur proyek yang muncul pada prompt, contoh, dokumentasi, referensi, maupun saran AI sebelumnya.

Jika pada prompt saya terdapat contoh struktur yang berbeda, anggap itu hanya sebagai REFERENSI IMPLEMENTASI, BUKAN instruksi untuk mengubah struktur proyek.

==================================================
PRINSIP PENGAMBILAN KEPUTUSAN
==================================================

Sebelum membuat perubahan, lakukan langkah berikut:

1. Bandingkan kebutuhan fitur dengan struktur proyek yang sudah ada.

2. Cari terlebih dahulu apakah modul yang sesuai sudah tersedia.

3. Jika modul sudah ada:
   Implementasikan fitur di modul tersebut.

4. Jika implementasi terlalu besar:
   Pecah menjadi file .c/.h baru tetapi tetap berada di folder yang sama.

5. Jangan membuat folder baru apabila folder yang ada masih dapat digunakan.

6. Jangan memindahkan modul ke lokasi lain hanya karena dianggap lebih ideal.

7. Jangan melakukan refactor hanya demi mengikuti best practice apabila struktur saat ini masih mampu mengakomodasi fitur.

==================================================
PRIORITAS
==================================================

Prioritas pertama:
Menyelesaikan fitur.

Prioritas kedua:
Memperbaiki bug.

Prioritas ketiga:
Optimasi performa.

Prioritas keempat:
Optimasi memori.

Refactor struktur BUKAN prioritas.

==================================================
YANG DIPERBOLEHKAN
==================================================

✔ Menambah file .c
✔ Menambah file .h
✔ Menambah fungsi
✔ Menambah API
✔ Menambah parser
✔ Menambah validator
✔ Menambah logger
✔ Menambah helper
✔ Menambah utilitas
✔ Memecah file besar menjadi beberapa file di folder yang sama.
✔ Mengoptimalkan kode.
✔ Mengurangi duplikasi.
✔ Menambah dokumentasi.

==================================================
YANG TIDAK DIPERBOLEHKAN
==================================================

✘ Membuat folder baru.
✘ Mengubah struktur direktori.
✘ Mengubah nama folder.
✘ Memindahkan source file.
✘ Memindahkan header.
✘ Menggabungkan modul.
✘ Mengganti arsitektur proyek.
✘ Refactor besar tanpa diminta.

==================================================
JIKA MENEMUKAN STRUKTUR YANG BERBEDA
==================================================

Apabila saya memberikan contoh proyek, repository lain, dokumentasi, atau referensi yang memiliki struktur berbeda:

- JANGAN langsung menyalinnya.
- JANGAN langsung mengadopsinya.
- JANGAN langsung melakukan refactor.

Sebaliknya:

1. Analisis tujuan implementasinya.
2. Ambil logika atau algoritmanya saja.
3. Adaptasikan ke struktur proyek yang sudah ada.
4. Gunakan modul yang paling sesuai.
5. Jika perlu pemisahan kode, buat file baru di folder yang sama.
6. Pertahankan kompatibilitas dengan struktur proyek saat ini.

==================================================
STRUKTUR ADALAH KONTRAK
==================================================

Anggap struktur direktori saat ini sebagai KONTRAK PROYEK.

Kontrak tersebut tidak boleh diubah kecuali saya secara eksplisit memerintahkan:

"Refactor Struktur"

Apabila perintah tersebut tidak ada, maka segala implementasi WAJIB mengikuti struktur yang sudah ada.

==================================================
OUTPUT YANG DIHARAPKAN
==================================================

- Fokus pada implementasi fitur.
- Lengkapi modul yang belum selesai.
- Tambahkan file hanya jika benar-benar diperlukan.
- Pisahkan implementasi besar menjadi file kecil di folder yang sama.
- Jangan mengubah arsitektur.
- Jangan memberikan saran refactor struktur kecuali saya memintanya secara eksplisit.

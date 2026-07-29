package com.sivpn.cepat.vpn

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

object NativeSshTunnel {

    @JvmField
    var isLibraryLoaded = false

    init {
        try {
            System.loadLibrary("ssh")
            isLibraryLoaded = true
        } catch (e: UnsatisfiedLinkError) {
            Log.e("NativeSshTunnel", "Gagal memuat libssh.so", e)
            isLibraryLoaded = false
        }
    }

    @JvmStatic
    fun loadFromPath(path: String): Boolean {
        if (path.isBlank()) return false
        return try {
            System.load(path)
            isLibraryLoaded = true
            true
        } catch (e: Throwable) {
            Log.e("NativeSshTunnel", "Gagal memuat libssh dari path: $path", e)
            false
        }
    }

    @JvmStatic
    external fun startSshTunnel(): Int

    @JvmStatic
    external fun stopSshTunnel()
    @JvmStatic
    external fun startHttpProxyServer(port: Int): Int

    @JvmStatic
    external fun stopHttpProxyServer()

    
    @Volatile
    private var isSshRunning: Boolean = false

    @JvmStatic
    suspend fun startSshTunnelSafe(): Int = withContext(Dispatchers.IO) {
        if (!isLibraryLoaded) {
            val msg = "Gagal menjalankan startSshTunnelSafe, libssh belum dimuat!"
            Log.e("NativeSshTunnel", msg)
            LogManager.addLog("NativeSshTunnel: $msg")
            return@withContext -1
        }

        if (isSshRunning) {
            val msg = "SSH Tunnel sudah berjalan. Mengabaikan permintaan start."
            Log.i("NativeSshTunnel", msg)
            LogManager.addLog("NativeSshTunnel: $msg")
            return@withContext 0
        }
        
        isSshRunning = true
        var result = -1
        try {
            Log.i("NativeSshTunnel", "Memulai startSshTunnelSafe")
            LogManager.addLog("NativeSshTunnel: Memulai koneksi native SSH")
            result = startSshTunnel()
            Log.i("NativeSshTunnel", "startSshTunnelSafe selesai dengan result: $result")
        } catch (e: Exception) {
            val msg = "Error saat startSshTunnelSafe: ${e.message}"
            Log.e("NativeSshTunnel", msg, e)
            LogManager.addLog("NativeSshTunnel: $msg")
        } finally {
            isSshRunning = false
        }

        return@withContext result
    }

    @JvmStatic
    @Synchronized
    fun stopSshTunnelSafe() {
        if (!isLibraryLoaded) {
            val msg = "Gagal menjalankan stopSshTunnelSafe, libssh belum dimuat!"
            Log.e("NativeSshTunnel", msg)
            LogManager.addLog("NativeSshTunnel: $msg")
            return
        }

        if (!isSshRunning) {
            val msg = "SSH Tunnel belum berjalan. Mengabaikan permintaan stop."
            Log.d("NativeSshTunnel", msg)
            return
        }

        try {
            Log.i("NativeSshTunnel", "Menghentikan stopSshTunnelSafe")
            LogManager.addLog("NativeSshTunnel: Menghentikan stopSshTunnelSafe")
            stopSshTunnel()
        } catch (e: Exception) {
            val msg = "Error saat stopSshTunnelSafe: ${e.message}"
            Log.e("NativeSshTunnel", msg, e)
            LogManager.addLog("NativeSshTunnel: $msg")
        }
    }
    
    @JvmStatic
    fun isServiceRunning(): Boolean {
        return isSshRunning
    }
}

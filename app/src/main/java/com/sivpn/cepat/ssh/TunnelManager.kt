package com.sivpn.cepat.ssh

import android.content.Context
import android.os.ParcelFileDescriptor
import com.sivpn.cepat.config.SettingsManager
import com.sivpn.cepat.vpn.LogManager
import kotlinx.coroutines.*
import java.net.InetSocketAddress
import java.net.ServerSocket
import java.net.Socket
import java.io.InputStream

object TunnelManager {
    private var sshJob: Job? = null
    @Volatile var isSshRunning = false
    private var serverSocket: ServerSocket? = null
    
    suspend fun startTunnel(context: Context, scope: CoroutineScope): Boolean {
        val settings = SettingsManager(context)
        
        val proxyHost = settings.getProxyHost()
        val proxyPort = settings.getProxyPort()
        val payload = settings.getPayload()
        val sni = settings.getSni()
        val tlsVersion = settings.getForcingTls()
        
        var actualSshHost = settings.getSshHost()
        var actualSshPort = settings.getSshPort()
        val tfoEnabled = settings.getTcpFastOpenEnabled()
        val autoPingEnabled = settings.getAutoPing()
        
        val username = settings.getSshUsername()
        val password = settings.getSshPassword()
        val fingerprint = settings.getSshFingerprint()
        val knownHostsPath = java.io.File(context.filesDir, "known_hosts").absolutePath
        val socksPort = settings.getHevSocks5Port()

        if (payload.isNotEmpty() || sni.isNotEmpty() || (proxyHost.isNotEmpty() && proxyPort > 0)) {
            LogManager.addLog("Using Native Payload/SNI Injector Mode...")
        }
        
        val tlsVersionInt = if (sni.isNotEmpty()) 1 else 0

        // ---------------------------------------------------------
        // Call Native Engine to handle everything (Session, SOCKS, Multiplexing, Payload)
        // ---------------------------------------------------------
        LogManager.addLog("Memulai Native Channel Engine (C)...")
        isSshRunning = true
        
        settings.setStr("known_hosts_path", knownHostsPath)
        settings.setInt("tls_version_int", tlsVersionInt)

        val result = LibSsh2Client.startTunnel()
        
        if (result != 0) {
            LogManager.addLog("Gagal memulai Native Engine: error kode $result")
            isSshRunning = false
            return false
        }
        
        LogManager.addLog("Native Engine berjalan. Menunggu kesiapan port SOCKS: $socksPort...")

        // Wait until Native SOCKS listener is ready
        var socksReady = false
        withContext(Dispatchers.IO) {
            for (i in 1..150) {
                if (!isSshRunning) break
                try {
                    Socket().use { socket ->
                        socket.connect(InetSocketAddress("127.0.0.1", socksPort), 100)
                    }
                    socksReady = true
                    break
                } catch (e: Exception) {
                    delay(100)
                }
            }
        }
        
        return socksReady
    }
    
    suspend fun stopTunnel() {
        if (!isSshRunning) return
        LogManager.addLog("Stopping SSH tunnel...")
        isSshRunning = false
        LibSsh2Client.stopTunnel()
    }
}

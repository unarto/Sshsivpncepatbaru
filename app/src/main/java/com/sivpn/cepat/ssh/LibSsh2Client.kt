package com.sivpn.cepat.ssh

import com.sivpn.cepat.vpn.NativeSshTunnel

object LibSsh2Client {
    val isLibraryLoaded: Boolean
        get() = NativeSshTunnel.isLibraryLoaded

    fun startTunnel(): Int {
        return NativeSshTunnel.startSshTunnel()
    }

    fun stopTunnel() {
        NativeSshTunnel.stopSshTunnelSafe()
    }
}

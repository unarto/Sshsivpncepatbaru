========================================================
        ANDROID VPN SSH CHANNEL ENGINE ARCHITECTURE
========================================================


                    ANDROID APPLICATION
                            |
                            |
                    Android VpnService
                            |
                            |
                            ▼
                 +--------------------+
                 |    TUN Interface   |
                 |  (VPN Virtual NIC) |
                 +--------------------+
                            |
                            |
                            ▼
              +-------------------------+
              |   hev-socks5-tunnel     |
              |                         |
              | TUN Traffic Processor   |
              | TCP/UDP Forward Engine  |
              +-------------------------+
                            |
                            |
                 Local FD / Socket Bridge
                            |
                            ▼


========================================================
              NATIVE CHANNEL ENGINE (C/C++)
========================================================


                 +--------------------+
                 |  Channel Manager   |
                 |                    |
                 | - Channel Pool     |
                 | - Session Manager  |
                 | - Event Loop       |
                 | - Buffer Manager   |
                 +--------------------+
                            |
                            |
                            ▼

                 +--------------------+
                 |   libssh2 Engine   |
                 |                    |
                 | SSH Session        |
                 | Authentication     |
                 | Channel Multiplex  |
                 | Direct TCPIP       |
                 +--------------------+

                            |
                            |
              One SSH Session Multiple Channel

              +-------------+-------------+
              |             |             |
              ▼             ▼             ▼

        +----------+  +----------+  +----------+
        |Channel 1 |  |Channel 2 |  |Channel N |
        |TCP Flow  |  |TCP Flow  |  |TCP Flow  |
        +----------+  +----------+  +----------+

              |             |             |
              |             |             |
              ▼             ▼             ▼

        Destination A  Destination B  Destination C



========================================================
              SSH TRANSPORT SECURITY LAYER
========================================================


                 +--------------------+
                 |     mbedTLS        |
                 |                    |
                 | Cryptography       |
                 | SHA256             |
                 | AES                |
                 | HMAC               |
                 | Random Generator   |
                 | TLS Components    |
                 +--------------------+

                            |
                            |
                            ▼

                 Encrypted SSH Tunnel


========================================================
              CUSTOM PAYLOAD ENGINE
========================================================


                +----------------------+
                |  Payload Builder     |
                |                      |
                | - Custom Request     |
                | - HTTP Header        |
                | - SNI/Host           |
                | - Proxy Header       |
                | - Injection Data     |
                +----------------------+

                            |
                            |
                            ▼

                +----------------------+
                | TCP Connection Layer |
                |                      |
                | DNS Resolve         |
                | Socket Connect      |
                | Timeout             |
                | Keepalive           |
                +----------------------+

                            |
                            |
                            ▼


========================================================
                    SERVER SIDE
========================================================


                 Internet Network

                        |
                        |
                        ▼

              +-------------------+
              | SSH Server        |
              |                   |
              | SSH Daemon        |
              | Direct TCPIP      |
              +-------------------+

                        |
                        |
                        ▼

              Destination Internet



========================================================
                DATA FLOW DETAIL
========================================================


OUTBOUND:

Application
    |
    ▼
TUN
    |
    ▼
hev-socks5-tunnel
    |
    ▼
Native Channel Engine
    |
    ▼
Custom Payload (optional)
    |
    ▼
TCP Socket
    |
    ▼
libssh2
    |
    ▼
mbedTLS Crypto Layer
    |
    ▼
SSH Server
    |
    ▼
Internet



INBOUND:

Internet
    |
    ▼
SSH Server
    |
    ▼
libssh2 Channel
    |
    ▼
Decrypt / Verify
    |
    ▼
Channel Buffer
    |
    ▼
hev-socks5-tunnel
    |
    ▼
TUN Interface
    |
    ▼
Application



========================================================
                 COMPONENT RESPONSIBILITY
========================================================


hev-socks5-tunnel
|
+-- Mengurus TUN
+-- Routing traffic
+-- TCP/UDP forwarding
+-- Connection lifecycle


libssh2
|
+-- SSH handshake
+-- Authentication
+-- Channel creation
+-- Direct TCPIP tunnel
+-- Multiplexing


mbedTLS
|
+-- Cryptographic backend
+-- Hash
+-- Encryption support
+-- Secure random


Custom Payload
|
+-- Membentuk request awal
+-- Custom handshake
+-- Proxy compatibility


Channel Engine
|
+-- Manage many connections
+-- One SSH session
+-- Many SSH channels
+-- Low latency
+-- High concurrency



========================================================
                 FINAL ARCHITECTURE
========================================================


        VpnService
             |
             ▼
        hev-socks5-tunnel
             |
             ▼
      Channel Engine JNI
             |
             |
       +-----+------+
       |            |
       ▼            ▼
    libssh2      mbedTLS
       |
       ▼
 SSH Multiplex Session
       |
       ▼
 Multiple Direct TCP Channels
       |
       ▼
     Internet


========================================================

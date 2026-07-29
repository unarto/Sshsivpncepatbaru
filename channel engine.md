=========================================================
CHANNEL ENGINE (Native C)
=========================================================

Tujuan:
Channel Engine menjadi satu-satunya jalur komunikasi.
Semua data dari SSH, HTTP Payload, WebSocket, SSL, dan SOCKS5
harus melewati Channel Engine sehingga engine lain tidak perlu
mengetahui implementasi socket.

Flow:

VPNService (Kotlin)
        │
JNI Bridge
        │
Config Builder
        │
Native Config
        │
Channel Engine
        │
 ┌──────┼────────────────────────────────────────────┐
 │      │              │             │               │
 │      │              │             │               │
SSH   HTTP Payload   WebSocket    SSL/TLS      HTTP Proxy
 │      │              │             │               │
 └──────┴──────────────┴─────────────┴───────────────┘
                        │
                 TCP Transport
                        │
                   Internet

=========================================================
Folder Structure
=========================================================

native/

├── channel/
│
├── channel.c
├── channel.h
│
├── channel_manager.c
├── channel_manager.h
│
├── channel_config.c
├── channel_config.h
│
├── payload_builder.c
├── payload_builder.h
│
├── payload_parser.c
├── payload_parser.h
│
├── payload_variable.c
├── payload_variable.h
│
├── split_engine.c
├── split_engine.h
│
├── websocket.c
├── websocket.h
│
├── websocket_frame.c
├── websocket_frame.h
│
├── http_request.c
├── http_request.h
│
├── http_response.c
├── http_response.h
│
├── proxy_connect.c
├── proxy_connect.h
│
├── ssl_client.c
├── ssl_client.h
│
├── keepalive.c
├── keepalive.h
│
├── reconnect.c
├── reconnect.h
│
├── tcp_transport.c
├── tcp_transport.h
│
├── buffer.c
├── buffer.h
│
├── logger.c
├── logger.h
│
└── utils.c
└── utils.h

=========================================================
Channel Lifecycle
=========================================================

channel_create()

↓

channel_config_load()

↓

channel_connect()

↓

optional:
    ssl_connect()

↓

optional:
    proxy_connect()

↓

optional:
    websocket_handshake()

↓

optional:
    payload_inject()

↓

ssh_connect()

↓

hev-socks5-tunnel

↓

channel_loop()

↓

channel_close()

=========================================================
Channel API
=========================================================

channel_create()

channel_destroy()

channel_connect()

channel_disconnect()

channel_send()

channel_recv()

channel_flush()

channel_close()

channel_is_connected()

=========================================================
Payload Builder
=========================================================

Support:

GET

POST

CONNECT

HEAD

OPTIONS

PUT

DELETE

PATCH

=========================================================
Placeholder
=========================================================

[host]

[port]

[host_port]

[protocol]

[method]

[crlf]

[cr]

[lf]

[lfcr]

[ua]

[random]

[rotate]

[auth]

[proxy]

=========================================================
Custom Header
=========================================================

Host:

Connection:

Proxy-Connection:

Keep-Alive:

User-Agent:

Accept:

Accept-Encoding:

Accept-Language:

Referer:

Origin:

Pragma:

Cache-Control:

Upgrade:

Upgrade-Insecure-Requests:

Cookie:

X-Online-Host:

X-Forward-Host:

X-Forwarded-Host:

X-Forwarded-For:

X-Host:

X-Real-Host:

X-Requested-With:

=========================================================
Split Engine
=========================================================

NORMAL_SPLIT

INSTANT_SPLIT

DELAY_SPLIT

REVERSE_SPLIT

FRONT_SPLIT

BACK_SPLIT

CUSTOM_SPLIT

=========================================================
WebSocket
=========================================================

Handshake

Upgrade

Path

Host Override

Origin

User-Agent

Cookie

Sec-WebSocket-Key

Sec-WebSocket-Version

Sec-WebSocket-Extensions

Masking

Binary Frame

Text Frame

Ping

Pong

Close

=========================================================
SSL/TLS
=========================================================

TLS 1.2

TLS 1.3

SNI

ALPN

Certificate Verify

Hostname Verify

=========================================================
Proxy
=========================================================

Direct

HTTP Proxy

HTTPS Proxy

SOCKS4

SOCKS5

CONNECT

=========================================================
KeepAlive
=========================================================

TCP KeepAlive

SSH Ignore

WebSocket Ping

HTTP HEAD

HTTP GET

=========================================================
Reconnect
=========================================================

Auto Retry

Retry Delay

Retry Count

Exponential Backoff

Network Change

=========================================================
Buffer
=========================================================

Dynamic Buffer

Ring Buffer

Receive Buffer

Send Buffer

=========================================================
Transport
=========================================================

tcp_connect()

tcp_send()

tcp_recv()

tcp_close()

=========================================================
Main Data Flow
=========================================================

VPN

↓

hev-socks5-tunnel

↓

Channel Engine

↓

Payload Builder

↓

Split Engine

↓

SSL

↓

WebSocket

↓

HTTP CONNECT

↓

SSH (libssh2)

↓

Remote SSH Server

↓

SOCKS5 Channel

↓

Internet

=========================================================
Keuntungan
=========================================================

✔ Semua protokol memakai Channel Engine yang sama.

✔ Payload cukup dibangun sekali oleh Payload Builder.

✔ Split hanya bekerja pada payload, tidak mengganggu SSH.

✔ WebSocket, SSL, dan HTTP CONNECT dapat diaktifkan atau dimatikan melalui konfigurasi tanpa mengubah engine utama.

✔ Penambahan backend baru (misalnya Xray, Trojan, Hysteria) cukup membuat adapter/channel baru tanpa mengubah VpnService atau hev-socks5-tunnel.

✔ Arsitektur menjadi modular, mudah diuji, dan mudah diperluas.

#include "payload_template.h"

const char* payload_template_get(PayloadTemplateType type) {
    switch (type) {
        case PAYLOAD_TEMPLATE_DIRECT:
            return "GET http://[host_port]/ HTTP/1.1[crlf]Host: [host_port][crlf]User-Agent: [ua][crlf]Connection: Keep-Alive[crlf][crlf]";
            
        case PAYLOAD_TEMPLATE_PROXY:
            return "CONNECT [host_port] [protocol][crlf]Host: [host_port][crlf]User-Agent: [ua][crlf]Proxy-Connection: Keep-Alive[crlf][crlf]";
            
        case PAYLOAD_TEMPLATE_WEBSOCKET:
            return "GET / HTTP/1.1[crlf]Host: [host_port][crlf]Upgrade: websocket[crlf]Connection: Upgrade[crlf]User-Agent: [ua][crlf]Sec-WebSocket-Key: [random][crlf]Sec-WebSocket-Version: 13[crlf][crlf]";
            
        default:
            return "[method] [host_port] [protocol][crlf]Host: [host_port][crlf][crlf]";
    }
}

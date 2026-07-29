#include "config_loader.h"
#include "../mmkv/mmkv_bridge.h"
#include "../payload/payload_manager.h"
#include "../common/logger.h"
#include <string.h>

void config_manager_init(void) {
    mmkv_init();
}

bool config_manager_read_config(TunnelConfig *cfg) {
    if (!cfg) return false;
    memset(cfg, 0, sizeof(TunnelConfig));
    char str_buf[2048];

    if (mmkv_get_string("ssh_host", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->host, str_buf, sizeof(cfg->host) - 1);
    }
    cfg->port = mmkv_get_int("ssh_port", 22);

    if (mmkv_get_string("ssh_username", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->user, str_buf, sizeof(cfg->user) - 1);
    }
    if (mmkv_get_string("ssh_password", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->pass, str_buf, sizeof(cfg->pass) - 1);
    }

    if (mmkv_get_string("ssh_fingerprint", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->fingerprint, str_buf, sizeof(cfg->fingerprint) - 1);
    }
    if (mmkv_get_string("known_hosts_path", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->known_hosts_path, str_buf, sizeof(cfg->known_hosts_path) - 1);
    }

    cfg->socks_port = mmkv_get_int("hev_socks5_port", 1080);
    cfg->hotshare_enabled = mmkv_get_bool("hotshare_enabled", false);
    cfg->http_proxy_port = mmkv_get_int("hotshare_http_port", 8080);

    if (mmkv_get_string("proxy_host", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->proxy_host, str_buf, sizeof(cfg->proxy_host) - 1);
    }
    cfg->proxy_port = mmkv_get_int("proxy_port", 0);

    if (mmkv_get_string("sni", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->sni, str_buf, sizeof(cfg->sni) - 1);
    }
    cfg->tls_version = mmkv_get_int("tls_version_int", cfg->sni[0] != '\0' ? 1 : 0);
    cfg->auto_ping = mmkv_get_bool("auto_ping", false);

    // --- Custom Payload Engine Config ---
    if (mmkv_get_string("payload", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->raw_payload, str_buf, sizeof(cfg->raw_payload) - 1);
    }
    cfg->user_agent_type = mmkv_get_int("user_agent_type", 0);
    if (mmkv_get_string("custom_user_agent", str_buf, sizeof(str_buf), "") == 0) {
        strncpy(cfg->custom_user_agent, str_buf, sizeof(cfg->custom_user_agent) - 1);
    }
    cfg->payload_template_type = mmkv_get_int("payload_template_type", 0);
    // ------------------------------------

    cfg->modo_dropbear = mmkv_get_bool("modo_dropbear", false) || mmkv_get_bool("dropbear_mode", false);

    // Call Payload Manager to parse and populate payload_chunks dynamically!
    if (payload_manager_build(cfg) != 0) {
        LOGE("Failed to build payload chunks from config");
        cfg->num_payload_chunks = 0;
    }

    return true;
}

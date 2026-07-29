#include <jni.h>
#include <pthread.h>
#include <string.h>
#include "../native/ssh/channel.h"
#include "../native/config/config_loader.h"

static pthread_t engine_thread;
static TunnelConfig g_config;
static bool engine_running = false;

static void* engine_thread_func(void *arg) {
    if (ev_init() != 0) {
        LOGE("Failed to init event loop");
        return NULL;
    }
    
    cm_init();
    cm_start_socks_server(g_config.socks_port);
    
    if (!sm_start(&g_config)) {
        LOGE("Failed to start session manager");
    } else {
        ev_loop();
    }
    
    sm_stop();
    cm_cleanup();
    ev_cleanup();
    
    return NULL;
}

JNIEXPORT jint JNICALL Java_com_sivpn_cepat_vpn_NativeSshTunnel_startSshTunnel(JNIEnv *env, jclass clazz) {
    if (engine_running) return -1;

    config_manager_init();
    if (!config_manager_read_config(&g_config)) {
        LOGE("Failed to read config from MMKV");
        return -1;
    }

    libssh2_init(0);
    
    engine_running = true;
    pthread_create(&engine_thread, NULL, engine_thread_func, NULL);
    
    return 0;
}

JNIEXPORT void JNICALL Java_com_sivpn_cepat_vpn_NativeSshTunnel_stopSshTunnel(JNIEnv *env, jclass clazz) {
    if (!engine_running) return;
    
    ev_stop();
    pthread_join(engine_thread, NULL);
    engine_running = false;
    
    libssh2_exit();
}

JNIEXPORT jint JNICALL Java_com_sivpn_cepat_vpn_NativeSshTunnel_startHttpProxyServer(JNIEnv *env, jclass clazz, jint port) {
    cm_start_http_proxy(port);
    return 0;
}

JNIEXPORT void JNICALL Java_com_sivpn_cepat_vpn_NativeSshTunnel_stopHttpProxyServer(JNIEnv *env, jclass clazz) {
    cm_stop_http_proxy();
}

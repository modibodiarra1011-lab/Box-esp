#include "wifi_lab.h"
#include "lab_config.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs.h"
#include "mdns.h"
#include <string.h>
#include <stdio.h>

static const char *TAG="wifi_lab";
static bool s_sta_ok=false;
static char s_sta_ip[20]="-";
static esp_netif_t *s_ap=NULL,*s_sta=NULL;

static void evt(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_ok=false; strlcpy(s_sta_ip,"-",sizeof(s_sta_ip)); esp_wifi_connect();
    } else if (base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e=data; snprintf(s_sta_ip,sizeof(s_sta_ip),IPSTR,IP2STR(&e->ip_info.ip)); s_sta_ok=true;
        ESP_LOGI(TAG,"STA connected %s",s_sta_ip);
    }
}

void wifi_lab_start(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_ap=esp_netif_create_default_wifi_ap(); s_sta=esp_netif_create_default_wifi_sta();
    wifi_init_config_t icfg=WIFI_INIT_CONFIG_DEFAULT(); ESP_ERROR_CHECK(esp_wifi_init(&icfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&evt,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&evt,NULL));

    wifi_config_t ap={0}; strlcpy((char*)ap.ap.ssid,g_lab_cfg.ap_ssid,sizeof(ap.ap.ssid));
    strlcpy((char*)ap.ap.password,g_lab_cfg.ap_pass,sizeof(ap.ap.password));
    ap.ap.ssid_len=strlen(g_lab_cfg.ap_ssid); ap.ap.channel=1; ap.ap.max_connection=8; ap.ap.authmode=WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,&ap));
    wifi_config_t sta={0}; strlcpy((char*)sta.sta.ssid,g_lab_cfg.sta_ssid,sizeof(sta.sta.ssid)); strlcpy((char*)sta.sta.password,g_lab_cfg.sta_pass,sizeof(sta.sta.password));
    sta.sta.scan_method=WIFI_FAST_SCAN; sta.sta.sort_method=WIFI_CONNECT_AP_BY_SIGNAL;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    if(g_lab_cfg.sta_ssid[0]) esp_wifi_connect();
    if (mdns_init() == ESP_OK) {
        mdns_hostname_set("esp32-lab");
        mdns_instance_name_set("ESP32 LAB");
        mdns_service_add("ESP32 LAB", "_http", "_tcp", HTTP_PORT, NULL, 0);
        ESP_LOGI(TAG, "mDNS: esp32-lab.local");
    }
    ESP_LOGI(TAG,"AP %s at 192.168.4.1",g_lab_cfg.ap_ssid);
}

bool wifi_lab_sta_connected(void){return s_sta_ok;}
const char *wifi_lab_sta_ip(void){return s_sta_ip;}

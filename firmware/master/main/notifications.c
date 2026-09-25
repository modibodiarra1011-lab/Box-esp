#include "notifications.h"
#include "lab_config.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static void urlenc(const char*in,char*out,size_t cap){const char*h="0123456789ABCDEF";size_t o=0;while(*in&&o+4<cap){unsigned char c=*in++;if(isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')out[o++]=c;else{out[o++]='%';out[o++]=h[c>>4];out[o++]=h[c&15];}}out[o]=0;}
bool notifications_send(const char*text){if(!g_lab_cfg.whatsapp_phone[0]||!g_lab_cfg.whatsapp_api[0])return false;char msg[640],url[1024];urlenc(text,msg,sizeof(msg));snprintf(url,sizeof(url),"https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s",g_lab_cfg.whatsapp_phone,msg,g_lab_cfg.whatsapp_api);esp_http_client_config_t c={.url=url,.timeout_ms=15000,.crt_bundle_attach=esp_crt_bundle_attach};esp_http_client_handle_t h=esp_http_client_init(&c);if(!h)return false;esp_err_t r=esp_http_client_perform(h);int code=esp_http_client_get_status_code(h);esp_http_client_cleanup(h);return r==ESP_OK&&code>=200&&code<300;}
void notifications_test(char*out,size_t cap){bool ok=notifications_send("ESP32 LAB : coucou 👋 la liaison WhatsApp fonctionne.");snprintf(out,cap,"{\"ok\":%s}",ok?"true":"false");}

#include "lab_config.h"
#include "wifi_lab.h"
#include "storage.h"
#include "worker_pool.h"
#include "job_engine.h"
#include "web_server.h"
#include "dht11.h"
#include "led_status.h"
#include "notifications.h"
#include "agent.h"
#include "ota_manager.h"
#include "usb_avr.h"
#include "reports.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include <math.h>
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG="master"; float g_temp=NAN,g_humidity=NAN;
static void sensor_task(void*arg){while(1){float t,h;if(dht11_read(&t,&h)){g_temp=t;g_humidity=h;char b[120];snprintf(b,sizeof(b),"%lld,%.1f,%.1f\n",(long long)(esp_timer_get_time()/1000000),t,h);storage_append_text("/sd/LOGS/temperature.csv",b);}vTaskDelay(pdMS_TO_TICKS(2500));}}
static void heartbeat_task(void*arg){while(1){if(storage_ready())reports_write_snapshot("heartbeat");vTaskDelay(pdMS_TO_TICKS(60000));}}
void app_main(void){
    esp_err_t r=nvs_flash_init();if(r==ESP_ERR_NVS_NO_FREE_PAGES||r==ESP_ERR_NVS_NEW_VERSION_FOUND){ESP_ERROR_CHECK(nvs_flash_erase());r=nvs_flash_init();}ESP_ERROR_CHECK(r);
    lab_config_load(&g_lab_cfg);ESP_LOGI(TAG,"ESP32 LAB %s",LAB_VERSION);
    esp_chip_info_t info;esp_chip_info(&info);ESP_LOGI(TAG,"chip=%d cores=%d revision=%d",info.model,info.cores,info.revision);
    wifi_lab_start();storage_init();
    if(storage_ready())storage_append_text("/sd/LOGS/temperature.csv","timestamp,temperature,humidity\n");
    led_status_init();led_status_mode("ready");worker_pool_start();job_engine_start();reports_init();agent_start();ota_manager_start();
    r=usb_avr_init();ESP_LOGI(TAG,"USB AVR %s",r==ESP_OK?"READY":"NOT READY");
    web_server_start();
    xTaskCreate(sensor_task,"dht11",3072,NULL,4,NULL);xTaskCreate(heartbeat_task,"reporter",3072,NULL,2,NULL);
    esp_err_t confirm = esp_ota_mark_app_valid_cancel_rollback();
    if (confirm == ESP_OK) ESP_LOGI(TAG, "OTA application confirmed valid");
    else ESP_LOGI(TAG, "OTA confirmation state: %s", esp_err_to_name(confirm));
    ESP_LOGI(TAG,"MASTER %s READY at 192.168.4.1",LAB_VERSION);
    while(1){vTaskDelay(pdMS_TO_TICKS(1000));}
}

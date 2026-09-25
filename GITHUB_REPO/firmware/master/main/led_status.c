#include "led_status.h"
#include "lab_config.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
static const char*TAG="led"; static led_strip_handle_t s_strip=NULL; static TaskHandle_t s_task=NULL; static char s_mode[16]="ready";
void led_status_set(unsigned r,unsigned g,unsigned b){if(!s_strip)return; led_strip_set_pixel(s_strip,0,r,g,b); led_strip_refresh(s_strip);}
void led_status_init(void){led_strip_config_t c={.strip_gpio_num=g_lab_cfg.rgb_gpio,.max_leds=1}; led_strip_rmt_config_t r={.resolution_hz=10*1000*1000,.flags.with_dma=false}; if(led_strip_new_rmt_device(&c,&r,&s_strip)==ESP_OK)led_status_set(0,64,0);}
static void task(void*arg){while(1){if(strcmp(s_mode,"ready")==0)led_status_set(0,64,8);else if(strcmp(s_mode,"work")==0)led_status_set(0,32,120);else if(strcmp(s_mode,"flash")==0)led_status_set(90,0,120);else if(strcmp(s_mode,"update")==0)led_status_set(120,120,120);else if(strcmp(s_mode,"error")==0)led_status_set(120,0,0);else if(strcmp(s_mode,"warn")==0)led_status_set(120,80,0);else led_status_set(0,20,40);vTaskDelay(pdMS_TO_TICKS(250));}}
void led_status_mode(const char*mode){strlcpy(s_mode,mode,sizeof(s_mode)); if(!s_task)xTaskCreate(task,"led_anim",2048,NULL,2,&s_task);}

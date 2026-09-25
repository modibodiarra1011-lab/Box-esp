#include "dht11.h"
#include "lab_config.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static bool wait_level(int level, int timeout_us){
    int t=0; while(gpio_get_level(DHT_GPIO)!=level){esp_rom_delay_us(1); if(++t>=timeout_us)return false;} return true;
}

bool dht11_read(float *temp_c,float *humidity)
{
    uint8_t data[5]={0}; if(!temp_c||!humidity)return false;
    gpio_set_direction(DHT_GPIO,GPIO_MODE_OUTPUT); gpio_set_level(DHT_GPIO,0); vTaskDelay(pdMS_TO_TICKS(20)); gpio_set_level(DHT_GPIO,1); esp_rom_delay_us(30);
    gpio_set_direction(DHT_GPIO,GPIO_MODE_INPUT); gpio_pullup_en(DHT_GPIO); if(!wait_level(0,100))return false; if(!wait_level(1,100))return false; if(!wait_level(0,100))return false;
    for(int i=0;i<40;i++){
        if(!wait_level(1,100))return false; int us=0; while(gpio_get_level(DHT_GPIO)){esp_rom_delay_us(1); if(++us>100)return false;} data[i/8]<<=1; if(us>40)data[i/8]|=1;
    }
    uint8_t sum=(uint8_t)(data[0]+data[1]+data[2]+data[3]); if(sum!=data[4])return false;
    *humidity=(float)data[0]+data[1]*0.1f; *temp_c=(float)data[2]+data[3]*0.1f; return true;
}

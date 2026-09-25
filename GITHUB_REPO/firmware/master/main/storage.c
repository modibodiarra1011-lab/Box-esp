#include "storage.h"
#include "lab_config.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

static const char *TAG="storage"; static bool s_ready=false; static sdmmc_card_t *s_card=NULL;
const char *storage_root(void){return "/sd";} bool storage_ready(void){return s_ready;}

void storage_init(void)
{
    spi_bus_config_t bus={.mosi_io_num=SD_MOSI_GPIO,.miso_io_num=SD_MISO_GPIO,.sclk_io_num=SD_SCK_GPIO,.quadwp_io_num=-1,.quadhd_io_num=-1,.max_transfer_sz=4096};
    esp_err_t r=spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO); if(r!=ESP_OK){ESP_LOGE(TAG,"SPI init: %s",esp_err_to_name(r));return;}
    sdmmc_host_t host=SDSPI_HOST_DEFAULT(); sdspi_device_config_t slot=SDSPI_DEVICE_CONFIG_DEFAULT(); slot.gpio_cs=SD_CS_GPIO; slot.host_id=SPI2_HOST;
    esp_vfs_fat_mount_config_t m={.format_if_mount_failed=false,.max_files=12,.allocation_unit_size=16*1024};
    r=esp_vfs_fat_sdspi_mount("/sd",&host,&slot,&m,&s_card); if(r!=ESP_OK){ESP_LOGW(TAG,"SD not mounted: %s",esp_err_to_name(r)); return;}
    s_ready=true; ESP_LOGI(TAG,"SD mounted"); storage_prepare_tree();
}

static void mk(const char *p){mkdir(p,0775);}
esp_err_t storage_prepare_tree(void)
{
    if(!s_ready)return ESP_ERR_INVALID_STATE;
    const char *d[]={"/sd/FIRMWARE","/sd/FIRMWARE/MASTER","/sd/FIRMWARE/WORKER","/sd/FIRMWARE/ESP32","/sd/FIRMWARE/ESP32S3","/sd/FIRMWARE/AVR","/sd/FIRMWARE/AVR/UNO","/sd/FIRMWARE/AVR/NANO","/sd/PROJECTS","/sd/PROJECTS/PREPARED","/sd/PROJECTS/IMPORTED_35","/sd/PROJECTS/MY_PROJECTS","/sd/COMPONENTS","/sd/LIBRARIES","/sd/TESTS","/sd/REPORTS","/sd/LOGS","/sd/BACKUPS","/sd/CONFIG","/sd/DATABASE","/sd/UPDATES","/sd/AI"};
    for(size_t i=0;i<sizeof(d)/sizeof(d[0]);++i)mk(d[i]);
    storage_write_text("/sd/CONFIG/README.txt","ESP32 LAB : configuration locale. Les secrets ne sont pas stockes dans cette carte par defaut.\n");
    storage_write_text("/sd/FIRMWARE/README.txt","MASTER/WORKER/ESP32/ESP32S3/AVR sont organises automatiquement par les scripts du depot.\n");
    return ESP_OK;
}

static bool safe(const char *p){return p && strncmp(p,"/sd/",4)==0 && strstr(p,"..") == NULL && strlen(p)<240;}
esp_err_t storage_write_text(const char *p,const char *t){if(!safe(p)||!s_ready)return ESP_ERR_INVALID_ARG;FILE*f=fopen(p,"w");if(!f)return ESP_FAIL;fputs(t,f);fclose(f);return ESP_OK;}
esp_err_t storage_append_text(const char *p,const char *t){if(!safe(p)||!s_ready)return ESP_ERR_INVALID_ARG;FILE*f=fopen(p,"a");if(!f)return ESP_FAIL;fputs(t,f);fclose(f);return ESP_OK;}
esp_err_t storage_sha256_file(const char *p,char hex[65])
{
    if(!safe(p)||!hex)return ESP_ERR_INVALID_ARG; FILE*f=fopen(p,"rb"); if(!f)return ESP_ERR_NOT_FOUND;
    mbedtls_sha256_context c; mbedtls_sha256_init(&c); mbedtls_sha256_starts(&c,0); uint8_t b[4096]; size_t n;
    while((n=fread(b,1,sizeof(b),f))>0)mbedtls_sha256_update(&c,b,n); fclose(f); uint8_t d[32]; mbedtls_sha256_finish(&c,d); mbedtls_sha256_free(&c);
    const char*h="0123456789abcdef"; for(int i=0;i<32;i++){hex[i*2]=h[d[i]>>4];hex[i*2+1]=h[d[i]&15];}hex[64]=0;return ESP_OK;
}

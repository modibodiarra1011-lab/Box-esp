#include "storage.h"
#include "lab_config.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "psa/crypto.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
static const char *TAG="storage";
static bool s_ready=false;
static sdmmc_card_t *s_card=NULL;
const char *storage_root(void){return "/sd";}
bool storage_ready(void){return s_ready;}
static void mk(const char*p){if(mkdir(p,0775)!=0&&errno!=EEXIST)ESP_LOGW(TAG,"mkdir %s: %d",p,errno);}
static void write_if_missing(const char*p,const char*t){if(access(p,F_OK)!=0)(void)storage_write_text(p,t);}
static bool safe(const char*p){return p&&strncmp(p,"/sd/",4)==0&&strstr(p,"..")==NULL&&strlen(p)<240;}
static bool safe_name(const char*name){if(!name||!*name||strlen(name)>80||!strcmp(name,".")||!strcmp(name,".."))return false;for(const unsigned char*p=(const unsigned char*)name;*p;++p)if(!isalnum(*p)&&*p!='-'&&*p!='_'&&*p!='.'&&*p!=' ')return false;return true;}
static void storage_import_once(void){
    if(!s_ready)return; const char*inbox="/sd/INBOX"; const char*imported="/sd/PROJECTS/IMPORTED"; mk(imported);
    DIR*d=opendir(inbox); if(!d)return; struct dirent*e;
    while((e=readdir(d))){if(e->d_type!=DT_DIR||!safe_name(e->d_name)||!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        char src[240],dst[240];snprintf(src,sizeof(src),"%s/%s",inbox,e->d_name);snprintf(dst,sizeof(dst),"%s/%s",imported,e->d_name);
        if(access(dst,F_OK)==0)for(unsigned n=2;n<1000;n++){snprintf(dst,sizeof(dst),"%s/%s_%u",imported,e->d_name,n);if(access(dst,F_OK)!=0)break;}
        if(rename(src,dst)==0)ESP_LOGI(TAG,"imported project: %s",dst);else ESP_LOGW(TAG,"project import failed: %s -> %s (%d)",src,dst,errno);}
    closedir(d);
}
static void importer_task(void*arg){(void)arg;for(;;){storage_import_once();vTaskDelay(pdMS_TO_TICKS(STORAGE_IMPORT_INTERVAL_MS));}}
void storage_init(void){
    spi_bus_config_t bus={.mosi_io_num=SD_MOSI_GPIO,.miso_io_num=SD_MISO_GPIO,.sclk_io_num=SD_SCK_GPIO,.quadwp_io_num=-1,.quadhd_io_num=-1,.max_transfer_sz=4096};
    esp_err_t r=spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO);if(r!=ESP_OK){ESP_LOGE(TAG,"SPI init: %s",esp_err_to_name(r));return;}
    sdmmc_host_t host=SDSPI_HOST_DEFAULT();sdspi_device_config_t slot=SDSPI_DEVICE_CONFIG_DEFAULT();slot.gpio_cs=SD_CS_GPIO;slot.host_id=SPI2_HOST;
    esp_vfs_fat_mount_config_t m={.format_if_mount_failed=false,.max_files=16,.allocation_unit_size=16*1024};
    r=esp_vfs_fat_sdspi_mount("/sd",&host,&slot,&m,&s_card);if(r!=ESP_OK){ESP_LOGW(TAG,"SD not mounted: %s",esp_err_to_name(r));return;}
    s_ready=true;ESP_LOGI(TAG,"SD mounted");storage_prepare_tree();storage_import_once();xTaskCreate(importer_task,"sd_import",3072,NULL,2,NULL);
}
esp_err_t storage_prepare_tree(void){
    if(!s_ready)return ESP_ERR_INVALID_STATE;
    const char*d[]={"/sd/FIRMWARE","/sd/FIRMWARE/MASTER","/sd/FIRMWARE/WORKER","/sd/FIRMWARE/ESP32","/sd/FIRMWARE/ESP32S3","/sd/FIRMWARE/AVR","/sd/FIRMWARE/AVR/UNO","/sd/FIRMWARE/AVR/NANO","/sd/PROJECTS","/sd/PROJECTS/PREPARED","/sd/PROJECTS/IMPORTED","/sd/PROJECTS/MY_PROJECTS","/sd/INBOX","/sd/COMPONENTS","/sd/LIBRARIES","/sd/TESTS","/sd/REPORTS","/sd/LOGS","/sd/BACKUPS","/sd/CONFIG","/sd/DATABASE","/sd/UPDATES","/sd/AI"};
    for(size_t i=0;i<sizeof(d)/sizeof(d[0]);++i)mk(d[i]);
    write_if_missing("/sd/CONFIG/README.txt","ESP32 LAB : configuration locale. Les secrets ne sont pas stockes dans cette carte par defaut.\n");
    write_if_missing("/sd/INBOX/README.txt","Deposez ici un dossier de projet; il sera automatiquement deplace vers /sd/PROJECTS/IMPORTED.\n");
    write_if_missing("/sd/FIRMWARE/README.txt","MASTER/WORKER/ESP32/ESP32S3/AVR sont organises automatiquement par les scripts du depot.\n");return ESP_OK;
}
esp_err_t storage_mkdir(const char*p){if(!safe(p)||!s_ready)return ESP_ERR_INVALID_ARG;if(mkdir(p,0775)==0||errno==EEXIST)return ESP_OK;return ESP_FAIL;}
esp_err_t storage_import_inbox(void){if(!s_ready)return ESP_ERR_INVALID_STATE;storage_import_once();return ESP_OK;}
esp_err_t storage_write_text(const char*p,const char*t){if(!safe(p)||!s_ready||!t)return ESP_ERR_INVALID_ARG;FILE*f=fopen(p,"w");if(!f)return ESP_FAIL;fputs(t,f);fclose(f);return ESP_OK;}
esp_err_t storage_append_text(const char*p,const char*t){if(!safe(p)||!s_ready||!t)return ESP_ERR_INVALID_ARG;FILE*f=fopen(p,"a");if(!f)return ESP_FAIL;fputs(t,f);fclose(f);return ESP_OK;}
esp_err_t storage_sha256_file(const char*p,char hex[65]){
    if(!safe(p)||!hex||!s_ready)return ESP_ERR_INVALID_ARG;FILE*f=fopen(p,"rb");if(!f)return ESP_ERR_NOT_FOUND;
    if(psa_crypto_init()!=PSA_SUCCESS){fclose(f);return ESP_FAIL;} psa_hash_operation_t op=PSA_HASH_OPERATION_INIT;
    if(psa_hash_setup(&op,PSA_ALG_SHA_256)!=PSA_SUCCESS){fclose(f);return ESP_FAIL;} uint8_t b[4096];size_t n;psa_status_t st=PSA_SUCCESS;
    while((n=fread(b,1,sizeof(b),f))>0){st=psa_hash_update(&op,b,n);if(st!=PSA_SUCCESS)break;}fclose(f);uint8_t d[32];size_t out_len=0;
    if(st==PSA_SUCCESS)st=psa_hash_finish(&op,d,sizeof(d),&out_len);else(void)psa_hash_abort(&op);
    if(st!=PSA_SUCCESS||out_len!=sizeof(d))return ESP_FAIL;const char*h="0123456789abcdef";for(int i=0;i<32;i++){hex[i*2]=h[d[i]>>4];hex[i*2+1]=h[d[i]&15];}hex[64]=0;return ESP_OK;
}

#include "reports.h"
#include "storage.h"
#include "worker_pool.h"
#include "esp_timer.h"
#include <stdio.h>
void reports_init(void){storage_append_text("/sd/REPORTS/README.txt","Les rapports automatiques sont ecrits en HTML/JSON ou TXT selon la fonction.\n");}
void reports_write_snapshot(const char*kind){char p[160];snprintf(p,sizeof(p),"/sd/REPORTS/%s_%lld.txt",kind,(long long)(esp_timer_get_time()/1000000));char b[1200];int n=snprintf(b,sizeof(b),"ESP32 LAB report\nkind=%s\nworkers=%u\n",kind,(unsigned)worker_pool_count());for(size_t i=0;i<worker_pool_count()&&n<(int)sizeof(b)-160;i++){const worker_info_t*w=worker_pool_get(i);n+=snprintf(b+n,sizeof(b)-n,"W%d %s %s progress=%u heap=%u\n",w->id,w->ip,w->state,w->progress,w->heap);}storage_write_text(p,b);}

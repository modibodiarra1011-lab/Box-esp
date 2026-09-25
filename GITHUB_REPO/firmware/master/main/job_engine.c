#include "job_engine.h"
#include "worker_pool.h"
#include "led_status.h"
#include "storage.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define JOB_WORK_TIMEOUT_MS 45000
#define JOB_RETRY_MAX 2
#define JOB_SCHEDULER_INTERVAL_MS 60

typedef struct {
    int id;
    char type[32];
    int pr;
    int worker;
    int retries;
    char status[16];
    int progress;
    int64_t created, started, finished;
} job_t;

typedef struct {
    int slot;
    int job_id;
    uint8_t worker_id;
} job_task_arg_t;

static job_t q[JOB_MAX];
static int seq = 1;
static SemaphoreHandle_t mx;
static const char *TAG = "jobs";

static const worker_info_t *find_ready_worker(void)
{
    const worker_info_t *best = NULL;
    for (size_t k = 0; k < worker_pool_count(); k++) {
        const worker_info_t *t = worker_pool_get(k);
        if (!t || strcmp(t->state, "READY") != 0) continue;
        if (!best || t->heap > best->heap) best = t;
    }
    return best;
}

static void finish_slot(int slot, int job_id, const char *status)
{
    xSemaphoreTake(mx, portMAX_DELAY);
    if (slot >= 0 && slot < JOB_MAX && q[slot].id == job_id) {
        strlcpy(q[slot].status, status, sizeof(q[slot].status));
        q[slot].finished = esp_timer_get_time();
        if (!strcmp(status, "SUCCESS")) q[slot].progress = 100;
    }
    xSemaphoreGive(mx);
}

static void job_worker_task(void *arg)
{
    job_task_arg_t *a = (job_task_arg_t *)arg;
    if (!a) vTaskDelete(NULL);

    int slot = a->slot;
    int job_id = a->job_id;
    uint8_t worker_id = a->worker_id;
    free(a);

    char type[32] = {0};
    int priority = 0;
    xSemaphoreTake(mx, portMAX_DELAY);
    if (slot < 0 || slot >= JOB_MAX || q[slot].id != job_id) {
        xSemaphoreGive(mx);
        vTaskDelete(NULL);
        return;
    }
    strlcpy(type, q[slot].type, sizeof(type));
    priority = q[slot].pr;
    xSemaphoreGive(mx);

    esp_err_t r = worker_send_job(worker_id, type, priority);
    if (r != ESP_OK) {
        finish_slot(slot, job_id, "FAILED");
        vTaskDelete(NULL);
        return;
    }

    const int64_t deadline = esp_timer_get_time() + (int64_t)JOB_WORK_TIMEOUT_MS * 1000LL;
    bool done = false;
    while (esp_timer_get_time() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(200));
        const worker_info_t *now = worker_pool_get_by_id(worker_id);
        xSemaphoreTake(mx, portMAX_DELAY);
        if (q[slot].id != job_id) {
            xSemaphoreGive(mx);
            done = true;
            break;
        }
        if (!strcmp(q[slot].status, "CANCELLED")) {
            xSemaphoreGive(mx);
            done = true;
            break;
        }
        if (now) {
            q[slot].progress = (int)now->progress;
            if (!strcmp(now->state, "READY") && q[slot].progress >= 100) {
                strlcpy(q[slot].status, "SUCCESS", sizeof(q[slot].status));
                q[slot].finished = esp_timer_get_time();
                done = true;
            } else if (!strcmp(now->state, "ERROR") || !strcmp(now->state, "OFFLINE")) {
                if (q[slot].retries < JOB_RETRY_MAX) {
                    q[slot].retries++;
                    strlcpy(q[slot].status, "QUEUED", sizeof(q[slot].status));
                    q[slot].worker = 0;
                    done = true;
                } else {
                    strlcpy(q[slot].status, !strcmp(now->state, "OFFLINE") ? "TIMEOUT" : "FAILED", sizeof(q[slot].status));
                    q[slot].finished = esp_timer_get_time();
                    done = true;
                }
            }
        }
        xSemaphoreGive(mx);
        if (done) break;
    }

    if (!done) finish_slot(slot, job_id, "TIMEOUT");
    led_status_mode(worker_pool_count() ? "work" : "ready");
    vTaskDelete(NULL);
}

static void scheduler(void *arg)
{
    (void)arg;
    for (;;) {
        bool launched = false;
        for (;;) {
            int selected = -1;
            const worker_info_t *worker = NULL;
            xSemaphoreTake(mx, portMAX_DELAY);
            int best_pr = -2147483647;
            for (int i = 0; i < JOB_MAX; i++) {
                if (q[i].id && !strcmp(q[i].status, "QUEUED") && q[i].pr > best_pr) {
                    best_pr = q[i].pr;
                    selected = i;
                }
            }
            if (selected >= 0) worker = find_ready_worker();
            if (selected >= 0 && worker) {
                q[selected].worker = worker->id;
                q[selected].started = esp_timer_get_time();
                strlcpy(q[selected].status, "RUNNING", sizeof(q[selected].status));
                job_task_arg_t *a = calloc(1, sizeof(*a));
                if (!a) {
                    strlcpy(q[selected].status, "FAILED", sizeof(q[selected].status));
                    q[selected].finished = esp_timer_get_time();
                    xSemaphoreGive(mx);
                    break;
                }
                a->slot = selected;
                a->job_id = q[selected].id;
                a->worker_id = (uint8_t)worker->id;
                int slot = selected;
                int jid = q[selected].id;
                xSemaphoreGive(mx);
                if (xTaskCreate(job_worker_task, "job_worker", 4096, a, 6, NULL) != pdPASS) {
                    free(a);
                    xSemaphoreTake(mx, portMAX_DELAY);
                    if (q[slot].id == jid) {
                        strlcpy(q[slot].status, "FAILED", sizeof(q[slot].status));
                        q[slot].finished = esp_timer_get_time();
                        q[slot].worker = 0;
                    }
                    xSemaphoreGive(mx);
                } else {
                    launched = true;
                }
            } else {
                xSemaphoreGive(mx);
                break;
            }
        }
        if (launched) led_status_mode("work");
        else if (worker_pool_count() == 0) led_status_mode("ready");
        vTaskDelay(pdMS_TO_TICKS(JOB_SCHEDULER_INTERVAL_MS));
    }
}

void job_engine_start(void)
{
    mx = xSemaphoreCreateMutex();
    if (!mx) return;
    memset(q, 0, sizeof(q));
    xTaskCreate(scheduler, "scheduler", 4096, NULL, 7, NULL);
    ESP_LOGI(TAG, "job engine ready (parallel pool)");
}

int job_create(const char *type, int priority)
{
    if (!mx || !type || !*type) return -1;
    if (strlen(type) >= 31) return -1;
    if (priority < -100) priority = -100;
    if (priority > 100) priority = 100;
    xSemaphoreTake(mx, portMAX_DELAY);
    for (int i = 0; i < JOB_MAX; i++) {
        if (q[i].id == 0 || !strcmp(q[i].status, "SUCCESS") || !strcmp(q[i].status, "FAILED") ||
            !strcmp(q[i].status, "TIMEOUT") || !strcmp(q[i].status, "CANCELLED")) {
            memset(&q[i], 0, sizeof(q[i]));
            q[i].id = seq++;
            if (seq < 0) seq = 1;
            strlcpy(q[i].type, type, sizeof(q[i].type));
            q[i].pr = priority;
            strlcpy(q[i].status, "QUEUED", sizeof(q[i].status));
            q[i].created = esp_timer_get_time();
            int id = q[i].id;
            xSemaphoreGive(mx);
            return id;
        }
    }
    xSemaphoreGive(mx);
    return -1;
}

bool job_cancel(int id)
{
    if (!mx || id <= 0) return false;
    bool ok = false;
    uint8_t worker = 0;
    xSemaphoreTake(mx, portMAX_DELAY);
    for (int i = 0; i < JOB_MAX; i++) {
        if (q[i].id == id && (!strcmp(q[i].status, "QUEUED") || !strcmp(q[i].status, "RUNNING"))) {
            worker = (uint8_t)q[i].worker;
            strlcpy(q[i].status, "CANCELLED", sizeof(q[i].status));
            q[i].finished = esp_timer_get_time();
            ok = true;
            break;
        }
    }
    xSemaphoreGive(mx);
    if (ok && worker) (void)worker_cancel_job(worker);
    return ok;
}

size_t job_cancel_all(void)
{
    if (!mx) return 0;
    uint8_t workers[JOB_MAX];
    size_t count = 0;
    xSemaphoreTake(mx, portMAX_DELAY);
    for (int i = 0; i < JOB_MAX; ++i) {
        if (q[i].id && (!strcmp(q[i].status, "QUEUED") || !strcmp(q[i].status, "RUNNING"))) {
            if (!q[i].worker) {
                q[i].worker = 0;
            } else if (count < JOB_MAX) {
                workers[count++] = (uint8_t)q[i].worker;
            }
            strlcpy(q[i].status, "CANCELLED", sizeof(q[i].status));
            q[i].finished = esp_timer_get_time();
        }
    }
    xSemaphoreGive(mx);
    for (size_t i = 0; i < count; ++i) (void)worker_cancel_job(workers[i]);
    led_status_mode("ready");
    return count;
}

size_t job_json(char *out, size_t cap)
{
    if (!out || cap < 4 || !mx) return 0;
    size_t n = 0;
    n += snprintf(out + n, cap - n, "[");
    bool first = true;
    xSemaphoreTake(mx, portMAX_DELAY);
    for (int i = 0; i < JOB_MAX && n + 120 < cap; i++) {
        if (!q[i].id) continue;
        if (!first) n += snprintf(out + n, cap - n, ",");
        first = false;
        n += snprintf(out + n, cap - n,
            "{\"id\":%d,\"type\":\"%s\",\"priority\":%d,\"worker\":%d,\"retries\":%d,\"status\":\"%s\",\"progress\":%d,\"created_us\":%lld,\"started_us\":%lld,\"finished_us\":%lld}",
            q[i].id, q[i].type, q[i].pr, q[i].worker, q[i].retries, q[i].status, q[i].progress,
            (long long)q[i].created, (long long)q[i].started, (long long)q[i].finished);
    }
    xSemaphoreGive(mx);
    if (n + 2 < cap) n += snprintf(out + n, cap - n, "]");
    else out[cap - 1] = 0;
    return n < cap ? n : cap - 1;
}

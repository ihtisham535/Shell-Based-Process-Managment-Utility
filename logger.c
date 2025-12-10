#include "logger.h"
#include <stdarg.h>

static FILE *log_file = NULL;
static FILE *stats_file = NULL;

/* Initialize logger */
void init_logger(void) {
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
    }
    
    stats_file = fopen(STATS_FILE, "a");
    if (stats_file == NULL) {
        perror("Failed to open stats file");
    }
}

/* Close logger */
void close_logger(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    
    if (stats_file != NULL) {
        fclose(stats_file);
        stats_file = NULL;
    }
}

/* Log general message */
void log_message(const char *format, ...) {
    va_list args;
    time_t now;
    struct tm *tm_info;
    char time_str[64];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (log_file != NULL) {
        fprintf(log_file, "[%s] ", time_str);
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
    
    /* Also print to stdout */
    printf("[%s] ", time_str);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/* Log resource usage */
void log_resource_usage(process_info_t *info) {
    if (info == NULL) return;
    
    if (stats_file != NULL) {
        fprintf(stats_file, "PID: %d, CPU: %.2f%%, MEM: %.2f%%, VSIZE: %lu, RSS: %ld\n",
                info->pid, info->cpu_percent, info->mem_percent,
                info->vsize, info->rss);
        fflush(stats_file);
    }
}

/* Log historical statistics */
void log_historical_stats(process_info_t *info) {
    if (info == NULL) return;
    
    time_t now;
    struct tm *tm_info;
    char time_str[64];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (stats_file != NULL) {
        fprintf(stats_file, "[%s] PID=%d, NAME=%s, CPU=%.2f%%, MEM=%.2f%%, STATE=%d\n",
                time_str, info->pid, info->name, info->cpu_percent,
                info->mem_percent, info->state);
        fflush(stats_file);
    }
}

/* Log operation */
void log_operation(const char *operation, pid_t pid, const char *result) {
    time_t now;
    struct tm *tm_info;
    char time_str[64];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (log_file != NULL) {
        fprintf(log_file, "[%s] Operation: %s, PID: %d, Result: %s\n",
                time_str, operation, pid, result);
        fflush(log_file);
    }
}

/* Error exit */
void error_exit(const char *msg) {
    log_message("ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}


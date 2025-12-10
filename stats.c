#include "stats.h"
#include <sys/sysinfo.h>
#include <unistd.h>

static unsigned long last_total_cpu = 0;
static unsigned long last_idle_cpu = 0;
static time_t last_update = 0;

/* Read system statistics */
int read_system_stats(unsigned long *total_cpu_time, unsigned long *idle_time) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        return -1;
    }
    
    unsigned long user, nice, system, idle, iowait, irq, softirq;
    char line[256];
    
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    
    sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    
    fclose(fp);
    
    *total_cpu_time = user + nice + system + idle + iowait + irq + softirq;
    *idle_time = idle;
    
    return 0;
}

/* Calculate process CPU usage */
int calculate_process_cpu(pid_t pid, double *cpu_percent) {
    char stat_path[MAX_PATH_LEN];
    FILE *fp;
    unsigned long utime, stime;
    unsigned long starttime;
    unsigned long total_time;
    unsigned long system_uptime = 0;
    unsigned long total_cpu, idle_cpu;
    double elapsed_time;
    double cpu_usage;
    
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    fp = fopen(stat_path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    /* Read from /proc/pid/stat */
    if (fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
               "%lu %lu %*d %*d %*d %*d %*d %*d %*u %lu",
               &utime, &stime, &starttime) != 3) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    /* Read system uptime */
    fp = fopen("/proc/uptime", "r");
    if (fp != NULL) {
        fscanf(fp, "%lu", &system_uptime);
        fclose(fp);
    }
    
    /* Get total CPU time */
    if (read_system_stats(&total_cpu, &idle_cpu) != 0) {
        return -1;
    }
    
    total_time = utime + stime;
    elapsed_time = system_uptime - (starttime / sysconf(_SC_CLK_TCK));
    
    if (elapsed_time <= 0) {
        elapsed_time = 1.0;
    }
    
    /* Calculate CPU percentage */
    cpu_usage = ((double)total_time / sysconf(_SC_CLK_TCK)) / elapsed_time * 100.0;
    
    if (cpu_usage > 100.0) {
        cpu_usage = 100.0;
    }
    
    *cpu_percent = cpu_usage;
    return 0;
}

/* Get CPU usage for a process */
double get_cpu_usage(pid_t pid) {
    double cpu_percent = 0.0;
    calculate_process_cpu(pid, &cpu_percent);
    return cpu_percent;
}

/* Get memory usage for a process */
double get_memory_usage(pid_t pid) {
    char stat_path[MAX_PATH_LEN];
    FILE *fp;
    unsigned long vsize;
    long rss;
    struct sysinfo info;
    double mem_percent;
    
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    fp = fopen(stat_path, "r");
    if (fp == NULL) {
        return 0.0;
    }
    
    /* Read vsize and rss from /proc/pid/stat */
    if (fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
               "%*u %*u %*d %*d %*d %*d %*d %*d %*u %*u %lu %ld",
               &vsize, &rss) != 2) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);
    
    /* Get total system memory */
    if (sysinfo(&info) != 0) {
        return 0.0;
    }
    
    /* Calculate memory percentage (rss is in pages) */
    long page_size = sysconf(_SC_PAGESIZE);
    long total_mem_kb = info.totalram * (info.mem_unit / 1024);
    long process_mem_kb = rss * (page_size / 1024);
    
    if (total_mem_kb > 0) {
        mem_percent = ((double)process_mem_kb / total_mem_kb) * 100.0;
    } else {
        mem_percent = 0.0;
    }
    
    return mem_percent;
}

/* Update process statistics */
void update_process_statistics(process_info_t *info) {
    if (info == NULL) return;
    
    info->cpu_percent = get_cpu_usage(info->pid);
    info->mem_percent = get_memory_usage(info->pid);
    info->last_update = time(NULL);
}


#ifndef STATS_H
#define STATS_H

#include "common.h"

/* Statistics Functions */
double get_cpu_usage(pid_t pid);
double get_memory_usage(pid_t pid);
int read_system_stats(unsigned long *total_cpu_time, unsigned long *idle_time);
int calculate_process_cpu(pid_t pid, double *cpu_percent);
void update_process_statistics(process_info_t *info);

#endif /* STATS_H */


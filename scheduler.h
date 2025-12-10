#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

/* Scheduler Functions */
void init_scheduler(void);
void cleanup_scheduler(void);
priority_level_t get_update_priority(pid_t pid, double cpu_usage);
int get_update_interval(priority_level_t priority);
void* scheduler_thread(void *arg);

#endif /* SCHEDULER_H */


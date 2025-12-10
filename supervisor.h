#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include "common.h"

/* Supervisor Functions */
void init_supervisor(void);
void cleanup_supervisor(void);
void* zombie_cleanup_thread(void *arg);
int check_zombie(pid_t pid);
void reap_zombie(pid_t pid);

#endif /* SUPERVISOR_H */


#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include "common.h"

/* Shared Memory Functions */
process_table_t* attach_shared_memory(void);
void detach_shared_memory(process_table_t *table);
void destroy_shared_memory(void);

/* Semaphore Functions */
int init_semaphores(void);
void destroy_semaphores(void);
void lock_table(void);
void unlock_table(void);

/* Process Table Operations */
int find_process_index(process_table_t *table, pid_t pid);
void update_process_info(process_table_t *table, int index, process_info_t *info);
void remove_process(process_table_t *table, pid_t pid);
process_info_t* get_process(process_table_t *table, pid_t pid);

#endif /* PROCESS_TABLE_H */


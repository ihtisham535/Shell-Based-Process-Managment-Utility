#ifndef PROC_READER_H
#define PROC_READER_H

#include "common.h"

/* Process Reader Functions */
void* read_proc_info(void *arg);
int read_process_stat(pid_t pid, process_info_t *info);
int read_process_status(pid_t pid, process_info_t *info);
int read_process_cmdline(pid_t pid, char *cmdline, size_t max_len);
void collect_all_processes(void);
void start_proc_reader_threads(int num_threads);
void stop_proc_reader_threads(void);

#endif /* PROC_READER_H */


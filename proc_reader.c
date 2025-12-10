#include "proc_reader.h"
#include "process_table.h"
#include "stats.h"
#include "logger.h"
#include "memory_allocator.h"

static pthread_t *reader_threads = NULL;
static int num_threads = 4;
static int running = 0;
static process_table_t *table = NULL;

/* Read process stat file */
int read_process_stat(pid_t pid, process_info_t *info) {
    char stat_path[MAX_PATH_LEN];
    FILE *fp;
    unsigned long utime, stime;
    unsigned long vsize;
    long rss;
    char state_char;
    
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    fp = fopen(stat_path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    /* Parse /proc/pid/stat */
    if (fscanf(fp, "%d %*s %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
               "%lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %lu %ld",
               &info->pid, &state_char, &info->ppid,
               &utime, &stime, &vsize, &rss) < 7) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    info->utime = utime;
    info->stime = stime;
    info->vsize = vsize;
    info->rss = rss;
    
    /* Convert state character to enum */
    switch (state_char) {
        case 'R': info->state = PROC_RUNNING; break;
        case 'S': case 'D': info->state = PROC_SLEEPING; break;
        case 'T': case 't': info->state = PROC_STOPPED; break;
        case 'Z': info->state = PROC_ZOMBIE; info->is_zombie = 1; break;
        default: info->state = PROC_DEAD; break;
    }
    
    return 0;
}

/* Read process status file */
int read_process_status(pid_t pid, process_info_t *info) {
    char status_path[MAX_PATH_LEN];
    FILE *fp;
    char line[256];
    
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
    fp = fopen(status_path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    /* Read Name field */
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name:\t%s", info->name);
            break;
        }
    }
    fclose(fp);
    
    return 0;
}

/* Read process cmdline */
int read_process_cmdline(pid_t pid, char *cmdline, size_t max_len) {
    char cmdline_path[MAX_PATH_LEN];
    FILE *fp;
    size_t len;
    
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
    fp = fopen(cmdline_path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    len = fread(cmdline, 1, max_len - 1, fp);
    cmdline[len] = '\0';
    
    /* Replace null bytes with spaces */
    for (size_t i = 0; i < len; i++) {
        if (cmdline[i] == '\0') {
            cmdline[i] = ' ';
        }
    }
    
    fclose(fp);
    return 0;
}

/* Thread function to read process info */
void* read_proc_info(void *arg) {
    pid_t start_pid = (pid_t)(long)arg;
    pid_t pid;
    process_info_t *info;
    
    while (running) {
        /* Scan processes assigned to this thread */
        for (pid = start_pid; pid < start_pid + 1000 && running; pid++) {
            /* Allocate memory for process info */
            info = (process_info_t*)alloc_mem(sizeof(process_info_t));
            if (info == NULL) {
                continue;
            }
            
            memset(info, 0, sizeof(process_info_t));
            info->pid = pid;
            info->is_zombie = 0;
            
            /* Try to read process information */
            if (read_process_stat(pid, info) == 0) {
                read_process_status(pid, info);
                read_process_cmdline(pid, info->cmdline, sizeof(info->cmdline));
                
                /* Update statistics */
                update_process_statistics(info);
                
                /* Update process table */
                lock_table();
                int index = find_process_index(table, pid);
                if (index < 0 && table->count < MAX_PROCESSES) {
                    index = table->count;
                    table->count++;
                }
                
                if (index >= 0) {
                    update_process_info(table, index, info);
                    log_historical_stats(info);
                }
                unlock_table();
            }
            
            free_mem(info);
            
            /* Small delay to prevent overwhelming the system */
            usleep(1000);
        }
        
        /* Sleep before next scan cycle */
        sleep(2);
    }
    
    return NULL;
}

/* Collect all processes from /proc */
void collect_all_processes(void) {
    DIR *proc_dir;
    struct dirent *entry;
    pid_t pid;
    process_info_t *info;
    
    if (table == NULL) {
        table = attach_shared_memory();
        if (table == NULL) {
            return;
        }
    }
    
    lock_table();
    table->count = 0;
    unlock_table();
    
    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir /proc");
        return;
    }
    
    while ((entry = readdir(proc_dir)) != NULL && running) {
        /* Check if entry is a process directory (numeric) */
        if (isdigit(entry->d_name[0])) {
            pid = (pid_t)atoi(entry->d_name);
            
            info = (process_info_t*)alloc_mem(sizeof(process_info_t));
            if (info == NULL) {
                continue;
            }
            
            memset(info, 0, sizeof(process_info_t));
            info->pid = pid;
            info->is_zombie = 0;
            
            if (read_process_stat(pid, info) == 0) {
                read_process_status(pid, info);
                read_process_cmdline(pid, info->cmdline, sizeof(info->cmdline));
                update_process_statistics(info);
                
                lock_table();
                if (table->count < MAX_PROCESSES) {
                    int index = table->count;
                    update_process_info(table, index, info);
                    table->count++;
                    log_historical_stats(info);
                }
                unlock_table();
            }
            
            free_mem(info);
        }
    }
    
    closedir(proc_dir);
    
    lock_table();
    table->last_sync = time(NULL);
    unlock_table();
}

/* Start process reader threads */
void start_proc_reader_threads(int thread_count) {
    if (running) {
        return;
    }
    
    num_threads = thread_count > 0 ? thread_count : 4;
    reader_threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (reader_threads == NULL) {
        error_exit("Failed to allocate thread array");
    }
    
    table = attach_shared_memory();
    if (table == NULL) {
        error_exit("Failed to attach shared memory");
    }
    
    running = 1;
    
    /* Create threads */
    for (int i = 0; i < num_threads; i++) {
        pid_t start_pid = (pid_t)(i * 1000);
        if (pthread_create(&reader_threads[i], NULL, read_proc_info, (void*)(long)start_pid) != 0) {
            perror("pthread_create");
            running = 0;
            break;
        }
    }
    
    /* Also start a periodic full scan thread */
    log_message("Process reader threads started (%d threads)\n", num_threads);
}

/* Stop process reader threads */
void stop_proc_reader_threads(void) {
    if (!running) {
        return;
    }
    
    running = 0;
    
    /* Wait for all threads to finish */
    if (reader_threads != NULL) {
        for (int i = 0; i < num_threads; i++) {
            pthread_join(reader_threads[i], NULL);
        }
        free(reader_threads);
        reader_threads = NULL;
    }
    
    log_message("Process reader threads stopped\n");
}


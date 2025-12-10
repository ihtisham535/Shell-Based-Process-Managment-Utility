#include "scheduler.h"
#include "process_table.h"
#include "logger.h"
#include "proc_reader.h"
#include "stats.h"

static pthread_t scheduler_tid;
static int scheduler_running = 0;

/* Get update priority based on process characteristics */
priority_level_t get_update_priority(pid_t pid, double cpu_usage) {
    if (cpu_usage > 50.0) {
        return PRIORITY_HIGH;
    } else if (cpu_usage > 10.0) {
        return PRIORITY_MEDIUM;
    } else {
        return PRIORITY_LOW;
    }
}

/* Get update interval in seconds */
int get_update_interval(priority_level_t priority) {
    switch (priority) {
        case PRIORITY_HIGH:
            return 1;
        case PRIORITY_MEDIUM:
            return 3;
        case PRIORITY_LOW:
            return 5;
        default:
            return 5;
    }
}

/* Scheduler thread function */
void* scheduler_thread(void *arg) {
    process_table_t *table;
    time_t last_update[MAX_PROCESSES];
    priority_level_t priorities[MAX_PROCESSES];
    int update_intervals[MAX_PROCESSES];
    
    table = attach_shared_memory();
    if (table == NULL) {
        return NULL;
    }
    
    /* Initialize update times */
    time_t now = time(NULL);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        last_update[i] = now;
        priorities[i] = PRIORITY_LOW;
        update_intervals[i] = 5;
    }
    
    log_message("Scheduler thread started\n");
    
    while (scheduler_running) {
        sleep(1);  /* Check every second */
        
        lock_table();
        
        for (int i = 0; i < table->count; i++) {
            process_info_t *proc = &table->processes[i];
            
            if (proc->pid == 0) continue;
            
            /* Determine priority based on CPU usage */
            priorities[i] = get_update_priority(proc->pid, proc->cpu_percent);
            update_intervals[i] = get_update_interval(priorities[i]);
            
            /* Check if it's time to update this process */
            time_t current_time = time(NULL);
            if (current_time - last_update[i] >= update_intervals[i]) {
                /* Trigger update by collecting process info */
                process_info_t info;
                memset(&info, 0, sizeof(info));
                info.pid = proc->pid;
                
                if (read_process_stat(proc->pid, &info) == 0) {
                    read_process_status(proc->pid, &info);
                    read_process_cmdline(proc->pid, info.cmdline, sizeof(info.cmdline));
                    update_process_statistics(&info);
                    
                    /* Update in table */
                    update_process_info(table, i, &info);
                    last_update[i] = current_time;
                }
            }
        }
        
        unlock_table();
    }
    
    log_message("Scheduler thread stopped\n");
    return NULL;
}

/* Initialize scheduler */
void init_scheduler(void) {
    if (scheduler_running) {
        return;
    }
    
    scheduler_running = 1;
    
    if (pthread_create(&scheduler_tid, NULL, scheduler_thread, NULL) != 0) {
        perror("pthread_create scheduler");
        scheduler_running = 0;
        return;
    }
    
    log_message("Scheduler initialized\n");
}

/* Cleanup scheduler */
void cleanup_scheduler(void) {
    if (!scheduler_running) {
        return;
    }
    
    scheduler_running = 0;
    pthread_join(scheduler_tid, NULL);
    
    log_message("Scheduler cleaned up\n");
}


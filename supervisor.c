#include "supervisor.h"
#include "process_table.h"
#include "logger.h"
#include <sys/wait.h>

static pthread_t supervisor_tid;
static int supervisor_running = 0;

/* Check if process is a zombie */
int check_zombie(pid_t pid) {
    char stat_path[MAX_PATH_LEN];
    FILE *fp;
    char state_char;
    int pid_read;
    
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    fp = fopen(stat_path, "r");
    if (fp == NULL) {
        return 0;  /* Process doesn't exist */
    }
    
    /* Read state from stat file */
    if (fscanf(fp, "%d %*s %c", &pid_read, &state_char) != 2) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    
    return (state_char == 'Z');
}

/* Reap zombie process */
void reap_zombie(pid_t pid) {
    int status;
    pid_t result;
    
    /* Try to reap the zombie */
    result = waitpid(pid, &status, WNOHANG);
    
    if (result > 0) {
        log_operation("ZOMBIE_REAP", pid, "Success");
        log_message("Reaped zombie process %d\n", pid);
        
        /* Remove from process table */
        process_table_t *table = attach_shared_memory();
        if (table != NULL) {
            remove_process(table, pid);
        }
    } else if (result == 0) {
        /* Process still exists but not a zombie yet */
    } else {
        /* Error or process doesn't exist */
        if (errno != ECHILD) {
            log_message("Error reaping process %d: %s\n", pid, strerror(errno));
        }
    }
}

/* Supervisor thread function */
void* zombie_cleanup_thread(void *arg) {
    process_table_t *table;
    time_t last_scan = 0;
    const int scan_interval = 5;  /* Scan every 5 seconds */
    
    table = attach_shared_memory();
    if (table == NULL) {
        return NULL;
    }
    
    log_message("Supervisor thread started\n");
    
    while (supervisor_running) {
        sleep(1);
        
        time_t current_time = time(NULL);
        
        /* Scan for zombies periodically */
        if (current_time - last_scan >= scan_interval) {
            lock_table();
            
            /* Scan all processes in table */
            for (int i = 0; i < table->count; i++) {
                process_info_t *proc = &table->processes[i];
                
                if (proc->pid == 0) continue;
                
                /* Check if process is zombie */
                if (check_zombie(proc->pid)) {
                    log_message("Found zombie process: PID %d\n", proc->pid);
                    unlock_table();
                    
                    /* Reap the zombie */
                    reap_zombie(proc->pid);
                    
                    lock_table();
                }
            }
            
            unlock_table();
            last_scan = current_time;
        }
        
        /* Also try to reap any zombie children in a non-blocking way */
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            /* Reaped a zombie child */
        }
    }
    
    log_message("Supervisor thread stopped\n");
    return NULL;
}

/* Initialize supervisor */
void init_supervisor(void) {
    if (supervisor_running) {
        return;
    }
    
    supervisor_running = 1;
    
    if (pthread_create(&supervisor_tid, NULL, zombie_cleanup_thread, NULL) != 0) {
        perror("pthread_create supervisor");
        supervisor_running = 0;
        return;
    }
    
    log_message("Supervisor initialized\n");
}

/* Cleanup supervisor */
void cleanup_supervisor(void) {
    if (!supervisor_running) {
        return;
    }
    
    supervisor_running = 0;
    pthread_join(supervisor_tid, NULL);
    
    log_message("Supervisor cleaned up\n");
}


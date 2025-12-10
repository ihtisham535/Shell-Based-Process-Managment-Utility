#include "common.h"
#include "process_table.h"
#include "message_queue.h"
#include "proc_reader.h"
#include "scheduler.h"
#include "supervisor.h"
#include "logger.h"
#include "memory_allocator.h"

static int daemon_mode = 0;
static int server_running = 0;

/* Handle command messages */
void handle_command(process_msg_t *msg) {
    process_table_t *table = attach_shared_memory();
    process_info_t *proc;
    int result = 0;
    char response[256];
    
    if (table == NULL) {
        strcpy(response, "Error: Failed to access process table");
        send_response(msg->mtype, response);
        return;
    }
    
    proc = get_process(table, msg->target_pid);
    
    switch (msg->cmd) {
        case MSG_KILL:
            if (proc == NULL) {
                strcpy(response, "Error: Process not found");
            } else {
                result = kill(msg->target_pid, msg->signal > 0 ? msg->signal : SIGTERM);
                if (result == 0) {
                    snprintf(response, sizeof(response), "Success: Sent signal %d to process %d",
                            msg->signal > 0 ? msg->signal : SIGTERM, msg->target_pid);
                    log_operation("KILL", msg->target_pid, response);
                } else {
                    snprintf(response, sizeof(response), "Error: Failed to kill process: %s",
                            strerror(errno));
                    log_operation("KILL", msg->target_pid, response);
                }
            }
            break;
            
        case MSG_SUSPEND:
            if (proc == NULL) {
                strcpy(response, "Error: Process not found");
            } else {
                result = kill(msg->target_pid, SIGSTOP);
                if (result == 0) {
                    strcpy(response, "Success: Process suspended");
                    log_operation("SUSPEND", msg->target_pid, response);
                } else {
                    snprintf(response, sizeof(response), "Error: Failed to suspend: %s",
                            strerror(errno));
                    log_operation("SUSPEND", msg->target_pid, response);
                }
            }
            break;
            
        case MSG_RESUME:
            if (proc == NULL) {
                strcpy(response, "Error: Process not found");
            } else {
                result = kill(msg->target_pid, SIGCONT);
                if (result == 0) {
                    strcpy(response, "Success: Process resumed");
                    log_operation("RESUME", msg->target_pid, response);
                } else {
                    snprintf(response, sizeof(response), "Error: Failed to resume: %s",
                            strerror(errno));
                    log_operation("RESUME", msg->target_pid, response);
                }
            }
            break;
            
        case MSG_UPDATE:
            collect_all_processes();
            strcpy(response, "Success: Process table updated");
            break;
            
        case MSG_SHUTDOWN:
            server_running = 0;
            strcpy(response, "Success: Shutting down");
            break;
            
        default:
            strcpy(response, "Error: Unknown command");
            break;
    }
    
    send_response(msg->mtype, response);
}

/* Server loop to handle commands */
void* command_server(void *arg) {
    process_msg_t msg;
    
    log_message("Command server started\n");
    
    while (server_running) {
        if (receive_command(&msg) == 0) {
            handle_command(&msg);
        }
        usleep(100000);  /* 100ms delay */
    }
    
    log_message("Command server stopped\n");
    return NULL;
}

/* Print process information */
void print_process(process_info_t *proc) {
    const char *state_str[] = {
        "Running", "Sleeping", "Stopped", "Zombie", "Dead"
    };
    
    printf("%-8d %-8d %-20s %-12s %8.2f%% %8.2f%% %12lu %10ld\n",
           proc->pid, proc->ppid, proc->name,
           state_str[proc->state],
           proc->cpu_percent, proc->mem_percent,
           proc->vsize / 1024, proc->rss);
}

/* List all processes */
void list_processes(int show_all) {
    process_table_t *table = attach_shared_memory();
    
    if (table == NULL) {
        printf("Error: Failed to access process table\n");
        return;
    }
    
    printf("\n%-8s %-8s %-20s %-12s %10s %10s %12s %10s\n",
           "PID", "PPID", "NAME", "STATE", "CPU%", "MEM%", "VSIZE(KB)", "RSS(KB)");
    printf("%s\n", "-------------------------------------------------------------------------------------------");
    
    lock_table();
    
    for (int i = 0; i < table->count; i++) {
        process_info_t *proc = &table->processes[i];
        
        if (proc->pid == 0) continue;
        
        if (!show_all && proc->state == PROC_ZOMBIE) {
            continue;
        }
        
        print_process(proc);
    }
    
    unlock_table();
    printf("\nTotal processes: %d\n", table->count);
}

/* Show process details */
void show_process_details(pid_t pid) {
    process_table_t *table = attach_shared_memory();
    process_info_t *proc;
    
    if (table == NULL) {
        printf("Error: Failed to access process table\n");
        return;
    }
    
    proc = get_process(table, pid);
    
    if (proc == NULL) {
        printf("Process %d not found\n", pid);
        return;
    }
    
    printf("\nProcess Details:\n");
    printf("  PID: %d\n", proc->pid);
    printf("  PPID: %d\n", proc->ppid);
    printf("  Name: %s\n", proc->name);
    printf("  Command: %s\n", proc->cmdline);
    printf("  State: %d\n", proc->state);
    printf("  CPU Usage: %.2f%%\n", proc->cpu_percent);
    printf("  Memory Usage: %.2f%%\n", proc->mem_percent);
    printf("  Virtual Size: %lu KB\n", proc->vsize / 1024);
    printf("  Resident Set Size: %ld KB\n", proc->rss);
    printf("  User Time: %lu\n", proc->utime);
    printf("  System Time: %lu\n", proc->stime);
    printf("\n");
}

/* Print usage information */
void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] [COMMAND] [ARGS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -d          Run as daemon\n");
    printf("  -h          Show this help message\n");
    printf("\nCommands:\n");
    printf("  list              List all processes\n");
    printf("  list -a           List all processes (including zombies)\n");
    printf("  show <pid>        Show details of a specific process\n");
    printf("  kill <pid>        Kill a process (SIGTERM)\n");
    printf("  kill <pid> <sig>  Kill a process with specific signal\n");
    printf("  suspend <pid>     Suspend a process (SIGSTOP)\n");
    printf("  resume <pid>      Resume a process (SIGCONT)\n");
    printf("  update            Update process table\n");
    printf("  stats             Show system statistics\n");
    printf("\n");
}

/* Main function */
int main(int argc, char *argv[]) {
    pthread_t server_tid;
    int opt;
    
    /* Initialize components */
    init_logger();
    init_allocator();
    
    if (init_semaphores() == -1) {
        error_exit("Failed to initialize semaphores");
    }
    
    if (init_message_queue() == -1) {
        error_exit("Failed to initialize message queue");
    }
    
    if (attach_shared_memory() == NULL) {
        error_exit("Failed to attach shared memory");
    }
    
    /* Parse command line options */
    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* If no command specified, start daemon/server mode */
    if (optind >= argc) {
        daemon_mode = 1;
    }
    
    if (daemon_mode) {
        /* Start background services */
        server_running = 1;
        
        /* Start process reader threads */
        start_proc_reader_threads(4);
        
        /* Initial process collection */
        collect_all_processes();
        
        /* Start scheduler */
        init_scheduler();
        
        /* Start supervisor */
        init_supervisor();
        
        /* Start command server */
        if (pthread_create(&server_tid, NULL, command_server, NULL) != 0) {
            error_exit("Failed to create server thread");
        }
        
        log_message("PSX daemon started\n");
        
        /* Wait for server thread */
        pthread_join(server_tid, NULL);
        
        /* Cleanup */
        stop_proc_reader_threads();
        cleanup_scheduler();
        cleanup_supervisor();
        cleanup_allocator();
        destroy_shared_memory();
        destroy_semaphores();
        destroy_message_queue();
        close_logger();
        
        return 0;
    }
    
    /* Handle commands */
    if (strcmp(argv[optind], "list") == 0) {
        int show_all = (optind + 1 < argc && strcmp(argv[optind + 1], "-a") == 0);
        list_processes(show_all);
        
    } else if (strcmp(argv[optind], "show") == 0) {
        if (optind + 1 >= argc) {
            printf("Error: PID required\n");
            return 1;
        }
        pid_t pid = atoi(argv[optind + 1]);
        show_process_details(pid);
        
    } else if (strcmp(argv[optind], "kill") == 0) {
        if (optind + 1 >= argc) {
            printf("Error: PID required\n");
            return 1;
        }
        pid_t pid = atoi(argv[optind + 1]);
        int sig = (optind + 2 < argc) ? atoi(argv[optind + 2]) : SIGTERM;
        send_command(MSG_KILL, pid, sig);
        printf("Kill command sent to process %d\n", pid);
        
    } else if (strcmp(argv[optind], "suspend") == 0) {
        if (optind + 1 >= argc) {
            printf("Error: PID required\n");
            return 1;
        }
        pid_t pid = atoi(argv[optind + 1]);
        send_command(MSG_SUSPEND, pid, 0);
        printf("Suspend command sent to process %d\n", pid);
        
    } else if (strcmp(argv[optind], "resume") == 0) {
        if (optind + 1 >= argc) {
            printf("Error: PID required\n");
            return 1;
        }
        pid_t pid = atoi(argv[optind + 1]);
        send_command(MSG_RESUME, pid, 0);
        printf("Resume command sent to process %d\n", pid);
        
    } else if (strcmp(argv[optind], "update") == 0) {
        send_command(MSG_UPDATE, 0, 0);
        printf("Update command sent\n");
        
    } else if (strcmp(argv[optind], "stats") == 0) {
        process_table_t *table = attach_shared_memory();
        if (table != NULL) {
            lock_table();
            printf("\nSystem Statistics:\n");
            printf("  Total Processes: %d\n", table->count);
            printf("  Last Sync: %s", ctime(&table->last_sync));
            unlock_table();
            
            printf("\nMemory Allocator:\n");
            printf("  Total Allocated: %zu bytes\n", get_total_allocated());
            printf("  Total Free: %zu bytes\n", get_total_free());
        }
        
    } else {
        printf("Unknown command: %s\n", argv[optind]);
        print_usage(argv[0]);
        return 1;
    }
    
    cleanup_allocator();
    detach_shared_memory(attach_shared_memory());
    close_logger();
    
    return 0;
}


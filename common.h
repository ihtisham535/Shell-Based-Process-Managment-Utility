#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/* Constants */
#define MAX_PROCESSES 4096
#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512
#define SHM_KEY 0x12345
#define MSG_KEY 0x54321
#define SEM_KEY 0xABCDE
#define LOG_FILE "psx_log.txt"
#define STATS_FILE "psx_stats.log"

/* Process States */
typedef enum {
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_STOPPED,
    PROC_ZOMBIE,
    PROC_DEAD
} proc_state_t;

/* Process Information Structure */
typedef struct {
    pid_t pid;
    pid_t ppid;
    char name[64];
    char cmdline[MAX_CMD_LEN];
    proc_state_t state;
    unsigned long utime;      // User time
    unsigned long stime;      // System time
    unsigned long vsize;      // Virtual memory size
    long rss;                 // Resident set size (KB)
    double cpu_percent;       // CPU usage percentage
    double mem_percent;       // Memory usage percentage
    time_t last_update;
    int is_zombie;
} process_info_t;

/* Process Table Structure */
typedef struct {
    int count;
    process_info_t processes[MAX_PROCESSES];
    time_t last_sync;
    int active;
} process_table_t;

/* Message Types */
typedef enum {
    MSG_KILL,
    MSG_SUSPEND,
    MSG_RESUME,
    MSG_UPDATE,
    MSG_SHUTDOWN
} msg_type_t;

/* Message Structure */
typedef struct {
    long mtype;
    msg_type_t cmd;
    pid_t target_pid;
    int signal;
    char response[256];
} process_msg_t;

/* Memory Block Header for Allocator */
typedef struct mem_block {
    size_t size;
    int in_use;
    struct mem_block *next;
} mem_block_t;

/* Scheduler Priority Levels */
typedef enum {
    PRIORITY_HIGH = 1,    // Update every 1 second
    PRIORITY_MEDIUM = 3,  // Update every 3 seconds
    PRIORITY_LOW = 5      // Update every 5 seconds
} priority_level_t;

/* Function Declarations */
void log_message(const char *format, ...);
void error_exit(const char *msg);

#endif /* COMMON_H */


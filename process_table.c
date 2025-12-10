#include "process_table.h"
#include "logger.h"

static int shm_id = -1;
static int sem_id = -1;
static process_table_t *shared_table = NULL;

/* Initialize semaphores */
int init_semaphores(void) {
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        return -1;
    }
    
    /* Set initial semaphore value to 1 (binary semaphore for mutual exclusion) */
    semctl(sem_id, 0, SETVAL, 1);
    log_message("Semaphores initialized\n");
    return 0;
}

/* Destroy semaphores */
void destroy_semaphores(void) {
    if (sem_id != -1) {
        semctl(sem_id, 0, IPC_RMID, 0);
        sem_id = -1;
        log_message("Semaphores destroyed\n");
    }
}

/* Lock the process table */
void lock_table(void) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;  /* Decrement (wait) */
    op.sem_flg = SEM_UNDO;
    
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop lock");
    }
}

/* Unlock the process table */
void unlock_table(void) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;   /* Increment (signal) */
    op.sem_flg = SEM_UNDO;
    
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop unlock");
    }
}

/* Attach to shared memory */
process_table_t* attach_shared_memory(void) {
    if (shared_table != NULL) {
        return shared_table;
    }
    
    /* Create or get shared memory segment */
    shm_id = shmget(SHM_KEY, sizeof(process_table_t), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return NULL;
    }
    
    /* Attach to shared memory */
    shared_table = (process_table_t*)shmat(shm_id, NULL, 0);
    if (shared_table == (void*)-1) {
        perror("shmat");
        shared_table = NULL;
        return NULL;
    }
    
    /* Initialize if first time */
    if (shared_table->active == 0) {
        shared_table->count = 0;
        shared_table->last_sync = time(NULL);
        shared_table->active = 1;
        memset(shared_table->processes, 0, sizeof(shared_table->processes));
    }
    
    log_message("Shared memory attached\n");
    return shared_table;
}

/* Detach from shared memory */
void detach_shared_memory(process_table_t *table) {
    if (table != NULL && table == shared_table) {
        shmdt(table);
        shared_table = NULL;
        log_message("Shared memory detached\n");
    }
}

/* Destroy shared memory */
void destroy_shared_memory(void) {
    if (shared_table != NULL) {
        shmdt(shared_table);
        shared_table = NULL;
    }
    
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
        shm_id = -1;
        log_message("Shared memory destroyed\n");
    }
}

/* Find process index by PID */
int find_process_index(process_table_t *table, pid_t pid) {
    if (table == NULL) return -1;
    
    for (int i = 0; i < table->count; i++) {
        if (table->processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

/* Update process information */
void update_process_info(process_table_t *table, int index, process_info_t *info) {
    if (table == NULL || info == NULL) return;
    
    lock_table();
    
    if (index >= 0 && index < MAX_PROCESSES) {
        if (index >= table->count) {
            table->count = index + 1;
        }
        memcpy(&table->processes[index], info, sizeof(process_info_t));
        table->last_sync = time(NULL);
    }
    
    unlock_table();
}

/* Remove process from table */
void remove_process(process_table_t *table, pid_t pid) {
    if (table == NULL) return;
    
    lock_table();
    
    int index = find_process_index(table, pid);
    if (index >= 0) {
        /* Shift remaining processes */
        for (int i = index; i < table->count - 1; i++) {
            memcpy(&table->processes[i], &table->processes[i + 1], sizeof(process_info_t));
        }
        table->count--;
        table->last_sync = time(NULL);
    }
    
    unlock_table();
}

/* Get process by PID */
process_info_t* get_process(process_table_t *table, pid_t pid) {
    if (table == NULL) return NULL;
    
    lock_table();
    
    int index = find_process_index(table, pid);
    process_info_t *result = NULL;
    
    if (index >= 0) {
        result = &table->processes[index];
    }
    
    unlock_table();
    return result;
}


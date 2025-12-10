#include "message_queue.h"

static int msg_queue_id = -1;

/* Initialize message queue */
int init_message_queue(void) {
    msg_queue_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_queue_id == -1) {
        perror("msgget");
        return -1;
    }
    
    log_message("Message queue initialized\n");
    return 0;
}

/* Destroy message queue */
void destroy_message_queue(void) {
    if (msg_queue_id != -1) {
        msgctl(msg_queue_id, IPC_RMID, NULL);
        msg_queue_id = -1;
        log_message("Message queue destroyed\n");
    }
}

/* Send command to message queue */
int send_command(msg_type_t cmd, pid_t pid, int signal) {
    if (msg_queue_id == -1) {
        if (init_message_queue() == -1) {
            return -1;
        }
    }
    
    process_msg_t msg;
    msg.mtype = 1;  /* Server message type */
    msg.cmd = cmd;
    msg.target_pid = pid;
    msg.signal = signal;
    strcpy(msg.response, "");
    
    if (msgsnd(msg_queue_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return -1;
    }
    
    log_message("Command sent: type=%d, pid=%d\n", cmd, pid);
    return 0;
}

/* Receive command from message queue */
int receive_command(process_msg_t *msg) {
    if (msg_queue_id == -1) {
        if (init_message_queue() == -1) {
            return -1;
        }
    }
    
    if (msgrcv(msg_queue_id, msg, sizeof(*msg) - sizeof(long), 1, IPC_NOWAIT) == -1) {
        if (errno != ENOMSG) {
            perror("msgrcv");
        }
        return -1;
    }
    
    return 0;
}

/* Send response */
int send_response(long mtype, const char *response) {
    if (msg_queue_id == -1) {
        return -1;
    }
    
    process_msg_t msg;
    msg.mtype = mtype;
    strncpy(msg.response, response, sizeof(msg.response) - 1);
    msg.response[sizeof(msg.response) - 1] = '\0';
    
    if (msgsnd(msg_queue_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd response");
        return -1;
    }
    
    return 0;
}


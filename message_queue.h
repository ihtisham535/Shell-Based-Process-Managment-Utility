#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "common.h"

/* Message Queue Functions */
int init_message_queue(void);
void destroy_message_queue(void);
int send_command(msg_type_t cmd, pid_t pid, int signal);
int receive_command(process_msg_t *msg);
int send_response(long mtype, const char *response);

#endif /* MESSAGE_QUEUE_H */


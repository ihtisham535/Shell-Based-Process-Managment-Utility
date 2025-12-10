#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

/* Logger Functions */
void init_logger(void);
void close_logger(void);
void log_resource_usage(process_info_t *info);
void log_historical_stats(process_info_t *info);
void log_operation(const char *operation, pid_t pid, const char *result);

#endif /* LOGGER_H */


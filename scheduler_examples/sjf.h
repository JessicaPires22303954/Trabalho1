#ifndef SJF_H
#define SJF_H

#include <stdint.h>
#include "queue.h" /* ou o header que define queue_t e pcb_t no teu projecto */

void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif /* SJF_H */

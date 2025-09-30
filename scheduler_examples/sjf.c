#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "msg.h"

/**
 * Shortest Job First (non-preemptive selection by remaining time).
 *
 * - Se houver uma tarefa a correr atualiza o elapsed e testa conclusão (igual ao FIFO).
 * - Se o CPU estiver livre, escolhe da ready-queue o PCB com menor tempo REMANESCENTE
 *   (time_ms - ellapsed_time_ms) e coloca-o no CPU.
 *
 * Nota: usa dequeue_pcb(rq) e enqueue_pcb(rq, pcb) — funções presentes no código base.
 */
void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    /* Se já existe tarefa a correr, avança e verifica se terminou */
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
    }

    /* Se CPU está livre, escolhe o processo de menor tempo remanescente */
    if (*cpu_task == NULL) {
        pcb_t *p = NULL;
        size_t cap = 16, n = 0;
        pcb_t **arr = malloc(cap * sizeof(pcb_t *));
        if (!arr) {
            perror("malloc");
            return;
        }

        pcb_t *min_pcb = NULL;

        /* Esvazia a ready queue para um array dinâmico, encontrando o mínimo */
        while ((p = dequeue_pcb(rq)) != NULL) {
            if (n == cap) {
                cap *= 2;
                pcb_t **tmp = realloc(arr, cap * sizeof(pcb_t *));
                if (!tmp) {
                    perror("realloc");
                    /* se falhar, re-enfila os já retirados e sai */
                    for (size_t i = 0; i < n; ++i) enqueue_pcb(rq, arr[i]);
                    free(arr);
                    return;
                }
                arr = tmp;
            }
            arr[n++] = p;

            uint32_t remaining = (p->time_ms > p->ellapsed_time_ms) ? (p->time_ms - p->ellapsed_time_ms) : 0;
            if (min_pcb == NULL) {
                min_pcb = p;
            } else {
                uint32_t min_remaining = (min_pcb->time_ms > min_pcb->ellapsed_time_ms) ? (min_pcb->time_ms - min_pcb->ellapsed_time_ms) : 0;
                if (remaining < min_remaining) {
                    min_pcb = p;
                }
            }
        }

        /* Se encontramos um processo mínimo, re-enfila os restantes e atribui CPU */
        if (min_pcb) {
            for (size_t i = 0; i < n; ++i) {
                if (arr[i] == min_pcb) continue;
                enqueue_pcb(rq, arr[i]);
            }
            *cpu_task = min_pcb;
        }

        free(arr);
    }
}

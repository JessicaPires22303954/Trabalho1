#include "rr.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "msg.h"

#define RR_QUANTUM_MS 500

/**
 * Round Robin scheduling (quantum = 500 ms).
 *
 * - Executa a tarefa atual durante até 500 ms.
 * - Se acabar antes, envia DONE e liberta.
 * - Se gastar todo o quantum e não terminar, é re-enfileirada no fim da ready queue.
 */
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    static uint32_t quantum_used_ms = 0;  // tempo usado pela tarefa atual

    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        quantum_used_ms += TICKS_MS;

        /*Envia mensagem PROCESS_REQUEST_DONE ao cliente.
        *Libera memória do processo.
        *Libera CPU.
        *Reseta quantum usado.*/
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Tarefa terminou
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };

            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
            quantum_used_ms = 0;

            /*Preempção: processo volta para o fim da fila.
            Libera CPU.
            Quantum reinicia.*/
        } else if (quantum_used_ms >= RR_QUANTUM_MS) {
            // Quantum esgotado → re-enfileira a tarefa
            enqueue_pcb(rq, *cpu_task);
            *cpu_task = NULL;
            quantum_used_ms = 0;
        }
    }

    // Se CPU estiver livre, vai buscar próximo
    if (*cpu_task == NULL) {
        *cpu_task = dequeue_pcb(rq);
    }
}

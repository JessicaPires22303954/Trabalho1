#include "mlfq.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "msg.h"

#define MLFQ_LEVELS 3
#define MLFQ_QUANTUM_MS 500

/* metadata para cada PCB que o MLFQ controla */
/*Estrutura que guarda informações extras por processo (nível atual e quanto já consumiu do quantum).
Ligada numa lista meta_head.*/
typedef struct mlfq_meta {
    pcb_t *pcb;
    int level;                  /* 0 .. MLFQ_LEVELS-1 (0 = highest) */
    uint32_t quantum_used_ms;   /* quanto já usou neste quantum */
    struct mlfq_meta *next;
} mlfq_meta_t;

/* filas internas do MLFQ (uma queue_t por nível) */
/*Uma fila por nível de prioridade (mlfq_queues).
Lista ligada com todos os metadados (meta_head).
initialized garante que a estrutura só é inicializada uma vez.*/
static queue_t mlfq_queues[MLFQ_LEVELS];
static mlfq_meta_t *meta_head = NULL;
static int initialized = 0;

/* --- Helpers meta --- */
/*Procura o metadado de um processo (pcb) na lista.*/
static mlfq_meta_t *find_meta(pcb_t *p) {
    mlfq_meta_t *m = meta_head;
    while (m) {
        if (m->pcb == p) return m;
        m = m->next;
    }
    return NULL;
}
/*Cria um novo metadado para p e insere no início da lista.*/
static void add_meta(pcb_t *p, int level) {
    mlfq_meta_t *m = malloc(sizeof(mlfq_meta_t));
    if (!m) return;
    m->pcb = p;
    m->level = level;
    m->quantum_used_ms = 0;
    m->next = meta_head;
    meta_head = m;
}
/*Remove o metadado associado a p (quando processo termina).*/
static void remove_meta(pcb_t *p) {
    mlfq_meta_t **prev = &meta_head;
    mlfq_meta_t *cur = meta_head;
    while (cur) {
        if (cur->pcb == p) {
            *prev = cur->next;
            free(cur);
            return;
        }
        prev = &cur->next;
        cur = cur->next;
    }
}

/* inicializa as filas internas (uma vez) */
/*Garante que todas as filas internas começam vazias.
Só é chamado uma vez no início.*/
static void mlfq_init(void) {
    if (initialized) return;
    for (int i = 0; i < MLFQ_LEVELS; ++i) {
        mlfq_queues[i].head = NULL;
        mlfq_queues[i].tail = NULL;
    }
    meta_head = NULL;
    initialized = 1;
}

/*
 * Absorve novos processos da ready-queue geral (rq) e coloca no nível 0.
 * Se já existir metadata para esse PCB, mantém o nível guardado (útil quando
 * um processo volta do BLOCK).
 */
/*Pega todos os processos da fila global (rq) e coloca-os no MLFQ interno.
Novos processos começam sempre no nível 0.
Processos que estavam bloqueados regressam ao nível que tinham antes.*/
static void absorb_ready_queue(queue_t *rq) {
    pcb_t *p;
    while ((p = dequeue_pcb(rq)) != NULL) {
        mlfq_meta_t *m = find_meta(p);
        if (!m) {
            add_meta(p, 0);
            enqueue_pcb(&mlfq_queues[0], p);
        } else {
            /* se tinha meta (p. ex. voltou do blocked), usa nível guardado */
            enqueue_pcb(&mlfq_queues[m->level], p);
        }
    }
}

/**
 * mlfq_scheduler
 *
 * - current_time_ms: tempo actual (ms)
 * - rq: ready queue (ossim enfileira aqui novos processos)
 * - cpu_task: ponteiro para a pcb que corre actualmente (ou NULL)
 *
 * Política implementada:
 * - 3 níveis (0..2), todos os novos chegam ao nível 0.
 * - quantum por nível = 500 ms.
 * - se gastar o quantum sem terminar -> desce 1 nível (até ao 2) e é re-enfileirado.
 * - se terminar antes do quantum -> DONE (liberta) e meta removida.
 * - se bloquear (handled por ossim), mantemos metadata para quando voltar.
 *
 * NOTA: MLFQ apenas gere a dispatch/quantum. A gestão de BLOCK/RUN no ossim
 * deve continuar a enfileirar em ready/blocked/command como já está.
 */
void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    mlfq_init();

    /* 1) absorve novos processos do ready_queue global */
    absorb_ready_queue(rq);

    /* 2) Actualiza tarefa corrente (se existir) */
    /*Se há processo a correr, incrementa o tempo decorrido e o quantum usado.
     *Garante que existe metadado para o processo (caso excecional).*/
    if (*cpu_task) {
        mlfq_meta_t *m = find_meta(*cpu_task);
        if (!m) {
            /* criamos metadata por segurança (pode acontecer em casos especiais) */
            add_meta(*cpu_task, 0);
            m = find_meta(*cpu_task);
        }

        (*cpu_task)->ellapsed_time_ms += TICKS_MS;
        m->quantum_used_ms += TICKS_MS;

        /*Envia mensagem DONE.
         *Remove metadado e liberta memória.
         *CPU fica livre.*/
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            /* envia DONE para a aplicação */
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            /* remove metadata e liberta */
            remove_meta(*cpu_task);
            free(*cpu_task);
            *cpu_task = NULL;
            /*Se acabou o quantum, o processo é rebaixado (até ao último nível) e volta para a fila.
             *CPU fica livre.*/
        } else if (m->quantum_used_ms >= MLFQ_QUANTUM_MS) {
            /* quantum esgotado -> desce nível (se possível) e re-enfileira */
            if (m->level < MLFQ_LEVELS - 1) m->level++;
            m->quantum_used_ms = 0;
            enqueue_pcb(&mlfq_queues[m->level], *cpu_task);
            *cpu_task = NULL;
            /*processo permanece na CPU.*/
        } else {
            /* ainda dentro do quantum -> continua a correr */
        }
    }

    /* 3) Se CPU livre, escolhe da fila de maior prioridade disponível */
    /*Se CPU está livre, percorre as filas por prioridade (0 → 2).
     *Escolhe o primeiro processo disponível.
     *Garante que tem metadado associado.*/
    if (*cpu_task == NULL) {
        for (int lvl = 0; lvl < MLFQ_LEVELS; ++lvl) {
            pcb_t *next = dequeue_pcb(&mlfq_queues[lvl]);
            if (next) {
                /* garante meta e actualiza nível */
                mlfq_meta_t *m = find_meta(next);
                if (!m) add_meta(next, lvl);
                else m->level = lvl;
                /* assume que quantum_used_ms é mantido/reset conforme política:
                   mantemos o quantum_used_ms existente só que, normalmente, ao
                   descer de nível quantum_used_ms já terá sido 0. */
                *cpu_task = next;
                break;
            }
        }
    }
}

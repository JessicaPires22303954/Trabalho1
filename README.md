Jéssica Pires -a22303954

Link do repositório: https://github.com/JessicaPires22303954/Trabalho1_Jessica_Pires-a22303954

| Aplicação | Métrica (segs) | FIFO   | SJF    | RR     | MLFQ   |
|-----------|----------------|--------|--------|--------|--------|
| A         | Tempo Execução | 10.000 | 10.000 | 10.000 | 10.000 |
| A         | Tempo Resposta | 0      | 0      | 0      | 0      |
| B         | Tempo Execução | 24.990 | 25.000 | 25.000 | 24.980 |
| B         | Tempo Resposta | 9.990  | 10.000 | 10.000 | 9.980  |
| C         | Tempo Execução | 44.980 | 45.000 | 44.990 | 44.990 |
| C         | Tempo Resposta | 24.980 | 25.000 | 24.990 | 24.990 |


Tabela Resumo

| Cenário | Métrica (segs)       | FIFO   | SJF    | RR     | MLFQ   |
|---------|----------------------|--------|--------|--------|--------|
| 1       | Tempo Médio Execução | 26.657 | 26.667 | 26.663 | 26.657 |
| 1       | Tempo Médio Resposta | 11.657 | 11.667 | 11.663 | 11.657 |
| 2       | Tempo Médio Execução | 26.663 | 28.327 | 28.333 | 37.830 |
| 2       | Tempo Médio Resposta | 11.663 | 13.327 | 13.333 | 22.830 |
| 3       | Tempo Médio Execução | 28.323 | 26.667 | 28.330 | 37.993 |
| 3       | Tempo Médio Resposta | 13.323 | 11.667 | 13.330 | 22.993 |

No cenário 1, todos os algoritmos apresentaram resultados muito semelhantes, com tempos médios de execução em torno dos 26,6 segundos e tempos médios de resposta próximos de 11,6 segundos.
Isto deve-se ao facto de as tarefas terem durações semelhantes e chegarem praticamente em simultâneo, não existindo grande diferença entre executar por ordem de chegada (FIFO), escolher o mais curto primeiro (SJF), alternar com quantum fixo (RR) ou aplicar múltiplas filas (MLFQ).
Concluindo neste cenário o tipo de escalonador não influencia de forma significativa o desempenho global.

No cenário 2, observa-se uma maior diferenciação entre os algoritmos. O SJF apresentou o menor tempo médio de resposta (≈ 13,3s), evidenciando a sua eficácia ao priorizar processos mais curtos. Tanto o FIFO como o RR mostraram desempenhos muito semelhantes entre si, com tempos de execução entre 26,6s e 28,3s e respostas médias em torno de 11,6s a 13,3s.
Por outro lado, o MLFQ obteve resultados claramente superiores em tempo de execução (≈ 37,8s) e em tempo de resposta (≈ 22,8s), o que indica que, neste cenário específico, a sua política de múltiplas filas e penalização de processos longos aumentou a espera média dos processos.
Concluindo neste cenário o SJF foi o mais eficiente neste cenário, enquanto o MLFQ revelou-se menos adequado.

No cenário 3, o comportamento manteve-se semelhante ao anterior. O SJF voltou a apresentar o melhor tempo médio de resposta (≈ 11,6s) e de execução (≈ 26,6s), confirmando a sua eficiência em cargas com múltiplos processos de diferentes durações. O FIFO e o RR mantiveram valores intermédios e bastante próximos entre si, enquanto o MLFQ voltou a registar os piores resultados, com execução média em torno de 38s e tempo de resposta próximo de 23s.
Concluindo o SJF continuou a ser o mais eficiente, enquanto o MLFQ não trouxe vantagens no contexto testado.
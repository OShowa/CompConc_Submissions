# CompConc_Submissions

Repositório para a submissão de atividades para Computação Concorrente (período 2021.2).

## Laboratório 3 - Relatório de Desempenho

Abaixo, estão as acelerações (tempo sequencial / tempo concorrente) de cada tamanho de vetor para cada quantidade de threads. Cada teste foi feito **5** vezes. Nota-se que para alguns tamanhos de vetor, a solução concorrente pode não valer à pena. 
Os testes foram realizados em uma máquina Windows, no WSL, com um
processador AMD Ryzen 5 3600, que possui 6 cores e 12 threads.

### 10<sup>5</sup>:

- 1 thread:  **0,48**
- 2 threads: **0,86**
- 4 threads: **1,05**

### 10<sup>6</sup>:

- 1 thread: **0,78**
- 2 threads: **1,59**
- 4 threads: **3,15**

### 10<sup>8</sup>:

- 1 thread: **0,82**
- 2 threads: **1,61**
- 4 threads: **3,2**

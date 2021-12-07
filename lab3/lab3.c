#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "timer.h"

#define FLOAT_MAX 1000.0 // valor máximo dos floats gerados

float * vector; // vetor que será criado
float L_i, L_s; // limiar inferior e limiar superior
long long int N; // tamanho do vetor
int nthreads; // número de threads usadas


void init_vector(long long int N) { // inicializa o vetor com floats aleatórios entre 0 e FLOAT_MAX

    vector = malloc(N * sizeof(float));

    if (vector == NULL) {
        printf("--ERROR!-- init_vector -> malloc\n");
        exit(2);
    }

    for (long long int i = 0; i < N; i++) {
        vector[i] = ((float) rand() / (float) (RAND_MAX)) * FLOAT_MAX; // transforma o int retornado por rand() em um float no intervalo desejado
    }

}

void print_vector(long long int N) { // printa o vetor (para debug)

    for (long long int i = 0; i < N; i++) {
        printf("%.2f ", vector[i]);
    }

    printf("\n\n");

}

void query_user() { // pede ao usuário para que insira uma limiar inferior e uma limiar superior

    do {
        printf("Select a lower bound (float): ");
        scanf("%f", &L_i);
        printf("Select an upper bound (float): ");
        scanf("%f", &L_s);
        if (L_s < L_i) printf("Upper bound must be larger than lower bound!\n");
    } while (L_s < L_i); // só para quando o usuário entregar um intervalo válido

}

long long int seq_check(long long int N) { // verifica a quantidade de floats no intervalo dado de forma sequencial

    long long int count = 0;

    for (long long int i = 0; i < N; i++) {
        if ((vector[i] < L_s) && (vector[i] > L_i))
            count++; // se o float atual está no intervalo, conta mais um
    }

    return count;

}

void * conc_check(void * arg) { // verifica a quantidade de floats no intervalo dado de forma concorrente

    long int id = (long int) arg; // o argumento é passado na forma de (void *), então vira long int para não haver warning
    long long int count = 0;

    for (long long int i = id; i < N; i+=nthreads) { // checa o vetor de forma alternada
        if ((vector[i] < L_s) && (vector[i] > L_i)) {
            count++; // se o float atual está no intervalo, conta mais um
        }
    }

    pthread_exit((void *) count); // retorna a contagem na forma de (void *) por conformidade, pois o que importa é o valor absoluto

}

void print_result(long long int seq_count, long long int conc_count) { // printa o resultado e compara os vetores para checar se houve sucesso

    printf("Sequential count: %lld\nConcurrent count: %lld\n", seq_count, conc_count);

    if (seq_count == conc_count) printf("Success!\n"); else printf("Fail!\n");

}


int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Usage: %s <vector_size> <number_of_threads>\n", argv[0]);
        return 1;
    }

    N = atoll(argv[1]);
    nthreads = atoi(argv[2]);

    query_user();

    srand(time(NULL)); // cria uma seed para os floats aleatórios

    init_vector(N);

    double init, end, seq_time, conc_time; // variáveis de medição de desempenho


    GET_TIME(init); // começo da parte sequencial
    long long int seq_count = seq_check(N);
    GET_TIME(end); // fim da parte sequencial

    seq_time = end - init;
    printf("Sequential time elapsed: %e\n", seq_time);


    GET_TIME(init); // começo da parte concorrente
    pthread_t * system_tid = malloc(nthreads * sizeof(pthread_t));
    long long int * local_sum = malloc(nthreads * sizeof(long long int));
    long long int conc_count = 0;

    if ((system_tid == NULL) || (local_sum == NULL)) {
        printf("--ERROR!-- main -> malloc\n");
        exit(2);
    }


    for (long int i = 0; i < nthreads; i++) {
        
        if (pthread_create(&system_tid[i], NULL, conc_check, (void *) i)) {
            printf("--ERROR!-- pthread_create\n");
            return 3;
        }

    }

    for (int i = 0; i < nthreads; i++) {

        if (pthread_join(system_tid[i], (void *) &local_sum[i])) { // espera todas as threads acabarem e recebe suas respectivas somas
            printf("--ERROR!-- pthread_join\n");
            return 4;
        }

        conc_count += local_sum[i]; // soma as somas locais para obter o total

    }

    GET_TIME(end); // fim da parte concorrente

    conc_time = end - init;
    printf("Concurrent time elapsed: %e\nTotal gain: %.2f\n", conc_time, seq_time/conc_time);


    print_result(seq_count, conc_count); // printa o resultado e checa se foi bem-sucedido

    // print_vector(N); // para debug

    free(vector);
    free(system_tid);
    free(local_sum);

    return 0;

}
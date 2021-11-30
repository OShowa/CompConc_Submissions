#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "timer.h"

#define MAX_ELEMENT 10 // tamanho máximo de um inteiro nas matrizes geradas.

typedef struct _thread_args { // struct que servirá pra passar os argumentos para as novas threads.
    int tid, dim, nthreads;
    int *m1,*m2;
} thread_args;

int * multithread_output; // a matriz resultante da multiplicação no método concorrente.


int * generate_matrix(int dim) { // função que gera matrizes aleatórias para multiplicar.

    int * matrix = malloc(dim * dim * sizeof(int)); // uma matriz é um vetor unidimensional de tamanho dim^2 nessa implementação.

    if (matrix == NULL) {
        printf("--ERROR!-- generate_matrix -> malloc \n");
        exit(3);
    };

    for (int i = 0; i < dim * dim; i++)
        matrix[i] = rand() % MAX_ELEMENT;

    return matrix;


}

int * sequential_mult(int * m1, int * m2, int dim) { // multiplicação de matrizes para a parte sequencial do programa.

    int * output = malloc(dim * dim * sizeof(int));

    if (output == NULL) {
        printf("--ERROR!-- sequential_mult -> malloc \n");
        exit(3);
    }

    for (int i = 0; i < dim*dim; i++) {

        int current_row = i / dim; // linha atual da matriz resultante baseada no índice i.
        int current_column = i % dim; // coluna atual da matriz resultante baseada no índice i.
        output[i] = 0;

        for (int j = 0; j < dim; j++) {
            output[i] += m1[current_row * dim + j] * m2[current_column + j * dim]; // output[current_row][current_column] += m1[current_row][j] * m2[j][current_column];
        }
    }

    return output;

}

void * multithread_mult(void * arg) { // multiplicação de matrizes para a parte concorrente do programa.
    
    thread_args args = *(thread_args *) arg;

    int dim = args.dim;
    int tid = args.tid;
    int nthreads = args.nthreads;
    int *m1 = args.m1;
    int *m2 = args.m2;

    for (int i = tid; i < dim*dim; i+=nthreads) { // para esta implementação, calculamos célula por célula de forma alternada entre as threads.

        int current_row = i / dim;
        int current_column = i % dim;
        multithread_output[i] = 0;

        for (int j = 0; j < dim; j++) {
            multithread_output[i] += m1[current_row * dim + j] * m2[current_column + j * dim];
        }
    }

    pthread_exit(NULL);
}

void print_matrix(int * matrix, int dim) { // função para debug: printa uma matriz.

    for (int i = 0; i < dim*dim; i++) {
        if ((i != 0) && (i % dim == 0))
            printf("\n");
        printf("%d ", matrix[i]);
    }

    printf("\n\n");

} 

int check_matrix(int * m1, int * m2, int dim) { // função que verifica se uma matriz é igual a outra (para verificar o resultado).

    for (int i = 0; i < dim*dim; i++)
        if (m1[i] != m2[i]) return 0;

    return 1;
}


int main(int argc, char * argv[]) {

    int nthreads, dim; // número de threads e a dimensão dim x dim das matrizes.
    int *m1, *m2, *sequential_output; // matriz fator m1, matriz fator m2 e matriz produto sequencial sequential_output.

    if (argc != 3) { // caso o número de argumentos esteja errado.
        printf("Usage: ./matrix_mult <number of threads> <matrix size>\n");
        return 1;
    }

    nthreads = atoi(argv[1]);
    dim = atoi(argv[2]);

    if ((nthreads == 0) || (dim == 0)) {
        printf("--ERROR!-- atoi returned 0\n");
        return 2;
    }

    srand(time(NULL)); // seed para a geração de matrizes aleatórias.

    m1 = generate_matrix(dim);
    m2 = generate_matrix(dim);

    double start, finish, seq_time, mult_time; // variáveis para o controle do tempo e medição de desempenho

    GET_TIME(start); // começa a contar o tempo do algoritmo sequencial.

    sequential_output = sequential_mult(m1, m2, dim);

    GET_TIME(finish); // termina de contar o tempo do algoritmo sequencial.

    seq_time = finish - start;

    printf("Sequential multiplication took %e seconds with %d dimension.\n", seq_time, dim);

    GET_TIME(start); // começa a contar o tempo do algoritmo concorrente.

    pthread_t * system_tid = malloc(nthreads * sizeof(pthread_t)); // identificador de thread do sistema.
    thread_args * args = malloc(nthreads * sizeof(thread_args)); // argumentos passados para a função multithread_mult.
    multithread_output = malloc(dim * dim * sizeof(int)); // aloca memória para a matriz resultante concorrente.

    if ((system_tid == NULL) || (args == NULL) || (multithread_output == NULL)) {
        printf("--ERROR!-- main -> malloc\n");
        return 3;
    }

    for (int i = 0; i < nthreads; i++) { // inicializa as threads a serem usadas.
        
        args[i].dim = dim;
        args[i].tid = i;
        args[i].nthreads = nthreads;
        args[i].m1 = m1;
        args[i].m2 = m2;
        
        if (pthread_create(&system_tid[i], NULL, multithread_mult, (void *) &args[i])) {
            printf("--ERROR!-- pthread_create\n");
            return 4;
        }
    }

    for (int i = 0; i < nthreads; i++) { // espera todas as threads acabarem.
        if (pthread_join(system_tid[i], NULL)) {
            printf("--ERROR!-- pthread_join\n");
            return 5;
        }
    }

    GET_TIME(finish); // termina de contar o tempo do algoritmo concorrente.

    mult_time = finish - start;

    printf("Multithread multiplication took %e seconds with %d threads and %d dimension.\n", mult_time, nthreads, dim);

    // funções de print pra debug.

    // print_matrix(m1, dim);
    // print_matrix(m2, dim);
    // print_matrix(sequential_output, dim);
    // print_matrix(multithread_output, dim);

    if (check_matrix(sequential_output, multithread_output, dim)) { // se os resultados batem, exibe uma mensagem de sucesso e o ganho de desempenho.
        printf("Successfully multiplied matrices!\n");
        printf("Performance gain: %.2f\n", seq_time/mult_time);
    } else printf("Something went wrong while multiplying...\n");

    pthread_exit(NULL);

    return 0;

}

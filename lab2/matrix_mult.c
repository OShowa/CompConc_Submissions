#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "timer.h"

#define MAX_ELEMENT 10

typedef struct _thread_args {
    int tid, dim, nthreads;
    int *m1,*m2;
} thread_args;

int * multithread_output;


int * generate_matrix(int dim) {

    int * matrix = malloc(dim * dim * sizeof(int));

    if (matrix == NULL) {
        printf("--ERROR!-- generate_matrix -> malloc \n");
        exit(3);
    };

    for (int i = 0; i < dim * dim; i++)
        matrix[i] = rand() % MAX_ELEMENT;

    return matrix;


}

int * sequential_mult(int * m1, int * m2, int dim) {

    int * output = malloc(dim * dim * sizeof(int));

    if (output == NULL) {
        printf("--ERROR!-- sequential_mult -> malloc \n");
        exit(3);
    }

    for (int i = 0; i < dim*dim; i++) {

        int current_row = i / dim;
        int current_column = i % dim;
        output[i] = 0;

        for (int j = 0; j < dim; j++) {
            output[i] += m1[current_row * dim + j] * m2[current_column + j * dim];
        }
    }

    return output;

}

void * multithread_mult(void * arg) {
    
    thread_args args = *(thread_args *) arg;

    int dim = args.dim;
    int tid = args.tid;
    int nthreads = args.nthreads;
    int *m1 = args.m1;
    int *m2 = args.m2;

    for (int i = tid; i < dim*dim; i+=nthreads) {

        int current_row = i / dim;
        int current_column = i % dim;
        multithread_output[i] = 0;

        for (int j = 0; j < dim; j++) {
            multithread_output[i] += m1[current_row * dim + j] * m2[current_column + j * dim];
        }
    }

    pthread_exit(NULL);
}

void print_matrix(int * matrix, int dim) {

    for (int i = 0; i < dim*dim; i++) {
        if ((i != 0) && (i % dim == 0))
            printf("\n");
        printf("%d ", matrix[i]);
    }

    printf("\n\n");

} 

int check_matrix(int * m1, int * m2, int dim) {

    for (int i = 0; i < dim*dim; i++)
        if (m1[i] != m2[i]) return 0;

    return 1;
}


int main(int argc, char * argv[]) {

    int nthreads, dim;
    int *m1, *m2, *sequential_output;

    if (argc != 3) {
        printf("Usage: ./matrix_mult <number of threads> <matrix size>\n");
        return 1;
    }

    nthreads = atoi(argv[1]);
    dim = atoi(argv[2]);

    if ((nthreads == 0) || (dim == 0)) {
        printf("--ERROR!-- atoi returned 0\n");
        return 2;
    }

    srand(time(NULL));

    m1 = generate_matrix(dim);
    m2 = generate_matrix(dim);

    double start, finish, total;

    GET_TIME(start);

    sequential_output = sequential_mult(m1, m2, dim);

    GET_TIME(finish);

    total = finish - start;

    printf("Sequential multiplication took %e seconds with %d dimension.\n", total, dim);

    pthread_t * system_tid = malloc(nthreads * sizeof(pthread_t));
    thread_args * args = malloc(nthreads * sizeof(thread_args));
    multithread_output = malloc(dim * dim * sizeof(int));

    if ((system_tid == NULL) || (args == NULL) || (multithread_output == NULL)) {
        printf("--ERROR!-- main -> malloc\n");
        return 3;
    }

    GET_TIME(start);

    for (int i = 0; i < nthreads; i++) {
        
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

    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(system_tid[i], NULL)) {
            printf("--ERROR!-- pthread_join\n");
            return 5;
        }
    }

    GET_TIME(finish);

    total = finish - start;

    printf("Multithread multiplication took %e seconds with %d threads and %d dimension.\n", total, nthreads, dim);

    // print_matrix(m1, dim);
    // print_matrix(m2, dim);
    // print_matrix(sequential_output, dim);
    // print_matrix(multithread_output, dim);

    if (check_matrix(sequential_output, multithread_output, dim)) {
        printf("Successfully multiplied matrices!\n");
    } else printf("Something went wrong while multiplying...\n");

    pthread_exit(NULL);

    return 0;

}

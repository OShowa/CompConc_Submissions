/* Disciplina: Computacao Concorrente */
/* Miguel Santos Uchoa da Fonseca - DRE 120036412 */
/* Modulo 1 - Laboratorio: 1 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NELEMENTS 10000 // numero de elementos no vetor: nesse caso, 10000, como pedido no enunciado.

#define NTHREADS 2 // numero de threads a serem utilizadas: nesse caso, 2, como pedido no enunciado.

int vector[NELEMENTS];

// funcao para inicializar um vetor de NELEMENTS inteiros.
void init_vector() {

    for (int i = 0; i < NELEMENTS; i++) {
        vector[i] = i+1;
    }

}

// funcao para checar se o vetor esta correto.

int test_vector() {

    for (int i = 0 ; i < NELEMENTS; i++) {
        if (vector[i] != (i+1) * (i+1)) {
            return 0;
        }
    }
    
    return 1;

}

// funcao para printar o vetor.
void print_vector() {

    for (int i = 0; i < NELEMENTS; i++) {
        printf("%d, ", vector[i]);
    }

    printf("\n");

}

// funcao que eleva todos os elementos do vetor ao quadrado.
void * square (void * arg) {

    int thread_number = * (int*) arg;

    /* esse loop eleva apenas os elementos com indice i que satisfazem thread_number - 1 = i % NTHREADS,
    impedindo assim que mais de uma thread opere sobre os mesmos elementos. */
    for (int i = thread_number-1; i < NELEMENTS; i += NTHREADS) {
        vector[i] = vector[i] * vector[i];
    }

    pthread_exit(NULL);

}


// funcao principal
int main() {
    pthread_t system_tid[NTHREADS]; // identificadores de thread do sistema.
    int thread_number[NTHREADS]; // identificador de thread local.
    int thread; // variavel auxiliar.

    init_vector(); // inicializa o vetor com os valores de 1 a NELEMENTS.

    // cria as threads.
    for (thread = 0; thread < NTHREADS; thread++) {
        thread_number[thread] = thread+1;
        if (pthread_create(&system_tid[thread], NULL, square, (void*) &thread_number[thread])) {
            printf("ERROR! Could not create thread %d\n", thread_number[thread]); exit(-1);
        }
    }

    // espera todas as threads acabarem de rodar.
    for (thread = 0; thread < NTHREADS; thread++) {
        if (pthread_join(system_tid[thread], NULL)) {
            printf("ERROR! pthread_join()\n"); exit(-1);
        }
    }

    if (test_vector()) printf("Vetor correto!\n"); // testa se o vetor realmente foi elevado ao quadrado.

    // print_vector() // printa o vetor (para debug).

    pthread_exit(NULL);

    return 0;
}
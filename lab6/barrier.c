#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

/* Variáveis globais */
int *vector, blocked = 0, nthreads;
pthread_mutex_t mutex_var;
pthread_cond_t barrier_cond;
double *local_sum;

void initialize_vector() { // inicializa o vetor com nthreads elementos aleatórios de 0 a 9.
    int i; 
    vector = malloc(sizeof(int)*nthreads);
    for (i = 0; i < nthreads; i++) {
        vector[i] = rand() % 10;
    }
}

void print_vector(int * v) { // printa um vetor dado.
    int i;
    for (i = 0; i < nthreads; i++) {
        printf("%d ", v[i]);
    }
    printf("\n");
}

void check_result(int * v) { // confere se um vetor só tem elementos iguais
    int i;
    for (i = 0; i < nthreads - 1; i++) {
        if (v[i] != v[i+1]) {
            printf("Erro de conta!\n");
            return;
        }
    }
    printf("Sucesso!\n");
}

void barrier(int tnumber) { // função barreira, que garante que todas as threads estejam no mesmo lugar pra progredir
    pthread_mutex_lock(&mutex_var); // tudo é feito com mutex, já que mexemos em variáveis globais

    if (blocked == (nthreads - 1)) { // se a thread é a última a "chegar", libera as outras e reinicia contador
        blocked = 0;
        pthread_cond_broadcast(&barrier_cond);
        printf("Threads liberadas!\n");
    } else { // do contrário, incrementa o contador de bloqueadas e espera o sinal de liberação
        blocked++;
        printf("Thread %d esperando término das outras threads. Bloqueadas: %d\n", tnumber, blocked);
        pthread_cond_wait(&barrier_cond, &mutex_var);
    }

    pthread_mutex_unlock(&mutex_var);
}

void * sum (void * arg) { // função que fará a tarefa de todas as threads (somar elementos do vetor e trocá-los)
    int tnumber = (int) arg; // número (ID) da thread
    long long int local_sum = 0; // soma parcial, só é long long int para silenciar warnings

    printf("Thread %d criada!\n", tnumber);

    for (int i = 0; i < nthreads; i++) { // nthreads iterações
        printf("Thread %d somando vetor pela %da vez.\n", tnumber, i+1);
        for (int j = 0; j < nthreads; j++) { // soma todos os elementos do vetor
            local_sum += vector[j];
        }
        barrier(tnumber); // espera as outras threads terminarem de somar
        vector[tnumber] = rand() % 10; // substitui um elemento do vetor por outro aleatório
        printf("vector[%d] = %d\n", tnumber, vector[tnumber]);
        barrier(tnumber); // espera as outras threads fazerem o mesmo para continuar
    }

    pthread_exit((void*) local_sum); // por fim, retorna a soma depois de nthreads iterações

}


int main(int argc, char * argv[]) { 
    
    if (argc < 2) { // caso o programa seja chamado sem um valor para nthreads
        printf("Uso: ./barrier <nthreads>\n");
        return -1;
    }

    nthreads = atoi(argv[1]);

    if (nthreads <= 0) { // caso o número dado seja inválido
        printf("Número inválido de threads!\n");
        return -2;
    }
    
    /* Alocações de memória */
    local_sum = malloc(sizeof(int) * nthreads);
    pthread_t *global_tid = malloc(sizeof(pthread_t) * nthreads);

    if (local_sum == NULL || global_tid == NULL) {
        printf("--ERROR!-- malloc\n");
        return 3;
    }

    /* Inicialização de mutex e variável condicional */
    pthread_mutex_init(&mutex_var, NULL);
    pthread_cond_init(&barrier_cond, NULL);

    srand(time(NULL)); // gera uma seed para números aleatórios

    initialize_vector();
    print_vector(vector);

    /* Criação de threads - i é long long int para silenciar warnings */
    for (long long int i = 0; i < nthreads; i++) {
        if (pthread_create(&global_tid[i], NULL, sum, (void*) i)) {
            printf("--ERROR!-- pthread_create\n");
            return 1;
        }
    }

    /* Espera todas as threads terminarem sua execução e guarda seus retornos */
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(global_tid[i], (void*) &local_sum[i])) {
            printf("--ERROR!-- pthread_join\n");
            return 2;
        }
    }

    /* Printa o vetor e checa se está certo */
    print_vector(local_sum);
    check_result(local_sum);

    /* Liberação de memória */
    pthread_mutex_destroy(&mutex_var);
    pthread_cond_destroy(&barrier_cond);
    free(global_tid);
    free(local_sum);

    return 0;
}

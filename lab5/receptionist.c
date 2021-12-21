/* Disciplina: Computação Concorrente */
/* Miguel Santos Uchôa da Fonseca - DRE 120036412 */
/* Laboratório: 5 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NTHREADS 5

/* Variáveis globais */
int x = 0; // contador para o número de mensagens impressas
pthread_mutex_t x_mutex;
pthread_cond_t welcomed, taken_care; // duas variáveis condicionais: uma pra segunda mensagem e outra pra última.


void * welcome(void * arg) { // função que printa boas-vindas antes das outras.

    printf("Seja bem-vindo!\n");
    x++; // aumenta o contador de mensagens em 1
    pthread_cond_broadcast(&welcomed); // avisa a outras threads que já foram dadas boas-vindas

    pthread_exit(NULL);
}

void * offerWater(void * arg) { // função que oferece água e funciona entre as boas-vindas e o volte sempre

    pthread_mutex_lock(&x_mutex); // vamos ler e editar x, então travamos as outras threads

    if (x == 0) {
        pthread_cond_wait(&welcomed, &x_mutex); // se ainda não foram dadas boas-vindas, esperamos
    }
    x++; // dadas as boas-vindas, podemos printar e incrementar o número de mensagens
    pthread_mutex_unlock(&x_mutex);

    printf("Aceita um copo d'água?\n");

    if (x == 4) {
        pthread_cond_signal(&taken_care); // se já foram as 4 mensagens, avisa à thread de volte sempre que pode prosseguir
    }

    pthread_exit(NULL);
}

void * sitDown(void * arg) { // função que oferece assento - similar a offerWater

    pthread_mutex_lock(&x_mutex);

    if (x == 0) {
        pthread_cond_wait(&welcomed, &x_mutex);
    }
    x++;
    pthread_mutex_unlock(&x_mutex);

    printf("Sente-se por favor.\n");

    if (x == 4) {
        pthread_cond_signal(&taken_care);
    }

    pthread_exit(NULL);
}

void * feelFree(void * arg) { // função que conforta o visitante - similar a offerWater

    pthread_mutex_lock(&x_mutex);

    if (x == 0) {
        pthread_cond_wait(&welcomed, &x_mutex);
    }
    x++;
    pthread_mutex_unlock(&x_mutex);

    printf("Fique à vontade.\n");

    if (x == 4) {
        pthread_cond_signal(&taken_care);
    }
    
    pthread_exit(NULL);
}

void * comeAgain(void * arg) { // função que printa "Volte sempre!" depois de todas as outras mensagens

    pthread_mutex_lock(&x_mutex); // vamos ler x, e para evitar problemas, travamos os outros fluxos

    if (x != 4) {
        pthread_cond_wait(&taken_care, &x_mutex); // se ainda não foram as 4 mensagens, esperamos
    }
    pthread_mutex_unlock(&x_mutex); // como é a última mensagem não é necessário incrementar x

    printf("Volte sempre!\n");

    pthread_exit(NULL);
}

int main() {
    int i;
    pthread_t threads[NTHREADS];

    /* Inicializando variáveis mutex e de condição */
    pthread_mutex_init(&x_mutex, NULL);
    pthread_cond_init(&welcomed, NULL);
    pthread_cond_init(&taken_care, NULL);

    /* Criando as threads de uma vez só, cada uma com sua função */
    pthread_create(&threads[0], NULL, comeAgain, NULL);
    pthread_create(&threads[1], NULL, feelFree, NULL);
    pthread_create(&threads[2], NULL, sitDown, NULL);
    pthread_create(&threads[3], NULL, offerWater, NULL);
    pthread_create(&threads[4], NULL, welcome, NULL);

    /* Esperando todas terminarem para encerrar o programa */
    for (i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Encerrando variáveis mutex e de condição */
    pthread_mutex_destroy(&x_mutex);
    pthread_cond_destroy(&welcomed);
    pthread_cond_destroy(&taken_care);

    return 0;
}
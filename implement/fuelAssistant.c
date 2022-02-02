#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "timer.h"

/* Uma maneira de escrever o path sem depender do SO. */
const char kPathSeparator =
#ifdef _WIN32
                            '\\';
#else
                            '/';
#endif

#define INPUT_SIZE 100 // tamanho máximo do nome dos arquivos
#define FUEL_VECTOR_SIZE 200 // tamanho do vetor de carros aleatório
#define MAX_FUEL_EFF 22.0 // eficiência máxima de combustível
#define RESOLUTION 0.000001 // precisão do programa

typedef struct _POLYNOMIAL { // descreve um polinômio e o fim de seu respectivo intervalo na função

    int degree;
    double * coefficients;
    double intervalEnding; // x no qual o polinômio acaba


} _polynomial;

typedef struct _FUNC { // descreve uma função através de polinômios e intervalos aos quais se aplicam

    _polynomial * polyList;
    int intervalCount;

} _func;


/* Global Variables */
int carAmount, nthreads, blocked = 0; // variáveis de contagem e o número de threads
_func road; // função que descreve a velocidade de um carro em uma via durante o tempo
double total_sum, *fuelEfficiencyList, *local_sum, *fuelSpentByCar;
pthread_mutex_t mutex_var; // variável para mutex
pthread_cond_t barrier_cond; // variável de condição



void printFuelVector(double * fuelData) { // função que printa o vetor de carros ou o resultado
    for (int i = 0; i < carAmount; i++) {
        printf("%.3lf\n", fuelData[i]);
    }
}

void printPoly(_func road, int i) { // função de debug para printar polinômios

    printf("Func %d, degree %d: ", i, road.polyList[i].degree);

    for (int j = 0; j < road.polyList[i].degree + 1; j++) {
        printf("%.3lf ", road.polyList[i].coefficients[j]);
    }
    printf("ends at %.3lf.\n", road.polyList[i].intervalEnding);

}

_func readInput(char * filename) { // função que lê o arquivo dado pelo usuário

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("--ERROR!-- readInput->fopen\n");
        printf("(File not found, please check the spelling and retry.)\n");
        exit(2);
    }

    _func road;

    int i = 0;

    fscanf(file, "%d", &road.intervalCount); // o primeiro valor é o número de polinômios

    road.polyList = malloc(road.intervalCount * sizeof(_polynomial));


    while(i < road.intervalCount) {

        fscanf(file, "%d", &road.polyList[i].degree); // lemos o grau do polinômio

        road.polyList[i].coefficients = malloc((road.polyList[i].degree + 1) * sizeof(double));

        for (int j = 0; j < road.polyList[i].degree + 1; j++) {
            fscanf(file, "%lf", &road.polyList[i].coefficients[j]); // baseado no grau, pegamos quem é coeficiente
        }

        fscanf(file, "%lf", &road.polyList[i].intervalEnding); // quem sobrou é o final do intervalo

        //printPoly(road, i);
        
        i++;

    }


    fclose(file);

    return road;

}

char * selectInput() { // pergunta para o usuário qual arquivo ler

    char roadName[INPUT_SIZE], *input = malloc(INPUT_SIZE*sizeof(char));
    char sepString[2]; // variável para transformar o caractere separador em string

    if (input == NULL) {
        printf("--ERROR!-- selectInput->malloc\n");
        exit(1);
    }

    sepString[0] = kPathSeparator;
    sepString[1] = '\0';

    printf("Insert name of your road file (without extension): ");
    fgets(roadName, INPUT_SIZE, stdin);
    roadName[strcspn(roadName, "\n")] = 0; // tira newline do nome dado

    input = strcat(input, "roads");
    input = strcat(input, sepString);
    input = strcat(input, roadName);
    input = strcat(input, ".txt"); // constrói o path para o arquivo dado


    return input;
}

double * getFileFuelData() { // pega os rendimentos de um arquivo
    
    char fileName[INPUT_SIZE], *input = malloc(INPUT_SIZE*sizeof(char));
    char sepString[2]; // transforma caractere em string

    if (input == NULL) {
        printf("--ERROR!-- getFileFuelData->malloc\n");
        exit(1);
    }
    
    sepString[0] = kPathSeparator;
    sepString[1] = '\0';
    
    printf("Insert name of your fuel data file (without extension): ");
    getchar();
    fgets(fileName, INPUT_SIZE, stdin);
    fileName[strcspn(fileName, "\n")] = 0; // tira o newline
    
    input = strcat(input, "fuelData");
    input = strcat(input, sepString);
    input = strcat(input, fileName);
    input = strcat(input, ".txt"); // constrói o path

    FILE *file = fopen(input, "r");

    if (file == NULL) {
        printf("--ERROR!-- getFileFuelData->fopen\n");
        printf("(File not found, please check the spelling and retry.)\n");
        exit(2);
    }

    double value;
    int vectorSize = 1, i = 0;
    double * fuelData = malloc(sizeof(double));

    fscanf(file, "%lf", &value); // lê o primeiro valor do vetor
    do {  
        
        if (i + 1 > vectorSize) { // altera conforme necessário o tamanho do vetor
            vectorSize *= 2;
            fuelData = realloc(fuelData, vectorSize * sizeof(double));
        }

        fuelData[i] = value;
        i++;

        fscanf(file, "%lf", &value); // lê cada valor do vetor 
    } while(!feof (file));

    fscanf(file, "%lf", &value); // lê o último valor do vetor
    fuelData[i] = value;

    carAmount = i + 1; // corrige o número de carros total, que estava menor em 1

    fclose(file);

    fuelSpentByCar = malloc(vectorSize * sizeof(double));

    return fuelData;
}

double * getRandomFuelData() { // cria um vetor aleatório de rendimentos

    double * fuelData = malloc(FUEL_VECTOR_SIZE * sizeof(double));

    if (fuelData == NULL) {
        printf("--ERROR!-- getRandomFuelData->malloc\n");
        exit(1);
    }

    srand(time(NULL)); // seed para o aleatorizador

    for (int i = 0; i < FUEL_VECTOR_SIZE; i++) {
        fuelData[i] = ((double) rand() / (double) (RAND_MAX)) * MAX_FUEL_EFF; // transforma o int retornado por rand() em um double no intervalo desejado
        fuelData[i]++; // 1 é o mínimo de rendimento (evita divisões por 0 no futuro)
    }

    fuelSpentByCar = malloc(FUEL_VECTOR_SIZE * sizeof(double));
    carAmount = FUEL_VECTOR_SIZE;

    return fuelData;

}

double * queryFuelData() { // pergunta para o usuário se quer um vetor aleatório ou pré-definido

    char answer;
    double *fuelData;
    int keepAsking = 1;

    while(keepAsking) {

        printf("Use random values for fuel data (y/n)? ");
        answer = getchar();

        switch(answer) {
            case 'Y':
            case 'y':
                printf("Creating %d element vector...\n", FUEL_VECTOR_SIZE);
                fuelData = getRandomFuelData();
                printFuelVector(fuelData);
                keepAsking = 0;
                break;
            case 'N':
            case 'n':
                fuelData = getFileFuelData();
                // printFuelVector(fuelData);
                keepAsking = 0;
                break;
            default:
                printf("Unknown answer! ");
                getchar();
                break;
        }
    }

    return fuelData;
}

double calculateY(_polynomial poly, double x) { // calcula o y de um polinômio dado um x

    double y = 0;

    for (int i = 0; i <= poly.degree; i++) { // passa por cada coeficiente
        y += pow(x, i) * poly.coefficients[poly.degree-i];
    }

    return y;

}

int isDifferentiable(_func function) { // checa se a função é diferenciável, olhando pras "fronteiras" entre polinômios

    for (int i = 0; i < function.intervalCount-1; i++) {

        double firstY = calculateY(function.polyList[i], function.polyList[i].intervalEnding);
        double secondY = calculateY(function.polyList[i+1], function.polyList[i].intervalEnding);
        
        if (firstY - secondY > RESOLUTION) { // como se trata de doubles, checamos a igualdade baseado na precisão
                printf("In transition between poly %d and %d: ", i, i+1);
                printf("%lf is different from %lf!\n", firstY, secondY);
                return 0;
            }
        
    }

    return 1;

}

double polyIntegrate(_polynomial poly, double intervalStart, int id) { // calcula área embaixo de um polinômio (apenas em seu intervalo)

    double sum = 0;
    double trapeziumStartX = intervalStart + RESOLUTION * id; // x que define o início de um trapézio

    while (trapeziumStartX < poly.intervalEnding) { // enquanto o trapézio estiver dentro do intervalo
        double trapeziumEndX = trapeziumStartX + RESOLUTION; // x que define o fim de um trapézio
        
        if (trapeziumEndX > poly.intervalEnding) trapeziumEndX = poly.intervalEnding; 
        // se o x do final estiver fora do intervalo, pega o próprio fim como x

        double trapeziumStartY = calculateY(poly, trapeziumStartX); // y do início do trapézio
        double trapeziumEndY = calculateY(poly, trapeziumEndX); // y do fim do trapézio
        double trapeziumArea = trapeziumStartY * RESOLUTION;
        trapeziumArea += (trapeziumEndY - trapeziumStartY) * RESOLUTION / 2; // área do trapézio
        /* OBS: se o segundo y for menor que o primeiro (função decrescente), ainda funciona */

        sum += trapeziumArea; // soma a área do trapézio ao total
        trapeziumStartX += nthreads * RESOLUTION; // escolhe o próximo trapézio de forma intercalada com outras threads

    }

    return sum;

}

double partialIntegrate(int id) { // integra 1/nthreads da função, polinômio por polinômio

    double partial_sum = 0; // essa soma será 1/nthreads da área debaixo da função no final

    for (int i = 0; i < road.intervalCount; i++) { // passamos por todos os polinômios e calculamos suas áreas parciais

        double intervalStart;

        intervalStart = (i == 0) ? 0 : road.polyList[i-1].intervalEnding; // assumimos que começa do 0, ou do início do polinômio anterior

        partial_sum += polyIntegrate(road.polyList[i], intervalStart, id); // soma a área parcial deste polinômio

    }

    return partial_sum;

}

void barrier(int tid) { // barreira que evita que threads mexam no vetor antes da hora
    pthread_mutex_lock(&mutex_var); // tudo é feito com mutex, já que mexemos em variáveis globais

    if (blocked == (nthreads - 1)) { // se a thread é a última a "chegar", libera as outras e reinicia contador
        blocked = 0;
        pthread_cond_broadcast(&barrier_cond);
        printf("Threads liberated!\n");
    } else { // do contrário, incrementa o contador de bloqueadas e espera o sinal de liberação
        blocked++;
        printf("Thread %d waiting for others. Blocked: %d\n", tid, blocked);
        pthread_cond_wait(&barrier_cond, &mutex_var);
    }

    pthread_mutex_unlock(&mutex_var);
}

double integrate() { // integra a função inteira somando suas integrais parciais

    double total_sum = 0;

    for (int i = 0; i < nthreads; i++) {
        total_sum += local_sum[i];
    }

    return total_sum;

}

void calcFuelSpent(int id) { // divide o deslocamento total (integral) por cada rendimento e guarda em um novo vetor

    for (int i = 0; i < carAmount; i++) {
        fuelSpentByCar[i] = total_sum / (fuelEfficiencyList[i] * 1000);
    }


}

void * threadMain (void * arg) { // principal fluxo feito por cada thread
    int local_tid = *(int *) arg; // id local da thread (um número de 0 a nthreads)

    local_sum[local_tid] = partialIntegrate(local_tid); // integra sua parte da função
    barrier(local_tid); // espera o resto terminar
    total_sum = integrate(); // sem necessidade de mutex pois todos os valores são iguais

    calcFuelSpent(local_tid); // calcula sua parte do vetor de combustível gasto


    pthread_exit(NULL);

}

void createOutputFile() { // cria o arquivo de output com os gastos de combustível

    char fileName[INPUT_SIZE], *input = malloc(INPUT_SIZE*sizeof(char));
    char sepString[2]; // transforma caractere em string

    if (input == NULL) {
        printf("--ERROR!-- createOutputFile->malloc\n");
        exit(1);
    }
    
    sepString[0] = kPathSeparator;
    sepString[1] = '\0';

    printf("Insert name of your output file (without extension): ");
    getchar();
    fgets(fileName, INPUT_SIZE, stdin);
    fileName[strcspn(fileName, "\n")] = 0; // limpa o newline
    
    input = strcat(input, "output");
    input = strcat(input, sepString);
    input = strcat(input, fileName);
    input = strcat(input, ".txt"); // monta o path para o arquivo

    FILE *file = fopen(input, "w"); // cria o arquivo ou abre-o, se ele já existe

    if (file == NULL) {
        printf("--ERROR!-- createOutputFile->fopen");
        exit(5);
    }

    for (int i = 0; i < carAmount; i++) {
        fprintf(file, "Car %d - %.3lfL\n", i, fuelSpentByCar[i]);
    }


}


void askToPrintResult() { // pergunta ao usuário se quer que printe o resultado na linha de comando também

    char answer;
    int keepAsking = 1;

    while(keepAsking) {

        printf("Print result (y/n)? ");
        getchar();
        answer = getchar();

        switch(answer) {
            case 'Y':
            case 'y':
                printf("Fuel consumption by car:\n");
                printFuelVector(fuelSpentByCar);
            case 'N':
            case 'n':
                createOutputFile();
                keepAsking = 0;
                break;
            default:
                printf("Unknown answer! ");
                getchar();
                break;
        }
    }


}


int main() {

    char *inputRoad = selectInput(); // pega o caminho para o arquivo

    road = readInput(inputRoad); // cria a função que representa a estrada
    if(!isDifferentiable(road)) {printf("Function is non-differentiable, check the input!\n"); exit(2);}
    // checa se a função é diferenciável
    fuelEfficiencyList = queryFuelData(); // pergunta ao usuário e cria um vetor de rendimentos

  
    printf("Use how many threads? "); // pergunta o número de threads a serem utilizadas
    scanf("%d", &nthreads);
    
    if (nthreads == 0) { // se for 0 (ou algum caracter inesperado que vira 0), assume 1
        printf("Invalid number of threads! Using 1.\n");
        nthreads = 1;
    }
    
    double finish, start; // para cronometrar o cálculo

    GET_TIME(start); // começa o cronômetro

    local_sum = malloc(sizeof(double) * nthreads); // somas locais de cada thread
    int *local_tid = malloc(sizeof(int) * nthreads); // id local de cada thread (0 a nthreads-1)
    pthread_t *system_tid = malloc(sizeof(pthread_t) * nthreads); // id do sistema operacional pra threads

    pthread_mutex_init(&mutex_var, NULL); // inicializa variáveis mutex e condicionais
    pthread_cond_init(&barrier_cond, NULL);

    for (int i = 0; i < nthreads; i++) { // cria as threads
        local_tid[i] = i;
        if (pthread_create(&system_tid[i], NULL, threadMain, (void *) &local_tid[i])) {
            printf("--ERROR!-- pthread_create\n");
            return 3;
        }
    }

    for (int i = 0; i < nthreads; i++) { // espera todas as threads acabarem
        if (pthread_join(system_tid[i], NULL)) {
            printf("--ERROR!-- pthread_join\n");
            return 4;
        }
    }

    GET_TIME(finish); // termina o cronômetro

    printf("Road extension is %.3lfkm.\n", total_sum/1000); // dá o resultado da integral para o usuário
    // printFuelVector(fuelSpentByCar);

    askToPrintResult(); // printa o resultado se o usuário quiser

    printf("Integrating and dividing by vector took %e seconds.\n", finish-start); // dá o tempo gasto pelo cálculo

    pthread_mutex_destroy(&mutex_var); // libera as variávei mutex e condicionais
    pthread_cond_destroy(&barrier_cond);



    return 0;
}

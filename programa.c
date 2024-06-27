#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Estrutura para dados passados para threads de operações matriciais
typedef struct
{
    int *A, *B, *C, *D, *E;
    int start, end, size;
} ThreadData;

// Estrutura para dados passados para threads de leitura e escrita de arquivos
typedef struct
{
    const char *filename;
    int *matrix;
    int size;
} FileReadData, FileWriteData;

// Mutex para sincronização de redução
pthread_mutex_t reduction_mutex = PTHREAD_MUTEX_INITIALIZER;
long long reduction_result = 0;

// Função para ler matriz de um arquivo
void read_matrix(const char *filename, int *matrix, int size)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Erro ao abrir arquivo de leitura");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            fscanf(file, "%d", &matrix[i * size + j]);
        }
    }
    fclose(file);
}

// Função para escrever matriz em um arquivo
void write_matrix(const char *filename, int *matrix, int size)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Erro ao abrir arquivo de escrita");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            fprintf(file, "%d ", matrix[i * size + j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Função executada por thread para ler matriz de um arquivo
void *read_matrix_thread(void *arg)
{
    FileReadData *data = (FileReadData *)arg;
    read_matrix(data->filename, data->matrix, data->size);
    return NULL;
}
// Função executada por thread para escrever matriz em um arquivo
void *write_matrix_thread(void *arg)
{
    FileWriteData *data = (FileWriteData *)arg;
    write_matrix(data->filename, data->matrix, data->size);
    return NULL;
}

// Função executada por thread para somar matrizes A e B para obter D
void *sum_matrices(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->start; i < data->end; i++)
    {
        for (int j = 0; j < data->size; j++)
        {
            data->D[i * data->size + j] = data->A[i * data->size + j] + data->B[i * data->size + j];
        }
    }
    return NULL;
}

// Função executada por thread para multiplicar matrizes D e C para obter E
void *multiply_matrices(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->start; i < data->end; i++)
    {
        for (int j = 0; j < data->size; j++)
        {
            data->E[i * data->size + j] = 0;
            for (int k = 0; k < data->size; k++)
            {
                data->E[i * data->size + j] += data->D[i * data->size + k] * data->C[k * data->size + j];
            }
        }
    }
    return NULL;
}

// Função executada por thread para reduzir matriz E para um único valor
void *reduce_matrix(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    long long local_sum = 0;
    for (int i = data->start; i < data->end; i++)
    {
        for (int j = 0; j < data->size; j++)
        {
            local_sum += data->E[i * data->size + j];
        }
    }
    pthread_mutex_lock(&reduction_mutex);
    reduction_result += local_sum;
    pthread_mutex_unlock(&reduction_mutex);
    return NULL;
}

// Função para alocar memória para uma matriz de tamanho size x size
int *allocate_matrix(int size)
{
    return (int *)malloc(size * size * sizeof(int));
}

// Função para calcular a diferença de tempo entre dois timespec
double get_time_diff(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[])
{
    // Verifica se o número de argumentos é correto
    if (argc != 8)
    {
        fprintf(stderr, "Uso: %s T n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Obtém os argumentos da linha de comando
    int T = atoi(argv[1]);      // Número de threads
    int n = atoi(argv[2]);      // Tamanho das matrizes
    const char *arqA = argv[3]; // Nome do arquivo de entrada A
    const char *arqB = argv[4]; // Nome do arquivo de entrada B
    const char *arqC = argv[5]; // Nome do arquivo de entrada C
    const char *arqD = argv[6]; // Nome do arquivo de saída D
    const char *arqE = argv[7]; // Nome do arquivo de saída E

    pthread_t threads[T];      // Vetor de threads
    ThreadData thread_data[T]; // Vetor de estruturas ThreadData

    // Alocar memória para as matrizes A, B, C, D, E
    int *A = allocate_matrix(n);
    int *B = allocate_matrix(n);
    int *C = allocate_matrix(n);
    int *D = allocate_matrix(n);
    int *E = allocate_matrix(n);

    // Struct para lidar com o cálculo dos tempos
    struct timespec start_time, end_time;

    // Leitura das matrizes A e B em threads simultaneamente
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    FileReadData file_read_data_A = {arqA, A, n};
    FileReadData file_read_data_B = {arqB, B, n};
    pthread_t read_thread_A, read_thread_B;
    pthread_create(&read_thread_A, NULL, read_matrix_thread, &file_read_data_A);
    pthread_create(&read_thread_B, NULL, read_matrix_thread, &file_read_data_B);
    pthread_join(read_thread_A, NULL);
    pthread_join(read_thread_B, NULL);

    // Soma das matrizes A e B para obter D
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    int rows_per_thread = n / T;
    for (int i = 0; i < T; i++)
    {
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].D = D;
        thread_data[i].start = i * rows_per_thread;
        thread_data[i].end = (i == T - 1) ? n : (i + 1) * rows_per_thread;
        thread_data[i].size = n;
        pthread_create(&threads[i], NULL, sum_matrices, (void *)&thread_data[i]);
    }
    for (int i = 0; i < T; i++)
    {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double sum_time = get_time_diff(start_time, end_time);

    // Gravação da matriz D e leitura da matriz C simultaneamente
    pthread_t write_thread_D, read_thread_C;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    FileWriteData file_write_data_D = {arqD, D, n};
    FileReadData file_read_data_C = {arqC, C, n};
    pthread_create(&write_thread_D, NULL, write_matrix_thread, &file_write_data_D);
    pthread_create(&read_thread_C, NULL, read_matrix_thread, &file_read_data_C);
    pthread_join(write_thread_D, NULL);
    pthread_join(read_thread_C, NULL);

    // Multiplicação das matrizes D e C para obter E
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i = 0; i < T; i++)
    {
        thread_data[i].C = C;
        thread_data[i].E = E;
        pthread_create(&threads[i], NULL, multiply_matrices, (void *)&thread_data[i]);
    }
    for (int i = 0; i < T; i++)
    {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double mult_time = get_time_diff(start_time, end_time);

    // Gravação da matriz E e redução simultaneamente
    pthread_t write_thread_E;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    FileWriteData file_write_data_E = {arqE, E, n};
    pthread_create(&write_thread_E, NULL, write_matrix_thread, &file_write_data_E);
    for (int i = 0; i < T; i++)
    {
        pthread_create(&threads[i], NULL, reduce_matrix, (void *)&thread_data[i]);
    }
    pthread_join(write_thread_E, NULL);
    for (int i = 0; i < T; i++)
    {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Calcular tempo de redução da matriz E para um único valor
    double reduction_time = get_time_diff(start_time, end_time);

    // Imprimir resultado da redução e os tempos
    double total_time = sum_time + mult_time + reduction_time;
    printf("Resultado da redução: %lld\n", reduction_result);
    printf("Tempo para somar matrizes: %.6f segundos\n", sum_time);
    printf("Tempo para multiplicar matrizes: %.6f segundos\n", mult_time);
    printf("Tempo para reduzir matriz: %.6f segundos\n", reduction_time);
    printf("Tempo total: %.3f segundos.\n", total_time);

    // Liberar memória alocada
    free(A);
    free(B);
    free(C);
    free(D);
    free(E);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include "avltree.h"
#include "hash.h"
#include "casanova.h"
#include "crypto.h"

#define NUM_THREADS_DEFAULT 8
#define NUM_THREADS_MAX     16

int num_threads;

hash_table_t *hash;

int get_socket;
int put_socket;

FILE* outputfile;
int get_ahead[NUM_THREADS_MAX];
char smallest_put_clock[ID_SIZE];
char get_clock[NUM_THREADS_MAX][ID_SIZE];
char put_clock[NUM_THREADS_MAX][ID_SIZE];

sem_t mutex_clocks;
sem_t mutex_smallest_put_clock;
sem_t sem_put_outrun[NUM_THREADS_MAX];
sem_t mutex_reviver;
sem_t mutex_output;

unsigned char *key = "01234567890123456789012345678901";


int connect_socket (char *socket_path);

#define GET_MESSAGES_NUM 200
void retrieve (void *arg)
{
    int output;
    int thread_id = * ((int *) arg);
    EVP_CIPHER_CTX *context_e = EVP_CIPHER_CTX_new(); //gerando novo contexto para a biblioteca de criptografia
    EVP_CIPHER_CTX *context_d = EVP_CIPHER_CTX_new(); //gerando novo contexto para a biblioteca de criptografia
    unsigned char decrypted_phone[10];
    unsigned char crypted_name[16];
    node_t *result;
    int count=0, read_ret=0, read_total=0;
    char *get_buffer = malloc (GET_MESSAGE_SIZE * GET_MESSAGES_NUM);


    do {
        read_total=0;

        do {
            read_ret = read(get_socket, get_buffer, GET_MESSAGE_SIZE * GET_MESSAGES_NUM-read_total);
            read_total += read_ret;
        } while (read_total < GET_MESSAGE_SIZE*GET_MESSAGES_NUM && read_ret > 0);

        if (read_ret <= 0)
        {
            free(get_buffer);
            printf("GET %d RETURNED\n", thread_id);
            return;
        }

        for (int i = 0; i < read_total/GET_MESSAGE_SIZE; i++)
        {
            memcpy (get_clock[thread_id], &get_buffer[GET_MESSAGE_SIZE * (read_total/GET_MESSAGES_NUM)], ID_SIZE);
            sem_wait(&mutex_smallest_put_clock); // protegendo os valores dos atuais

            //se o get for maior, vai precisar esperar o put andar
            if (strncmp (get_clock[thread_id], smallest_put_clock, ID_SIZE) >= 0)
            {
                sem_post(&mutex_smallest_put_clock);
                get_ahead[thread_id] = 1;
                sem_wait(&sem_put_outrun[thread_id]);
            }
            else
                sem_post(&mutex_smallest_put_clock);

            crypto_Encrypt (context_e, &get_buffer[ID_SIZE + GET_MESSAGE_SIZE * i], NOME_SIZE, key, crypted_name);

            if (result = hash_search (hash, crypted_name))
            {
                crypto_Decrypt(context_d, result->phone_number, 16, key, decrypted_phone);
                decrypted_phone[9] = '\0';

                output = atoi((unsigned char*) decrypted_phone);
                sem_wait (&mutex_output);
                fwrite (&output, sizeof(int), 1, outputfile);
                sem_post (&mutex_output);
            }
            else
            {
                get_ahead[thread_id] = 1;
                sem_wait(&sem_put_outrun[thread_id]);
                i--;
            }
            
        }

    } while (read_ret > 0 );


    free(get_buffer);

    printf("GET %d EXITED\n", thread_id);
    return;
}

#define PUT_MESSAGES_NUM 100
void store (void *arg)
{
    int thread_id = * ((int *) arg);
    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new(); //gera novo contexto para a biblioteca de criptografia
    unsigned char *original_name;
    unsigned char *original_phone;
    unsigned char crypted_name[16];
    unsigned char crypted_phone[16];
    int count = 0,read_ret = 0, read_total = 0;
    char *put_buffer = malloc (PUT_MESSAGE_SIZE * PUT_MESSAGES_NUM * sizeof(char));
    

    do {
        read_total = 0;

        do {
            read_ret = read(put_socket, put_buffer, PUT_MESSAGE_SIZE*PUT_MESSAGES_NUM-read_total);
            read_total += read_ret;
        } while (read_total < PUT_MESSAGE_SIZE * PUT_MESSAGES_NUM && read_ret > 0 );

        if (read_ret <= 0) //end of file or error
        {
            printf("PUT %d RETURNED\n", thread_id);
            return;
        }

        /* PROCESS GETS */
        for (int i = 0; i < read_total/PUT_MESSAGE_SIZE; ++i)
        {
            original_name  = &put_buffer[9  + PUT_MESSAGE_SIZE * i];
            original_phone = &put_buffer[24 + PUT_MESSAGE_SIZE * i];
            crypto_Encrypt (context, original_phone, 9, key, crypted_phone);
            crypto_Encrypt (context, original_name, 15, key, crypted_name);
            hash_insert (hash, crypted_name, crypted_phone);
        }


        if (sem_trywait (&mutex_reviver) == LOCK_ACQUIRED)
        {
            //atualiza o smallest_put_clock
            sem_wait (&mutex_smallest_put_clock);
            memcpy (put_clock[thread_id], put_buffer, ID_SIZE); 
            for (int i = 0; i < num_threads; ++i)
            {
                if (strncmp (put_clock[thread_id], put_clock[i], ID_SIZE) > 0) //if my_put_clock > any other put_clock
                    break;
                else
                    memcpy (smallest_put_clock, put_clock[thread_id], ID_SIZE);
            }        
            sem_post (&mutex_smallest_put_clock);


            //revive gets bloqueadas
            for (int i = 0; i < num_threads; ++i)
            {
                if (get_ahead[i])
                {
                    sem_wait (&mutex_smallest_put_clock);
                    if (strncmp (smallest_put_clock, get_clock[i], ID_SIZE) > 0) //if smallest_put_clock > get_clock
                    {
                        sem_post (&mutex_smallest_put_clock);
                        get_ahead[i] = 0;
                        sem_post (&sem_put_outrun[i]);
                    }
                    sem_post (&mutex_smallest_put_clock);
                }
            }
            sem_post(&mutex_reviver);
        }

    } while (read_ret > 0);

    memcpy (smallest_put_clock, "FFFFFFFFF", ID_SIZE);
    for (int i = 0; i < num_threads; ++i)
    {
        get_ahead[i] = 0;
        sem_post (&sem_put_outrun[i]);
        sem_post (&sem_put_outrun[i]);
    }

    printf("PUT %d RETURNED_end\n", thread_id); fflush(stdout);
    return;
}


int main(int argc, char **argv)
{
    int cores, n;
    int thread_id[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    crypto_Init();
    crypto_Thread_setup();
    hash_init(&hash);

    get_socket = connect_socket(SOCK_GET_PATH);
    put_socket = connect_socket(SOCK_PUT_PATH);

    if (argc > 1)
    {
        num_threads = atoi(argv[1]);
        if (num_threads < 1)
        {
            printf("numero de threads invalido, running default");
            num_threads = NUM_THREADS_DEFAULT;
        }
    }
    else
    {
        FILE* nprocfp = popen ("nproc", "r");

        if (nprocfp != NULL)
        {
            n=fscanf(nprocfp, "%d", &cores);
            num_threads = (cores/2) + 1;
            fclose(nprocfp);
        }
        else
            num_threads = NUM_THREADS_DEFAULT;
    }

    printf("num_cores: %d\n", cores);
    printf("num_threads: %d\n", num_threads);

	pthread_t get_thread[num_threads], put_thread[num_threads];
    strncpy (smallest_put_clock, "000000000", ID_SIZE);
    sem_init (&mutex_clocks,             0, 1);
    sem_init (&mutex_smallest_put_clock, 0, 1);
    sem_init (&mutex_reviver,            0, 1);
    sem_init (&mutex_output,             0, 1);

    outputfile=fopen(OUTPUTFILENAME, "w+");
    if (outputfile==NULL)
    {
        perror("OPEN:");
        exit(1);
    }

    for (int i = 0; i < num_threads; ++i)
    {
        sem_init (&sem_put_outrun[i], 0, 0);
        get_ahead[i] = 0;
        strncpy (get_clock[i], "000000000", ID_SIZE);
        strncpy (put_clock[i], "000000000", ID_SIZE);
        pthread_create (&put_thread[i], NULL, (void *) &store,    &thread_id[i]);
        pthread_create (&get_thread[i], NULL, (void *) &retrieve, &thread_id[i]);
    }


    for (int i = 0; i < num_threads; ++i)
        pthread_join(put_thread[i], NULL);

    
    memcpy (smallest_put_clock, "FFFFFFFFF", ID_SIZE);
    for (int i = 0; i < num_threads; ++i)
    {
        get_ahead[i] = 0;
        sem_post (&sem_put_outrun[i]);
        sem_post (&sem_put_outrun[i]);
    }

    for (int i = 0; i < num_threads; ++i)
        pthread_join(get_thread[i], NULL);

    printf("destroying hash\n");
    hash_destroy (hash);
    fclose(outputfile);

    crypto_Thread_cleanup();
    crypto_Cleanup();

	return 0;
}

int connect_socket (char *socket_path)
{
    int server_sockfd, client_sockfd;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    socklen_t addr_size;

    /* inicializa SOCK_STREAM */
    unlink(socket_path);
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, socket_path);
    bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));

    /* aguarda conexão */
    listen(server_sockfd, 5);

    fprintf(stderr, "%s WAITTING\n",  socket_path);
    addr_size = sizeof(client_address);
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &addr_size);
    fprintf(stderr, "%s CONNECTED\n", socket_path);

    return client_sockfd;
}
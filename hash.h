#ifndef __HASH__
#define __HASH__

#include <pthread.h>
#include <semaphore.h>

#define HT_SIZE 1000000

typedef struct hash_table_t
{
    node_t* entry[HT_SIZE];
} hash_table_t;

sem_t *mutex;

int hash_function (unsigned char *str);
void hash_init (hash_table_t **hash);
void hash_destroy (hash_table_t *hash);
void hash_insert (hash_table_t *hash, char *name, char *phone_number);
node_t* hash_search (hash_table_t *hash, char *name);
void imprime_hash (hash_table_t *hash, FILE* file);
unsigned long int get_hash_max_depth (hash_table_t *hash);
unsigned long int get_hash_max_size (hash_table_t *hash);
unsigned long int get_hash_min_size (hash_table_t *hash);


#endif
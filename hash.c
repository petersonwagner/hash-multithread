#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "avltree.h"

#define HT_SIZE 1000000

sem_t *mutex;

typedef struct hash_table_t
{
    node_t* entry[HT_SIZE];
} hash_table_t;


/*hash_function que retorna um valor para ser usado como indice na tabela hash,
baseado no algoritmo FNV */
int hash_function (unsigned char *str)
{

    unsigned long hash = 2166136261;
    int i = 0;

    for (int i = 0; i < 16; ++i)
    {
        hash ^= str[i];
        hash *= 16777619;
    }

    return hash % HT_SIZE;
}

/*O procedimento hash_init recebe como parametro uma variavel do tipo
hash_table_t e inicializa todos os valores da tabela*/
void hash_init (hash_table_t **hash)
{
    *hash = malloc (sizeof(hash_table_t));
    memset (*hash, 0, sizeof(hash_table_t));

    mutex      = malloc (sizeof(sem_t) * HT_SIZE);

    for (int i = 0; i < HT_SIZE; ++i)
        sem_init (&mutex[i], 0, 1);

}

/*O procedimento hash_insert insere os elementos
na tabela e resolve colisões com encadeamento direto */
void hash_insert (hash_table_t *hash, char *name, char *phone_number)
{
    int key = hash_function(name);
    node_t *node_to_insert = new_node (name, phone_number);

    sem_wait (&mutex[key]);

    hash->entry[key] = tree_insert (hash->entry[key], node_to_insert);

    sem_post (&mutex[key]);
}

node_t* hash_search (hash_table_t *hash, char *name)
{
    node_t *result;
    int hash_key = hash_function (name);
    unsigned long int avl_key = hash_function2 (name);

    sem_wait (&mutex[hash_key]);
    result = tree_search (hash->entry[hash_key], name, avl_key);
    sem_post (&mutex[hash_key]);

    return result;
}

void hash_destroy (hash_table_t *hash)
{
    int ht_index = 0;
    for (ht_index = 0; ht_index < HT_SIZE; ++ht_index)
        delete_tree (hash->entry[ht_index]);

    free(hash);
}
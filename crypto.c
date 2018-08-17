#include "crypto.h"

#include <pthread.h>
#include <semaphore.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/engine.h>

static pthread_mutex_t *thread_lock;


void handle_errors(void)
{
  ERR_print_errors_fp(stderr);
}

static void lock_thread(int mode, int lock, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    pthread_mutex_lock(&thread_lock[lock]);
  else
    pthread_mutex_unlock(&thread_lock[lock]);
}

static unsigned long id_thread(void)
{
	return (unsigned long)pthread_self();
}

void crypto_Init(void)
{
  ERR_load_crypto_strings();
  OpenSSL_add_all_ciphers();
  OPENSSL_config(NULL);
}

void crypto_Cleanup(void)
{
  FIPS_mode_set(0);
  ENGINE_cleanup();
  CONF_modules_unload(1);
  ERR_remove_state(0);
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
  ERR_free_strings();
}

void crypto_Thread_setup(void)
{
  thread_lock = (pthread_mutex_t*)OPENSSL_malloc(CRYPTO_num_locks()
                                               * sizeof(pthread_mutex_t));
  for (int lock = 0; lock < CRYPTO_num_locks(); ++lock)
    pthread_mutex_init(&thread_lock[lock], NULL);

  CRYPTO_set_id_callback((unsigned long(*)())id_thread);
  CRYPTO_set_locking_callback((void(*)(int, int, const char*, int))lock_thread);

}

void crypto_Thread_cleanup(void)
{
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);

  for (int lock = 0; lock < CRYPTO_num_locks(); ++lock)
    pthread_mutex_destroy(&thread_lock[lock]);
  OPENSSL_free(thread_lock);
  thread_lock = NULL;
}

void crypto_Encrypt(EVP_CIPHER_CTX *context, unsigned char *source, int source_size,
                    unsigned char *key, unsigned char *destination)
{
  int destination_size;

  if (1 != EVP_EncryptInit_ex(context, EVP_aes_256_ecb(), NULL, key, NULL))
    handle_errors();

  if (1 != EVP_EncryptUpdate(context, destination, &destination_size,
                             source, source_size))
    handle_errors();

  if (1 != EVP_EncryptFinal_ex(context, destination + destination_size,
                               &destination_size)) 
    handle_errors();
}

void crypto_Decrypt (EVP_CIPHER_CTX *context, unsigned char *source, int source_size,
                      unsigned char *key, unsigned char *destination)
{
  int destination_size;

  if (1 != EVP_DecryptInit_ex(context, EVP_aes_256_ecb(), NULL, key, NULL))
    handle_errors();

  EVP_CIPHER_CTX_set_padding(context, 0);
  if (1 != EVP_DecryptUpdate(context, destination, &destination_size,
                             source, source_size))
    handle_errors();

  if (1 != EVP_DecryptFinal_ex(context, destination + destination_size,
                               &destination_size))
    handle_errors();
}

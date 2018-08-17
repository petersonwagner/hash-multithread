#ifndef __CRYPTO__
#define __CRYPTO__

#include <openssl/evp.h>

void crypto_Init(void);
void crypto_Cleanup(void);
void crypto_Thread_setup(void);
void crypto_Thread_cleanup(void);
void crypto_Encrypt(EVP_CIPHER_CTX *, unsigned char*, int, unsigned char*, unsigned char*);
void crypto_Decrypt(EVP_CIPHER_CTX *, unsigned char*, int, unsigned char*, unsigned char*);

#endif
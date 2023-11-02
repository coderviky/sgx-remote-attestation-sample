

#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <stdio.h>

// decryption
#include <openssl/aes.h>

#if defined(__cplusplus)
extern "C"
{
#endif

    // void printf(const char *fmt, ...);
    int addition(int a, int b);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */

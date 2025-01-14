#include "Enclave.h"
#include "Enclave_t.h" /* print_string */

#include "../config.h"

#include <string.h>
#include <sgx_utils.h>

#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>

// for generated public key
#include "Pubkey.h"

#define PSE_RETRIES 5 /* Arbitrary. Not too long, not too short. */

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

int addition(int a, int b)
{
    // printf("Enclave.cpp : addition(%d, %d) \n", a,b);

    return a + b;
}

//----------------------------------------------------------------------
// REMOTE ATTESTATION FUNCTIONS
//----------------------------------------------------------------------

/*----------------------------------------------------------------------
 * WARNING
 *----------------------------------------------------------------------
 *
 * End developers should not normally be calling these functions
 * directly when doing remote attestation:
 *
 *    sgx_get_ps_sec_prop()
 *    sgx_get_quote()
 *    sgx_get_quote_size()
 *    sgx_get_report()
 *    sgx_init_quote()
 *
 * These functions short-circuits the RA process in order
 * to generate an enclave quote directly!
 *
 * The high-level functions provided for remote attestation take
 * care of the low-level details of quote generation for you:
 *
 *   sgx_ra_init()
 *   sgx_ra_get_msg1
 *   sgx_ra_proc_msg2
 *
 *----------------------------------------------------------------------
 */

/*
 * This doesn't really need to be a C++ source file, but a bug in
 * 2.1.3 and earlier implementations of the SGX SDK left a stray
 * C++ symbol in libsgx_tkey_exchange.so so it won't link without
 * a C++ compiler. Just making the source C++ was the easiest way
 * to deal with that.
 */

sgx_status_t get_report(sgx_report_t *report, sgx_target_info_t *target_info)
{
#ifdef SGX_HW_SIM
    return sgx_create_report(NULL, NULL, report);
#else
    return sgx_create_report(target_info, NULL, report);
#endif
}

#ifdef _WIN32
size_t get_pse_manifest_size()
{
    return sizeof(sgx_ps_sec_prop_desc_t);
}

sgx_status_t get_pse_manifest(char *buf, size_t sz)
{
    sgx_ps_sec_prop_desc_t ps_sec_prop_desc;
    sgx_status_t status = SGX_ERROR_SERVICE_UNAVAILABLE;
    int retries = PSE_RETRIES;

    do
    {
        status = sgx_create_pse_session();
        if (status != SGX_SUCCESS)
            return status;
    } while (status == SGX_ERROR_BUSY && retries--);
    if (status != SGX_SUCCESS)
        return status;

    status = sgx_get_ps_sec_prop(&ps_sec_prop_desc);
    if (status != SGX_SUCCESS)
        return status;

    memcpy(buf, &ps_sec_prop_desc, sizeof(ps_sec_prop_desc));

    sgx_close_pse_session();

    return status;
}
#endif

sgx_status_t enclave_ra_init(sgx_ec256_public_t key, int b_pse,
                             sgx_ra_context_t *ctx, sgx_status_t *pse_status)
{
    sgx_status_t ra_status;

    /*
     * If we want platform services, we must create a PSE session
     * before calling sgx_ra_init()
     */

#ifdef _WIN32
    if (b_pse)
    {
        int retries = PSE_RETRIES;
        do
        {
            *pse_status = sgx_create_pse_session();
            if (*pse_status != SGX_SUCCESS)
                return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS)
            return SGX_ERROR_UNEXPECTED;
    }

    ra_status = sgx_ra_init(&key, b_pse, ctx);

    if (b_pse)
    {
        int retries = PSE_RETRIES;
        do
        {
            *pse_status = sgx_close_pse_session();
            if (*pse_status != SGX_SUCCESS)
                return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS)
            return SGX_ERROR_UNEXPECTED;
    }
#else
    ra_status = sgx_ra_init(&key, 0, ctx);
#endif

    return ra_status;
}

sgx_status_t enclave_ra_init_def(int b_pse, sgx_ra_context_t *ctx,
                                 sgx_status_t *pse_status)
{
    return enclave_ra_init(def_service_public_key, b_pse, ctx, pse_status);
}

/*
 * Return a SHA256 hash of the requested key. KEYS SHOULD NEVER BE
 * SENT OUTSIDE THE ENCLAVE IN PLAIN TEXT. This function let's us
 * get proof of possession of the key without exposing it to untrusted
 * memory.
 */

sgx_status_t enclave_ra_get_key_hash(sgx_status_t *get_keys_ret,
                                     sgx_ra_context_t ctx, sgx_ra_key_type_t type, sgx_sha256_hash_t *hash, unsigned char *ciphertext)
{
    sgx_status_t sha_ret;
    sgx_ra_key_128_t k;

    // First get the requested key which is one of:
    //  * SGX_RA_KEY_MK
    //  * SGX_RA_KEY_SK
    // per sgx_ra_get_keys().

    *get_keys_ret = sgx_ra_get_keys(ctx, type, &k);
    if (*get_keys_ret != SGX_SUCCESS)
        return *get_keys_ret;

    printf("Ciphertext: ");
    for (int i = 0; i < 16; i++)
    {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");

    // if sk then ciphertext to pliantext
    if (type == SGX_RA_KEY_SK)
    {
        AES_KEY aesKey;
        AES_set_decrypt_key(k, 128, &aesKey);

        printf("SK         = %s\n", k);

        unsigned char plaintext[1000];
        AES_decrypt(ciphertext, plaintext, &aesKey);

        printf("Decrypted Text: %s\n", plaintext);
        // memcpy(k, plaintext, sizeof(k));
    }

    /* Now generate a SHA hash */

    sha_ret = sgx_sha256_msg((const uint8_t *)&k, sizeof(k),
                             (sgx_sha256_hash_t *)hash); // Sigh.

    /* Let's be thorough */

    memset(k, 0, sizeof(k));

    return sha_ret;
}

sgx_status_t enclave_ra_close(sgx_ra_context_t ctx)
{
    sgx_status_t ret;
    ret = sgx_ra_close(ctx);
    return ret;
}

//------------- Remote Attestation Functions END -----------------
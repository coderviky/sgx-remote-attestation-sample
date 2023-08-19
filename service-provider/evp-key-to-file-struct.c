#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ec.h>

#define PUBKEY_H_FILE "Enclave/Pubkey.h"

#define PRVKEY_PEM_FILE "service-provider/sp_private_key.pem"

/* run command
> gcc -o sp-keygen evp-key-to-file-struct.c -lssl -lcrypto && ./sp-keygen
*/

static enum _error_type {
    e_none,
    e_crypto,
    e_system,
    e_api
} error_type = e_none;

#define SGX_ECP256_KEY_SIZE 32

typedef struct _sgx_ec256_public_t
{
    uint8_t gx[SGX_ECP256_KEY_SIZE];
    uint8_t gy[SGX_ECP256_KEY_SIZE];
} sgx_ec256_public_t;

//
void crypto_init()
{
    /* Load error strings for libcrypto */
    // ERR_load_crypto_strings();

    /* Load digest and ciphers */
    OpenSSL_add_all_algorithms();
}

void crypto_destroy()
{
    EVP_cleanup();

    CRYPTO_cleanup_all_ex_data();

    // ERR_free_strings();
}

int save_private_key_to_pem(EVP_PKEY *key, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("Error opening file");
        return 0;
    }

    int ret = PEM_write_PKCS8PrivateKey(fp, key, NULL, NULL, 0, NULL, NULL);
    fclose(fp);

    return ret;
}

int key_to_sgx_ec256(sgx_ec256_public_t *k, EVP_PKEY *key)
{
    EC_KEY *eckey = NULL;
    const EC_POINT *ecpt = NULL;
    EC_GROUP *ecgroup = NULL;
    BIGNUM *gx = NULL;
    BIGNUM *gy = NULL;

    error_type = e_none;

    eckey = EVP_PKEY_get1_EC_KEY(key);
    if (eckey == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    if (ecgroup == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    ecpt = EC_KEY_get0_public_key(eckey);

    gx = BN_new();
    if (gx == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    gy = BN_new();
    if (gy == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!EC_POINT_get_affine_coordinates_GFp(ecgroup, ecpt, gx, gy, NULL))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!BN_bn2lebinpad(gx, k->gx, sizeof(k->gx)))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!BN_bn2lebinpad(gy, k->gy, sizeof(k->gy)))
    {
        error_type = e_crypto;
        goto cleanup;
    }

cleanup:
    if (gy != NULL)
        BN_free(gy);
    if (gx != NULL)
        BN_free(gx);
    if (ecgroup != NULL)
        EC_GROUP_free(ecgroup);
    return (error_type == e_none);
}

EVP_PKEY *key_generate()
{
    EVP_PKEY *key = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY_CTX *kctx = NULL;
    EVP_PKEY *params = NULL;

    error_type = e_none;

    /* Set up the parameter context */
    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (pctx == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    /* Generate parameters for the P-256 curve */

    if (!EVP_PKEY_paramgen_init(pctx))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!EVP_PKEY_paramgen(pctx, &params))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    /* Generate the key */

    kctx = EVP_PKEY_CTX_new(params, NULL);
    if (kctx == NULL)
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!EVP_PKEY_keygen_init(kctx))
    {
        error_type = e_crypto;
        goto cleanup;
    }

    if (!EVP_PKEY_keygen(kctx, &key))
    {
        error_type = e_crypto;
        EVP_PKEY_free(key);
        key = NULL;
    }

cleanup:
    if (kctx != NULL)
        EVP_PKEY_CTX_free(kctx);
    if (params != NULL)
        EVP_PKEY_free(params);
    if (pctx != NULL)
        EVP_PKEY_CTX_free(pctx);

    return key;
}

// main
int main()
{

    EVP_PKEY *pkey = NULL;
    sgx_ec256_public_t public_key;

    crypto_init();

    // genearate key
    pkey = key_generate();

    // Save private key to PEM file
    if (!save_private_key_to_pem(pkey, PRVKEY_PEM_FILE))
    {
        printf("Error saving private key to PEM file.\n");
    }

    printf("PRIVATE KEY : service providers private key is saved to %s file successfully.\n", PRVKEY_PEM_FILE);

    // convert key to sgx_ec256_public_t
    key_to_sgx_ec256(&public_key, pkey);

    // // Print the gx and gy components of the sgx_ec256_public_t
    // printf("gx: ");
    // for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
    // {
    //     printf("0x%02x, ", public_key.gx[i]);
    // }
    // printf("\n");

    // printf("gy: ");
    // for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
    // {
    //     printf("0x%02x, ", public_key.gy[i]);
    // }
    // printf("\n");

    // Example: Print the coordinates in the C-style array format
    printf("static const sgx_ec256_public_t def_service_public_key = {\n");
    printf("    {");
    for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
    {
        printf("0x%02x", public_key.gx[i]);
        if (i != SGX_ECP256_KEY_SIZE - 1)
            printf(", ");
    }
    printf("},\n");

    printf("    {");
    for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
    {
        printf("0x%02x", public_key.gy[i]);
        if (i != SGX_ECP256_KEY_SIZE - 1)
            printf(", ");
    }
    printf("}\n};\n");

    // printf("\n\n /* copy this def_service_public_key variable and paste in Enclave.h */");

    // Write the output to Pubkey.h
    FILE *pubkey_file = fopen(PUBKEY_H_FILE, "w");
    if (pubkey_file != NULL)
    {
        fprintf(pubkey_file, "#ifndef PUBKEY_H\n");
        fprintf(pubkey_file, "#define PUBKEY_H\n\n");
        fprintf(pubkey_file, "#include <stdint.h>\n\n");
        fprintf(pubkey_file, "#include \"sgx_tkey_exchange.h\"\n");
        fprintf(pubkey_file, "#include \"sgx_tcrypto.h\"\n\n");
        fprintf(pubkey_file, "static const sgx_ec256_public_t def_service_public_key = {\n");
        fprintf(pubkey_file, "    {");
        for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
        {
            fprintf(pubkey_file, "0x%02x", public_key.gx[i]);
            if (i != SGX_ECP256_KEY_SIZE - 1)
                fprintf(pubkey_file, ", ");
        }
        fprintf(pubkey_file, "},\n");

        fprintf(pubkey_file, "    {");
        for (int i = 0; i < SGX_ECP256_KEY_SIZE; i++)
        {
            fprintf(pubkey_file, "0x%02x", public_key.gy[i]);
            if (i != SGX_ECP256_KEY_SIZE - 1)
                fprintf(pubkey_file, ", ");
        }
        fprintf(pubkey_file, "}\n\n};\n\n");
        fprintf(pubkey_file, "#endif // PUBKEY_H\n");
        fclose(pubkey_file);
        printf("PUBLIC KEY : service providers public key is added in %s file with sgx_ec256_public_t def_service_public_key successfully.\n", PUBKEY_H_FILE);

        // fclose(pubkey_file);
    }
    else
    {
        printf("Error creating %s file.\n", PUBKEY_H_FILE);
    }

    // Clean up
    EVP_PKEY_free(pkey);

    //
    crypto_destroy();

    return 0;
}
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#define MAX_PATH FILENAME_MAX

#include "App.h"
#include "Enclave_u.h"

using namespace std;

#include "../config.h"

#if !defined(SGX_HW_SIM) && !defined(_WIN32)
#include "../service-provider/sgx_stub.h"
#endif
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <sgx_urts.h>
#include <sys/stat.h>

#include <openssl/evp.h>
#include <getopt.h>
#include <unistd.h>
#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>
#include <sgx_uae_quote_ex.h>
#include <string>
#include "../service-provider/common.h"
#include "../service-provider/protocol.h"
#include "../service-provider/sgx_detect.h"
#include "../service-provider/hexutil.h"
#include "../service-provider/fileio.h"
#include "../service-provider/base64.h"
#include "../service-provider/crypto.h"
#include "../service-provider/msgio.h"
#include "../service-provider/logfile.h"
#include "../service-provider/quote_size.h"

#define MAX_LEN 80

#define _rdrand64_step(x) ({ unsigned char err; asm volatile("rdrand %0; setc %1":"=r"(*x), "=qm"(err)); err; })

#ifdef __x86_64__
#define DEF_LIB_SEARCHPATH "/lib:/lib64:/usr/lib:/usr/lib64"
#else
#define DEF_LIB_SEARCHPATH "/lib:/usr/lib"
#endif

// --------------- ABE App.cpp includes ----------------
// read file and padding glib
#include <glib.h>
#include <sys/stat.h>
#define MAX_PATH FILENAME_MAX
#include <string.h>
#include <assert.h>

typedef struct config_struct
{
    char mode;
    uint32_t flags;
    sgx_spid_t spid;
    sgx_ec256_public_t pubkey;
    sgx_quote_nonce_t nonce;
    char *server;
    char *port;
} config_t;

int file_in_searchpath(const char *file, const char *search, char *fullpath,
                       size_t len);

sgx_status_t sgx_create_enclave_search(
    const char *filename,
    const int edebug,
    sgx_launch_token_t *token,
    int *updated,
    sgx_enclave_id_t *eid,
    sgx_misc_attribute_t *attr);

void usage();
int do_quote(sgx_enclave_id_t eid, config_t *config);
int do_attestation(sgx_enclave_id_t eid, config_t *config);

char debug = 0;
char verbose = 0;

#define MODE_ATTEST 0x0
#define MODE_EPID 0x1
#define MODE_QUOTE 0x2

#define OPT_PSE 0x01
#define OPT_NONCE 0x02
#define OPT_LINK 0x04
#define OPT_PUBKEY 0x08

/* Macros to set, clear, and get the mode and options */

#define SET_OPT(x, y) x |= y
#define CLEAR_OPT(x, y) x = x & ~y
#define OPT_ISSET(x, y) x &y

#define ENCLAVE_NAME "enclave.signed.so"

// ----------------------- ABE App.cpp --------------------------
/* Global EID shared by multiple threads */
sgx_enclave_id_t eid = 0;

typedef struct _sgx_errlist_t
{
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED,
     "Unexpected error occurred.",
     NULL},
    {SGX_ERROR_INVALID_PARAMETER,
     "Invalid parameter.",
     NULL},
    {SGX_ERROR_OUT_OF_MEMORY,
     "Out of memory.",
     NULL},
    {SGX_ERROR_ENCLAVE_LOST,
     "Power transition occurred.",
     "Please refer to the sample \"PowerTransition\" for details."},
    {SGX_ERROR_INVALID_ENCLAVE,
     "Invalid enclave image.",
     NULL},
    {SGX_ERROR_INVALID_ENCLAVE_ID,
     "Invalid enclave identification.",
     NULL},
    {SGX_ERROR_INVALID_SIGNATURE,
     "Invalid enclave signature.",
     NULL},
    {SGX_ERROR_OUT_OF_EPC,
     "Out of EPC memory.",
     NULL},
    {SGX_ERROR_NO_DEVICE,
     "Invalid SGX device.",
     "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT,
     "Memory map conflicted.",
     NULL},
    {SGX_ERROR_INVALID_METADATA,
     "Invalid enclave metadata.",
     NULL},
    {SGX_ERROR_DEVICE_BUSY,
     "SGX device was busy.",
     NULL},
    {SGX_ERROR_INVALID_VERSION,
     "Enclave version was invalid.",
     NULL},
    {SGX_ERROR_INVALID_ATTRIBUTE,
     "Enclave was not authorized.",
     NULL},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS,
     "Can't open enclave file.",
     NULL},
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++)
    {
        if (ret == sgx_errlist[idx].err)
        {
            if (NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

int initialize_enclave(void)
{
    // char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;

    /* ---------------- Step 2: call sgx_create_enclave to initialize an enclave instance  ---------------*/
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_NAME, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
    if (ret != SGX_SUCCESS)
    {
        print_error_message(ret);
        // if (fp != NULL)
        //     fclose(fp);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    printf("%s", str);
}

// -------------------------- file IO START -----------------------------------
FILE *
fopen_write_or_die(const char *file)
{
    FILE *f;

    if (!(f = fopen(file, "w")))
        printf("can't write file: %s\n", file);

    return f;
}

FILE *fopen_read_or_die(const char *file)
{
    FILE *f;

    if (!(f = fopen(file, "r")))
        printf("can't read file: %s\n", file);

    return f;
}

// ---------------------------- file IO END -----------------------------------

// ########################################################
// --------------------------------------------------------
// ------ File Read & Padding Functions (GByteArray) ------
// --------------------------------------------------------
// ########################################################

// ---------------- Read File GByteArray ----------------
GByteArray *suck_file_GByteArray(char *filename)
{
    FILE *f;
    struct stat s;

    GByteArray *bdata;
    bdata = g_byte_array_new();

    stat(filename, &s);
    g_byte_array_set_size(bdata, s.st_size);

    // f = fopen_read_or_die(filename);
    if (!(f = fopen(filename, "r")))
    {
        printf("can't read filename: %s\n", filename);
        return NULL;
    }

    fread(bdata->data, 1, s.st_size, f);
    fclose(f);

    return bdata;
}

// ---------------------- Add Padding GByteArray Function----------------------
void add_padding_GByteArray(GByteArray *pt)
{
    // --------- add padding -----------
    guint8 len[4];
    guint8 zero;

    // stuff in real length (big endian) before padding
    len[0] = (pt->len & 0xff000000) >> 24;
    len[1] = (pt->len & 0xff0000) >> 16;
    len[2] = (pt->len & 0xff00) >> 8;
    len[3] = (pt->len & 0xff) >> 0;

    g_byte_array_prepend(pt, len, 4);

    // pad out to multiple of 128 bit (16 byte) blocks
    zero = 0;
    while (pt->len % 16)
        g_byte_array_append(pt, &zero, 1);

    // return pt;
}

// --- File Read & Padding Functions (GByteArray) END -----

// ########################################################
// --------------------------------------------------------
// File Write (both) & Remove Padding (unsigned_char)
// --------------------------------------------------------
// ########################################################

// ------------------ Remove Padding ------------------
GByteArray *remove_padding_unsigned_char(const unsigned char *newpt, uint32_t newpt_len)
{ // ------------- remove padding ----------------

    // creat newpt GbyteArray
    GByteArray *pt = g_byte_array_new();
    g_byte_array_set_size(pt, newpt_len);

    // copy newpt to pt
    memcpy(pt->data, newpt, newpt_len);

    unsigned int len;

    // get real length
    len = 0;
    len = len | ((pt->data[0]) << 24) | ((pt->data[1]) << 16) | ((pt->data[2]) << 8) | ((pt->data[3]) << 0);

    g_byte_array_remove_index(pt, 0);
    g_byte_array_remove_index(pt, 0);
    g_byte_array_remove_index(pt, 0);
    g_byte_array_remove_index(pt, 0);

    /* truncate any garbage from the padding */
    g_byte_array_set_size(pt, len);

    return pt;
}

// ------------------ Write File GByteArray ------------------
int spit_file_GByteArray(const char *filename, GByteArray *bdata, int free)
{
    // --------- write file -----------
    FILE *f;

    // f = fopen_write_or_die(filename);

    if (!(f = fopen(filename, "w")))
    {
        printf("can't write file: %s\n", filename);
        return 1; // error
    }

    fwrite(bdata->data, 1, bdata->len, f);
    fclose(f);

    if (free)
        g_byte_array_free(bdata, 1);

    return 0; // success
}

// ------------------ Write File unsigned char ------------------
int spit_file_unsigned_char(const char *filename, const unsigned char *ct, uint32_t ct_len)
{
    // --------- write file -----------
    FILE *f;

    // f = fopen_write_or_die(filename);
    if (!(f = fopen(filename, "w")))
    {
        printf("can't write file: %s\n", filename);
        return 1; // error
    }

    fwrite(ct, sizeof(ct[0]), ct_len, f);

    fclose(f);

    return 0;
}

// ------------ File Write & Remove Padding unsigned_char END -------

// ----------------------- ABE App.cpp END --------------------------

int attestation_with_custom_config()
{
    config_t config;
    sgx_launch_token_t token = {0};
    sgx_status_t status;
    // sgx_enclave_id_t eid = 0;
    int updated = 0;
    int sgx_support;
    uint32_t i;
    // char have_spid = 0;
    // char flag_stdio = 0;

    /* Create a logfile to capture debug output and actual msg data */
    fplog = create_logfile("client.log");
    dividerWithText(fplog, "Client Log Timestamp");

    const time_t timeT = time(NULL);
    struct tm lt, *ltp;

    ltp = localtime(&timeT);
    if (ltp == NULL)
    {
        perror("localtime");
        return 1;
    }
    lt = *ltp;

    fprintf(fplog, "%4d-%02d-%02d %02d:%02d:%02d\n",
            lt.tm_year + 1900,
            lt.tm_mon + 1,
            lt.tm_mday,
            lt.tm_hour,
            lt.tm_min,
            lt.tm_sec);
    divider(fplog);

    // create config
    memset(&config, 0, sizeof(config));
    config.mode = MODE_ATTEST;

    // Default to localhost
    config.server = strdup("localhost");
    if (config.server == NULL)
    {
        perror("malloc");
        return 1;
    }

    // set run-client commands
    // -r random nonce
    for (i = 0; i < 2; ++i)
    {
        int retry = 10;
        unsigned char ok = 0;
        uint64_t *np = (uint64_t *)&config.nonce;

        while (!ok && retry)
            ok = _rdrand64_step(&np[i]);
        if (ok == 0)
        {
            fprintf(stderr, "nonce: RDRAND underflow\n");
            exit(1);
        }
    }
    SET_OPT(config.flags, OPT_NONCE);

    // -l linkable
    SET_OPT(config.flags, OPT_LINK);

    // -d debug
    debug = 1;

    // -v verbose
    verbose = 1;

    // initialize enclave with eid
    /* Initialize the enclave */
    if (initialize_enclave() < 0)
    {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    // attestation
    do_attestation(eid, &config);

    // ------ next part ------

    // -------

    close_logfile(fplog);

    return 0;
}

int main_client(int argc, char *argv[])
{
    config_t config;
    sgx_launch_token_t token = {0};
    sgx_status_t status;
    // sgx_enclave_id_t eid = 0;
    int updated = 0;
    int sgx_support;
    uint32_t i;
    EVP_PKEY *service_public_key = NULL;
    char have_spid = 0;
    char flag_stdio = 0;

    /* Create a logfile to capture debug output and actual msg data */
    fplog = create_logfile("client.log");
    dividerWithText(fplog, "Client Log Timestamp");

    const time_t timeT = time(NULL);
    struct tm lt, *ltp;

    ltp = localtime(&timeT);
    if (ltp == NULL)
    {
        perror("localtime");
        return 1;
    }
    lt = *ltp;

    fprintf(fplog, "%4d-%02d-%02d %02d:%02d:%02d\n",
            lt.tm_year + 1900,
            lt.tm_mon + 1,
            lt.tm_mday,
            lt.tm_hour,
            lt.tm_min,
            lt.tm_sec);
    divider(fplog);

    memset(&config, 0, sizeof(config));
    config.mode = MODE_ATTEST;

    static struct option long_opt[] =
        {
            {"help", no_argument, 0, 'h'},
            {"debug", no_argument, 0, 'd'},
            {"epid-gid", no_argument, 0, 'e'},

            {"nonce", required_argument, 0, 'n'},
            {"nonce-file", required_argument, 0, 'N'},
            {"rand-nonce", no_argument, 0, 'r'},
            {"spid", required_argument, 0, 's'},
            {"spid-file", required_argument, 0, 'S'},
            {"linkable", no_argument, 0, 'l'},
            {"pubkey", optional_argument, 0, 'p'},
            {"pubkey-file", required_argument, 0, 'P'},
            {"quote", no_argument, 0, 'q'},
            {"verbose", no_argument, 0, 'v'},
            {"stdio", no_argument, 0, 'z'},
            {0, 0, 0, 0}};

    /* Parse our options */

    while (1)
    {
        int c;
        int opt_index = 0;
        unsigned char keyin[64];

        c = getopt_long(argc, argv, "N:P:S:dehlmn:p:qrs:vz", long_opt,
                        &opt_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            break;
        case 'N':
            if (!from_hexstring_file((unsigned char *)&config.nonce,
                                     optarg, 16))
            {

                fprintf(stderr, "nonce must be 32-byte hex string\n");
                exit(1);
            }
            SET_OPT(config.flags, OPT_NONCE);

            break;
        case 'P':
            if (!key_load_file(&service_public_key, optarg, KEY_PUBLIC))
            {
                fprintf(stderr, "%s: ", optarg);
                crypto_perror("key_load_file");
                exit(1);
            }

            if (!key_to_sgx_ec256(&config.pubkey, service_public_key))
            {
                fprintf(stderr, "%s: ", optarg);
                crypto_perror("key_to_sgx_ec256");
                exit(1);
            }
            SET_OPT(config.flags, OPT_PUBKEY);

            break;
        case 'S':
            if (!from_hexstring_file((unsigned char *)&config.spid,
                                     optarg, 16))
            {

                fprintf(stderr, "SPID must be 32-byte hex string\n");
                exit(1);
            }
            ++have_spid;

            break;
        case 'd':
            debug = 1;
            break;
        case 'e':
            config.mode = MODE_EPID;
            break;
        case 'l':
            SET_OPT(config.flags, OPT_LINK);
            break;
        case 'm':
            SET_OPT(config.flags, OPT_PSE);
            break;
        case 'n':
            if (strlen(optarg) < 32)
            {
                fprintf(stderr, "nonce must be 32-byte hex string\n");
                exit(1);
            }
            if (!from_hexstring((unsigned char *)&config.nonce,
                                (unsigned char *)optarg, 16))
            {

                fprintf(stderr, "nonce must be 32-byte hex string\n");
                exit(1);
            }

            SET_OPT(config.flags, OPT_NONCE);

            break;
        case 'p':
            if (!from_hexstring((unsigned char *)keyin,
                                (unsigned char *)optarg, 64))
            {

                fprintf(stderr, "key must be 128-byte hex string\n");
                exit(1);
            }

            /* Reverse the byte stream to make a little endien style value */
            for (i = 0; i < 32; ++i)
                config.pubkey.gx[i] = keyin[31 - i];
            for (i = 0; i < 32; ++i)
                config.pubkey.gy[i] = keyin[63 - i];

            SET_OPT(config.flags, OPT_PUBKEY);

            break;
        case 'q':
            config.mode = MODE_QUOTE;
            break;
        case 'r':
            for (i = 0; i < 2; ++i)
            {
                int retry = 10;
                unsigned char ok = 0;
                uint64_t *np = (uint64_t *)&config.nonce;

                while (!ok && retry)
                    ok = _rdrand64_step(&np[i]);
                if (ok == 0)
                {
                    fprintf(stderr, "nonce: RDRAND underflow\n");
                    exit(1);
                }
            }
            SET_OPT(config.flags, OPT_NONCE);
            break;
        case 's':
            if (strlen(optarg) < 32)
            {
                fprintf(stderr, "SPID must be 32-byte hex string\n");
                exit(1);
            }
            if (!from_hexstring((unsigned char *)&config.spid,
                                (unsigned char *)optarg, 16))
            {

                fprintf(stderr, "SPID must be 32-byte hex string\n");
                exit(1);
            }
            ++have_spid;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'z':
            flag_stdio = 1;
            break;
        case 'h':
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    if (argc > 1)
        usage();

    /* Remaining argument is host[:port] */

    if (flag_stdio && argc)
        usage();
    else if (!flag_stdio && !argc)
    {
        // Default to localhost
        config.server = strdup("localhost");
        if (config.server == NULL)
        {
            perror("malloc");
            return 1;
        }
    }
    else if (argc)
    {
        char *cp;

        config.server = strdup(argv[optind]);
        if (config.server == NULL)
        {
            perror("malloc");
            return 1;
        }

        /* If there's a : then we have a port, too */
        cp = strchr(config.server, ':');
        if (cp != NULL)
        {
            *cp++ = '\0';
            config.port = cp;
        }
    }

    if (!have_spid && config.mode != MODE_EPID)
    {
        fprintf(stderr, "SPID required. Use one of --spid or --spid-file \n");
        return 1;
    }

    /* Can we run SGX? */

#ifndef SGX_HW_SIM
    sgx_support = get_sgx_support();
    if (sgx_support & SGX_SUPPORT_NO)
    {
        fprintf(stderr, "This system does not support Intel SGX.\n");
        return 1;
    }
    else
    {
        if (sgx_support & SGX_SUPPORT_ENABLE_REQUIRED)
        {
            fprintf(stderr, "Intel SGX is supported on this system but disabled in the BIOS\n");
            return 1;
        }
        else if (sgx_support & SGX_SUPPORT_REBOOT_REQUIRED)
        {
            fprintf(stderr, "Intel SGX will be enabled after the next reboot\n");
            return 1;
        }
        else if (!(sgx_support & SGX_SUPPORT_ENABLED))
        {
            fprintf(stderr, "Intel SGX is supported on this sytem but not available for use\n");
            fprintf(stderr, "The system may lock BIOS support, or the Platform Software is not available\n");
            return 1;
        }
    }
#endif

    /* Launch the enclave */

    status = sgx_create_enclave_search(ENCLAVE_NAME,
                                       SGX_DEBUG_FLAG, &token, &updated, &eid, 0);
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "sgx_create_enclave: %s: %08x\n",
                ENCLAVE_NAME, status);
        if (status == SGX_ERROR_ENCLAVE_FILE_ACCESS)
            fprintf(stderr, "Did you forget to set LD_LIBRARY_PATH?\n");
        return 1;
    }

    /* Are we attesting, or just spitting out a quote? */

    if (config.mode == MODE_ATTEST)
    {
        do_attestation(eid, &config);
    }
    else if (config.mode == MODE_EPID || config.mode == MODE_QUOTE)
    {
        do_quote(eid, &config);
    }
    else
    {
        fprintf(stderr, "Unknown operation mode.\n");
        return 1;
    }

    close_logfile(fplog);

    return 0;
}

int do_attestation(sgx_enclave_id_t eid, config_t *config)
{
    sgx_status_t status, sgxrv, pse_status;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t *msg2 = NULL;
    sgx_ra_msg3_t *msg3 = NULL;
    ra_msg4_t *msg4 = NULL;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3_sz;
    uint32_t flags = config->flags;
    sgx_ra_context_t ra_ctx = 0xdeadbeef;
    int rv;
    MsgIO *msgio;
    size_t msg4sz = 0;
    int enclaveTrusted = NotTrusted; // Not Trusted
    int b_pse = OPT_ISSET(flags, OPT_PSE);

    if (config->server == NULL)
    {
        msgio = new MsgIO();
    }
    else
    {
        try
        {
            msgio = new MsgIO(config->server, (config->port == NULL) ? DEFAULT_PORT : config->port);
        }
        catch (...)
        {
            exit(1);
        }
    }

    /*
     * WARNING! Normally, the public key would be hardcoded into the
     * enclave, not passed in as a parameter. Hardcoding prevents
     * the enclave using an unauthorized key.
     *
     * This is diagnostic/test application, however, so we have
     * the flexibility of a dynamically assigned key.
     */

    /* Executes an ECALL that runs sgx_ra_init() */

    if (OPT_ISSET(flags, OPT_PUBKEY))
    {
        if (debug)
            fprintf(stderr, "+++ using supplied public key\n");
        status = enclave_ra_init(eid, &sgxrv, config->pubkey, b_pse,
                                 &ra_ctx, &pse_status);
    }
    else
    {
        if (debug)
            fprintf(stderr, "+++ using default public key\n");
        status = enclave_ra_init_def(eid, &sgxrv, b_pse, &ra_ctx,
                                     &pse_status);
    }

    /* Did the ECALL succeed? */
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "enclave_ra_init: %08x\n", status);
        delete msgio;
        return 1;
    }

    /* Did sgx_ra_init() succeed? */
    if (sgxrv != SGX_SUCCESS)
    {
        fprintf(stderr, "sgx_ra_init: %08x\n", sgxrv);
        delete msgio;
        return 1;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_get_extended_epid_group_id: %08x\n", status);
        delete msgio;
        return 1;
    }
    if (verbose)
    {
        dividerWithText(stderr, "Msg0 Details");
        dividerWithText(fplog, "Msg0 Details");
        fprintf(stderr, "Extended Epid Group ID: ");
        fprintf(fplog, "Extended Epid Group ID: ");
        print_hexstring(stderr, &msg0_extended_epid_group_id,
                        sizeof(uint32_t));
        print_hexstring(fplog, &msg0_extended_epid_group_id,
                        sizeof(uint32_t));
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    /* Generate msg1 */

    status = sgx_ra_get_msg1(ra_ctx, eid, sgx_ra_get_ga, &msg1);
    if (status != SGX_SUCCESS)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_ra_get_msg1: %08x\n", status);
        fprintf(fplog, "sgx_ra_get_msg1: %08x\n", status);
        delete msgio;
        return 1;
    }

    if (verbose)
    {
        dividerWithText(stderr, "Msg1 Details");
        dividerWithText(fplog, "Msg1 Details");
        fprintf(stderr, "msg1.g_a.gx = ");
        fprintf(fplog, "msg1.g_a.gx = ");
        print_hexstring(stderr, msg1.g_a.gx, 32);
        print_hexstring(fplog, msg1.g_a.gx, 32);
        fprintf(stderr, "\nmsg1.g_a.gy = ");
        fprintf(fplog, "\nmsg1.g_a.gy = ");
        print_hexstring(stderr, msg1.g_a.gy, 32);
        print_hexstring(fplog, msg1.g_a.gy, 32);
        fprintf(stderr, "\nmsg1.gid    = ");
        fprintf(fplog, "\nmsg1.gid    = ");
        print_hexstring(stderr, msg1.gid, 4);
        print_hexstring(fplog, msg1.gid, 4);
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    /*
     * Send msg0 and msg1 concatenated together (msg0||msg1). We do
     * this for efficiency, to eliminate an additional round-trip
     * between client and server. The assumption here is that most
     * clients have the correct extended_epid_group_id so it's
     * a waste to send msg0 separately when the probability of a
     * rejection is astronomically small.
     *
     * If it /is/ rejected, then the client has only wasted a tiny
     * amount of time generating keys that won't be used.
     */

    dividerWithText(fplog, "Msg0||Msg1 ==> SP");
    fsend_msg_partial(fplog, &msg0_extended_epid_group_id,
                      sizeof(msg0_extended_epid_group_id));
    fsend_msg(fplog, &msg1, sizeof(msg1));
    divider(fplog);

    dividerWithText(stderr, "Copy/Paste Msg0||Msg1 Below to SP");
    msgio->send_partial(&msg0_extended_epid_group_id,
                        sizeof(msg0_extended_epid_group_id));
    msgio->send(&msg1, sizeof(msg1));
    divider(stderr);

    fprintf(stderr, "Waiting for msg2\n");

    /* Read msg2
     *
     * msg2 is variable length b/c it includes the revocation list at
     * the end. msg2 is malloc'd in readZ_msg do free it when done.
     */

    rv = msgio->read((void **)&msg2, NULL);
    if (rv == 0)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "protocol error reading msg2\n");
        delete msgio;
        exit(1);
    }
    else if (rv == -1)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "system error occurred while reading msg2\n");
        delete msgio;
        exit(1);
    }

    if (verbose)
    {
        dividerWithText(stderr, "Msg2 Details");
        dividerWithText(fplog, "Msg2 Details (Received from SP)");
        fprintf(stderr, "msg2.g_b.gx      = ");
        fprintf(fplog, "msg2.g_b.gx      = ");
        print_hexstring(stderr, &msg2->g_b.gx, sizeof(msg2->g_b.gx));
        print_hexstring(fplog, &msg2->g_b.gx, sizeof(msg2->g_b.gx));
        fprintf(stderr, "\nmsg2.g_b.gy      = ");
        fprintf(fplog, "\nmsg2.g_b.gy      = ");
        print_hexstring(stderr, &msg2->g_b.gy, sizeof(msg2->g_b.gy));
        print_hexstring(fplog, &msg2->g_b.gy, sizeof(msg2->g_b.gy));
        fprintf(stderr, "\nmsg2.spid        = ");
        fprintf(fplog, "\nmsg2.spid        = ");
        print_hexstring(stderr, &msg2->spid, sizeof(msg2->spid));
        print_hexstring(fplog, &msg2->spid, sizeof(msg2->spid));
        fprintf(stderr, "\nmsg2.quote_type  = ");
        fprintf(fplog, "\nmsg2.quote_type  = ");
        print_hexstring(stderr, &msg2->quote_type, sizeof(msg2->quote_type));
        print_hexstring(fplog, &msg2->quote_type, sizeof(msg2->quote_type));
        fprintf(stderr, "\nmsg2.kdf_id      = ");
        fprintf(fplog, "\nmsg2.kdf_id      = ");
        print_hexstring(stderr, &msg2->kdf_id, sizeof(msg2->kdf_id));
        print_hexstring(fplog, &msg2->kdf_id, sizeof(msg2->kdf_id));
        fprintf(stderr, "\nmsg2.sign_ga_gb  = ");
        fprintf(fplog, "\nmsg2.sign_ga_gb  = ");
        print_hexstring(stderr, &msg2->sign_gb_ga, sizeof(msg2->sign_gb_ga));
        print_hexstring(fplog, &msg2->sign_gb_ga, sizeof(msg2->sign_gb_ga));
        fprintf(stderr, "\nmsg2.mac         = ");
        fprintf(fplog, "\nmsg2.mac         = ");
        print_hexstring(stderr, &msg2->mac, sizeof(msg2->mac));
        print_hexstring(fplog, &msg2->mac, sizeof(msg2->mac));
        fprintf(stderr, "\nmsg2.sig_rl_size = ");
        fprintf(fplog, "\nmsg2.sig_rl_size = ");
        print_hexstring(stderr, &msg2->sig_rl_size, sizeof(msg2->sig_rl_size));
        print_hexstring(fplog, &msg2->sig_rl_size, sizeof(msg2->sig_rl_size));
        fprintf(stderr, "\nmsg2.sig_rl      = ");
        fprintf(fplog, "\nmsg2.sig_rl      = ");
        print_hexstring(stderr, &msg2->sig_rl, msg2->sig_rl_size);
        print_hexstring(fplog, &msg2->sig_rl, msg2->sig_rl_size);
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    if (debug)
    {
        fprintf(stderr, "+++ msg2_size = %zu\n",
                sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size);
        fprintf(fplog, "+++ msg2_size = %zu\n",
                sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size);
    }

    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

    msg3 = NULL;

    status = sgx_ra_proc_msg2(ra_ctx, eid,
                              sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
                              sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
                              &msg3, &msg3_sz);

    free(msg2);

    if (status != SGX_SUCCESS)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_ra_proc_msg2: %08x\n", status);
        fprintf(fplog, "sgx_ra_proc_msg2: %08x\n", status);

        delete msgio;
        return 1;
    }

    if (debug)
    {
        fprintf(stderr, "+++ msg3_size = %u\n", msg3_sz);
        fprintf(fplog, "+++ msg3_size = %u\n", msg3_sz);
    }

    if (verbose)
    {
        dividerWithText(stderr, "Msg3 Details");
        dividerWithText(fplog, "Msg3 Details");
        fprintf(stderr, "msg3.mac         = ");
        fprintf(fplog, "msg3.mac         = ");
        print_hexstring(stderr, msg3->mac, sizeof(msg3->mac));
        print_hexstring(fplog, msg3->mac, sizeof(msg3->mac));
        fprintf(stderr, "\nmsg3.g_a.gx      = ");
        fprintf(fplog, "\nmsg3.g_a.gx      = ");
        print_hexstring(stderr, msg3->g_a.gx, sizeof(msg3->g_a.gx));
        print_hexstring(fplog, msg3->g_a.gx, sizeof(msg3->g_a.gx));
        fprintf(stderr, "\nmsg3.g_a.gy      = ");
        fprintf(fplog, "\nmsg3.g_a.gy      = ");
        print_hexstring(stderr, msg3->g_a.gy, sizeof(msg3->g_a.gy));
        print_hexstring(fplog, msg3->g_a.gy, sizeof(msg3->g_a.gy));

        fprintf(stderr, "\nmsg3.quote       = ");
        fprintf(fplog, "\nmsg3.quote       = ");
        print_hexstring(stderr, msg3->quote, msg3_sz - sizeof(sgx_ra_msg3_t));
        print_hexstring(fplog, msg3->quote, msg3_sz - sizeof(sgx_ra_msg3_t));
        fprintf(fplog, "\n");
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    dividerWithText(stderr, "Copy/Paste Msg3 Below to SP");
    msgio->send(msg3, msg3_sz);
    divider(stderr);

    dividerWithText(fplog, "Msg3 ==> SP");
    fsend_msg(fplog, msg3, msg3_sz);
    divider(fplog);

    if (msg3)
    {
        free(msg3);
        msg3 = NULL;
    }

    /* Read Msg4 provided by Service Provider, then process */

    rv = msgio->read((void **)&msg4, &msg4sz);
    if (rv == 0)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "protocol error reading msg4\n");
        delete msgio;
        exit(1);
    }
    else if (rv == -1)
    {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "system error occurred while reading msg4\n");
        delete msgio;
        exit(1);
    }

    edividerWithText("Enclave Trust Status from Service Provider");

    enclaveTrusted = msg4->status;
    if (enclaveTrusted == Trusted)
    {
        eprintf("Enclave TRUSTED\n");
    }
    else if (enclaveTrusted == NotTrusted)
    {
        eprintf("Enclave NOT TRUSTED\n");
    }
    else if (enclaveTrusted == Trusted_ItsComplicated)
    {
        // Trusted, but client may be untrusted in the future unless it
        // takes action.

        eprintf("Enclave Trust is TRUSTED and COMPLICATED. The client is out of date and\nmay not be trusted in the future depending on the service provider's  policy.\n");
    }
    else
    {
        // Not Trusted, but client may be able to take action to become
        // trusted.

        eprintf("Enclave Trust is NOT TRUSTED and COMPLICATED. The client is out of date.\n");
    }

    /* check to see if we have a PIB by comparing to empty PIB */
    sgx_platform_info_t emptyPIB;
    memset(&emptyPIB, 0, sizeof(sgx_platform_info_t));

    int retPibCmp = memcmp(&emptyPIB, (void *)(&msg4->platformInfoBlob), sizeof(sgx_platform_info_t));

    if (retPibCmp == 0)
    {
        if (verbose)
            eprintf("A Platform Info Blob (PIB) was NOT provided by the IAS\n");
    }
    else
    {
        if (verbose)
            eprintf("A Platform Info Blob (PIB) was provided by the IAS\n");

        if (debug)
        {
            eprintf("+++ PIB: ");
            print_hexstring(stderr, &msg4->platformInfoBlob, sizeof(sgx_platform_info_t));
            print_hexstring(fplog, &msg4->platformInfoBlob, sizeof(sgx_platform_info_t));
            eprintf("\n");
        }

        /* We have a PIB, so check to see if there are actions to take */
        sgx_update_info_bit_t update_info;
        sgx_status_t ret = sgx_report_attestation_status(&msg4->platformInfoBlob,
                                                         enclaveTrusted, &update_info);

        if (debug)
            eprintf("+++ sgx_report_attestation_status ret = 0x%04x\n", ret);

        edivider();

        /* Check to see if there is an update needed */
        if (ret == SGX_ERROR_UPDATE_NEEDED)
        {

            edividerWithText("Platform Update Required");
            eprintf("The following Platform Update(s) are required to bring this\n");
            eprintf("platform's Trusted Computing Base (TCB) back into compliance:\n\n");
            if (update_info.pswUpdate)
            {
                eprintf("  * Intel SGX Platform Software needs to be updated to the latest version.\n");
            }

            if (update_info.csmeFwUpdate)
            {
                eprintf("  * The Intel Management Engine Firmware Needs to be Updated.  Contact your\n");
                eprintf("    OEM for a BIOS Update.\n");
            }

            if (update_info.ucodeUpdate)
            {
                eprintf("  * The CPU Microcode needs to be updated.  Contact your OEM for a platform\n");
                eprintf("    BIOS Update.\n");
            }
            eprintf("\n");
            edivider();
        }
    }

    /*
     * If the enclave is trusted, fetch a hash of the the MK and SK from
     * the enclave to show proof of a shared secret with the service
     * provider.
     */

    if (enclaveTrusted == Trusted)
    {
        sgx_status_t key_status, sha_status;
        sgx_sha256_hash_t mkhash, skhash;

        // First the MK

        if (debug)
            eprintf("+++ fetching SHA256(MK)\n");
        status = enclave_ra_get_key_hash(eid, &sha_status, &key_status, ra_ctx,
                                         SGX_RA_KEY_MK, &mkhash);
        if (debug)
            eprintf("+++ ECALL enclage_ra_get_key_hash (MK) ret= 0x%04x\n",
                    status);

        if (debug)
            eprintf("+++ sgx_ra_get_keys (MK) ret= 0x%04x\n", key_status);
        // Then the SK

        if (debug)
            eprintf("+++ fetching SHA256(SK)\n");
        status = enclave_ra_get_key_hash(eid, &sha_status, &key_status, ra_ctx,
                                         SGX_RA_KEY_SK, &skhash);
        if (debug)
            eprintf("+++ ECALL enclage_ra_get_key_hash (MK) ret= 0x%04x\n",
                    status);

        if (debug)
            eprintf("+++ sgx_ra_get_keys (MK) ret= 0x%04x\n", key_status);
        if (verbose)
        {
            eprintf("SHA256(MK) = ");
            print_hexstring(stderr, mkhash, sizeof(mkhash));
            print_hexstring(fplog, mkhash, sizeof(mkhash));
            eprintf("\n");
            eprintf("SHA256(SK) = ");
            print_hexstring(stderr, skhash, sizeof(skhash));
            print_hexstring(fplog, skhash, sizeof(skhash));
            eprintf("\n");
        }
    }

    free(msg4);

    enclave_ra_close(eid, &sgxrv, ra_ctx);
    delete msgio;

    return 0;
}

/*----------------------------------------------------------------------
 * do_quote()
 *
 * Generate a quote from the enclave.
 *----------------------------------------------------------------------
 * WARNING!
 *
 * DO NOT USE THIS SUBROUTINE AS A TEMPLATE FOR IMPLEMENTING REMOTE
 * ATTESTATION. do_quote() short-circuits the RA process in order
 * to generate an enclave quote directly!
 *
 * The high-level functions provided for remote attestation take
 * care of the low-level details of quote generation for you:
 *
 *   sgx_ra_init()
 *   sgx_ra_get_msg1
 *   sgx_ra_proc_msg2
 *
 * End developers should not normally be calling these functions
 * directly when doing remote attestation:
 *
 *    sgx_get_ps_sec_prop()
 *    sgx_get_quote()
 *    sgx_get_quote_size()
 *    sgx_calc_quote_size()
 *    sgx_get_report()
 *    sgx_init_quote()
 *
 *----------------------------------------------------------------------
 */

int do_quote(sgx_enclave_id_t eid, config_t *config)
{
    sgx_status_t status, sgxrv;
    sgx_quote_t *quote;
    sgx_report_t report;
    sgx_report_t qe_report;
    sgx_target_info_t target_info;
    sgx_epid_group_id_t epid_gid;
    uint32_t sz = 0;
    uint32_t flags = config->flags;
    sgx_quote_sign_type_t linkable = SGX_UNLINKABLE_SIGNATURE;

    char *b64quote = NULL;
    char *b64manifest = NULL;

    if (OPT_ISSET(flags, OPT_LINK))
        linkable = SGX_LINKABLE_SIGNATURE;

    /* Platform services info. Win32 only. */

    /* Get our quote */

    memset(&report, 0, sizeof(report));

    status = sgx_init_quote(&target_info, &epid_gid);
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "sgx_init_quote: %08x\n", status);
        return 1;
    }

    /* Did they ask for just the EPID? */
    if (config->mode == MODE_EPID)
    {
        printf("%08x\n", *(uint32_t *)epid_gid);
        exit(0);
    }

    status = get_report(eid, &sgxrv, &report, &target_info);
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "get_report: %08x\n", status);
        return 1;
    }
    if (sgxrv != SGX_SUCCESS)
    {
        fprintf(stderr, "sgx_create_report: %08x\n", sgxrv);
        return 1;
    }

    // sgx_get_quote_size() has been deprecated, but our PSW may be too old
    // so use a wrapper function.

    if (!get_quote_size(&status, &sz))
    {
        fprintf(stderr, "PSW missing sgx_get_quote_size() and sgx_calc_quote_size()\n");
        return 1;
    }
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "SGX error while getting quote size: %08x\n", status);
        return 1;
    }

    quote = (sgx_quote_t *)malloc(sz);
    if (quote == NULL)
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    memset(quote, 0, sz);
    status = sgx_get_quote(&report, linkable, &config->spid,
                           (OPT_ISSET(flags, OPT_NONCE)) ? &config->nonce : NULL,
                           NULL, 0,
                           (OPT_ISSET(flags, OPT_NONCE)) ? &qe_report : NULL,
                           quote, sz);
    if (status != SGX_SUCCESS)
    {
        fprintf(stderr, "sgx_get_quote: %08x\n", status);
        return 1;
    }

    /* Print our quote */

    b64quote = base64_encode((char *)quote, sz);
    if (b64quote == NULL)
    {
        eprintf("Could not base64 encode quote\n");
        return 1;
    }

    printf("{\n");
    printf("\"isvEnclaveQuote\":\"%s\"", b64quote);
    if (OPT_ISSET(flags, OPT_NONCE))
    {
        printf(",\n\"nonce\":\"");
        print_hexstring(stdout, &config->nonce, 16);
        printf("\"");
    }

    printf("\n}\n");

#ifdef SGX_HW_SIM
    fprintf(stderr, "WARNING! Built in h/w simulation mode. This quote will not be verifiable.\n");
#endif

    free(b64quote);

    return 0;
}

/*
 * Search for the enclave file and then try and load it.
 */

sgx_status_t sgx_create_enclave_search(const char *filename, const int edebug,
                                       sgx_launch_token_t *token, int *updated, sgx_enclave_id_t *eid,
                                       sgx_misc_attribute_t *attr)
{
    struct stat sb;
    char epath[PATH_MAX]; /* includes NULL */

    /* Is filename an absolute path? */

    if (filename[0] == '/')
        return sgx_create_enclave(filename, edebug, token, updated, eid, attr);

    /* Is the enclave in the current working directory? */

    if (stat(filename, &sb) == 0)
        return sgx_create_enclave(filename, edebug, token, updated, eid, attr);

    /* Search the paths in LD_LBRARY_PATH */

    if (file_in_searchpath(filename, getenv("LD_LIBRARY_PATH"), epath, PATH_MAX))
        return sgx_create_enclave(epath, edebug, token, updated, eid, attr);

    /* Search the paths in DT_RUNPATH */

    if (file_in_searchpath(filename, getenv("DT_RUNPATH"), epath, PATH_MAX))
        return sgx_create_enclave(epath, edebug, token, updated, eid, attr);

    /* Standard system library paths */

    if (file_in_searchpath(filename, DEF_LIB_SEARCHPATH, epath, PATH_MAX))
        return sgx_create_enclave(epath, edebug, token, updated, eid, attr);

    /*
     * If we've made it this far then we don't know where else to look.
     * Just call sgx_create_enclave() which assumes the enclave is in
     * the current working directory. This is almost guaranteed to fail,
     * but it will insure we are consistent about the error codes that
     * get reported to the calling function.
     */

    return sgx_create_enclave(filename, edebug, token, updated, eid, attr);
}

int file_in_searchpath(const char *file, const char *search, char *fullpath,
                       size_t len)
{
    char *p, *str;
    size_t rem;
    struct stat sb;

    if (search == NULL)
        return 0;
    if (strlen(search) == 0)
        return 0;

    str = strdup(search);
    if (str == NULL)
        return 0;

    p = strtok(str, ":");
    while (p != NULL)
    {
        size_t lp = strlen(p);

        if (lp)
        {

            strncpy(fullpath, p, len - 1);
            rem = (len - 1) - lp - 1;
            fullpath[len - 1] = 0;

            strncat(fullpath, "/", rem);
            --rem;

            strncat(fullpath, file, rem);

            if (stat(fullpath, &sb) == 0)
            {
                free(str);
                return 1;
            }
        }

        p = strtok(NULL, ":");
    }

    free(str);

    return 0;
}

void usage()
{
    fprintf(stderr, "usage: client [ options ] [ host[:port] ]\n\n");
    fprintf(stderr, "Required:\n");
    fprintf(stderr, "  -N, --nonce-file=FILE    Set a nonce from a file containing a 32-byte\n");
    fprintf(stderr, "                             ASCII hex string\n");
    fprintf(stderr, "  -P, --pubkey-file=FILE   File containing the public key of the service\n");
    fprintf(stderr, "                             provider.\n");
    fprintf(stderr, "  -S, --spid-file=FILE     Set the SPID from a file containing a 32-byte\n");
    fprintf(stderr, "                             ASCII hex string\n");
    fprintf(stderr, "  -d, --debug              Show debugging information\n");
    fprintf(stderr, "  -e, --epid-gid           Get the EPID Group ID instead of performing\n");
    fprintf(stderr, "                             an attestation.\n");
    fprintf(stderr, "  -l, --linkable           Specify a linkable quote (default: unlinkable)\n");
#ifdef _WIN32
    fprintf(stderr, "  -m, --pse-manifest       Include the PSE manifest in the quote\n");
#endif
    fprintf(stderr, "  -n, --nonce=HEXSTRING    Set a nonce from a 32-byte ASCII hex string\n");
    fprintf(stderr, "  -p, --pubkey=HEXSTRING   Specify the public key of the service provider\n");
    fprintf(stderr, "                             as an ASCII hex string instead of using the\n");
    fprintf(stderr, "                             default.\n");
    fprintf(stderr, "  -q                       Generate a quote instead of performing an\n");
    fprintf(stderr, "                             attestation.\n");
    fprintf(stderr, "  -r                       Generate a nonce using RDRAND\n");
    fprintf(stderr, "  -s, --spid=HEXSTRING     Set the SPID from a 32-byte ASCII hex string\n");
    fprintf(stderr, "  -v, --verbose            Print decoded RA messages to stderr\n");
    fprintf(stderr, "  -z                       Read from stdin and write to stdout instead\n");
    fprintf(stderr, "                             connecting to a server.\n");
    fprintf(stderr, "\nOne of --spid OR --spid-file is required for generating a quote or doing\nremote attestation.\n");
    exit(1);
}

int add_nums(int a, int b)
{
    /* Initialize the enclave */
    if (initialize_enclave() < 0)
    {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    int sum = 0;

    addition(eid, &sum, a, b); // this fuction in Enclave_u.c and defined in Enclave/Enclave.cpp

    printf("App.cpp : add_nums() -> returned sum %d\n", sum);

    /* Destroy the enclave */
    sgx_destroy_enclave(eid);

    return sum;
}

/* Application entry */
// int SGX_CDECL main(int a, int b)
int SGX_CDECL main(int argc, char *argv[])
{
    // (void)(argc);
    // (void)(argv);

    if (argc < 3)
    {
        printf("Please provide 2 arguments\nexample : ./app 2 3\n");
        return -1;
    }

    /* Initialize the enclave */
    if (initialize_enclave() < 0)
    {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    // add enclave functions to be call

    // printf_helloworld(global_eid);

    int sum = 0;

    int a = atoi(argv[1]);
    int b = atoi(argv[2]);

    addition(eid, &sum, a, b); // this fuction in Enclave_u.c and defined in Enclave/Enclave.cpp

    printf("App.cpp : main() -> returned sum %d\n", sum);

    // end enclave functions

    /* Destroy the enclave */
    sgx_destroy_enclave(eid);

    // remote attestation function with pre data
    attestation_with_custom_config();

    return sum;
}
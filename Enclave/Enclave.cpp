

// #include <stdarg.h>
// #include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h" /* print_string */

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
// void printf(const char *fmt, ...)
// {
//     char buf[BUFSIZ] = {'\0'};
//     va_list ap;
//     va_start(ap, fmt);
//     vsnprintf(buf, BUFSIZ, fmt, ap);
//     va_end(ap);
//     ocall_print_string(buf);
// }

// void bswabe_setup(bswabe_pub_t **pub, bswabe_msk_t **msk)
// {
//     element_t alpha;

//     /* initialize */

//     *pub = malloc(sizeof(bswabe_pub_t));
//     *msk = malloc(sizeof(bswabe_msk_t));

//     (*pub)->pairing_desc = strdup(TYPE_A_PARAMS);
//     pairing_init_set_buf((*pub)->p, (*pub)->pairing_desc, strlen((*pub)->pairing_desc));

//     element_init_G1((*pub)->g, (*pub)->p);
//     element_init_G1((*pub)->h, (*pub)->p);
//     element_init_G2((*pub)->gp, (*pub)->p);
//     element_init_GT((*pub)->g_hat_alpha, (*pub)->p);
//     element_init_Zr(alpha, (*pub)->p);
//     element_init_Zr((*msk)->beta, (*pub)->p);
//     element_init_G2((*msk)->g_alpha, (*pub)->p);

//     /* compute */

//     element_random(alpha);
//     element_random((*msk)->beta);
//     element_random((*pub)->g);
//     element_random((*pub)->gp);

//     element_pow_zn((*msk)->g_alpha, (*pub)->gp, alpha);
//     element_pow_zn((*pub)->h, (*pub)->g, (*msk)->beta);
//     pairing_apply((*pub)->g_hat_alpha, (*pub)->g, (*msk)->g_alpha, (*pub)->p);

//     // printf("setup done\n");
// }

int addition(int a, int b)
{
    // printf("Enclave.cpp : addition(%d, %d) \n", a,b);

    // // create int vector and do some operation
    // std::__1::vector<int> v;
    // v.push_back(1);
    // v.push_back(2);

    // int x = v[0]; // 1

    // bswabe setup

    return a + b;
}

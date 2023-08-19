#ifndef PUBKEY_H
#define PUBKEY_H

#include <stdint.h>

#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"

static const sgx_ec256_public_t def_service_public_key = {
    {0x74, 0x5f, 0x33, 0xdb, 0xf9, 0x36, 0xc0, 0x79, 0x8c, 0x2a, 0x68, 0xbe, 0xc8, 0x91, 0x54, 0x87, 0x8b, 0x78, 0x36, 0xd3, 0x87, 0x7d, 0x4a, 0x24, 0x55, 0xd1, 0x3f, 0xd8, 0xe7, 0x4e, 0xbb, 0x9d},
    {0x82, 0xed, 0x76, 0x45, 0x6b, 0x82, 0x71, 0x9b, 0x5c, 0x45, 0x9b, 0x22, 0x90, 0x76, 0x45, 0x53, 0xf0, 0x0f, 0x9a, 0xec, 0x2c, 0xbf, 0xfb, 0x47, 0x8a, 0xe2, 0x43, 0x21, 0x88, 0xdf, 0x8f, 0x5a}

};

#endif // PUBKEY_H

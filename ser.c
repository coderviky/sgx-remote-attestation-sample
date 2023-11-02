#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>

// Replace these keys with your own
// unsigned char SK[16] = "79c8452e27ef0c2575e1ac3a9827f13d";
// Replace the Secret Key with your own in hexadecimal format
unsigned char SK[] = {0x79, 0xc8, 0x45, 0x2e, 0x27, 0xef, 0x0c, 0x25, 0x75, 0xe1, 0xac, 0x3a, 0x98, 0x27, 0xf1, 0x3d};

// 128-bit Secret Key
unsigned char MK[16] = "YourMasterKey456"; // 128-bit Master Key

void encryptString(const unsigned char *plaintext, unsigned char *ciphertext)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(SK, 128, &aesKey);
    AES_encrypt(plaintext, ciphertext, &aesKey);
}

void decryptString(const unsigned char *ciphertext, unsigned char *plaintext)
{
    AES_KEY aesKey;
    AES_set_decrypt_key(SK, 128, &aesKey);
    AES_decrypt(ciphertext, plaintext, &aesKey);
}

int main()
{
    unsigned char plaintext[] = "Hello, Client!";
    unsigned char ciphertext[AES_BLOCK_SIZE];

    encryptString(plaintext, ciphertext);

    // Send the ciphertext to the client.
    // (Implement network communication here)

    printf("Ciphertext: ");
    for (int i = 0; i < AES_BLOCK_SIZE; i++)
    {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");

    unsigned char plaintext2[AES_BLOCK_SIZE];

    decryptString(ciphertext, plaintext2);

    printf("Decrypted Text: %s\n", plaintext2);

    return 0;
}

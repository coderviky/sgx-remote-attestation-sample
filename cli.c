#include <stdio.h>
#include <openssl/aes.h>

// Replace these keys with your own
// unsigned char SK[16] = "79c8452e27ef0c2575e1ac3a9827f13d";
// Replace the Secret Key with your own in hexadecimal format
unsigned char SK[] = {0x79, 0xc8, 0x45, 0x2e, 0x27, 0xef, 0x0c, 0x25, 0x75, 0xe1, 0xac, 0x3a, 0x98, 0x27, 0xf1, 0x3d};

// 128-bit Secret Key
unsigned char MK[16] = "YourMasterKey456"; // 128-bit Master Key

void decryptString(const unsigned char *ciphertext, unsigned char *plaintext)
{
    AES_KEY aesKey;
    AES_set_decrypt_key(SK, 128, &aesKey);
    AES_decrypt(ciphertext, plaintext, &aesKey);
}

int main()
{
    unsigned char ciphertext[AES_BLOCK_SIZE]; // Received ciphertext

    // Receive the ciphertext from the server.
    // (Implement network communication here)
    printf("Enter the ciphertext in hexadecimal format (e.g., 2b7e151628aed2a6abf7158809cf4f3c): ");
    if (scanf("%32s", ciphertext) != 1)
    {
        printf("Error reading ciphertext.\n");
        return 1;
    }

    unsigned char plaintext[AES_BLOCK_SIZE];

    decryptString(ciphertext, plaintext);

    printf("Decrypted Text: %s\n", plaintext);

    return 0;
}

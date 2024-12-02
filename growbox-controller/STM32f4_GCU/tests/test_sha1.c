/* test_sha1.c */
#include "sha1.h"
#include "base64.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Testfall 1: SHA-1 von "abc"
    const char *test_str1 = "abc";
    uint8_t hash1[20];
    sha1((uint8_t *)test_str1, strlen(test_str1), hash1);

    printf("SHA-1(\"%s\") = ", test_str1);
    for(int i = 0; i < 20; i++) {
        printf("%02x", hash1[i]);
    }
    printf("\n");
    // Erwartetes Ergebnis: a9993e364706816aba3e25717850c26c9cd0d89d

    // Testfall 2: SHA-1 von leerem String
    const char *test_str2 = "";
    uint8_t hash2[20];
    sha1((uint8_t *)test_str2, strlen(test_str2), hash2);

    printf("SHA-1(\"%s\") = ", test_str2);
    for(int i = 0; i < 20; i++) {
        printf("%02x", hash2[i]);
    }
    printf("\n");
    // Erwartetes Ergebnis: da39a3ee5e6b4b0d3255bfef95601890afd80709

    // Testfall 3: Base64 von "hello"
    const char *test_str3 = "hello";
    char encoded3[100];
    size_t encoded_len3 = base64_encode((uint8_t *)test_str3, strlen(test_str3), encoded3, sizeof(encoded3) - 1);
    encoded3[encoded_len3] = '\0'; // Nullterminierung

    printf("Base64(\"%s\") = %s\n", test_str3, encoded3);
    // Erwartetes Ergebnis: aGVsbG8=

    // Testfall 4: Base64 von leerem String
    const char *test_str4 = "";
    char encoded4[100];
    size_t encoded_len4 = base64_encode((uint8_t *)test_str4, strlen(test_str4), encoded4, sizeof(encoded4) - 1);
    encoded4[encoded_len4] = '\0'; // Nullterminierung

    printf("Base64(\"%s\") = %s\n", test_str4, encoded4);
    // Erwartetes Ergebnis: (leer)

    return 0;
}

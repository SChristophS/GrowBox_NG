#include <stdio.h>
#include <string.h>
#include "base64.h"

int main() {
    // Testfall: Base64 von "hello"
    const char *test_str = "hello";
    char encoded[25];
    size_t encoded_len = base64_encode((uint8_t *)test_str, strlen(test_str), encoded, sizeof(encoded) - 1);
    encoded[encoded_len] = '\0'; // Nullterminierung
    printf("Base64(\"%s\") = %s\n", test_str, encoded);
    // Erwartetes Ergebnis: aGVsbG8=
    
    // Testfall: Base64 von 16 zufälligen Bytes
    uint8_t random_key[16] = {0x58, 0x87, 0x3B, 0x38, 0x4A, 0x38, 0xE9, 0x14, 0x93, 0x25, 0x58, 0x91, 0x43, 0x4F, 0x59, 0x8D};
    char encoded_key[25];
    encoded_len = base64_encode(random_key, 16, encoded_key, sizeof(encoded_key) - 1);
    encoded_key[encoded_len] = '\0';
    printf("Base64(random_key) = %s\n", encoded_key);
    // Erwartetes Ergebnis (Beispiel): W0y4OkiNSViRRPsSK77iXA==

    return 0;
}

#include <stdint.h>
#include <stddef.h>

// Definition der Base64-Tabelle
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64-Kodierungsfunktion
size_t base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_len) {
    size_t out_index = 0;
    size_t i = 0;

    while (i < in_len) {
        uint32_t triple = 0;
        int bytes_in_triple = 0;

        // Lesen von bis zu 3 Bytes
        for (int j = 0; j < 3; j++) {
            triple <<= 8;
            if (i < in_len) {
                triple |= in[i++];
                bytes_in_triple++;
            } else {
                triple |= 0;
            }
        }

        // Kodierung der 24 Bits in vier Base64-Zeichen
        if (out_index + 4 > out_len) {
            break; // Ausgabepuffer voll
        }

        out[out_index++] = base64_table[(triple >> 18) & 0x3F];
        out[out_index++] = base64_table[(triple >> 12) & 0x3F];

        if (bytes_in_triple > 1) {
            out[out_index++] = base64_table[(triple >> 6) & 0x3F];
        } else {
            out[out_index++] = '=';
        }

        if (bytes_in_triple > 2) {
            out[out_index++] = base64_table[triple & 0x3F];
        } else {
            out[out_index++] = '=';
        }
    }

    // Nullterminierung, falls Platz vorhanden
    if (out_index < out_len) {
        out[out_index] = '\0';
    }

    return out_index;
}

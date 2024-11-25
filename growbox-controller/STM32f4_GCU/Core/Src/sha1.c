/* sha1.c */
#include "sha1.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// SHA-1 context structure
typedef struct {
    uint32_t state[5];
    uint64_t bitcount;
    uint8_t buffer[64];
} SHA1_CTX;

// SHA-1 functions
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))

void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]);
void SHA1Init(SHA1_CTX *context);
void SHA1Update(SHA1_CTX *context, const uint8_t *data, size_t len);
void SHA1Final(uint8_t hash[20], SHA1_CTX *context);

// **Vorwärtsdeklaration der swap64-Funktion**
uint64_t swap64(uint64_t val);

void sha1(const uint8_t *data, size_t len, uint8_t hash[20]) {
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, data, len);
    SHA1Final(hash, &ctx);
}

void SHA1Init(SHA1_CTX *context) {
    context->bitcount = 0;
    // Initial Hash Values
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
}

void SHA1Update(SHA1_CTX *context, const uint8_t *data, size_t len) {
    size_t i = 0;

    for (i = 0; i < len; ++i) {
        context->buffer[context->bitcount / 8 % 64] = data[i];
        context->bitcount += 8;
        if ((context->bitcount / 8) % 64 == 0) {
            SHA1Transform(context->state, context->buffer);
        }
    }
}

void SHA1Final(uint8_t hash[20], SHA1_CTX *context) {
    size_t i = context->bitcount / 8 % 64;
    context->buffer[i++] = 0x80;

    if (i > 56) {
        while (i < 64) {
            context->buffer[i++] = 0x00;
        }
        SHA1Transform(context->state, context->buffer);
        i = 0;
    }

    while (i < 56) {
        context->buffer[i++] = 0x00;
    }

    // Append the total message length in bits
    uint64_t bitcount_be = swap64(context->bitcount); // Verwende swap64, siehe unten
    memcpy(&context->buffer[i], &bitcount_be, 8);
    SHA1Transform(context->state, context->buffer);

    // Convert state to big endian
    for (i = 0; i < 5; i++) {
        uint32_t tmp = context->state[i];
        hash[i*4 + 0] = (tmp >> 24) & 0xFF;
        hash[i*4 + 1] = (tmp >> 16) & 0xFF;
        hash[i*4 + 2] = (tmp >> 8) & 0xFF;
        hash[i*4 + 3] = tmp & 0xFF;
    }
}

#define K0 0x5A827999
#define K1 0x6ED9EBA1
#define K2 0x8F1BBCDC
#define K3 0xCA62C1D6

void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a, b, c, d, e, i, t, W[80];
    const uint8_t *p = buffer;

    // Prepare message schedule W
    for (i = 0; i < 16; ++i) {
        W[i] = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
        p += 4;
    }
    for (; i < 80; ++i) {
        W[i] = ROTLEFT(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (i = 0; i < 80; ++i) {
        if (i < 20) {
            t = ROTLEFT(a, 5) + ((b & c) | (~b & d)) + e + W[i] + K0;
        } else if (i < 40) {
            t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + W[i] + K1;
        } else if (i < 60) {
            t = ROTLEFT(a, 5) + ((b & c) | (b & d) | (c & d)) + e + W[i] + K2;
        } else {
            t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + W[i] + K3;
        }
        e = d;
        d = c;
        c = ROTLEFT(b, 30);
        b = a;
        a = t;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

// **Definition der swap64-Funktion**
uint64_t swap64(uint64_t val) {
    return ((val & 0x00000000000000FFULL) << 56) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x00000000FF000000ULL) << 8) |
           ((val & 0x000000FF00000000ULL) >> 8) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0xFF00000000000000ULL) >> 56);
}

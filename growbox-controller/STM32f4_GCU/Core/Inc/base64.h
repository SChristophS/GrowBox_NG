/* base64.h */
#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <stdint.h>

size_t base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_len);

#endif // BASE64_H

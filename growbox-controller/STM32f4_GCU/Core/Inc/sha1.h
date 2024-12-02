/* sha1.h */
#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <stddef.h>

void sha1(const uint8_t *data, size_t len, uint8_t hash[20]);

#endif // SHA1_H

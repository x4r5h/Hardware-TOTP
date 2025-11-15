#ifndef TOTP_CORE_H
#define TOTP_CORE_H

#include <Arduino.h>

// Base32 decoding
int base32CharValue(char c);
int decodeBase32(const char *encoded, uint8_t *output, int maxLen);

// HMAC-SHA1
void computeHMAC(const uint8_t *key, int keyLen,
                 const uint8_t *msg, int msgLen,
                 uint8_t *result);

// TOTP/HOTP generation
uint32_t generateHOTP(const char *base32Secret, uint64_t counter);
uint32_t generateTOTP(const char *secret);

#endif

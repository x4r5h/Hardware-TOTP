#include "totp_core.h"
#include "config.h"
#include <mbedtls/md.h>

int base32CharValue(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '2' && c <= '7') return c - '2' + 26;
  return -1;
}

int decodeBase32(const char *encoded, uint8_t *output, int maxLen) {
  int buffer = 0, bitsLeft = 0, count = 0;
  for (; *encoded && count < maxLen; ++encoded) {
    int val = base32CharValue(toupper(*encoded));
    if (val < 0) continue;
    buffer = (buffer << 5) | (val & 0x1F);
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      output[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
      bitsLeft -= 8;
    }
  }
  return count;
}

void computeHMAC(const uint8_t *key, int keyLen,
                 const uint8_t *msg, int msgLen,
                 uint8_t *result) {
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, info, 1);
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  mbedtls_md_hmac_update(&ctx, msg, msgLen);
  mbedtls_md_hmac_finish(&ctx, result);
  mbedtls_md_free(&ctx);
}

uint32_t generateHOTP(const char *base32Secret, uint64_t counter) {
  uint8_t key[64];
  int keyLen = decodeBase32(base32Secret, key, sizeof(key));

  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) {
    msg[i] = counter & 0xFF;
    counter >>= 8;
  }

  uint8_t hash[20];
  computeHMAC(key, keyLen, msg, 8, hash);

  int offset = hash[19] & 0x0F;
  uint32_t code = ((hash[offset] & 0x7f) << 24) |
                  ((hash[offset + 1] & 0xff) << 16) |
                  ((hash[offset + 2] & 0xff) << 8) |
                  (hash[offset + 3] & 0xff);

  return code % 1000000;
}

uint32_t generateTOTP(const char *secret) {
  time_t now = time(nullptr);
  uint64_t counter = now / TOTP_INTERVAL;
  return generateHOTP(secret, counter);
}

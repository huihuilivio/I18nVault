// SM4 block cipher (GB/T 32907-2016)
// 128-bit key, 128-bit block
#ifndef SM4_H
#define SM4_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16
#define SM4_NUM_ROUNDS 32

typedef struct {
  uint32_t rk[SM4_NUM_ROUNDS]; // round keys
} sm4_ctx;

// Key schedule: expand 128-bit key into 32 round keys
void sm4_init(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]);

// Encrypt a single 16-byte block in-place
void sm4_encrypt_block(const sm4_ctx *ctx, const uint8_t in[SM4_BLOCK_SIZE],
                       uint8_t out[SM4_BLOCK_SIZE]);

// Decrypt a single 16-byte block in-place
void sm4_decrypt_block(const sm4_ctx *ctx, const uint8_t in[SM4_BLOCK_SIZE],
                       uint8_t out[SM4_BLOCK_SIZE]);

// CBC mode encrypt. iv is updated in-place. out must have room for
// padded_len (next multiple of 16). Returns bytes written.
// PKCS#7 padding is appended automatically.
size_t sm4_cbc_encrypt(const sm4_ctx *ctx, uint8_t iv[SM4_BLOCK_SIZE],
                       const uint8_t *in, size_t in_len, uint8_t *out);

// CBC mode decrypt. iv is updated in-place. Returns plaintext length
// after removing PKCS#7 padding, or 0 on padding error.
size_t sm4_cbc_decrypt(const sm4_ctx *ctx, uint8_t iv[SM4_BLOCK_SIZE],
                       const uint8_t *in, size_t in_len, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif // SM4_H

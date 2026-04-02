// GCM mode (CTR + GHASH) backed by SM4
// NIST SP 800-38D compliant
#ifndef GCM_H
#define GCM_H

#include "sm4.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define GCM_TAG_SIZE 16
#define GCM_IV_SIZE  12

    typedef struct
    {
        sm4_ctx cipher;
        uint8_t H[16]; /* GHASH subkey = E(K, 0^128) */
    } gcm_ctx;

    void gcm_init(gcm_ctx* ctx, const uint8_t key[SM4_KEY_SIZE]);

    int gcm_encrypt(const gcm_ctx* ctx, const uint8_t* iv, size_t iv_len, const uint8_t* aad, size_t aad_len,
                    const uint8_t* pt, size_t pt_len, uint8_t* ct, uint8_t tag[GCM_TAG_SIZE]);

    /* Returns 0 on success, -1 on authentication failure.
       On failure the plaintext buffer is zeroed. */
    int gcm_decrypt(const gcm_ctx* ctx, const uint8_t* iv, size_t iv_len, const uint8_t* aad, size_t aad_len,
                    const uint8_t* ct, size_t ct_len, uint8_t* pt, const uint8_t tag[GCM_TAG_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* GCM_H */

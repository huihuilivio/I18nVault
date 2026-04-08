// SM4-GCM one-shot AEAD interface
#ifndef SM4_GCM_H
#define SM4_GCM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SM4_GCM_KEY_SIZE 16
#define SM4_GCM_IV_SIZE  12
#define SM4_GCM_TAG_SIZE 16

    /* Encrypt + authenticate. Returns 0. */
    int sm4_gcm_encrypt(const uint8_t key[SM4_GCM_KEY_SIZE], const uint8_t* iv, size_t iv_len, const uint8_t* aad,
                        size_t aad_len, const uint8_t* pt, size_t pt_len, uint8_t* ct, uint8_t tag[SM4_GCM_TAG_SIZE]);

    /* Decrypt + verify. Returns 0 on success, -1 on auth failure.
       On failure the plaintext buffer is zeroed. */
    int sm4_gcm_decrypt(const uint8_t key[SM4_GCM_KEY_SIZE], const uint8_t* iv, size_t iv_len, const uint8_t* aad,
                        size_t aad_len, const uint8_t* ct, size_t ct_len, uint8_t* pt,
                        const uint8_t tag[SM4_GCM_TAG_SIZE]);

    /* Securely zero memory — resists compiler dead-store elimination. */
    void secure_wipe(void* p, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SM4_GCM_H */

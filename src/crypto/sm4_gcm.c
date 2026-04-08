/*
 * SM4-GCM one-shot wrapper
 * Creates a temporary gcm_ctx, runs encrypt/decrypt, wipes context.
 */
#include "sm4_gcm.h"

#include "gcm.h"

#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

void secure_wipe(void* p, size_t len)
{
#if defined(_WIN32)
    SecureZeroMemory(p, len);
#elif defined(__STDC_LIB_EXT1__) || (defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__)
    memset_s(p, len, 0, len);
#else
    /* Fallback: volatile pointer trick */
    volatile uint8_t* vp = (volatile uint8_t*)p;
    while (len--)
        *vp++ = 0;
#endif
}

int sm4_gcm_encrypt(const uint8_t key[SM4_GCM_KEY_SIZE], const uint8_t* iv, size_t iv_len, const uint8_t* aad,
                    size_t aad_len, const uint8_t* pt, size_t pt_len, uint8_t* ct, uint8_t tag[SM4_GCM_TAG_SIZE])
{
    gcm_ctx ctx;
    gcm_init(&ctx, key);
    int ret = gcm_encrypt(&ctx, iv, iv_len, aad, aad_len, pt, pt_len, ct, tag);
    secure_wipe(&ctx, sizeof(ctx));
    return ret;
}

int sm4_gcm_decrypt(const uint8_t key[SM4_GCM_KEY_SIZE], const uint8_t* iv, size_t iv_len, const uint8_t* aad,
                    size_t aad_len, const uint8_t* ct, size_t ct_len, uint8_t* pt, const uint8_t tag[SM4_GCM_TAG_SIZE])
{
    gcm_ctx ctx;
    gcm_init(&ctx, key);
    int ret = gcm_decrypt(&ctx, iv, iv_len, aad, aad_len, ct, ct_len, pt, tag);
    secure_wipe(&ctx, sizeof(ctx));
    return ret;
}

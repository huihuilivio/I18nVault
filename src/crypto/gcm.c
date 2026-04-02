/*
 * GCM mode (NIST SP 800-38D)
 * CTR encryption + GHASH authentication over SM4
 */
#include "gcm.h"

#include <string.h>


/* ---- helpers ---- */

static void put_be64(uint8_t* b, uint64_t v)
{
    b[0] = (uint8_t)(v >> 56);
    b[1] = (uint8_t)(v >> 48);
    b[2] = (uint8_t)(v >> 40);
    b[3] = (uint8_t)(v >> 32);
    b[4] = (uint8_t)(v >> 24);
    b[5] = (uint8_t)(v >> 16);
    b[6] = (uint8_t)(v >> 8);
    b[7] = (uint8_t)(v);
}

static void xor_block(uint8_t* dst, const uint8_t* src, size_t len)
{
    for (size_t i = 0; i < len; i++)
        dst[i] ^= src[i];
}

/* Increment rightmost 32 bits (big-endian) of a 128-bit counter */
static void inc32(uint8_t ctr[16])
{
    for (int i = 15; i >= 12; i--)
    {
        if (++ctr[i] != 0)
            break;
    }
}

/* ---- GF(2^128) multiplication (Algorithm 1, NIST SP 800-38D) ---- */
/* R = 0xE1 || 0^120 */

static void gf128_mul(uint8_t out[16], const uint8_t X[16], const uint8_t Y[16])
{
    uint8_t Z[16] = {0};
    uint8_t V[16];
    memcpy(V, Y, 16);

    for (int i = 0; i < 128; i++)
    {
        if (X[i / 8] & (1 << (7 - (i % 8))))
            xor_block(Z, V, 16);

        int carry = V[15] & 1;
        for (int j = 15; j > 0; j--)
            V[j] = (V[j] >> 1) | (V[j - 1] << 7);
        V[0] >>= 1;
        if (carry)
            V[0] ^= 0xE1;
    }
    memcpy(out, Z, 16);
}

/* ---- GHASH ---- */

/* Process data (with implicit zero-padding of partial last block) */
static void ghash_update(uint8_t Y[16], const uint8_t H[16], const uint8_t* data, size_t len)
{
    uint8_t tmp[16];
    while (len >= 16)
    {
        xor_block(Y, data, 16);
        gf128_mul(tmp, Y, H);
        memcpy(Y, tmp, 16);
        data += 16;
        len -= 16;
    }
    if (len > 0)
    {
        xor_block(Y, data, len);
        gf128_mul(tmp, Y, H);
        memcpy(Y, tmp, 16);
    }
}

/* GHASH over (AAD || pad || CT || pad || len_A || len_C) */
static void ghash_full(const uint8_t H[16], const uint8_t* aad, size_t aad_len, const uint8_t* ct, size_t ct_len,
                       uint8_t out[16])
{
    uint8_t len_block[16];
    memset(out, 0, 16);
    ghash_update(out, H, aad, aad_len);
    ghash_update(out, H, ct, ct_len);
    put_be64(len_block, (uint64_t)aad_len * 8);
    put_be64(len_block + 8, (uint64_t)ct_len * 8);
    ghash_update(out, H, len_block, 16);
}

/* ---- GCTR (counter-mode XOR) ---- */

static void gctr(const sm4_ctx* cipher, const uint8_t icb[16], const uint8_t* in, size_t len, uint8_t* out)
{
    uint8_t cb[16], ks[16];
    memcpy(cb, icb, 16);

    while (len >= 16)
    {
        sm4_encrypt_block(cipher, cb, ks);
        xor_block(ks, in, 16);
        memcpy(out, ks, 16);
        inc32(cb);
        in += 16;
        out += 16;
        len -= 16;
    }
    if (len > 0)
    {
        sm4_encrypt_block(cipher, cb, ks);
        xor_block(ks, in, len);
        memcpy(out, ks, len);
    }
}

/* ---- J0 computation ---- */

static void compute_j0(const gcm_ctx* ctx, const uint8_t* iv, size_t iv_len, uint8_t J0[16])
{
    if (iv_len == GCM_IV_SIZE)
    {
        memcpy(J0, iv, 12);
        J0[12] = 0;
        J0[13] = 0;
        J0[14] = 0;
        J0[15] = 1;
    }
    else
    {
        /* J0 = GHASH_H(IV || 0^{s+64} || [len(IV)]_64) */
        uint8_t len_block[16];
        memset(J0, 0, 16);
        ghash_update(J0, ctx->H, iv, iv_len);
        memset(len_block, 0, 8);
        put_be64(len_block + 8, (uint64_t)iv_len * 8);
        ghash_update(J0, ctx->H, len_block, 16);
    }
}

/* Constant-time comparison */
static int ct_memcmp(const uint8_t* a, const uint8_t* b, size_t len)
{
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++)
        diff |= a[i] ^ b[i];
    return (int)diff;
}

/* ---- Public API ---- */

void gcm_init(gcm_ctx* ctx, const uint8_t key[SM4_KEY_SIZE])
{
    uint8_t zero[16] = {0};
    sm4_init(&ctx->cipher, key);
    sm4_encrypt_block(&ctx->cipher, zero, ctx->H);
}

int gcm_encrypt(const gcm_ctx* ctx, const uint8_t* iv, size_t iv_len, const uint8_t* aad, size_t aad_len,
                const uint8_t* pt, size_t pt_len, uint8_t* ct, uint8_t tag[GCM_TAG_SIZE])
{
    uint8_t J0[16], J0_inc[16], S[16];

    compute_j0(ctx, iv, iv_len, J0);
    memcpy(J0_inc, J0, 16);
    inc32(J0_inc);

    gctr(&ctx->cipher, J0_inc, pt, pt_len, ct);
    ghash_full(ctx->H, aad, aad_len, ct, pt_len, S);
    /* Tag = GCTR(K, J0, S) — single-block GCTR */
    sm4_encrypt_block(&ctx->cipher, J0, tag);
    xor_block(tag, S, 16);
    return 0;
}

int gcm_decrypt(const gcm_ctx* ctx, const uint8_t* iv, size_t iv_len, const uint8_t* aad, size_t aad_len,
                const uint8_t* ct, size_t ct_len, uint8_t* pt, const uint8_t tag[GCM_TAG_SIZE])
{
    uint8_t J0[16], J0_inc[16], S[16], T[16];

    compute_j0(ctx, iv, iv_len, J0);
    memcpy(J0_inc, J0, 16);
    inc32(J0_inc);

    /* Verify tag BEFORE exposing plaintext */
    ghash_full(ctx->H, aad, aad_len, ct, ct_len, S);
    sm4_encrypt_block(&ctx->cipher, J0, T);
    xor_block(T, S, 16);

    if (ct_memcmp(T, tag, GCM_TAG_SIZE) != 0)
    {
        memset(pt, 0, ct_len);
        return -1;
    }

    gctr(&ctx->cipher, J0_inc, ct, ct_len, pt);
    return 0;
}

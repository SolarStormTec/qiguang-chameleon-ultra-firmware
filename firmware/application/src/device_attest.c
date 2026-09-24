#include "device_attest.h"

#include <string.h>
#include "nrf.h"
#include "nrf_crypto.h"
#include "fds_util.h"
#include "fds_ids.h"
#include "utils.h"

#define ATTEST_BLOB_MAGIC    0x53414B31u
#define ATTEST_BLOB_VERSION  1

#define ATTEST_FLAG_HAS_KEY  (1u << 0)
#define ATTEST_FLAG_HAS_CERT (1u << 1)
#define ATTEST_FLAG_LOCKED   (1u << 2)

typedef struct ALIGN_U32 {
    uint32_t magic;
    uint8_t  version;
    uint8_t  flags;
    uint8_t  reserved[2];
    uint8_t  priv[ATTEST_PRIV_LEN];
    uint8_t  pub[ATTEST_PUB_LEN];
    uint8_t  cert[ATTEST_CERT_LEN];
} attest_blob_t;

static attest_blob_t m_blob;
static bool          m_crypto_ready = false;

static void secure_zero(void *buffer, size_t length) {
    volatile uint8_t *byte = buffer;
    while (length != 0) {
        *byte++ = 0;
        --length;
    }
}

void device_attest_get_chip_id(uint8_t out[ATTEST_CHIP_ID_LEN]) {
    uint32_t hsw = NRF_FICR->DEVICEID[1];
    uint32_t lsw = NRF_FICR->DEVICEID[0];
    out[0] = (uint8_t)(hsw >> 24); out[1] = (uint8_t)(hsw >> 16);
    out[2] = (uint8_t)(hsw >> 8);  out[3] = (uint8_t)(hsw);
    out[4] = (uint8_t)(lsw >> 24); out[5] = (uint8_t)(lsw >> 16);
    out[6] = (uint8_t)(lsw >> 8);  out[7] = (uint8_t)(lsw);
}

static bool blob_valid(void) {
    return m_blob.magic == ATTEST_BLOB_MAGIC && m_blob.version == ATTEST_BLOB_VERSION;
}

static bool blob_save(void) {
    return fds_write_sync(FDS_DEVICE_ATTEST_FILE_ID, FDS_DEVICE_ATTEST_RECORD_KEY,
                          sizeof(m_blob), (void *)&m_blob);
}

void device_attest_init(void) {
    memset(&m_blob, 0, sizeof(m_blob));
    uint16_t len = sizeof(m_blob);
    if (!fds_read_sync(FDS_DEVICE_ATTEST_FILE_ID, FDS_DEVICE_ATTEST_RECORD_KEY, &len, (uint8_t *)&m_blob)
        || !blob_valid()) {
        memset(&m_blob, 0, sizeof(m_blob));
        m_blob.magic   = ATTEST_BLOB_MAGIC;
        m_blob.version = ATTEST_BLOB_VERSION;
        m_blob.flags   = 0;
    }
}

bool device_attest_has_key(void)   { return (m_blob.flags & ATTEST_FLAG_HAS_KEY) != 0; }
bool device_attest_is_locked(void) { return (m_blob.flags & ATTEST_FLAG_LOCKED) != 0; }
bool device_attest_has_cert(void)  { return (m_blob.flags & ATTEST_FLAG_HAS_CERT) != 0; }

static int ensure_crypto(void) {
    if (m_crypto_ready) return 0;
    if (nrf_crypto_init() != NRF_SUCCESS) return -1;
    m_crypto_ready = true;
    return 0;
}

int device_attest_ensure_key(uint8_t out_pub[ATTEST_PUB_LEN]) {
    if (device_attest_has_key()) {
        memcpy(out_pub, m_blob.pub, ATTEST_PUB_LEN);
        return 0;
    }
    if (device_attest_is_locked()) return -1;
    if (ensure_crypto() != 0)      return -2;

    nrf_crypto_ecc_key_pair_generate_context_t gen_ctx;
    nrf_crypto_ecc_private_key_t priv;
    nrf_crypto_ecc_public_key_t  pub;
    if (nrf_crypto_ecc_key_pair_generate(&gen_ctx, &g_nrf_crypto_ecc_secp256r1_curve_info,
                                         &priv, &pub) != NRF_SUCCESS) {
        return -3;
    }

    uint8_t raw_priv[ATTEST_PRIV_LEN];
    uint8_t raw_pub[ATTEST_PUB_LEN - 1];
    size_t  priv_sz = sizeof(raw_priv);
    size_t  pub_sz  = sizeof(raw_pub);
    ret_code_t e1 = nrf_crypto_ecc_private_key_to_raw(&priv, raw_priv, &priv_sz);
    ret_code_t e2 = nrf_crypto_ecc_public_key_to_raw(&pub, raw_pub, &pub_sz);
    nrf_crypto_ecc_private_key_free(&priv);
    nrf_crypto_ecc_public_key_free(&pub);
    if (e1 != NRF_SUCCESS || e2 != NRF_SUCCESS ||
        priv_sz != ATTEST_PRIV_LEN || pub_sz != (ATTEST_PUB_LEN - 1)) {
        secure_zero(raw_priv, sizeof(raw_priv));
        return -4;
    }

    memcpy(m_blob.priv, raw_priv, ATTEST_PRIV_LEN);
    m_blob.pub[0] = 0x04;
    memcpy(&m_blob.pub[1], raw_pub, ATTEST_PUB_LEN - 1);
    m_blob.flags |= ATTEST_FLAG_HAS_KEY;
    secure_zero(raw_priv, sizeof(raw_priv));

    if (!blob_save()) {
        m_blob.flags &= ~ATTEST_FLAG_HAS_KEY;
        secure_zero(m_blob.priv, sizeof(m_blob.priv));
        memset(m_blob.pub, 0, sizeof(m_blob.pub));
        return -5;
    }
    memcpy(out_pub, m_blob.pub, ATTEST_PUB_LEN);
    return 0;
}

int device_attest_sign(const uint8_t *nonce, uint16_t nonce_len, uint8_t out_sig[ATTEST_SIG_LEN]) {
    if (!device_attest_has_key())                    return -1;
    if (nonce == NULL || nonce_len < 8 || nonce_len > 64) return -2;
    if (ensure_crypto() != 0)                        return -3;

    uint8_t msg[64 + ATTEST_CHIP_ID_LEN];
    memcpy(msg, nonce, nonce_len);
    device_attest_get_chip_id(&msg[nonce_len]);
    size_t msg_len = (size_t)nonce_len + ATTEST_CHIP_ID_LEN;

    nrf_crypto_hash_context_t hash_ctx;
    uint8_t digest[NRF_CRYPTO_HASH_SIZE_SHA256];
    size_t  digest_len = sizeof(digest);
    if (nrf_crypto_hash_calculate(&hash_ctx, &g_nrf_crypto_hash_sha256_info,
                                  msg, msg_len, digest, &digest_len) != NRF_SUCCESS
        || digest_len != NRF_CRYPTO_HASH_SIZE_SHA256) {
        return -4;
    }

    nrf_crypto_ecc_private_key_t priv;
    if (nrf_crypto_ecc_private_key_from_raw(&g_nrf_crypto_ecc_secp256r1_curve_info,
                                            &priv, m_blob.priv, ATTEST_PRIV_LEN) != NRF_SUCCESS) {
        return -5;
    }

    nrf_crypto_ecdsa_sign_context_t sign_ctx;
    size_t sig_len = ATTEST_SIG_LEN;
    ret_code_t err = nrf_crypto_ecdsa_sign(&sign_ctx, &priv, digest, digest_len, out_sig, &sig_len);
    nrf_crypto_ecc_private_key_free(&priv);
    if (err != NRF_SUCCESS || sig_len != ATTEST_SIG_LEN) return -6;
    return 0;
}

bool device_attest_get_cert(uint8_t out[ATTEST_CERT_LEN]) {
    if (!device_attest_has_cert()) return false;
    memcpy(out, m_blob.cert, ATTEST_CERT_LEN);
    return true;
}

int device_attest_set_cert(const uint8_t cert[ATTEST_CERT_LEN]) {
    if (device_attest_is_locked()) return -1;
    if (!device_attest_has_key())  return -2;
    if (cert == NULL)              return -3;

    if (cert[0] != 0x01) return -4;
    uint8_t chip_id[ATTEST_CHIP_ID_LEN];
    device_attest_get_chip_id(chip_id);
    if (memcmp(&cert[1], chip_id, ATTEST_CHIP_ID_LEN) != 0)                 return -5;
    if (memcmp(&cert[1 + ATTEST_CHIP_ID_LEN], m_blob.pub, ATTEST_PUB_LEN) != 0) return -6;

    memcpy(m_blob.cert, cert, ATTEST_CERT_LEN);
    m_blob.flags |= ATTEST_FLAG_HAS_CERT;
    if (!blob_save()) {
        m_blob.flags &= ~ATTEST_FLAG_HAS_CERT;
        return -7;
    }
    return 0;
}

int device_attest_lock(void) {
    if (device_attest_is_locked()) return 0;
    if (!device_attest_has_key() || !device_attest_has_cert()) return -2;
    m_blob.flags |= ATTEST_FLAG_LOCKED;
    if (!blob_save()) {
        m_blob.flags &= ~ATTEST_FLAG_LOCKED;
        return -1;
    }
    return 0;
}

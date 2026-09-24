#ifndef DEVICE_ATTEST_H
#define DEVICE_ATTEST_H

#include <stdint.h>
#include <stdbool.h>


#define ATTEST_CHIP_ID_LEN   8
#define ATTEST_PRIV_LEN      32
#define ATTEST_PUB_LEN       65
#define ATTEST_SIG_LEN       64
#define ATTEST_CERT_LEN      147

void device_attest_init(void);

bool device_attest_has_key(void);
bool device_attest_is_locked(void);
bool device_attest_has_cert(void);

void device_attest_get_chip_id(uint8_t out[ATTEST_CHIP_ID_LEN]);

int device_attest_sign(const uint8_t *nonce, uint16_t nonce_len, uint8_t out_sig[ATTEST_SIG_LEN]);

int device_attest_ensure_key(uint8_t out_pub[ATTEST_PUB_LEN]);

bool device_attest_get_cert(uint8_t out[ATTEST_CERT_LEN]);

int device_attest_set_cert(const uint8_t cert[ATTEST_CERT_LEN]);

int device_attest_lock(void);

#endif

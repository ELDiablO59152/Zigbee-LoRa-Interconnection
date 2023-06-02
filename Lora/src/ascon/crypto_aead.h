int crypto_aead_encrypt(unsigned char *c, uint8_t *clen,
                        const unsigned char *m, uint8_t mlen,
                        const unsigned char *ad, unsigned long long adlen,
                        const unsigned char *nsec, const unsigned char *npub,
                        const unsigned char *k);

int crypto_aead_decrypt(unsigned char *m, uint8_t *mlen,
                        unsigned char *nsec, const unsigned char *c,
                        uint8_t clen, const unsigned char *ad,
                        unsigned long long adlen, const unsigned char *npub,
                        const unsigned char *k);

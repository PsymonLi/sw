// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "include/sdk/base.hpp"
#include "platform/elba/elba_barco_asym_apis.hpp"

namespace sdk {
namespace platform {
namespace elba {

/* secp256r1 */
uint8_t p[] = {
    0xFF, 0xFF, 0xFF, 0xFF,     0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t n[] = {
    0xFF, 0xFF, 0xFF, 0xFF,     0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF, 0xFF,
    0xBC, 0xE6, 0xFA, 0xAD,     0xA7, 0x17, 0x9E, 0x84,
    0xF3, 0xB9, 0xCA, 0xC2,     0xFC, 0x63, 0x25, 0x51
};

uint8_t xg[] = {
    0x6B, 0x17, 0xD1, 0xF2,     0xE1, 0x2C, 0x42, 0x47,
    0xF8, 0xBC, 0xE6, 0xE5,     0x63, 0xA4, 0x40, 0xF2,
    0x77, 0x03, 0x7D, 0x81,     0x2D, 0xEB, 0x33, 0xA0,
    0xF4, 0xA1, 0x39, 0x45,     0xD8, 0x98, 0xC2, 0x96
};

uint8_t yg[] = {
    0x4F, 0xE3, 0x42, 0xE2,     0xFE, 0x1A, 0x7F, 0x9B,
    0x8E, 0xE7, 0xEB, 0x4A,     0x7C, 0x0F, 0x9E, 0x16,
    0x2B, 0xCE, 0x33, 0x57,     0x6B, 0x31, 0x5E, 0xCE,
    0xCB, 0xB6, 0x40, 0x68,     0x37, 0xBF, 0x51, 0xF5
};

uint8_t a[] = {
    0xFF, 0xFF, 0xFF, 0xFF,     0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,     0xFF, 0xFF, 0xFF, 0xFC
};

uint8_t b[] = {
    0x5A, 0xC6, 0x35, 0xD8,     0xAA, 0x3A, 0x93, 0xE7,
    0xB3, 0xEB, 0xBD, 0x55,     0x76, 0x98, 0x86, 0xBC,
    0x65, 0x1D, 0x06, 0xB0,     0xCC, 0x53, 0xB0, 0xF6,
    0x3B, 0xCE, 0x3C, 0x3E,     0x27, 0xD2, 0x60, 0x4B
};

uint8_t k[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x09
};

/* Expected output for ECC Point Mul */
uint8_t xq[] = {
    0xEA, 0x68, 0xD7, 0xB6,     0xFE, 0xDF, 0x0B, 0x71,
    0x87, 0x89, 0x38, 0xD5,     0x1D, 0x71, 0xF8, 0x72,
    0x9E, 0x0A, 0xCB, 0x8C,     0x2C, 0x6D, 0xF8, 0xB3,
    0xD7, 0x9E, 0x8A, 0x4B,     0x90, 0x94, 0x9E, 0xE0
};
uint8_t yq[] = {
    0x2A, 0x27, 0x44, 0xC9,     0x72, 0xC9, 0xFC, 0xE7,
    0x87, 0x01, 0x4A, 0x96,     0x4A, 0x8E, 0xA0, 0xC8,
    0x4D, 0x71, 0x4F, 0xEA,     0xA4, 0xDE, 0x82, 0x3F,
    0xE8, 0x5A, 0x22, 0x4A,     0x4D, 0xD0, 0x48, 0xFA
};

sdk_ret_t
elba_barco_asym_ecc_point_mul_p256_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    uint8_t             x3[32];
    uint8_t             y3[32];

    ret = elba_barco_asym_ecc_point_mul_p256(p, n,
        xg, yg, a, b, xg, yg, k, x3, y3);

    return ret;
}

sdk_ret_t
elba_barco_asym_ecdsa_sig_gen_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    uint8_t             r[32];
    uint8_t             s[32];

    /* Using k for private key (dA)     */
    /* Using n for the message hash h   */

    ret = elba_barco_asym_ecdsa_p256_sig_gen(-1, p, n,
                                              xg, yg, a, b,
                                              k/*da*/, k, n/*h*/, r, s,
                                              false, NULL);
    return ret;
}

uint8_t r[] = {
    0xa9, 0xc0, 0xbe, 0xa4,     0xb0, 0x64, 0xc6, 0x20,
    0xdb, 0x60, 0x16, 0x72,     0x41, 0x41, 0xfb, 0x55,
    0x22, 0xca, 0x97, 0x4d,     0x48, 0x3f, 0x19, 0x26,
    0x54, 0xbb, 0x21, 0xd1,     0xcb, 0xb4, 0xa4, 0x8f
};

uint8_t s[] = {
    0xed, 0x5e, 0x31, 0x00,     0xeb, 0x28, 0x40, 0xac,
    0xe3, 0xed, 0xe4, 0xdc,     0x12, 0xd9, 0xa6, 0x1e,
    0x1a, 0x7c, 0xa4, 0xb8,     0x26, 0x10, 0xe9, 0xb0,
    0x9e, 0x7b, 0x93, 0x58,     0x19, 0xf8, 0xbd, 0x52
};

sdk_ret_t
elba_barco_asym_ecdsa_sig_verify_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;

    ret = elba_barco_asym_ecdsa_p256_sig_verify(p, n,
                                                 xg, yg, a, b, xq, yq,
                                                 r, s, n/*h*/, false, NULL);
    return ret;
}

/*----------------------- RSA Tests -----------------------*/
/* Public Key */
uint8_t modulo_n[] = {
    0xb2, 0x5b, 0x74, 0x04,     0x4e, 0x0b, 0xd1, 0xc0,
    0x36, 0x3f, 0x75, 0xb1,     0x77, 0x0d, 0xa7, 0xbf,
    0x0b, 0x5d, 0x1a, 0x2a,     0x3d, 0x2b, 0xe0, 0xb9,
    0xb7, 0xb3, 0x7b, 0xd2,     0x0b, 0x9c, 0x3e, 0xf1,
    0xbb, 0x80, 0x66, 0xe2,     0x51, 0x31, 0xd9, 0xcb,
    0x03, 0x61, 0x5d, 0xad,     0x44, 0xbf, 0x11, 0x8a,
    0x41, 0xdd, 0x2b, 0x08,     0x07, 0xbb, 0xb1, 0xa5,
    0x03, 0x99, 0x14, 0x82,     0xe1, 0x41, 0xe7, 0x4d,
    0x4c, 0x69, 0x7e, 0x28,     0x40, 0xfe, 0xe6, 0xba,
    0xe7, 0x2f, 0xd7, 0x5c,     0x9e, 0xd6, 0x43, 0x06,
    0x4a, 0x57, 0x7a, 0x48,     0x05, 0xf7, 0x13, 0x7e,
    0xdb, 0x97, 0x43, 0x25,     0x4d, 0xa6, 0x89, 0xe7,
    0x67, 0xb4, 0x47, 0x87,     0x9a, 0x18, 0xe5, 0xa6,
    0x9c, 0x63, 0x97, 0x53,     0x1a, 0x61, 0xcf, 0x7f,
    0xe7, 0xe5, 0x8a, 0xb4,     0x23, 0x08, 0x52, 0xf0,
    0x61, 0xf2, 0x19, 0xe9,     0x5c, 0x35, 0xb6, 0x98,
    0x7d, 0x72, 0x8d, 0x24,     0x48, 0xc7, 0xe0, 0x8a,
    0x5d, 0xc7, 0xe3, 0xb3,     0x3b, 0x30, 0xde, 0x5a,
    0x63, 0x63, 0x5e, 0x0b,     0x36, 0x7a, 0x11, 0xf8,
    0x9b, 0x85, 0x9c, 0xac,     0xf9, 0xb0, 0x5f, 0xaf,
    0xbd, 0xc1, 0xab, 0xbe,     0x78, 0x92, 0x29, 0x44,
    0x76, 0x82, 0x42, 0x68,     0x6a, 0xde, 0xdc, 0x9e,
    0x52, 0x7c, 0xe1, 0xe8,     0xc8, 0x81, 0xb8, 0x17,
    0xaf, 0xc4, 0xe0, 0xbc,     0x93, 0x97, 0xce, 0x5d,
    0x0b, 0xbf, 0x04, 0xc7,     0xfa, 0x03, 0x65, 0x60,
    0x6f, 0xcb, 0x59, 0x51,     0x73, 0x54, 0x82, 0xad,
    0xf0, 0x26, 0xe3, 0x62,     0xe8, 0xda, 0xe2, 0xd0,
    0xf7, 0x5c, 0x77, 0xed,     0x22, 0xe3, 0x13, 0x0c,
    0x1a, 0xd0, 0xcc, 0xb0,     0xaa, 0x4f, 0xfe, 0x7b,
    0x20, 0x83, 0xf1, 0xaf,     0x93, 0xfc, 0xa8, 0x7f,
    0x14, 0x1f, 0x82, 0x30,     0x2c, 0xfe, 0x4e, 0x1b,
    0x0e, 0xf0, 0x00, 0xbe,     0x20, 0x0e, 0xdf, 0x81
};

uint8_t e[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x01, 0x00, 0x01,
};

uint8_t m[] = {
    0x01, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x01,
};

/* Private Key */
uint8_t rsa_p[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0xed, 0x9b, 0xb0, 0x9f,     0x30, 0x83, 0xc4, 0x84,
    0xfb, 0x0e, 0xe2, 0xc7,     0x33, 0xbf, 0xe9, 0x0a,
    0x49, 0x20, 0x96, 0x5a,     0x2e, 0x56, 0x8c, 0xb2,
    0x7e, 0x74, 0x14, 0x24,     0xce, 0x90, 0x22, 0x1f,
    0x74, 0xbe, 0x73, 0xae,     0xbf, 0x81, 0x5b, 0x0f,
    0xbb, 0xac, 0x90, 0xf7,     0xe2, 0xa5, 0x98, 0x23,
    0x55, 0x80, 0x8e, 0x97,     0x91, 0x56, 0x5a, 0xfa,
    0xc0, 0x2c, 0x54, 0x47,     0x7d, 0xa1, 0x2f, 0x61,
    0x91, 0xae, 0x4c, 0x9f,     0xb0, 0x2d, 0x26, 0x12,
    0x9e, 0x2a, 0xcf, 0x54,     0xa2, 0x3f, 0x1e, 0x6b,
    0x90, 0xca, 0x6f, 0x7b,     0x62, 0xd6, 0x17, 0xf4,
    0x04, 0xfb, 0x5d, 0x39,     0x9b, 0xd9, 0x3d, 0xf6,
    0xc3, 0x0e, 0xfa, 0x9a,     0x60, 0xf4, 0x41, 0x35,
    0xad, 0xcd, 0x46, 0x56,     0xa9, 0x30, 0x53, 0x47,
    0xd5, 0xf8, 0x4b, 0x43,     0xd7, 0xd9, 0x90, 0x03,
    0xe6, 0x6b, 0xc5, 0x1e,     0xf3, 0xc9, 0xd4, 0x3f
};

uint8_t rsa_q[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0xc0, 0x29, 0xae, 0x20,     0x1c, 0xb8, 0x25, 0xe2,
    0xcf, 0xd9, 0xab, 0xc4,     0x3e, 0xbb, 0x6f, 0x9a,
    0x43, 0x6c, 0xf9, 0x49,     0x4e, 0xd7, 0x64, 0x3e,
    0x95, 0xb8, 0xa0, 0x27,     0x88, 0x81, 0x98, 0x97,
    0x98, 0x13, 0xaf, 0x37,     0xf4, 0x7d, 0x8d, 0xac,
    0xc2, 0xfa, 0x8a, 0xda,     0xaa, 0x7f, 0xe8, 0x5b,
    0xd0, 0x65, 0x3a, 0x46,     0xdc, 0x18, 0x1a, 0xb7,
    0x55, 0x2d, 0x2a, 0xac,     0x68, 0x53, 0xb1, 0xe0,
    0x8d, 0x30, 0x4a, 0x5e,     0x30, 0x56, 0x13, 0x64,
    0x0e, 0xe0, 0x5b, 0xae,     0x38, 0x65, 0x4c, 0x1d,
    0x43, 0x03, 0xa8, 0xf3,     0x2b, 0x9b, 0x9d, 0x4b,
    0x17, 0xdd, 0xf1, 0x4c,     0xc8, 0x7b, 0x2c, 0x77,
    0xba, 0xd7, 0x1e, 0xf8,     0x6b, 0xb0, 0xda, 0xd7,
    0xd5, 0xa7, 0x03, 0xe4,     0x95, 0x26, 0xe2, 0x21,
    0x7b, 0xcb, 0x92, 0x38,     0x5b, 0x60, 0x25, 0xbe,
    0x42, 0xe0, 0x1b, 0x6d,     0x4b, 0xa3, 0x5c, 0x3f
};

uint8_t rsa_d[] = {
    0x72, 0x37, 0xb0, 0xbf,     0x44, 0xff, 0xba, 0xae,
    0x1d, 0xcf, 0x5b, 0xde,     0x6f, 0x00, 0x56, 0xa9,
    0x38, 0x6c, 0xc1, 0xe1,     0xc4, 0xd4, 0xc1, 0x90,
    0x0d, 0x3d, 0x2a, 0x91,     0x23, 0x90, 0x46, 0x9a,
    0xe5, 0x59, 0x60, 0x09,     0x94, 0xb7, 0x98, 0xe2,
    0xb2, 0x62, 0x7a, 0xec,     0x07, 0xf7, 0x58, 0x13,
    0x33, 0x04, 0xa0, 0x96,     0xfe, 0xe4, 0xca, 0xe9,
    0x82, 0xb9, 0x58, 0x72,     0x4c, 0x30, 0xb9, 0x20,
    0x3e, 0x4b, 0xdc, 0x57,     0x88, 0xef, 0xf3, 0xf0,
    0x43, 0x36, 0xd6, 0xf2,     0xe0, 0x61, 0x14, 0x01,
    0x06, 0x40, 0xa8, 0xf5,     0x50, 0xa4, 0x9e, 0x5e,
    0x81, 0xdf, 0x87, 0x47,     0x6f, 0x47, 0xb4, 0x4e,
    0x75, 0x91, 0xf4, 0xb0,     0xb9, 0x15, 0x32, 0x94,
    0x14, 0xd8, 0x8e, 0x42,     0xd0, 0xc5, 0x4c, 0x6d,
    0x7b, 0xa2, 0xfa, 0xc5,     0x4b, 0x1d, 0xfc, 0x87,
    0x26, 0x22, 0x35, 0x47,     0xc5, 0x1e, 0x3c, 0xb4,
    0xad, 0x80, 0x19, 0x12,     0xe0, 0x28, 0x0c, 0x52,
    0xad, 0xbd, 0xc9, 0xb9,     0xb5, 0x85, 0xd1, 0x7f,
    0xd6, 0x79, 0x8b, 0x39,     0xcf, 0x67, 0x1a, 0x4b,
    0xc8, 0xf8, 0x3d, 0xb0,     0x0c, 0xd4, 0x0d, 0x8f,
    0x99, 0x37, 0x7c, 0x69,     0xb8, 0xe0, 0x17, 0xd6,
    0x3a, 0xfa, 0xa3, 0xe6,     0x1b, 0x5c, 0xa5, 0xca,
    0xec, 0xf2, 0x42, 0x3d,     0xe8, 0x7f, 0x17, 0xe9,
    0x1b, 0x0f, 0xa3, 0x4a,     0x31, 0x30, 0x02, 0x29,
    0x59, 0x02, 0x8c, 0x2c,     0x7d, 0x52, 0x20, 0x4d,
    0x0d, 0xc3, 0xa6, 0xa3,     0xfc, 0x91, 0xe0, 0x94,
    0x7b, 0x67, 0xbd, 0x68,     0xe8, 0x31, 0x2c, 0xd6,
    0x97, 0x99, 0xa1, 0x3c,     0xc3, 0xd1, 0x75, 0xd7,
    0xa1, 0x72, 0x87, 0x8c,     0x1b, 0xb5, 0x60, 0x55,
    0x93, 0xdc, 0x3f, 0x7f,     0x77, 0x8d, 0x81, 0x7a,
    0xb0, 0xc1, 0xa1, 0x63,     0x04, 0x13, 0x4b, 0x00,
    0xba, 0xe6, 0xf5, 0x1b,     0x1b, 0xe0, 0x4e, 0xc5
};

uint8_t rsa_dp[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x1a, 0xb7, 0x7b, 0xd7,     0x9a, 0x73, 0xe6, 0x7f,
    0xf1, 0x5e, 0xce, 0x1f,     0x09, 0xf1, 0x95, 0x39,
    0x83, 0xd9, 0x77, 0x2e,     0x72, 0xb1, 0x66, 0xa6,
    0x97, 0x53, 0x64, 0x04,     0x73, 0x79, 0x7f, 0x6c,
    0xbc, 0x0a, 0xc2, 0x25,     0x2f, 0x01, 0x53, 0x84,
    0xe4, 0x5c, 0x55, 0xfc,     0x99, 0x6e, 0x77, 0x39,
    0xd9, 0xde, 0x57, 0xaa,     0x31, 0x3c, 0x5d, 0x84,
    0x7e, 0x61, 0x3d, 0xa4,     0xc0, 0x3a, 0x84, 0x82,
    0x5b, 0x08, 0x17, 0x33,     0x89, 0x72, 0xba, 0x2a,
    0x33, 0xc0, 0xaa, 0x89,     0x60, 0xa8, 0xea, 0x39,
    0xbc, 0x11, 0x17, 0x11,     0xef, 0x9e, 0x15, 0x19,
    0x6a, 0x09, 0xfd, 0x84,     0x81, 0xc2, 0x9e, 0x96,
    0x05, 0x7e, 0xc4, 0xac,     0xe1, 0x23, 0xf5, 0xc5,
    0x1c, 0x62, 0xcd, 0x7a,     0xe7, 0x11, 0x38, 0xfc,
    0x05, 0xd2, 0x22, 0x5c,     0x61, 0x83, 0xe4, 0x0e,
    0x9c, 0x35, 0x17, 0xf8,     0x0f, 0xff, 0x6c, 0xaf
};

uint8_t rsa_dq[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x14, 0x66, 0xd8, 0x73,     0xd4, 0x58, 0xc0, 0xbc,
    0xf9, 0xf0, 0x54, 0x4a,     0x3b, 0x96, 0xce, 0xdc,
    0x83, 0xf8, 0x17, 0xe7,     0x6e, 0x95, 0x73, 0xb1,
    0x29, 0x58, 0x36, 0xb8,     0xbb, 0xc7, 0x76, 0x99,
    0xf1, 0xad, 0x75, 0x56,     0xed, 0x80, 0x3f, 0x00,
    0x6e, 0x9b, 0x07, 0x0e,     0xfc, 0x37, 0x24, 0x46,
    0x4b, 0x33, 0xd4, 0x22,     0x1d, 0xcf, 0xf9, 0x56,
    0x29, 0x96, 0xe8, 0x06,     0xf1, 0xf4, 0xa0, 0xd8,
    0x04, 0x65, 0x72, 0x1f,     0xd9, 0xe5, 0xe9, 0x9d,
    0x1f, 0xef, 0x36, 0x0c,     0xa3, 0x34, 0x2e, 0x06,
    0x95, 0x4c, 0xd8, 0x2d,     0x29, 0x1e, 0x16, 0x6f,
    0x18, 0x93, 0x99, 0xc0,     0xdb, 0x30, 0x28, 0xa2,
    0x75, 0x95, 0xcd, 0x55,     0xf6, 0xa9, 0x0a, 0x33,
    0x0c, 0x1d, 0xf5, 0x4d,     0xd9, 0x80, 0x0d, 0x56,
    0x1c, 0xbc, 0x9a, 0x43,     0x9d, 0x7f, 0xf1, 0xc1,
    0xe7, 0x7b, 0xbf, 0xad,     0xb4, 0xf0, 0x1d, 0x25
};

uint8_t rsa_qinv[] = {
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,
    0x5e, 0x5d, 0x41, 0x73,     0xdb, 0xc4, 0x12, 0xc3,
    0x7f, 0xac, 0x73, 0xd0,     0x8e, 0x76, 0xfe, 0x1d,
    0x9d, 0xd0, 0x88, 0xa6,     0x4c, 0x67, 0x3a, 0x68,
    0x98, 0x04, 0x7b, 0xe6,     0xe8, 0x71, 0x0b, 0x5f,
    0x79, 0xa9, 0xe4, 0xc1,     0xa1, 0x5f, 0xa9, 0x28,
    0xe1, 0x98, 0xc6, 0x27,     0x70, 0x7f, 0x52, 0x1e,
    0xfe, 0x56, 0xd6, 0x3c,     0x42, 0x0f, 0x7d, 0x73,
    0xeb, 0x25, 0x3b, 0xb7,     0x01, 0xc9, 0x09, 0x6f,
    0x98, 0x6a, 0x60, 0x73,     0xe9, 0xe4, 0x6e, 0x71,
    0xa2, 0x66, 0x27, 0xd8,     0xb9, 0x56, 0x44, 0x5f,
    0xd4, 0x97, 0x35, 0x67,     0x94, 0x28, 0xbf, 0xaa,
    0xcd, 0x2d, 0x8a, 0xf3,     0x7a, 0xd0, 0x15, 0x80,
    0x73, 0x12, 0xce, 0x0b,     0x9d, 0x53, 0x8f, 0x66,
    0xcd, 0x9c, 0x37, 0x1b,     0x41, 0xf2, 0x3b, 0x8b,
    0xa6, 0x8d, 0xa4, 0xb8,     0x3e, 0xf7, 0xc9, 0xaf,
    0xca, 0x5a, 0xf2, 0xd5,     0x35, 0x6c, 0x2d, 0x65
};

uint8_t             out_c[256];

sdk_ret_t
elba_barco_asym_rsa2k_encrypt_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    ret = elba_barco_asym_rsa2k_encrypt(modulo_n, e, m, out_c, false, NULL);

    return ret;
}

sdk_ret_t
elba_barco_asym_rsa2k_decrypt_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    uint8_t             out_m[256];

    ret = elba_barco_asym_rsa2k_decrypt(modulo_n, rsa_d, out_c, out_m);

    return ret;
}

sdk_ret_t
elba_barco_asym_rsa2k_crt_decrypt_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    uint8_t             out_m[256];

    ret = elba_barco_asym_rsa2k_crt_decrypt(-1, rsa_p, rsa_q, rsa_dp,
                                             rsa_dq, rsa_qinv, out_c,
                                             out_m, false, NULL);

    return ret;
}

uint8_t             out_s[256];
sdk_ret_t
elba_barco_asym_rsa2k_sig_gen_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;

    ret = elba_barco_asym_rsa2k_sig_gen(-1, modulo_n, rsa_d,
                                         m /* h */, out_s, false, NULL);

    return ret;
}

sdk_ret_t
elba_barco_asym_rsa2k_sig_verify_test (void)
{
    sdk_ret_t           ret = SDK_RET_OK;

    ret = elba_barco_asym_rsa2k_sig_verify(modulo_n, e,
            m /* h */, out_s);

    return ret;
}

sdk_ret_t
elba_barco_asym_run_tests (void)
{
    sdk_ret_t       ret = SDK_RET_OK;

    ret = elba_barco_asym_ecc_point_mul_p256_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_ecc_point_mul_p256_test failed");
    }
    ret = elba_barco_asym_ecdsa_sig_gen_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_ecdsa_sig_gen_test failed");
    }
    ret = elba_barco_asym_ecdsa_sig_verify_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_ecdsa_sig_verify_test failed");
    }
    ret = elba_barco_asym_rsa2k_encrypt_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_rsa2k_encrypt failed");
    }
    ret = elba_barco_asym_rsa2k_decrypt_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_rsa2k_encrypt failed");
    }
    ret = elba_barco_asym_rsa2k_crt_decrypt_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_rsa2k_encrypt failed");
    }
    ret = elba_barco_asym_rsa2k_sig_gen_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_rsa2k_sig_gen_test failed");
    }
    ret = elba_barco_asym_rsa2k_sig_verify_test();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("elba_barco_asym_rsa2k_sig_verify_test failed");
    }

    return ret;
}

}    // namespace elba
}    // namespace platform
}    // namespace sdk
#ifndef __HASH_H__
#define __HASH_H__

#include "rssks.h"

#define STATS                   1'000'000
#define DIST_THRESHOLD          0.1

#define CORES                   8
#define HASH_TO_CORE(hash)      (hash % CORES)

#define BYTE_FROM_BYTES(bb, b)  ((bb)[b] & 0xff)
#define BIT_FROM_BYTE(b, i)     (((b) >> (i)) & 1)
#define BIT_FROM_KEY(b, k)      (BIT_FROM_BYTE(k[(b) / 8], 7 - ((b) % 8)))

#define _4_RSSKS_BYTE_T_TO_UINT32_T(v) ((uint32_t) (\
    ((BYTE_FROM_BYTES((v), 0)) << 24) + ((BYTE_FROM_BYTES((v), 1)) << 16) + \
    ((BYTE_FROM_BYTES((v), 2)) <<  8) + ((BYTE_FROM_BYTES((v), 3)) <<  0) ))

#define _3_RSSKS_BYTE_T_TO_UINT32_T(v) ((uint32_t) (\
    ((BYTE_FROM_BYTES((v), 0)) << 16) + ((BYTE_FROM_BYTES((v), 1)) <<  8) + \
    ((BYTE_FROM_BYTES((v), 2)) <<  0) ))

#define _2_RSSKS_BYTE_T_TO_UINT32_T(v) ((uint16_t) (\
    ((BYTE_FROM_BYTES((v), 0)) <<  8) + ((BYTE_FROM_BYTES((v), 1)) <<  0) ))

typedef unsigned packet_fields_t;

// big-endian version
unsigned packet_field_offset_be_bits(RSSKS_cfg_t cfg, RSSKS_pf_t pf);

// little-endian version
unsigned packet_field_offset_le_bits(RSSKS_cfg_t cfg, RSSKS_pf_t pf);

// DEBUG
void print_key(RSSKS_key_t k);
void print_headers(RSSKS_cfg_t cfg, RSSKS_headers_t headers);
void print_hash_input(RSSKS_cfg_t cfg, RSSKS_in_t hi);
void print_hash_output(RSSKS_out_t output);

RSSKS_headers_t rand_headers();
void rand_key(RSSKS_key_t key);
void zero_key(RSSKS_key_t key);
bool is_zero_key(RSSKS_key_t key);
RSSKS_headers_t header_from_constraints(RSSKS_headers_t h);
RSSKS_out_t hash(RSSKS_cfg_t cfg, RSSKS_key_t k, RSSKS_headers_t h);
bool k_test_dist(RSSKS_cfg_t cfg, RSSKS_key_t k);

#endif
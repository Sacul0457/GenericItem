#ifndef HASH
#define HASH

#include "complex_generic_item.h"
#define IS_MATCHING_KEY(key, other_key, hash, other_hash) ((hash) == (other_hash) && item_compare((key), (other_key)))


#define EMPTY 0x80
#define TOMBSTONE 0xFE
#define FINGERPRINT(hash) (((hash) >> 57) & 0x7F)

#endif
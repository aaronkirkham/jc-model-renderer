#ifndef JCMR_GAME_NAME_HASH_LOOKUP_H_HEADER_GUARD
#define JCMR_GAME_NAME_HASH_LOOKUP_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
void        load_namehash_lookup_table();
const char* find_in_namehash_lookup_table(u32 namehash);
} // namespace jcmr

#endif // JCMR_GAME_NAME_HASH_LOOKUP_H_HEADER_GUARD

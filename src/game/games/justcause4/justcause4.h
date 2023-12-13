#ifndef JCMR_JUSTCAUSE4_JUSTCAUSE4_H_HEADER_GUARD
#define JCMR_JUSTCAUSE4_JUSTCAUSE4_H_HEADER_GUARD

#include "game/game.h"

namespace jcmr::game
{
struct JustCause4 : IGame {
    static JustCause4* create(App& app);
    static void        destroy(JustCause4* inst);
};
} // namespace jcmr::game

#endif // JCMR_JUSTCAUSE4_JUSTCAUSE4_H_HEADER_GUARD

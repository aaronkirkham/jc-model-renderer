#ifndef JCMR_JUSTCAUSE3_JUSTCAUSE3_H_HEADER_GUARD
#define JCMR_JUSTCAUSE3_JUSTCAUSE3_H_HEADER_GUARD

#include "game/game.h"

namespace jcmr::game
{
struct JustCause3 : IGame {
    static JustCause3* create(App& app);
    static void        destroy(JustCause3* inst);

    const char* get_title() const override { return "Just Cause 3"; }
};
} // namespace jcmr::game

#endif // JCMR_GAMES_JUSTCAUSE3_JUSTCAUSE3_H_HEADER_GUARD

#include "pch.h"

#include "game.h"

#include "game/games/justcause3/justcause3.h"
#include "game/games/justcause4/justcause4.h"

namespace jcmr
{
IGame* IGame::create(EGame game_type, App& app)
{
    switch (game_type) {
        case EGame::EGAME_JUSTCAUSE3: return game::JustCause3::create(app);
        case EGame::EGAME_JUSTCAUSE4: return game::JustCause4::create(app);
    }

    ASSERT(false);
    return nullptr;
}

void IGame::destroy(IGame* game)
{
    ASSERT(game != nullptr);
    delete game;
}
} // namespace jcmr
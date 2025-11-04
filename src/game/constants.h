//
// Created by AWAY on 25-11-4.
//

#pragma once

namespace uno {
    enum class DEAL_CARD_REASON {
        normal,
        draw,
        bad_uno,
        report,
        last_card
    };

    enum class PLAYER_LEFT_REASON {
        normal,
        offline_kick,
        kicked_by_owner
    };

};
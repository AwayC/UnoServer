//
// Created by AWAY on 25-11-4.
//

#pragma once

namespace uno {
    enum class deal_card_reason {
        normal,
        draw,
        bad_uno,
        report,
        last_card
    };

    enum class player_left_reason {
        normal,
        offline_kick,
        kicked_by_owner
    };

};
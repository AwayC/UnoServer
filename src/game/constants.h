//
// Created by AWAY on 25-11-4.
//

#pragma once

namespace uno {
    enum class deal_card_reason {
        normal, // 正常发牌
        draw, // 罚牌
        bad_uno, // 乱喊UNO
        report, // 没喊UNO被举报
        last_card // 以功能牌结束需要加时
    };

    enum class player_left_reason {
        normal, // 玩家正常退出
        offline_kick, // 断线被踢出
        kicked_by_owner // 被房主踢出
    };

};
//
// Created by AWAY on 25-11-19.
//

#include "room.h"

namespace uno
{
    RoomManager::RoomSnapshot RoomManager::get_snapshot(RoomPtr room, int uid)
    {
        RoomSnapshot ret = {
            .id = room->id,
            .state = room->state,
            .title = room->title,
            .owner = room->owner,
            .max_players = room->max_players,
            .curr_players = room->curr_players,
            .players = {},
            .last_win = room->last_win,
            .game_state = room->game_state,
            .cursor = room->cursor,
            .direction = room->direction,
            .dealer = room->dealer,
            .timer = room->timer,
            .last = room->last,
            .last_chg_color = room->last_chg_color,
            .draw = room->draw,
            .last_can_report = room->last_can_report,
            .can_play_ahead = room->can_play_ahead,

            .my_cards = {}
        };

        // 玩家简化数据
        for (auto& [uid, player] : room->players)
        {
            ret.players[uid] = get_player_snapshot(room, uid);
        }

        // 玩家手牌
        if (uid > 0)
        {
            auto player = room->players.find(uid);
            assert(player != room->players.end());
            ret.my_cards = player->second.rest;
        }

        return ret;
    }

    RoomManager::PlayerSnapshot RoomManager::get_player_snapshot(RoomPtr room, int uid)
    {
        auto it = room->players.find(uid);
        assert(it != room->players.end());

        auto& player = it->second;
        PlayerSnapshot ret = {
            .nick = player.nick,
            .email = player.email,
            .rest_count = static_cast<int>(player.rest.size()),
            .ready = player.ready,
            .offline = player.offline,
        };

        return ret;
    }


}
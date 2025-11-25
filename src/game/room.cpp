//
// Created by AWAY on 25-11-19.
//

#include "room.h"

constexpr int CARD_SET_COUNT = 3; // 固定发三套牌
constexpr int PLAYER_START_CARD_COUNT = 7; // 玩家起手牌数量
constexpr int DEALER_START_CARD_EXT_COUNT = 2; // 庄稼额外起手牌数量

// 以Tick计，一个Tick 100毫秒
constexpr int TIMER_DEALING_INTERVAL = 5; // 发牌间隔
constexpr int TIMER_SELECT_FIRST_CARD_INTERVAL = 2; // 选取第一张牌的时间间隔
constexpr int TIMER_PLAYER_THINKING = 20 * 10; // 玩家思考时间

constexpr int PUNISH_FOR_BAD_UNO = 2; // 乱喊UNO罚两张
constexpr int PUNISH_FOR_LAST_FUNC_CARD = 2; //以功能牌结尾，罚两张
constexpr int PUNISH_FOR_NO_UNO = 2; // 没有喊UNO罚牌

constexpr int PLAYER_AUTO_TICK_OFFLINE_TIME = 1 * 60 * 1000; // 超过1分钟自动踢出房间
constexpr int PLAYER_AUTO_PLAY_OFFLINE_TIME = 1 * 60 * 1000; // 超过1分钟自动托管
constexpr int PLAYER_AUTO_PLAY_THINKING_TIME = TIMER_PLAYER_THINKING * 0.85; // 自动托管时减少思考时间

constexpr int OWNER_UPDATE_SEAT_INTERVAL = 10 * 1000; // 允许房主重排座位的倒计时
constexpr int PLAYER_USE_VOICE_INTERVAL = 10 * 1000; // 玩家使用音效的倒计时

namespace uno
{
    RoomSnapshot RoomManager::get_snapshot(RoomPtr room, int uid)
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

    PlayerSnapshot RoomManager::get_player_snapshot(RoomPtr room, int uid)
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

    void RoomManager::game_start(RoomPtr room)
    {
        assert(room->state == Room::State::idle);
        assert(room->curr_players >= 2);

        // 初始化
        room->game_state = Room::GameState::idle;
        room->deal_turn = 0;
        room->direction = -1;
        room->direction = 1;
        room->dealer = -1;
        room->timer = 0;
        room->heap.clear();
        room->last = 0;
        room->last_chg_color = 0;
        room->draw = 0;
        room->last_can_report = -1;
        room->can_play_ahead = false;
        room->history.clear();
        for (auto& [uid, player]: room->players)
        {
            player.rest.clear();
            player.ready = false;
        }

        // 决定庄家
        if (room->last_win != -1)
        {
            // 1. 如果上场有赢家，则直接把赢家设置为庄家
            room->dealer = room->last_win;
        } else
        {
            room->dealer = helper::random_select(room->seq);
        }

        // 决定下家游标
        for (size_t i = 0;i < room->seq.size();i ++)
        {
            if (room->seq[i] == room->dealer)
            {
                room->cursor = i;
                break;
            }
        }

        assert(room->cursor != -1);
        assert(room->dealer != -1);

        // 决定发牌方向
        room->direction = -room->last_direction;
        assert(room->direction == 1 || room->direction == -1);
        assert(room->curr_players == room->seq.size());

        // 决定发牌方向
        for (size_t i = 0;i < CARD_SET_COUNT;i ++)
        {
            for (auto c : card::CARD_SET)
            {
                room->heap.push_back(c);
            }
        }
        assert(room->heap.size() >= (room->curr_players * PLAYER_START_CARD_COUNT + DEALER_START_CARD_EXT_COUNT));

        // 洗牌
        helper::random_shuffle(room->heap);

        // 准备游戏状态
        room->state = Room::State::playing;
        room->game_state = Room::GameState::dealing;
        room->timer = TIMER_DEALING_INTERVAL;
    }

    void RoomManager::game_over(RoomPtr room, int uid)
    {
        // 状态变动
        room->last_win = uid;
        room->last_direction = room->direction;
        room->state = Room::State::idle;

        // 清空游戏状态
        room->game_state = Room::GameState::idle;
        room->deal_turn = 0;
        room->cursor = -1;
        room->direction = 1;
        room->dealer = -1;
        room->timer = 0;
        room->heap.clear();
        room->last = 0;
        room->last_chg_color = 0;
        room->draw = 0;
        room->last_can_report = -1;
        room->can_play_ahead = false;
        room->history.clear();
        for (auto& [uid, player]: room->players)
        {
            player.rest.clear();
            player.ready = false;
        }

        // 通知玩家获胜
        broadcast_event(room, -1, 'player_win', uid);

        // 发送统计数据到lobby
        for (auto& [uid, player]: room->players)
        {
            auto stat = room->stater.get_stat(uid);
            CALL_S2S("round_flow", uid, );
        }

        // 展示分数面板
        broadcast_event(room, -1, 'game_stat', uid);

    }


}
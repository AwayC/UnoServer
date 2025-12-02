//
// Created by AWAY on 25-11-19.
//

#include "room.h"
#include "../core/ssmgr.h"

#define SSMGR    Ssmgr::instance()

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

constexpr int PLAYER_AUTO_TICK_OFFLINE_TIME = 1 * 60 * 100; // 超过1分钟自动踢出房间
constexpr int PLAYER_AUTO_PLAY_OFFLINE_TIME = 1 * 60 * 100; // 超过1分钟自动托管
constexpr int PLAYER_AUTO_PLAY_THINKING_TIME = TIMER_PLAYER_THINKING * 0.85; // 自动托管时减少思考时间

constexpr int OWNER_UPDATE_SEAT_INTERVAL = 10 * 1000; // 允许房主重排座位的倒计时
constexpr int PLAYER_USE_VOICE_INTERVAL = 10 * 1000; // 玩家使用音效的倒计时

namespace uno
{
    lept_value RoomManager::get_snapshot(RoomPtr room, int uid)
    {
        lept_value ret = {
#define KEY_VAL_INT(val) {#val, (int)room->val}
#define KEY_VAL_STR(val) {#val, room->val}

            KEY_VAL_INT(id),
            KEY_VAL_INT(state),
            KEY_VAL_STR(title),
            KEY_VAL_INT(owner),
            KEY_VAL_INT(max_players),
            KEY_VAL_INT(curr_players),
            KEY_VAL_INT(last_win),
            KEY_VAL_INT(game_state),
            KEY_VAL_INT(cursor),
            KEY_VAL_INT(direction),
            KEY_VAL_INT(dealer),
            KEY_VAL_INT(timer),
            KEY_VAL_INT(last),
            KEY_VAL_INT(last_chg_color),
            KEY_VAL_INT(draw),
            KEY_VAL_INT(last_can_report),
            KEY_VAL_INT(can_play_ahead),

#undef KEY_VAL_INT
        };

        // 玩家简化数据
        lept_value::object_t players;
        for (auto& [uid, player] : room->players)
        {
            players.emplace(std::to_string(uid), get_player_snapshot(room, uid));
        }

        // 玩家手牌
        lept_value::array_t my_cards;
        if (uid >= 0)
        {
            auto player = room->players.find(uid);
            assert(player != room->players.end());
            for (auto& card : player->second.rest)
            {
                my_cards.emplace_back(lept_value(card));
            }
        }

        lept_value::array_t seq;
        for (auto& u: room->seq)
        {
            seq.emplace_back(lept_value(u));
        }

        ret["seq"] = lept_value(std::move(seq));
        ret["my_cards"] = lept_value(std::move(my_cards));
        ret["players"] = lept_value(std::move(players));

        return ret;
    }

    lept_value RoomManager::get_player_snapshot(RoomPtr room, int uid)
    {
        auto it = room->players.find(uid);
        assert(it != room->players.end());

        auto& player = it->second;
        lept_value ret = {
            {"nick", player.nick},
            {"email", player.email},
            {"rest_count", (int)player.rest.size()},
            {"ready", player.ready},
            {"offline", player.offline},
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
        broadcast_event(room, -1, "player_win", uid);
        call_stat_event(false, room, -1, -1, "player_win", uid);

        // 发送统计数据到lobby
        for (auto& [uid, player]: room->players)
        {
            auto stat = room->stater.get_stat(uid);
            CALL_S2S("round_flow", uid, room->stater.get_stat(uid));
        }

        // 展示分数面板
        broadcast_event(room, -1, "game_stat",
                room->stater.get_display_stat(),
                uid, room->stater.get_winner_stealer());
    }

    void RoomManager::game_dismiss(RoomPtr room)
    {
        // 状态变动
        room->last_win = -1;
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

        // 清理玩家状态
        for (auto& [uid, player] : room->players)
        {
            auto session = SSMGR.find_session(uid);
            if (session && session->state() == Session::State::gaming &&
                session->data()["room_id"].get_integer() == room->id)
            {
                session->set_dirty();
                session->set_state(Session::State::logged);
            }
        }

        // 删除房间
        m_rooms.erase(room->id);

        // 通知玩家房间解散
        broadcast_event(room, -1, "game_dismiss");
    }

    void RoomManager::game_player_leave(RoomPtr room, int uid, player_left_reason reason)
    {
        auto it = room->players.find(uid);
        assert(it != room->players.end());
        auto& player = it->second;

        //todo: 发牌时不能退出
        assert(room->game_state != Room::GameState::dealing &&
            room->game_state != Room::GameState::dealing &&
            room->game_state != Room::GameState::select_first_card);

        // 玩家手牌归还牌堆
        if (room->state == Room::State::playing)
        {
            player.rest = helper::random_shuffle(player.rest);
            room->heap.insert(room->heap.begin(), player.rest.begin(), player.rest.end());
            player.rest.clear();
        }

        // 从房间列表删除
        for (size_t i = 0;i < room->seq.size();i ++)
        {
            if (room->seq[i] == uid)
            {
                // 如果游戏状态且刚好是下家，则跳过
                if (room->state == Room::State::playing && i == room->cursor)
                {
                    std::cout << "game_player_leave: move to next player, cursor: "
                        << i << ", direction: " << room->direction << std::endl;
                    game_cursor_move(room, -1, room->direction);
                    assert(room->cursor != i);

                    if (room->game_state >= Room::GameState::wait_player)
                    {
                        room->game_state = Room::GameState::wait_player;
                        room->timer = TIMER_DEALING_INTERVAL;
                    }
                }

                // 从seq中删除
                room->seq.erase(room->seq.begin() + i);

                // 调整cursor
                if (room->cursor > i)
                {
                    room->cursor = room->cursor - 1;
                }
                break;
            }
        }
        room->players.erase(uid);
        room->curr_players --;

        // 如果房间无人，则解散
        if (room->curr_players <= 0)
        {
            m_rooms.erase(room->id);
            return;
        }

        // 如果是房主，则转交给第一位
        if (room->owner == uid)
        {
            room->owner = room->seq[0];
            broadcast_event(room, uid, "owner_changed", room->owner);
        }

        // 如果是最后获胜的玩家，则清除
        if (room->last_win == uid)
        {
            room->last_win = -1;
        }

        // 通知玩家离开
        broadcast_event(room, uid, "player_left", (int)reason);
        call_stat_event(false, room, uid, uid, "player_left", reason);

        // 如果只有一人，则直接获胜
        if (room->state == Room::State::playing && room->curr_players == 1)
        {
            game_over(room, room->seq[0]);
            return ;
        }

        // 通知所有人刷新状态
        broadcast_event(room, -1, "thinking", room->seq[room->cursor], room->last,
            room->last_chg_color, room->direction, room->draw, room->last_can_report, room->can_play_ahead);
    }

    int RoomManager::game_cursor_move(RoomPtr room, int base, int step)
    {
        int cursor = room->cursor;

        // 如果需要换基
        if (base >= 0)
        {
            for (size_t i = 0;i < room->seq.size();i ++)
            {
                if (room->seq[i] == base)
                {
                    cursor = i;
                    break;
                }
            }
        }

        // 移动
        // fixme: 优化
        for (size_t i = 0;i < std::abs(step);i ++)
        {
            cursor += step > 0 ? 1 : step < 0 ? -1 : 0;
            if (cursor >= room->curr_players)
            {
                cursor = 0;
            } else if (cursor < 0)
            {
                cursor = room->curr_players - 1;
            }
        }

        room->cursor = cursor;
        return cursor;
    }

    card_t RoomManager::game_card_heap_deal(RoomPtr room)
    {
        if (room->heap.size() == 0)
        {
            room->heap = helper::random_shuffle(room->history);
            room->history.clear();
        }

        if (room->heap.size() == 0)
        {
            return -1;
        }

        auto ret = room->heap.back();
        room->heap.pop_back();
        return ret;
    }

    bool RoomManager::game_player_card_play(RoomPtr room, int uid, card_t c,
        bool with_uno, int chg_color)
    {
        auto it = room->players.find(uid);
        assert(it != room->players.end());
        auto& player = it->second;

        std::cout << "game_player_card_play: uid " << uid << ", card " << c
                << ", with_uno " << with_uno << ", chg_color " << chg_color << std::endl;

        int index = -1;
        for (size_t i = 0;i < player.rest.size();i ++)
        {
            if (player.rest[i] == c)
            {
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            std::cerr << "game_player_card_play: card " << c <<
                "not found with uid " << uid << std::endl;
            return false;
        }

        // 检查是否需要uno
        bool need_uno = false;
        if (player.rest.size() == 2)
        {
            need_uno = true;
        }

        // 检查功能牌是否需要提供更改花色，通常来说全色牌都需要
        int func = card::get_func(c);
        int color = card::get_color(c);
        if (color == card::COLOR_ALL)
        {
            if (chg_color !=  card::COLOR_RED && chg_color !=  card::COLOR_GREEN &&
                chg_color !=  card::COLOR_BLUE && chg_color !=  card::COLOR_YELLOW)
            {
                std::cerr << "game_player_card_play: need chg_color, uid " << uid << ", card " << c << std::endl;
                return false;
            }
        }

        // 是跟牌还是抢牌
        int cur = room->seq[room->cursor];
        bool ahead = (uid != cur); // 判断是否抢牌
        bool match = game_can_play_card(room, c, ahead);
        if (!match)
        {
            std::cerr << "game_player_card_play: cannot play card " << c
                << ", uid " << uid << ", last " << room->last  <<  std::endl;
            return false;
        }

        // --- 开始处理逻辑 ---
        // 牌堆变化
        player.rest.erase(player.rest.begin() + index);
        room->history.push_back(c);
        room->last = c;
        room->last_chg_color = 0;

        // 功能牌状态变化
        int direction = room->direction;
        if (color == card::COLOR_ALL)  // 全色卡总是需要给一个新的颜色
        {
            room->last_chg_color = chg_color;
        }
        if (func == card::FUNC_SKIP)
        {
            direction = direction * 2;
        } else if (func == card::FUNC_REVERSE)
        {
            direction = -direction;
            room->direction = direction;
        } else if (func == card::FUNC_DRAW2)
        {
            room->draw += 2;
        } else if (func == card::FUNC_DRAW4)
        {
            room->draw += 4;
        }

        // 发送通知
        int rest_count = static_cast<int>(player.rest.size());
        lept_value::array_t rest;
        for (auto& card : player.rest)
        {
            rest.emplace_back(card);
        }

        send_event(room, uid, uid, "card_play", c, need_uno, with_uno, rest_count, lept_value(std::move(rest)), ahead);
        call_stat_event(true, room,  uid, uid, "card_play", c, need_uno, with_uno, rest_count, player.rest, ahead);
        broadcast_event(room, uid, "card_play", c, need_uno, with_uno, rest_count, lept_value(), ahead);
        call_stat_event(false, room, uid, uid, "card_play", c, need_uno, with_uno, rest_count, std::vector<card_t>(), ahead);

        // 乱喊UNO, 罚牌
        if (!need_uno && with_uno)
        {
            std::cout << "game_player_card_play: uno is not needed for uid " << uid
                << "punish card" << std::endl;
            game_player_deal_card(room, uid, PUNISH_FOR_LAST_FUNC_CARD, deal_card_reason::last_card, -1);
        }

        // 如果最后一张是功能牌，则需要再抓两张
        if (rest_count <= 0 && card::is_func_card(c))
        {
            std::cout << "game_player_card_play: last card is funcion card, uid " << uid
                << ", card " << c << ", punish card" << std::endl;
            game_player_deal_card(room, uid, PUNISH_FOR_LAST_FUNC_CARD, deal_card_reason::last_card, -1);
        }

        // 如果无牌，则获胜（注意这里要重新判断）
        if (player.rest.size() == 0)
        {
            game_over(room, uid);
            return true;
        }

        // 移动到下家并刷新状态
        game_cursor_move(room, (ahead ? uid : -1), direction);
        room->game_state = Room::GameState::wait_player;
        room->timer = TIMER_PLAYER_THINKING;
        room->can_play_ahead = true; // 打完牌后总是可以别人抢

        // 如果当前玩家需要UNO, 但是没喊，则设置一个举报标记
        if (need_uno && !with_uno)
        {
            std::cout << "game_player_card_play: uid " << uid <<
                "can be reported" << std::endl;
            room->last_can_report = uid;
        } else
        {
            // 其他情况需要清理这个标记
            room->last_can_report = -1;
        }

        // 通知所有人
        broadcast_event(room, -1, "thinking", room->seq[room->cursor],
            room->timer, room->last, room->last_chg_color, room->direction,
            room->draw, room->last_can_report, room->can_play_ahead);
        return true;
    }

    // 玩家发牌动作（同时完成状态转移和消息发送）
    bool RoomManager::game_player_deal_card(RoomPtr room, int uid, int count, deal_card_reason req_reason, int req_by)
    {
        auto reason = req_reason;
        int cur = room->seq[room->cursor];
        auto it = room->players.find(uid);
        assert(it != room->players.end());
        auto& player = it->second;

        std::cout << "game_player_deal_card: uid " << ", count" << count <<
            ", req_reason " << (int)req_reason << ", req_by " << req_by << std::endl;

        // 正常由玩家触发的发牌请求，判断下状态
        if (req_reason == deal_card_reason::normal)
        {
            // 此时必须是下家才能主动发起请求
            if (cur != uid)
            {
                std::cerr << "game_player_deal_card: bad request, cur " << cur << ", uid " << uid << std::endl;
                return false;
            }

            // 此时不使用count值，依据状态设置count
            if (room->draw != 0)
            {
                // 检查是否罚牌
                count = room->draw;
                room->draw = 0;
                reason = deal_card_reason::draw;
            } else
            {
                count = 1;
            }
        } else
        {
            // 如果是系统触发发牌，则count必须有值
            assert(count > 0);
        }

        // 发牌逻辑
        std::vector<card_t> arr;
        for (size_t i = 0;i < count;i ++)
        {
            card_t c = game_card_heap_deal(room);
            if (c > 0)
            {
                arr.push_back(c);
                player.rest.push_back(c);
            } else
            {
                std::cerr << "game_player_deal_card: no card to deal, uid " << uid << std::endl;
            }
        }

        // 发消息
        lept_value::array_t lept_arr;
        for (auto& card : arr)
        {
            lept_arr.emplace_back(card);
        }

        send_event(room, uid, uid, "card_deal", (int)player.rest.size(), (int)arr.size(), lept_arr, (int)reason, req_by, room->cursor);
        call_stat_event(true, room, uid, uid, "card_deal", (int)player.rest.size(), arr.size(), arr, reason, req_by, room->cursor);
        broadcast_event(room, uid, "card_deal", (int)player.rest.size(), (int)arr.size(), std::move(lept_arr), (int)reason, req_by, room->cursor);
        call_stat_event(false, room, uid, uid, "card_deal", player.rest.size(), arr.size(), arr, reason, req_by, room->cursor);

        // 如果是下家，则刷新状态
        if (req_reason == deal_card_reason::normal)
        {
            assert(cur == uid);
            game_cursor_move(room, -1, room->direction);
            room->game_state = Room::GameState::wait_player;
            room->timer = TIMER_PLAYER_THINKING;

            // 要清理举报标记
            room->last_can_report = -1;

            // 拿完牌后不能抢牌
            room->can_play_ahead = false;

            // 通知所有人
            broadcast_event(room, -1, "thinking", room->seq[room->cursor], room->timer, room->last, room->last_chg_color,
                room->direction, room->draw, room->last_can_report, room->can_play_ahead);
        }
        return true;
    }

    // 玩家举报动作
    bool RoomManager::game_player_report_no_uno(RoomPtr room, int uid)
    {
        auto it = room->players.find(uid);
        assert(it != room->players.end());

        std::cout << "game_player_report_no_uno: uid " << uid << std::endl;

        // 如果没有人可以举报
        if (room->last_can_report < 0)
        {
            return false;
        }
        assert(room->players.contains(room->last_can_report));

        // 此时可以举报
        int be_punished = room->last_can_report;
        room->last_can_report = -1;

        // 加罚
        game_player_deal_card(room, be_punished, PUNISH_FOR_NO_UNO, deal_card_reason::report, uid);
        return true;
    }

    // 检查是否能打出某张牌
    bool RoomManager::game_can_play_card(RoomPtr room, card_t c, bool ahead)
    {
        if (ahead)
        {
            // 如果当前状态禁止抢牌， 则不能抢
            if (!room->can_play_ahead)
                return false;
            // 抢牌的情况下只有牌型和上一张一致才可以打
            return c == room->last;
        } else
        {
            // 完全一样肯定是可以跟牌的
            if (room->last == c)
                return true;

            int color = card::get_color(c);
            int func = card::get_func(c);
            int last_color = card::get_color(room->last);
            int last_func = card::get_func(room->last);

            // 如果是罚牌状态
            if (room->draw > 0)
            {
                // 如果上张是黑色卡
                if (last_color == card::COLOR_ALL)
                {
                    // 那么只能跟黑色
                    if (color != card::COLOR_ALL)
                        return false;
                    // 如果是+2罚牌，则可以跟+4，否则只能跟+4
                    // +2后跟王牌+2的逻辑属于相同牌规则，之前判断过了
                    if (last_func == card::FUNC_DRAW2 && func == card::FUNC_DRAW4)
                        return true;
                    return false;  // +4后跟+4的情况属于相同牌规则，之前判断过了
                }

                // 如果上张是+2
                if (last_func == card::FUNC_DRAW2)
                {
                    // 可以接任意+2
                    if (func == card::FUNC_DRAW2)
                        return true;
                    // 可以接同色+4或者王牌+4
                    if (func == card::FUNC_DRAW4 && (color == last_color || color == card::COLOR_ALL))
                        return true;
                    return false;
                }

                // 如果上张是+4
                if (last_func == card::FUNC_DRAW4)
                {
                    if (func == card::FUNC_DRAW4 && (color == last_color || color == card::COLOR_ALL))
                        return false;
                    return true;
                }

                std::cerr << "game_can_play_card: unexpected state, last " << room->last << ", last_func " << last_func << ", draw " << room->draw << std::endl;
                assert(false);
                return false;
            }

            // 对于上一张是全色牌的情况，可以衔接选择的颜色的牌
            if (last_color == card::COLOR_ALL && room->last_chg_color == color)
                return true;

            // 通用规则：可以跟同色，同功能或者无颜色的牌
            if ((color == last_color || color == card::COLOR_ALL) || func == last_func)
                return true;
            return false;
        }
    }

    void RoomManager::game_update(RoomPtr room, time_point now)
    {
        if (room->state == Room::State::idle)
        {
            // 如果玩家离线时间超过1分钟，则直接从房间踢出
            for (auto& [uid, player] : room->players)
            {
                if (player.offline && (now - player.offline_time >= std::chrono::microseconds(PLAYER_AUTO_TICK_OFFLINE_TIME)))
                {
                    std::cout << "game_update player kick cause timeout, now " << now << ", offline_time " <<  player.offline_time << std::endl;
                    game_player_leave(room, uid, player_left_reason::offline_kick);
                }

                return ;
            }
        }

        assert(room->state == Room::State::playing);

        // 状态机
        if (room->game_state == Room::GameState::dealing)
        {
            // 起始牌发牌逻辑
            room->timer = room->timer - 1;
            if (room->timer <= 0)
            {
                assert(room->cursor < room->seq.size());
                int uid = room->seq[room->cursor];
                auto it = room->players.find(uid);
                assert(it != room->players.end());
                auto& player = it->second;

                // 发牌
                card_t c = game_card_heap_deal(room);
                assert(c);
                player.rest.push_back(c);

                // 移动到下一玩家
                game_cursor_move(room, -1, room->direction);

                // 发给玩家
                std::vector<card_t> tmp = {c};
                send_event(room, uid, uid, "card_deal", (int)player.rest.size(), 1,
                    lept_value::array_t{c}, (int)deal_card_reason::normal, -1, room->cursor);
                call_stat_event(true, room, uid, uid, "card_deal", player.rest.size(), 1,
                    tmp, deal_card_reason::normal, -1, room->cursor);

                // 发送广播消息
                broadcast_event(room, uid, "card_deal", (int)player.rest.size(), 1,
                    lept_value(), (int)deal_card_reason::normal, -1, room->cursor);
                call_stat_event(false, room, uid, uid, "card_deal", player.rest.size(), 1,
                    lept_value::array_t(), deal_card_reason::normal, -1, room->cursor);

                // 更新状态
                room->deal_turn = room->deal_turn + 1;
                if (room->deal_turn >= room->curr_players * PLAYER_START_CARD_COUNT)
                {
                    assert(room->deal_turn == room->curr_players * PLAYER_START_CARD_COUNT);
                    assert(room->seq[room->cursor] == room->dealer);

                    // 如果玩家手牌都发完了， 则轮到给庄家发牌
                    room->game_state = Room::GameState::dealing_dealer;
                    room->deal_turn = 0;
                }

                room->timer = TIMER_DEALING_INTERVAL;
            }
        } else if (room->game_state == Room::GameState::dealing_dealer)
        {
            // 庄家发牌逻辑
            room->timer = room->timer - 1;
            if (room->timer <= 0)
            {
                assert(room->seq.size() > room->cursor);
                int uid = room->seq[room->cursor];
                assert(uid == room->dealer);
                auto it = room->players.find(uid);
                assert(it != room->players.end());
                auto& player = it->second;

                // 发牌
                card_t c = game_card_heap_deal(room);
                assert(c >= 0);
                player.rest.push_back(c);

                // 发给玩家
                std::vector<card_t> tmp = {c};
                send_event(room, uid, uid, "card_deal", (int)player.rest.size(), 1,
                    lept_value::array_t{c}, (int)deal_card_reason::normal, -1, room->cursor);
                call_stat_event(true, room, uid, uid, "card_deal", player.rest.size(), 1,
                    tmp, deal_card_reason::normal, -1, room->cursor);

                // 发送广播消息
                broadcast_event(room, uid, "card_deal", (int)player.rest.size(), 1, lept_value(), (int)deal_card_reason::normal, -1, room->cursor);
                call_stat_event(false, room, uid, uid, "card_deal", player.rest.size(), 1,
                    lept_value::array_t(), deal_card_reason::normal, -1, room->cursor);

                // 更新状态
                room->deal_turn = room->deal_turn + 1;
                if (room->deal_turn >= DEALER_START_CARD_EXT_COUNT)
                {
                    assert(room->deal_turn == DEALER_START_CARD_EXT_COUNT);

                    // 庄家额外的牌都发完了，进入抽取首张牌的状态
                    room->game_state = Room::GameState::select_first_card;
                    room->deal_turn = 0;
                    room->timer = TIMER_SELECT_FIRST_CARD_INTERVAL;
                    room->can_play_ahead = true; // 初始牌可以抢
                } else
                {
                    room->timer = TIMER_DEALING_INTERVAL;
                }
            }
        } else if (room->game_state == Room::GameState::select_first_card)
        {
            // 选取首张牌逻辑
            room->timer = room->timer - 1;
            if (room->timer <= 0)
            {
                // 从牌堆选一张牌
                card_t c;
                int limit = 100;
                while (true)
                {
                    c = game_card_heap_deal(room);
                    assert(c >= 0);
                    // 如果抽到非数字牌则丢回牌堆
                    if (limit >= 0 && card::is_func_card(c) && room->heap.size() > 1)
                    {
                        room->heap.push_back(c);

                        // 交换最后一张牌和任意一张牌然后重新抽
                        int idx = random() % room->heap.size();
                        card_t tmp = room->heap[idx];
                        room->heap[idx] = room->heap[room->heap.size() - 1];
                        room->heap[room->heap.size() - 1] = tmp;

                        // 保护计数器
                        limit --;
                        continue;
                    }
                    break;
                }

                // 广播
                broadcast_event(room, -1, "first_card", c);

                // 更新状态
                assert(room->seq[room->cursor] == room->dealer);  // 此时总是停留在庄家上
                room->game_state = Room::GameState::wait_player;
                room->last = c;
                room->last_chg_color = 0;
                room->history.push_back(c);
                room->timer = TIMER_PLAYER_THINKING;

                // 广播
                broadcast_event(room, -1, "thinking", room->seq[room->cursor],
                    room->timer, room->last, room->last_chg_color, room->direction,
                    room->draw, room->last_can_report, room->can_play_ahead);
            }
        } else if (room->game_state == Room::GameState::wait_player)
        {
            assert(room->seq.size() > room->cursor);
            int uid = room->seq[room->cursor];
            auto it = room->players.find(uid);
            assert(it != room->players.end());
            auto& player = it->second;

            // 更新计数器
            room->timer = room->timer - 1;

            // 如果倒计时终止或者玩家长时间不在线，则激活托管
            int threshold = 0;
            if (player.offline && (now - player.offline_time >= std::chrono::microseconds(PLAYER_AUTO_PLAY_OFFLINE_TIME)))
            {
                threshold = TIMER_PLAYER_THINKING;
            }

            // 玩家托管逻辑
            if (room->timer <= threshold)
            {
                std::cout << "game_update: player timeout, uid " << uid << ", room " << room->id << std::endl;

                // 检查玩家的手牌，选取可以打的手牌
                std::vector<card_t> can_play_cards;
                int red_cards = 0;
                int yellow_cards = 0;
                int green_cards = 0;
                int blue_cards = 0;
                for (card_t c : player.rest)
                {
                    if (game_can_play_card(room, c, false))
                        can_play_cards.push_back(c);
                    int color = card::get_color(c);
                    if (color == card::COLOR_RED)
                        red_cards ++;
                    else if (color == card::COLOR_YELLOW)
                        yellow_cards ++;
                    else if (color == card::COLOR_GREEN)
                        green_cards ++;
                    else if (color == card::COLOR_BLUE)
                        blue_cards ++;
                }

                // 如果没有可以打的牌，则发牌
                if (can_play_cards.empty())
                    game_player_deal_card(room, uid, 1, deal_card_reason::normal, -1);
                else
                {
                    // 否则，随机选取一张牌出来
                    card_t select = helper::random_select(can_play_cards);
                    int select_color = card::COLOR_ALL;

                    // 如果是王牌或者变色牌
                    if (card::get_color(select) == card::COLOR_ALL)
                    {
                        // 贪婪：选择当前颜色最多的牌
                        if (blue_cards != 0)
                            select_color = card::COLOR_BLUE;
                        else if (green_cards > blue_cards)
                            select_color = card::COLOR_GREEN;
                        else if (yellow_cards > green_cards)
                            select_color = card::COLOR_YELLOW;
                        else if (red_cards > yellow_cards)
                            select_color = card::COLOR_RED;

                        // 如果还没有选出来，则随机一个颜色
                        if (select_color == card::COLOR_ALL)
                        {
                            select_color = helper::random_select(
                                std::vector<int>({card::COLOR_BLUE, card::COLOR_GREEN, card::COLOR_YELLOW, card::COLOR_RED})
                                );
                        }
                    }

                    // 检查是否需UNO，给1/2几率没有UNO
                    bool with_uno = false;
                    if (player.rest.size() == 2 && rand() % 2)
                        with_uno = true;

                    // 打选中的牌
                    card_t ret = game_player_card_play(room, uid, select, with_uno, select_color);
                    assert(ret >= 0);
                }
            }
        }
    }

    /*
     * c2s functions
     */
    static void handle_create_room_req(SessionPtr ss, const std::string& room_name, int player_count)
    {
        RoomManager::instance().create_room_req(ss, room_name, player_count);
    }
    static Router::Registerer reg_create_room_req(
        Router::Registerer::c2s,
        "create_room_req",
        handle_create_room_req
    );
    void RoomManager::create_room_req(SessionPtr ss, const std::string& title, int player_count)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查参数
        if (title.length() < 3 || title.length() > 10)
        {
            ss->call("create_room_rsp", (int)ErrCode::api_room_invalid_title, nullptr);
            return;
        }
        if (player_count < 2 || player_count > 20)
        {
            ss->call("create_room_rsp", (int)ErrCode::api_room_invalid_title, nullptr);
        }

        // 检查状态
        if (ss->state() != Session::State::logged)
        {
            std::cerr << "create_room_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("create_room_rsp", (int)ErrCode::api_invalid_call, nullptr);
            return ;
        }
        if (data.contains("room_id") && data.at("room_id").is<std::string>())
        {
            std::cerr << "create_room_req: already in another room " << data.at("room_id").get_string() << std::endl;
            ss->call("create_room_rsp", (int)ErrCode::api_invalid_call, nullptr);
            return;
        }

        // 生成房间ID
        int id = (m_next_room_id++);
        if (m_next_room_id < 0)
            m_next_room_id = 1;
        while (m_rooms.contains(id))
        {
            id = (m_next_room_id++);
            if (m_next_room_id < 0)
                m_next_room_id = 1;
        }

        // 创建房间
        // m_rooms[id] = std::make_shared<Room>(Room{
        //     .id = id,
        //     .owner = uid,
        //     .max_players = player_count,
        //     .curr_players = 1,
        //     .title = title,
        //     .seq = {uid},
        // });
        auto room = std::make_shared<Room>();
        room->id = id;
        room->title = title;
        room->owner = uid;
        room->max_players = player_count;
        room->seq.push_back(uid);
        room->curr_players = player_count;

        m_rooms[id] = room;

        room->players[uid] =
            Room::Player{
                .nick = ss->nick(),
                .email = ss->email(),
                .ready = false,
                .offline = false,
                .offline_time = time_point::min(),
            };

        // 设置玩家状态
        data["room_id"] = id;
        ss->set_dirty();
        ss->set_state(Session::State::gaming);

        ss->call("create_room_rsp", (int)ErrCode::ok, get_snapshot(m_rooms[id], uid));
        std::cout << "create_room_req: create room successfully, uid " << uid << ", room_id " << id << std::endl;
    }

    static void handle_enter_room_req(SessionPtr ss, int room_id, bool re_enter)
    {
        RoomManager::instance().enter_room_req(ss, room_id, re_enter);
    }
    static Router::Registerer reg_enter_room_req(
        Router::Registerer::c2s,
        "enter_room_req",
        handle_enter_room_req
    );
    void RoomManager::enter_room_req(SessionPtr ss, int room_id, bool re_enter)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::logged)
        {
            std::cerr << "enter_room_req: bad state, " << (int)ss->state() << std::endl;

            // 针对重连情况需要清空room_id
            if (re_enter && data["room_id"].get_integer() == room_id)
            {
                data["room_id"] = -1;
                ss->set_dirty();
            }

            ss->call("enter_room_rsp", (int)ErrCode::api_invalid_call, nullptr);
            return ;
        }

        // 检查是否是重连
        if (re_enter)
        {
            if (room_id != data["room_id"].get_integer())
            {
                std::cerr << "enter_room_req: room_id mismatched, " << room_id << " != " << data["room_id"].get_integer() << std::endl;
                ss->call("enter_room_rsp", (int)ErrCode::api_invalid_call, nullptr);
                return ;
            }

            auto room_it = m_rooms.find(room_id);
            if (room_it == m_rooms.end() || room_it->second->players.contains(uid))
            {
                std::cerr << "enter_room_req: room or user not in, " << room_id << std::endl;

                // 针对重连情况需要清空room_id
                data["room_id"] = -1;
                ss->set_dirty();

                ss->call("enter_room_rsp", (int)ErrCode::api_cannot_reenter, nullptr);
                return ;
            }
            auto& room = room_it->second;

            // 设置玩家状态
            ss->set_state(Session::State::gaming);

            ss->call("enter_room_rsp", (int)ErrCode::ok, get_snapshot(room, uid));
            return ;
        }

        // 第一次加入房间，进行检查
        auto room_it = m_rooms.find(room_id);
        if (room_it != m_rooms.end())
        {
            ss->call("enter_room_rsp", (int)ErrCode::api_room_not_found, nullptr);
            return ;
        }
        auto& room = room_it->second;

        // 检查房间状态
        if (room->state != Room::State::idle)
        {
            ss->call("enter_room_rsp", (int)ErrCode::api_room_is_in_game, nullptr);
            return ;
        }

        // 检查玩家数
        if (room->curr_players >= room->max_players)
        {
            ss->call("enter_room_rsp", (int)ErrCode::api_room_is_full, nullptr);
            return ;
        }

        // 检查是否已经在房间内
        if (room->players.contains(uid))
        {
            std::cout << "enter_room_req: player " << uid << " already in room " << room_id << std::endl;
            assert(data["room_id"].is<int>() && data["room_id"].get_integer() == room_id);
            ss->call("enter_room_rsp", (int)ErrCode::api_invalid_call, nullptr);
            return ;
        }

        // 进入房间
        room->players[uid] = Room::Player{
            .nick = ss->nick(),
            .email = ss->email(),
            .ready = false,
            .offline = false,
            .offline_time = time_point::min(),
        };
        room->curr_players++;
        room->seq.push_back(uid);

        // 设置玩家状态
        data["room_id"] = room_id;
        ss->set_dirty();
        ss->set_state(Session::State::gaming);

        ss->call("enter_room_rsp", (int)ErrCode::ok, get_snapshot(room, uid));
        broadcast_event(room, uid, "player_joined", get_player_snapshot(room, uid));
    }

    static void handle_leave_room_req(SessionPtr ss, int room_id)
    {
        RoomManager::instance().leave_room_req(ss, room_id);
    }
    static Router::Registerer reg_leave_room_req(
        Router::Registerer::c2s,
        "leave_room_req",
        handle_leave_room_req
    );
    void RoomManager::leave_room_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "leave_room_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("leave_room_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("leave_room_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 离开房间
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "leave_room_req: room not found, room_id " << room_id << std::endl;
            data["room_id"] = -1;
            ss->set_dirty();
            ss->set_state(Session::State::logged);
            ss->call("leave_room_rsp", (int)ErrCode::ok);
            return ;
        }
        auto& room = room_it->second;

        // TODO: 发牌时不能退出
        if (room->game_state == Room::GameState::dealing ||
            room->game_state == Room::GameState::dealing_dealer ||
            room->game_state == Room::GameState::select_first_card)
        {
            ss->call("leave_room_rsp", (int)ErrCode::api_room_cannot_leave_at_this_time);
            return ;
        }

        game_player_leave(room, uid, player_left_reason::normal);

        // 清空玩家状态
        data["room_id"] = -1;
        ss->set_dirty();
        ss->set_state(Session::State::logged);

        ss->call("leave_room_rsp", (int)ErrCode::ok);
    }

    static void handle_get_ready_req(SessionPtr ss, int room_id)
    {
        RoomManager::instance().get_ready_req(ss, room_id);
    }
    static Router::Registerer reg_get_ready_req(
        Router::Registerer::c2s,
        "get_ready_req",
        handle_get_ready_req
    );
    void RoomManager::get_ready_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "get_ready_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("get_ready_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("get_ready_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "get_ready_req: room not found, room_id" << room_id << std::endl;
            ss->call("get_ready_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::idle)
        {
            ss->call("get_ready_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 设置玩家就绪
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());
        auto& player = player_it->second;
        if (!player.ready)
        {
            player.ready = true;
            broadcast_event(room, uid, "player_ready");
        }

        ss->call("get_ready_rsp", (int)ErrCode::ok);
    }

    static void handle_shuffle_room_seats_req(SessionPtr ss, int room_id)
    {
        RoomManager::instance().shuffle_room_seats_req(ss, room_id);
    }
    static Router::Registerer reg_shuffle_room_seats_req(
        Router::Registerer::c2s,
        "shuffle_room_seats_req",
        handle_shuffle_room_seats_req
    );
    void RoomManager::shuffle_room_seats_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "shuffle_room_seats_rsp: bad state, " << (int)ss->state() << std::endl;
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状况
        auto room_it = m_rooms.find(room_id);
        if (room_it != m_rooms.end())
        {
            std::cerr << "shuffle_room_seats_rsp: room not found, room_id: " << room_id << std::endl;
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::idle)
        {
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 检查玩家是否是房主
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());
        if (room->owner != uid)
        {
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_room_not_owner);
            return ;
        }

        // 检查是否冷却
        auto now = ROOM_NOW;
        if (now - room->last_update_seat_time <= std::chrono::microseconds(OWNER_UPDATE_SEAT_INTERVAL))
        {
            ss->call("shuffle_room_seats_rsp", (int)ErrCode::api_room_update_seat_cooldown);
            return ;
        }

        // 重新排列座位
        helper::random_shuffle(room->seq);
        room->last_update_seat_time = now;

        // 所有玩家设置为未准备状态
        for (auto& [uid, player] : room->players)
        {
            player.ready = false;
        }

        // 发送广播
        lept_value::array_t tmp;
        for (auto& it : room->seq)
        {
            tmp.emplace_back(it);
        }
        broadcast_event(room, -1, "update_seats", tmp);

        // 回包
        ss->call("shuffle_room_seats_rsp", (int)ErrCode::ok);
    }

    static void handle_room_use_voice_req(SessionPtr ss, int room_id, int voice_id)
    {
        RoomManager::instance().room_use_voice_req(ss, room_id, voice_id);
    }
    static Router::Registerer reg_room_use_voice_req(
        Router::Registerer::c2s,
        "room_use_voice_req",
        handle_room_use_voice_req
    );
    void RoomManager::room_use_voice_req(SessionPtr ss, int room_id, int voice_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "room_use_voice_rsp: bad state, " << (int)ss->state() << std::endl;
            ss->call("room_use_voice_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("room_use_voice_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状况
        auto room_it = m_rooms.find(room_id);
        if (room_it != m_rooms.end())
        {
            std::cerr << "room_use_voice_rsp: room not found, room_id: " << room_id << std::endl;
            ss->call("room_use_voice_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;

        // 获取玩家对象
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());
        auto& player = player_it->second;

        auto now = ROOM_NOW;
        if (now - player.last_use_voice_time < std::chrono::microseconds(PLAYER_USE_VOICE_INTERVAL))
        {
            ss->call("room_use_voice_rsp", (int)ErrCode::api_room_use_voice_cooldown);
            return ;
        }

        // 设置使用时间
        player.last_use_voice_time = now;

        // 发送广播
        broadcast_event(room, -1, "use_voice", uid, voice_id);

        // 回包
        ss->call("room_use_voice_rsp", (int)ErrCode::ok);
    }

    static void handle_room_kickout_player_req(SessionPtr ss, int room_id, int be_kicked)
    {
        RoomManager::instance().room_kickout_player_req(ss, room_id, be_kicked);
    }
    static Router::Registerer reg_room_kickout_player_req(
        Router::Registerer::c2s,
        "room_kickout_player_req",
        handle_room_kickout_player_req
    );
    void RoomManager::room_kickout_player_req(SessionPtr ss, int room_id, int be_kicked)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "room_kickout_player_rsp: bad state, " << (int)ss->state() << std::endl;
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "room_kickout_player_rsp: room not found, room_id: " << room_id << std::endl;
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::idle)
        {
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 检查是否是房主
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());
        if (room->owner != uid)
        {
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_room_not_owner);
            return ;
        }

        // 检查被踢玩家是否存在
        auto be_kicked_player_it = room->players.find(be_kicked);
        if (be_kicked_player_it == room->players.end())
        {
            ss->call("room_kickout_player_rsp", (int)ErrCode::api_room_player_not_found);
            return ;
        }

        // 发起踢人
        std::cout << "room_kickout_player_req: player " << be_kicked << " was kicked by owner " << uid << std::endl;
        send_event(room, be_kicked, uid, "be_kicked");

        // 清空玩家状态
        auto be_kicked_ss = SSMGR.find_session(be_kicked);
        if (be_kicked_ss)
        {
            auto be_kicked_data = be_kicked_ss->data();
            if (be_kicked_data.is<lept_value::object_t>())
            {
                be_kicked_data["room_id"] = -1;
                be_kicked_ss->set_dirty();
                be_kicked_ss->set_state(Session::State::logged);
            }
        }

        // 房间内逻辑
        game_player_leave(room, be_kicked, player_left_reason::kicked_by_owner);

        // 回包
        ss->call("room_kickout_player_rsp", (int)ErrCode::ok);
    }

    static void handle_start_game_req(SessionPtr ss, const std::string& room_name, int player_count)
    {
        RoomManager::instance().create_room_req(ss, room_name, player_count);
    }
    static Router::Registerer reg_start_game_req(
        Router::Registerer::c2s,
        "start_game_req",
        handle_start_game_req
    );
    void RoomManager::start_game_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "start_game_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("start_game_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("start_game_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "start_game_req: room not found, room_id: " << room_id << std::endl;
            ss->call("start_game_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::idle)
        {
            ss->call("start_game_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取玩家对象
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());

        // 检查是否是房主
        if (room->owner != uid)
        {
            std::cerr << "start_game_req: not owner, room_id: " << room_id << ", uid: " << uid << std::endl;
            ss->call("start_game_rsp", (int)ErrCode::api_room_not_owner);
            return ;
        }

        // 检查玩家数
        if (room->players.size() < 2)
        {
            ss->call("start_game_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 检查是否所有玩家就绪（除房主外）
        for (auto& [u, player]: room->players)
        {
            if (u != uid && !player.ready)
            {
                ss->call("start_game_rsp", (int)ErrCode::api_room_not_ready);
                return ;
            }
        }

        // 开局，初始话数据
        game_start(room);

        ss->call("start_game_rsp", (int)ErrCode::ok);
        broadcast_event(room, -1, "game_start", get_snapshot(room, -1));
        call_stat_event(false, room, -1, -1,  "game_start", nullptr);
    }

    static void handle_game_play_card_req(SessionPtr ss, int room_id, card_t c, bool with_uno, int chg_color)
    {
        RoomManager::instance().game_play_card_req(ss, room_id, c, with_uno, chg_color);
    }
    static Router::Registerer reg_game_play_card_req(
        Router::Registerer::c2s,
        "game_play_card_req",
        handle_game_play_card_req
    );
    void RoomManager::game_play_card_req(SessionPtr ss, int room_id, card_t c, bool with_uno, int chg_color)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming)
        {
            std::cerr << "game_play_card_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("game_play_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("game_play_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "game_play_card_req: room not found, room_id: " << room_id << std::endl;
            ss->call("game_play_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::playing)
        {
            ss->call("game_play_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取玩家对象
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());

        if (!game_player_card_play(room, uid, c, with_uno, chg_color))
        {
            ss->call("game_play_card_rsp", (int)ErrCode::api_game_cannot_play_card);
            return ;
        }

        ss->call("game_play_card_rsp", (int)ErrCode::ok);
    }

    static void handle_game_deal_card_req(SessionPtr ss, const std::string& room_name, int player_count)
    {
        RoomManager::instance().create_room_req(ss, room_name, player_count);
    }
    static Router::Registerer reg_game_deal_card_req(
        Router::Registerer::c2s,
        "game_deal_card_req",
        handle_game_deal_card_req
    );
    void RoomManager::game_deal_card_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming) {
            std::cerr << "game_play_card_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("game_deal_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("game_deal_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "game_deal_card_req: room not found, room_id: " << room_id << std::endl;
            ss->call("game_deal_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::playing)
        {
            ss->call("game_deal_card_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取玩家对象
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());

        if (!game_player_deal_card(room, uid, 1, deal_card_reason::normal, -1))
        {
            ss->call("game_deal_card_rsp", (int)ErrCode::api_game_cannot_deal_card);
            return ;
        }

        ss->call("game_deal_card_rsp", (int)ErrCode::ok);
    }

    static void handle_game_report_no_uno_req(SessionPtr ss, int room_id)
    {
        RoomManager::instance().game_report_no_uno_req(ss, room_id);
    }
    static Router::Registerer reg_game_report_no_uno_req(
        Router::Registerer::c2s,
        "game_report_no_uno_req",
        handle_game_report_no_uno_req
    );
    void RoomManager::game_report_no_uno_req(SessionPtr ss, int room_id)
    {
        int uid = ss->uid();
        auto& data = ss->data().get_object();

        // 检查状态
        if (ss->state() != Session::State::gaming) {
            std::cerr << "game_report_no_uno_req: bad state, " << (int)ss->state() << std::endl;
            ss->call("game_report_no_uno_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        if (!data["room_id"].is<int>() || data["room_id"].get_integer() != room_id)
        {
            ss->call("game_report_no_uno_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取房间 & 检查状态
        auto room_it = m_rooms.find(room_id);
        if (room_it == m_rooms.end())
        {
            std::cerr << "game_report_no_uno_req: room not found, room_id: " << room_id << std::endl;
            ss->call("game_report_no_uno_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }
        auto& room = room_it->second;
        if (room->state != Room::State::playing)
        {
            ss->call("game_report_no_uno_rsp", (int)ErrCode::api_invalid_call);
            return ;
        }

        // 获取玩家对象
        auto player_it = room->players.find(uid);
        assert(player_it != room->players.end());

        if (!game_player_report_no_uno(room, uid))
        {
            ss->call("game_report_no_uno_rsp", (int)ErrCode::api_game_cannot_report_no_uno);
            return ;
        }

        ss->call("game_report_no_uno_rsp", (int)ErrCode::ok);
    }

    static void handle_get_room_list_req(SessionPtr ss)
    {
        RoomManager::instance().get_room_list_req(ss);
    }
    static Router::Registerer reg_get_room_list_req(
        Router::Registerer::c2s,
        "get_room_list_req",
        handle_get_room_list_req
    );
    void RoomManager::get_room_list_req(SessionPtr ss)
    {
        // FIXME: 分页
        lept_value::array_t ret;
        for (auto& [room_id, room] : m_rooms)
        {
            ret.emplace_back(lept_value{
                {"id", room_id},
                {"title", room->title},
                {"curr_players", room->curr_players},
                {"max_players", room->max_players},
                {"playing ", room->state != Room::State::idle},
                {"joinable", room->state == Room::State::idle && room->curr_players <= room->max_players}
            });
        }

        ss->call("get_room_list_rsp", ret, (int)SSMGR.get_session_count());
    }


    /*
     * update
     */
    void RoomManager::update(time_point now)
    {
        for (auto& [room_id, room] : m_rooms)
        {
            try
            {
                game_update(room, now);
            } catch (std::exception& e)
            {
                // 强行解除更新失败的游戏
                std::cerr << "update: update room error, room_id " << room_id << std::endl;
                std::cerr << e.what() << std::endl;
                game_dismiss(room);
            }
        }
    }
    // todo: evtcenter.on("login");
    // todo: evtcenter.on("logout");

}
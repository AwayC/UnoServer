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
            CALL_S2S("round_flow", uid, room->stater.get_stat(uid));
        }

        // 展示分数面板
        broadcast_event(room, -1, 'game_stat',
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

    void RoomManager::game_player_leave(RoomPtr room, int uid, deal_card_reason reason)
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
        send_event(room, uid, uid, "card_play", c, need_uno, with_uno, rest_count, player.rest, ahead);
        broadcast_event(room, uid, 'card_play', c, need_uno, with_uno, rest_count, std::vector<card_t>(), ahead);

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
        send_event(room, uid, uid, "card_deal", player.rest.size(), arr.size(), arr, reason, req_by, room->cursor);
        broadcast_event(room, uid, "card_deal", player.rest.size(), arr.size(), arr, reason, req_by, room->cursor);

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
        auto& player = it->second;

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


}
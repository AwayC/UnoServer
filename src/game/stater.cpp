//
// Created by AWAY on 25-11-19.
//
#include "constants.h"
#include "stater.h"
#include "room.h"

namespace uno
{
    using Stater = GameStater;

    constexpr std::vector<Stater::StatEvent> STAT_EVENT_DISPLAY_TABLE = {
        {1, [](Stater::Stat& stat) { return stat.draw_by_report; }, 1, 4},
        {2, [](Stater::Stat& stat) { return stat.draw_by_draw_card; }, 1, 10 },
        {3, [](Stater::Stat& stat) { return stat.get_val(stat.draw_black_cards, card::FUNC_DRAW4); }, 5, 3},
        {4, [](Stater::Stat& stat)
            {
                return stat.get_val(stat.play_black_cards, card::FUNC_DRAW4) +
                    stat.get_val(stat.play_cards, card::FUNC_DRAW4);
            }, 2, 4},
        {5, [](Stater::Stat& stat) { return stat.report; }, 5, 3},
        {6, [](Stater::Stat& stat) { return stat.be_reported; }, 5, 3},
        {7, [](Stater::Stat& stat) { return stat.uno; }, 5, 3},
        {8, [](Stater::Stat& stat) { return stat.sp_forbid_others_uno; }, 6, 2},
        {9, [](Stater::Stat& stat) { return stat.sp_draw_others_eat_by_self; }, 6, 2},
        {10, [](Stater::Stat& stat) { return stat.sp_draw_by_others_max; }, 5, 10}
    };

    void Stater::check_uno_traceable(RoomPtr room)
    {
        bool anyone_uno = false;
        for (auto& [uid, player]: room->players)
        {
            if (player.rest.size() == 1)
            {
                anyone_uno = true;
                break;
            }
        }

        if (anyone_uno && !this->m_can_trace_winner_causer)
        {
            m_last_color_chger = -1;
            m_last_color = -1;
            m_last_play_card_by = -1;
            m_last_play_card = -1;
            m_can_trace_winner_causer = true;
        } else if (!anyone_uno && this->m_can_trace_winner_causer)
        {
            m_last_color_chger = -1;
            m_last_color = -1;
            m_last_play_card = -1;
            m_last_play_card_by = -1;
            m_can_trace_winner_causer = false;
        }
    }

    int Stater::check_winner_causer(int uid, card_t c, bool ahead)
    {
        int ret = -1;
        if (this->m_can_trace_winner_causer)
        {
            if (ahead && m_last_play_card == c)
            {
                ret = m_last_play_card_by;
                return ret;
            }
            if (!ahead && card::get_color(m_last_play_card) != card::get_color(c) &&
                card::get_func(m_last_play_card) == card::get_func(c))
            {
                ret = m_last_play_card_by;
                return ret;
            }
        }

        if (m_last_color_chger && m_last_color_chger != uid)
        {
            if (card::get_color(c) == m_last_color)
            {
                ret = m_last_color_chger;
                return ret;
            }
        }

        return ret;
    }

    lept_value Stater::get_stat(int uid)
    {
        auto stat = m_round_stat[uid];

#define KEY_VAL(key)  {#key, stat.key}
        lept_value ret = {
            KEY_VAL(game_count),
            KEY_VAL(total_draw),
            KEY_VAL(draw_by_sys),
            KEY_VAL(draw_by_report),
            KEY_VAL(total_play),
            KEY_VAL(play_ahead),
            KEY_VAL(report),
            KEY_VAL(be_reported),
            KEY_VAL(uno),
            KEY_VAL(forgot_uno),
            KEY_VAL(win),
            KEY_VAL(sp_forbid_others_uno),
            KEY_VAL(sp_cause_others_win),
            KEY_VAL(sp_draw_others_eat_by_self),
            KEY_VAL(sp_draw_by_others_max)
        };
#undef KEY_VAL

        return ret;
    }

    std::vector<Stater::DisplayStat>& Stater::get_display_stat()
    {
        return m_display_stat;
    }

    int Stater::get_winner_stealer()
    {
        for (auto& [uid, stat] : m_round_stat)
        {
            if (stat.sp_cause_others_win > 0)
            {
                return uid;
            }
        }

        return -1;
    }

    void Stater::on_game_start(RoomPtr room, int sender, RoomSnapshot* room_snapshot)
    {
        m_gaming = true;
        m_round_stat.clear();
        m_display_stat.clear();
        m_draw_starter = -1;
        m_last_play_draw = -1;
        m_last_color_chger = -1;
        m_last_color = -1;
        m_last_play_card_by = -1;
        m_last_play_card = -1;
        m_can_trace_winner_causer = false;

        for (auto& [uid, player]: room->players)
        {
            Stat stat;
            m_round_stat[uid] = stat;

            stat.game_count = 1;
        }
    }

    void Stater::on_player_left(RoomPtr room, int uid, std::string reason)
    {
        if (m_gaming)
        {
            m_round_stat.erase(uid);
        }
    }

    void Stater::on_player_win(RoomPtr room, int sender, int winner)
    {
        m_gaming = false;

        auto win_player = m_round_stat.find(winner);
        assert(win_player != m_round_stat.end());
        win_player->second.win = 1;

        // todo
        for (size_t i = 0;i < STAT_EVENT_DISPLAY_TABLE.size();i ++)
        {
            auto& e = STAT_EVENT_DISPLAY_TABLE[i];
            for (auto& [uid, stat]: m_round_stat)
            {
                int cnt = e.getField(stat);
                if (cnt && cnt >= e.minDisplayCount)
                {
                    int score = (cnt - e.minDisplayCount) * e.score;
                    m_display_stat.push_back({
                        i, uid, cnt, score
                    });
                }
            }
        }

        std::sort(m_display_stat.begin(), m_display_stat.end(), [](DisplayStat& a, DisplayStat& b)
        {
            return a.score > b.score;
        });

        // 保留前五
        if (m_display_stat.size() > 5)
            m_display_stat.resize(5);
    }

    void Stater::on_card_deal(RoomPtr room, int uid, size_t rest_count, size_t deal_count,
        std::vector<card_t>& deal_cards, deal_card_reason reason,
        int report_by, int cursor)
    {
        auto it = m_round_stat.find(uid);
        assert(it != m_round_stat.end());

        auto& player = it->second;
        if (reason == deal_card_reason::draw)
        {
            player.draw_by_draw_card += deal_count;
            player.sp_draw_by_others_max = std::max(player.sp_draw_by_others_max, deal_count);

            if (m_draw_starter == uid)
            {
                player.sp_draw_others_eat_by_self += 1;
            }

            if (rest_count - deal_count == 1)
            {
                auto last_play_draw_player = m_round_stat.find(m_last_play_draw);
                if (last_play_draw_player != m_round_stat.end())
                {
                    last_play_draw_player->second.sp_forbid_others_uno += 1;
                }
            }


            m_draw_starter = -1;
            m_last_play_draw = -1;
        } else if (reason == deal_card_reason::bad_uno)
        {
            player.draw_by_sys += deal_count;
        } else if (reason == deal_card_reason::report)
        {
            player.draw_by_report += deal_count;
            player.be_reported += 1;

            auto report_by_player = m_round_stat.find(report_by);
            if (report_by_player != m_round_stat.end())
            {
                report_by_player->second.report += 1;
            }
        } else
        {
            assert(reason == deal_card_reason::normal || reason == deal_card_reason::last_card);
        }

        for (size_t i = 0;i < deal_cards.size();i ++)
        {
            int func = card::get_func(deal_cards[i]);
            int color = card::get_color(deal_cards[i]);

            if (color == card::COLOR_ALL)
            {
                player.draw_black_cards[func] += 1;
            } else
            {
                player.draw_cards[func] += 1;
            }
        }

        check_uno_traceable(room);
    }

    void Stater::on_card_play(RoomPtr room, int uid, card_t c, bool need_uno,
            bool with_uno, int rest_count,
            std::vector<card_t>& cards, bool ahead)
    {
        auto it = m_round_stat.find(uid);
        assert(it != m_round_stat.end());

        auto& player = it->second;

        player.total_play += 1;
        if (ahead)
        {
            player.play_ahead += 1;
        }
        if (need_uno)
        {
            player.uno += 1;
            if (!with_uno)
            {
                player.forgot_uno += 1;
            }
        }

        int func = card::get_func(c);
        int color = card::get_color(c);

        if (color == card::COLOR_ALL)
        {
            player.play_black_cards[func] += 1;
        } else
        {
            player.play_cards[func] += 1;
        }

        if (func == card::FUNC_DRAW2 || func == card::FUNC_DRAW4)
        {
            if (!m_draw_starter)
                m_draw_starter = uid;

            m_last_play_draw = uid;
        }

        // 检查是否点炮
        if (rest_count == 0 && !card::is_func_card(c))
        {
            int cause_winner_by = check_winner_causer(uid, c, ahead);
            if (cause_winner_by != -1)
            {
                auto cause_winner_by_player = m_round_stat.find(cause_winner_by);
                if (cause_winner_by_player != m_round_stat.end())
                {
                    cause_winner_by_player->second.sp_cause_others_win += 1;
                }
            }
        }

        // 炮点追踪器
        check_uno_traceable(room);
        if (m_can_trace_winner_causer)
        {
            if (color == card::COLOR_ALL)
            {
                m_last_color_chger = uid;
                m_last_color = room->last_chg_color;
            }
            if (!ahead)
            {
                m_last_play_card = c;
                m_last_play_card_by = uid;
            }
        }


    }


}
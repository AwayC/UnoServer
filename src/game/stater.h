//
// Created by AWAY on 25-11-19.
//

#pragma once
#include "card.h"
#include <vector>

namespace uno {

    class GameStatr
    {
    public:
        struct Stat
        {
            int game_count = 0;

            int total_draw = 0;
            int draw_by_sys = 0;
            int draw_by_report = 0;
            int draw_by_draw_card = 0;
            std::vector<int> draw_cards;
            std::vector<int> draw_black_cards;

            int total_play = 0;
            int play_ahead = 0;
            std::vector<int> play_cards;

            int report = 0;
            int be_reported = 0;



            int uno = 0;
            int forgot_uno = 0;

            int win = 0;

            int sp_forbid_others_uno = 0;
            int sp_cause_others_win = 0;
            int sp_draw_others_eat_by_self = 0;
            int sp_draw_by_others_max = 0;
        };

    private:
        bool m_gaming = false;
        Stat m_round_stat;
        Stat m_display_stat;
        // draw_starter = null
        // last_play_draw = null
        //last_color_chger = null
        int m_last_color = card::COLOR_ALL;
        //last_play_card_by = null
        int m_last_play_card = -1;
        bool m_can_trace_winner_causer = false;


    };

};
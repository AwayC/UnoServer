//
// Created by AWAY on 25-11-3.
//

#pragma once
#include <cassert>
#include <utility>

namespace uno {

    namespace card
    {
        enum
        {
            COLOR_ALL,
            COLOR_RED,
            COLOR_YELLOW,
            COLOR_BLUE,
            COLOR_GREEN,
        };

        enum
        {
            FUNC_NONE,
            FUNC_0,
            FUNC_1,
            FUNC_2,
            FUNC_3,
            FUNC_4,
            FUNC_5,
            FUNC_6,
            FUNC_7,
            FUNC_8,
            FUNC_9,
            FUNC_SKIP,
            FUNC_REVERSE,
            FUNC_DRAW2,
            FUNC_DRAW4,
            FUNC_CHGCOLOR,
        };

        bool is_valid_color(int c)
        {
            return c >= COLOR_ALL && c <= COLOR_GREEN;
        }

        bool is_valid_func(int f)
        {
            return f >= FUNC_NONE && f <= FUNC_CHGCOLOR;
        }

        int get_color(int c)
        {
            int ret = c & 7;
            assert(is_valid_color(ret));
            return ret;
        }

        int get_func(int c)
        {
            int ret = (c >> 3) & 15;
            assert(is_valid_func(ret));
            return ret;
        }

        int compose_card(int c, int f)
        {
            assert(is_valid_color(c));
            assert(is_valid_func(f));
            return (c & 7) | ((f & 15) << 3);
        }

        std::pair<int, int> decompose_card(int c)
        {
            int color = get_color(c);
            int func = get_color(c);
            return { color, func };
        }

        bool is_valid_card(int c)
        {
            int color = c & 7;
            if (!is_valid_color(color))
            {
                return false;
            }
            int func = get_func(c);
            if (!is_valid_func(func))
            {
                return false;
            }

            if (func == FUNC_CHGCOLOR && color != COLOR_ALL)
            {
                return false;
            }

            return true;
        }

        bool is_func_card(int c)
        {
            int func = get_func(c);
            return (func == FUNC_SKIP || func == FUNC_REVERSE || func == FUNC_DRAW2 || func == FUNC_DRAW4 || func == FUNC_CHGCOLOR);
        }

        inline const int CARD_SET[] = {
            // 数字牌（0~9，4种颜色，其中1~9各2张，0 1张）
            compose_card(COLOR_RED, FUNC_0),
            compose_card(COLOR_RED, FUNC_1),
            compose_card(COLOR_RED, FUNC_1),
            compose_card(COLOR_RED, FUNC_2),
            compose_card(COLOR_RED, FUNC_2),
            compose_card(COLOR_RED, FUNC_3),
            compose_card(COLOR_RED, FUNC_3),
            compose_card(COLOR_RED, FUNC_4),
            compose_card(COLOR_RED, FUNC_4),
            compose_card(COLOR_RED, FUNC_5),
            compose_card(COLOR_RED, FUNC_5),
            compose_card(COLOR_RED, FUNC_6),
            compose_card(COLOR_RED, FUNC_6),
            compose_card(COLOR_RED, FUNC_7),
            compose_card(COLOR_RED, FUNC_7),
            compose_card(COLOR_RED, FUNC_8),
            compose_card(COLOR_RED, FUNC_8),
            compose_card(COLOR_RED, FUNC_9),
            compose_card(COLOR_RED, FUNC_9),

            compose_card(COLOR_YELLOW, FUNC_0),
            compose_card(COLOR_YELLOW, FUNC_1),
            compose_card(COLOR_YELLOW, FUNC_1),
            compose_card(COLOR_YELLOW, FUNC_2),
            compose_card(COLOR_YELLOW, FUNC_2),
            compose_card(COLOR_YELLOW, FUNC_3),
            compose_card(COLOR_YELLOW, FUNC_3),
            compose_card(COLOR_YELLOW, FUNC_4),
            compose_card(COLOR_YELLOW, FUNC_4),
            compose_card(COLOR_YELLOW, FUNC_5),
            compose_card(COLOR_YELLOW, FUNC_5),
            compose_card(COLOR_YELLOW, FUNC_6),
            compose_card(COLOR_YELLOW, FUNC_6),
            compose_card(COLOR_YELLOW, FUNC_7),
            compose_card(COLOR_YELLOW, FUNC_7),
            compose_card(COLOR_YELLOW, FUNC_8),
            compose_card(COLOR_YELLOW, FUNC_8),
            compose_card(COLOR_YELLOW, FUNC_9),
            compose_card(COLOR_YELLOW, FUNC_9),

            compose_card(COLOR_BLUE, FUNC_0),
            compose_card(COLOR_BLUE, FUNC_1),
            compose_card(COLOR_BLUE, FUNC_1),
            compose_card(COLOR_BLUE, FUNC_2),
            compose_card(COLOR_BLUE, FUNC_2),
            compose_card(COLOR_BLUE, FUNC_3),
            compose_card(COLOR_BLUE, FUNC_3),
            compose_card(COLOR_BLUE, FUNC_4),
            compose_card(COLOR_BLUE, FUNC_4),
            compose_card(COLOR_BLUE, FUNC_5),
            compose_card(COLOR_BLUE, FUNC_5),
            compose_card(COLOR_BLUE, FUNC_6),
            compose_card(COLOR_BLUE, FUNC_6),
            compose_card(COLOR_BLUE, FUNC_7),
            compose_card(COLOR_BLUE, FUNC_7),
            compose_card(COLOR_BLUE, FUNC_8),
            compose_card(COLOR_BLUE, FUNC_8),
            compose_card(COLOR_BLUE, FUNC_9),
            compose_card(COLOR_BLUE, FUNC_9),

            compose_card(COLOR_GREEN, FUNC_0),
            compose_card(COLOR_GREEN, FUNC_1),
            compose_card(COLOR_GREEN, FUNC_1),
            compose_card(COLOR_GREEN, FUNC_2),
            compose_card(COLOR_GREEN, FUNC_2),
            compose_card(COLOR_GREEN, FUNC_3),
            compose_card(COLOR_GREEN, FUNC_3),
            compose_card(COLOR_GREEN, FUNC_4),
            compose_card(COLOR_GREEN, FUNC_4),
            compose_card(COLOR_GREEN, FUNC_5),
            compose_card(COLOR_GREEN, FUNC_5),
            compose_card(COLOR_GREEN, FUNC_6),
            compose_card(COLOR_GREEN, FUNC_6),
            compose_card(COLOR_GREEN, FUNC_7),
            compose_card(COLOR_GREEN, FUNC_7),
            compose_card(COLOR_GREEN, FUNC_8),
            compose_card(COLOR_GREEN, FUNC_8),
            compose_card(COLOR_GREEN, FUNC_9),
            compose_card(COLOR_GREEN, FUNC_9),

            // 功能牌（禁止、反转、+2、+4，各2张）
            compose_card(COLOR_RED, FUNC_SKIP),
            compose_card(COLOR_RED, FUNC_SKIP),
            compose_card(COLOR_RED, FUNC_REVERSE),
            compose_card(COLOR_RED, FUNC_REVERSE),
            compose_card(COLOR_RED, FUNC_DRAW2),
            compose_card(COLOR_RED, FUNC_DRAW2),
            compose_card(COLOR_RED, FUNC_DRAW4),
            compose_card(COLOR_RED, FUNC_DRAW4),

            compose_card(COLOR_YELLOW, FUNC_SKIP),
            compose_card(COLOR_YELLOW, FUNC_SKIP),
            compose_card(COLOR_YELLOW, FUNC_REVERSE),
            compose_card(COLOR_YELLOW, FUNC_REVERSE),
            compose_card(COLOR_YELLOW, FUNC_DRAW2),
            compose_card(COLOR_YELLOW, FUNC_DRAW2),
            compose_card(COLOR_YELLOW, FUNC_DRAW4),
            compose_card(COLOR_YELLOW, FUNC_DRAW4),

            compose_card(COLOR_BLUE, FUNC_SKIP),
            compose_card(COLOR_BLUE, FUNC_SKIP),
            compose_card(COLOR_BLUE, FUNC_REVERSE),
            compose_card(COLOR_BLUE, FUNC_REVERSE),
            compose_card(COLOR_BLUE, FUNC_DRAW2),
            compose_card(COLOR_BLUE, FUNC_DRAW2),
            compose_card(COLOR_BLUE, FUNC_DRAW4),
            compose_card(COLOR_BLUE, FUNC_DRAW4),

            compose_card(COLOR_GREEN, FUNC_SKIP),
            compose_card(COLOR_GREEN, FUNC_SKIP),
            compose_card(COLOR_GREEN, FUNC_REVERSE),
            compose_card(COLOR_GREEN, FUNC_REVERSE),
            compose_card(COLOR_GREEN, FUNC_DRAW2),
            compose_card(COLOR_GREEN, FUNC_DRAW2),
            compose_card(COLOR_GREEN, FUNC_DRAW4),
            compose_card(COLOR_GREEN, FUNC_DRAW4),

            // 万能牌（变色、+4，各4张）
            compose_card(COLOR_ALL, FUNC_DRAW4),
            compose_card(COLOR_ALL, FUNC_DRAW4),
            compose_card(COLOR_ALL, FUNC_DRAW4),
            compose_card(COLOR_ALL, FUNC_DRAW4),
            compose_card(COLOR_ALL, FUNC_CHGCOLOR),
            compose_card(COLOR_ALL, FUNC_CHGCOLOR),
            compose_card(COLOR_ALL, FUNC_CHGCOLOR),
            compose_card(COLOR_ALL, FUNC_CHGCOLOR),
        };


    };

};
#pragma once

#include "card.h"
#include <vector>
#include <map>
#include "constants.h"
#include <memory>

#include "leptjson.h"

namespace uno
{
    struct Room;
    struct RoomSnapshot;

    class GameStater
    {
        using RoomPtr = std::shared_ptr<Room>;
    public:
        struct Stat
        {
            // 常规统计项
            int game_count = 0; // 游戏次数

            int total_draw = 0; // 总共抽牌数量
            int draw_by_sys = 0; // 被系统举报
            int draw_by_report = 0; // 被举报抽牌数量
            int draw_by_draw_card = 0; // 由于加抽牌被抽到的数量
            std::map<int, int> draw_cards; // 抽到各种牌的数量
            std::map<int, int> draw_black_cards; // 抽到各种王牌的数量

            int total_play = 0; // 总共打牌的数量
            int play_ahead = 0; // 抢牌数量
            std::map<int, int> play_cards; // 打出各种颜色牌的数量
            std::map<int, int> play_black_cards; // 打出各种王牌的数量

            int report = 0; // 举报别人次数
            int be_reported = 0; // 被举报次数


            int uno = 0; // UNO次数
            int forgot_uno = 0; // 忘记UNO次数

            int win = 0; //获胜次数

            // 特殊行为项
            int sp_forbid_others_uno = 0; // 阻止别人UNO的次数
            int sp_cause_others_win = 0; // 导致他人获胜计数
            int sp_draw_others_eat_by_self = 0; // 罚别人但最终被自己吃掉的次数
            int sp_draw_by_others_max = 0; // 单次被别人喂的牌的最大次数

            Stat() = default;
            ~Stat() = default;

            static int get_val(std::map<int, int>& map, int key)
            {
                auto it = map.find(key);
                return it == map.end() ? 0 : it->second;
            }
        };

        struct DisplayStat
        {
            size_t id; // 事件ID
            int uid; // 玩家ID
            int counter; // 当前计数
            int score; // 积分
        };;

        lept_value get_stat(int uid);
        std::vector<DisplayStat>& get_display_stat();
        int get_winner_stealer();


        void on_game_start(RoomPtr room, int sender, RoomSnapshot* room_snapshot);
        void on_player_left(RoomPtr room, int uid, std::string reason);

        void on_player_win(RoomPtr room, int sender, int winner);

        void on_card_deal(RoomPtr room, int uid, int rest_count,
            int deal_count,std::vector<card_t>& deal_cards,
            DEAL_CARD_REASON reason, int report_by, int cursor);

        void on_card_play(RoomPtr room, int uid, card_t c, bool need_uno,
            bool with_uno, int rest_count,
            std::vector<card_t>& cards, bool ahead);

        template<typename... Args>
        void on_event(std::string funcname, Args... args)
        {
#define APPLY(func) \
    if (funcname == #func) \
    { \
        /* 检查当前传入的 Args... 是否能用来调用 func */ \
        if constexpr (std::is_invocable_v<decltype(&GameStater::func), GameStater*, Args...>) \
        { \
            /* 只有参数匹配时，编译器才会生成这行代码 */ \
            /* 使用 lambda 捕获 this 来调用成员函数 */ \
            std::apply([this](auto&&... params){ \
                this->func(std::forward<decltype(params)>(params)...); \
            }, tup); \
            return; \
        } \
    }

            auto tup = std::make_tuple(args...);
            APPLY(on_game_start)
            APPLY(on_player_left)
            APPLY(on_player_win)
            APPLY(on_card_deal)
            APPLY(on_card_play)

#undef APPLY
        }

    private:


        bool m_gaming = false;
        std::unordered_map<int, Stat> m_round_stat;
        std::vector<DisplayStat> m_display_stat;
        int m_draw_starter = -1; // 开始罚牌的人
        int m_last_play_draw = -1; // 最后一次加牌的人

        // 点炮追踪器
        int m_last_color_chger = -1; //  最后一次改变颜色的人
        int m_last_color = -1; // 最后一次改变颜色的颜色
        int m_last_play_card_by = -1; // 最后一次出牌的人
        int m_last_play_card = -1; // 最后一次出牌的牌
        bool m_can_trace_winner_causer = false; // 是否可以开始追踪点炮的人

        void check_uno_traceable(RoomPtr room);
        int check_winner_causer(int uid, card_t c, bool ahead); // return player

        struct StatEvent
        {
            int id; // 事件ID
            std::function<int(Stat& stat)> getField; // 获取字段
            int score; // 单项积分
            int minDisplayCount; // 最低展示计数
            // 积分 = （当前计数 - 最低展示计数） * 单项积分
        };

    };

}

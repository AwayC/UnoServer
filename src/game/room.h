//
// Created by AWAY on 25-11-19.
//

#include <assert.h>
#include "../core/router.h"
#include "../core/session.h"
#include "../core/ssmgr.h"
#include "../core/errc.h"
#include "helper.h"
#include "card.h"
#include "constants.h"
#include <vector>
#include "stater.h"

namespace uno
{
    using time_stamp = std::chrono::system_clock::time_point;

    struct Room : public std::enable_shared_from_this<Room>
    {
        const int CARD_SET_COUNT = 3; // 固定发三套牌
        const int PLAYER_START_CARD_COUNT = 7; // 玩家起手牌数量
        const int DEALER_START_CARD_EXT_COUNT = 2; // 庄稼额外起手牌数量

        // 以Tick计，一个Tick 100毫秒
        const int TIMER_DEALING_INTERVAL = 5; // 发牌间隔
        const int TIMER_SELECT_FIRST_CARD_INTERVAL = 2; // 选取第一张牌的时间间隔
        const int TIMER_PLAYER_THINKING = 20 * 10; // 玩家思考时间

        const int PUNISH_FOR_BAD_UNO = 2; // 乱喊UNO罚两张
        const int PUNISH_FOR_LAST_FUNC_CARD = 2; //以功能牌结尾，罚两张
        const int PUNISH_FOR_NO_UNO = 2; // 没有喊UNO罚牌

        const int PLAYER_AUTO_TICK_OFFLINE_TIME = 1 * 60 * 1000; // 超过1分钟自动踢出房间
        const int PLAYER_AUTO_PLAY_OFFLINE_TIME = 1 * 60 * 1000; // 超过1分钟自动托管
        const int PLAYER_AUTO_PLAY_THINKING_TIME = TIMER_PLAYER_THINKING * 0.85; // 自动托管时减少思考时间

        const int OWNER_UPDATE_SEAT_INTERVAL = 10 * 1000; // 允许房主重排座位的倒计时
        const int PLAYER_USE_VOICE_INTERVAL = 10 * 1000; // 玩家使用音效的倒计时


        enum class State
        {
            idle, // 空闲
            playing // 游戏中
        };

        enum class GameState
        {
            idle,
            dealing, // 发牌状态
            dealing_dealer, // 庄家发牌状态
            select_first_card, // 抽取首张牌
            wait_player // 等待玩家思考
        };

        struct Player
        {
            std::string nick;   // 昵称
            std::vector<card_t> rest;   // 剩余手牌 （state == 1)
            bool ready = false;     // 是否就绪 （state == 0)
            bool offline = false;   // 是否离线
            time_stamp m_last_save_time = time_stamp::min(); // 最后保存时间
        };

        int id = 0;
        State state = State::idle;
        std::string title;  // 房间标题
        int owner = 0;    // 所有人
        int max_players = 5;    // 房间大小
        int curr_players = 1;    // 当前玩家数

        std::vector<Player> players; // 玩家列表

        //seq: [] 桌面顺序
        int last_win = 0;   // 最后赢家
        int last_direction = -1;    // 上把的方向
        time_stamp last_update_seat_time = time_stamp::min(); // 上把重排座位的时间

        GameState game_state = GameState::idle; // 游戏状态 (state == 1)
        int deal_turn = 0; // 发牌轮数 （state == 1）
        int cursor = 0; // 当前玩家 （state == 1）
        int direction = 1; // 方向 （state == 1）
        int dealer = 1000;  // 庄家UID (state == 1)
        int timer = 0; // 当前倒计时 （state == 1）
        // heep: [] 牌堆
        card_t last = 0;  // 最后一张牌
        int last_chg_color_ = 0; // 最后一次变化的颜色
        int draw = 0; // 连续罚牌计数
        int last_can_report = -1; // 可以被举报的玩家
        bool can_play_ahead = false; // 是否允许抢牌
        std::stack<card_t> history;

        GameStater stater;
    };

    using RoomPtr = std::shared_ptr<Room>;

    class RoomManager
    {
    public:
        static RoomManager& instance()
        {
            static RoomManager ins;
            return ins;
        }

        struct RoomSnapshot
        {
            int id;
            Room::State state;
            std::string title;
            int owner;
            int max_players;
            int curr_players;
            std::vector<Room::Player> players;
            // seq;
            int last_win;
            Room::GameState game_state;
            int cursor;
            int direction;
            int dealer;
            int timer;
            int last;
            int last_chg_color;
            int draw;
            int last_can_report;
            int can_play_ahead;

            // 额外对局状态
            std::vector<card_t> my_cards;
        };

        struct PlayerSnapshot
        {
            std::string nick;
            std::string email;
            int rest_count;
            bool ready;
            bool offline;
        };


    private:
        std::vector<RoomPtr> m_rooms;
        int m_next_room_id;

        RoomSnapshot get_snapshot(RoomPtr, int uid);
        void game_start(RoomPtr room);


    };
}
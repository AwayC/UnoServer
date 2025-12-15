//
// Created by AWAY on 25-11-19.
//
#pragma once

#include <assert.h>
#include "../core/router.h"
#include "../core/session.h"
#include "../core/ssmgr.h"
#include "../core/errc.h"
#include "helper.h"
#include "card.h"
#include "constants.h"
#include <unordered_map>
#include "stater.h"

#define ROOM_NOW        std::chrono::system_clock::now()

namespace uno
{
    using time_point = std::chrono::system_clock::time_point;

    struct Room : public std::enable_shared_from_this<Room>
    {
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
            std::string email;   // 邮箱
            std::vector<card_t> rest;   // 剩余手牌 （state == 1)
            bool ready = false;     // 是否就绪 （state == 0)
            bool offline = false;   // 是否离线
            time_point offline_time;    // 上次离线时间
            time_point last_use_voice_time; // 上一次使用音效的时间
        };

        using PlayerMap = std::unordered_map<int, Player>;

        int id;
        State state = State::idle;
        std::string title;  // 房间标题
        int owner = 0;    // 所有人
        int max_players = 0;    // 房间大小
        int curr_players = 0;    // 当前玩家数

        PlayerMap players; // 玩家列表

        std::vector<int> seq; //seq: [] 桌面顺序
        int last_win = -1;   // 最后赢家
        int last_direction = -1;    // 上把的方向
        time_point last_update_seat_time = time_point::min(); // 上把重排座位的时间

        GameState game_state = GameState::idle; // 游戏状态 (state == 1)
        int deal_turn = 0; // 发牌轮数 （state == 1）
        int cursor = 0; // 当前玩家 （state == 1）
        int direction = 0; // 方向 （state == 1）
        int dealer = 0;  // 庄家UID (state == 1)
        int timer = 0; // 当前倒计时 （state == 1）
        std::vector<card_t> heap; // heep: [] 牌堆
        card_t last = 0;  // 最后一张牌
        int last_chg_color = 0; // 最后一次变化的颜色
        int draw = 0; // 连续罚牌计数
        int last_can_report = -1; // 可以被举报的玩家
        bool can_play_ahead = false; // 是否允许抢牌
        std::vector<int> history;

        GameStater stater;
    };

    using RoomPtr = std::shared_ptr<Room>;

    struct PlayerSnapshot
    {
        std::string nick;
        std::string email;
        int rest_count;
        bool ready;
        bool offline;
    };

    struct RoomSnapshot
    {
        int id;
        Room::State state;
        std::string title;
        int owner;
        int max_players;
        int curr_players;
        std::unordered_map<int, PlayerSnapshot> players;
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

    class RoomManager
    {
    public:
        static RoomManager& instance()
        {
            static RoomManager ins;
            return ins;
        }

        void update(time_point now);

        /**
         * c2s funcs
         */
        void create_room_req(SessionPtr ss, const std::string& title, int player_count);

        void enter_room_req(SessionPtr ss, int room_id, bool re_enter);

        void leave_room_req(SessionPtr ss, int room_id);

        void get_ready_req(SessionPtr ss, int room_id);

        void shuffle_room_seats_req(SessionPtr ss, int room_id);

        void room_use_voice_req(SessionPtr ss, int room_id, int voice_id);

        void room_kickout_player_req(SessionPtr ss, int room_id, int be_kicked);

        void start_game_req(SessionPtr ss, int room_id);

        void game_play_card_req(SessionPtr ss, int room_id, card_t c, bool with_uno, card_t chg_color);

        void game_deal_card_req(SessionPtr ss, int room_id);

        void game_report_no_uno_req(SessionPtr ss, int room_id);

        void get_room_list_req(SessionPtr ss);

    private:
        using RoomMap = std::unordered_map<int, RoomPtr>;

        template<typename... Args>
        void send_event(RoomPtr room, int uid, int sender, const std::string& event, Args&&... args)
        {
            auto ss = Ssmgr::instance().find_session(uid);
            if (ss)
            {
                ss->call("room_event_ntf", room->id, sender, event, args...);
            } else
            {
                std::cerr << "send_event: ignore event " << event
                    << ", uid: " << uid << " , room: " << room->id
                    << std::endl;
            }
        }

        template<typename... Args>
        void broadcast_event(RoomPtr room, int sender, const std::string& event, Args&&... args)
        {
            for (auto& [uid, player] : room->players)
            {
                if (uid != sender)
                    send_event(room, uid, sender, event, args...);
            }
        }

        template<typename... Args>
        void call_stat_event(bool card_deal, RoomPtr room, int uid, int sender, const std::string& event, Args&&... args)
        {
            if (card_deal && event == "card_deal" && uid == sender)
            {
                try
                {
                    room->stater.on_event(event, room, sender, args...);
                } catch (const std::exception& e)
                {
                    std::cerr << "Unexpected error while do on_event, event " << event << std::endl;
                    std::cerr << e.what() << std::endl;
                }
            } else if (event != "card_deal")
            {
                try
                {
                    room->stater.on_event(event, room, sender, args...);
                } catch (const std::exception& e)
                {
                    std::cerr << "Unexpected error while do on_event, event " << event << std::endl;
                    std::cerr << e.what() << std::endl;
                }
            }
        }

        static lept_value get_snapshot(RoomPtr room, int uid);
        static lept_value get_player_snapshot(RoomPtr room, int uid);

        void game_start(RoomPtr room);

        void game_over(RoomPtr room, int uid);

        RoomMap::iterator game_dismiss(RoomMap::iterator room_it);

        Room::PlayerMap::iterator game_player_leave(RoomPtr room, int uid, player_left_reason reason);

        int game_cursor_move(RoomPtr room, int base, int step);

        card_t game_card_heap_deal(RoomPtr room);

        bool game_player_card_play(RoomPtr room, int uid, card_t c, bool with_uno, int chg_color);

        bool game_player_deal_card(RoomPtr room, int uid, int count, deal_card_reason reason, int req_by);

        bool game_player_report_no_uno(RoomPtr room, int uid);

        bool game_can_play_card(RoomPtr room, card_t c, bool ahead);

        RoomMap::iterator game_update(RoomMap::iterator room_it, time_point now);

        // 如果没有玩家了，清理房间, 必须是存在的room
        RoomMap::iterator clean_room(RoomMap::iterator room_it)
        {
            if (room_it->second->curr_players <= 0)
            {
                return m_rooms.erase(room_it);
            }

            return ++room_it;
        }


        RoomMap m_rooms;
        int m_next_room_id; // room_id > 0

        int generateIDBaseTime()
        {
            auto now = ROOM_NOW;
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            size_t id = (static_cast<size_t>(millis) & 0x7FFF) << 16;
            return id;
        }

        RoomManager()
        {
            m_next_room_id = generateIDBaseTime();

            // register event
            evtcenter.on("login", [this](SessionPtr ss)
            {
                int uid = ss->uid();
                lept_value& data = ss->data();
                if (!data.is<lept_value::object_t>())
                {
                    return ;
                }

                auto& data_obj = data.get<lept_value::object_t>();
                auto room_id_it = data_obj.find("room_id");
                if (room_id_it == data_obj.end() || !room_id_it->second.is<int>())
                {
                    return ;
                }
                int room_id = room_id_it->second.get<int>();

                auto room = m_rooms.find(room_id);
                if (room == m_rooms.end())
                {
                    return ;
                }

                auto player = room->second->players.find(uid);
                if (player == room->second->players.end())
                {
                    return ;
                }

                // 找到玩家，设置上线
                player->second.offline = true;
                player->second.offline_time = ROOM_NOW;

                std::cout << "login: broadcast online event for uid " << uid << std::endl;
                broadcast_event(room->second, uid, "player_online");
            });

            evtcenter.on("logout", [this](SessionPtr ss, std::string reason)
            {
                int uid = ss->uid();
                lept_value& data = ss->data();
                if (!data.is<lept_value::object_t>())
                {
                    return ;
                }

                int room_id = data["room_id"].get<int>();
                if (room_id < 0)
                {
                    return ;
                }

                auto room = m_rooms.find(room_id);
                if (room == m_rooms.end())
                {
                    return ;
                }

                auto player = room->second->players.find(uid);
                if (player == room->second->players.end())
                {
                    return ;
                }

                // 找到玩家，设置离线
                player->second.offline = true;
                player->second.offline_time = ROOM_NOW;

                std::cout << "logout: broadcast offline event for uid " << uid << std::endl;
                broadcast_event(room->second, uid, "player_offline");
            });
        }


    };

#define ROOM_MGR RoomManager::instance()
}
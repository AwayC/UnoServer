//
// Created by AWAY on 25-10-14.
//

#pragma once

namespace uno {

    enum class ErrCode{
        ok = 0,

        api_bad_req = -1000,
        api_internal_error = -1001,

        api_name_too_long = -1100,
        api_name_too_short = -1101,
        api_nick_too_long = -1102,
        api_nick_too_short = -1103,
        api_password_too_long = -1104,
        api_password_too_short = -1105,
        api_name_invalid = -1106,
        api_nick_invalid = -1107,
        api_bad_password = -1108,
        api_bad_token = -1109,
        api_invalid_call = -1110,
        api_nick_in_use = -1111,
        api_email_invalid = -1112,

        api_db_error = -1200,
        api_connect_reenter = -1201,
        api_room_not_found = -1202,
        api_room_is_in_game = -1203,
        api_is_full = -1204,
        api_room_invalid_title = -1205,
        api_room_invalid_param = -1206,
        api_room_not_ready = -1207,
        api_game_cannot_play_card = -1208,
        api_game_cannot_deal_card = -1209,
        api_game_cannot_report_no_uno = -1210,
        api_room_not_owner = -1211,
        api_room_update_seat_cooldown = -1212,
        api_room_use_voice_cooldown = -1213,
        api_room_player_not_found = -1214,
        api_room_cannot_leave_at_this_time = -1215,

        db_exists = -2000,
        db_not_exists = -2001,

    };

};
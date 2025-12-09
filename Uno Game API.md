# Uno Game API

### 1. HTTP api

1. **注册**

   - URL : `/api/register`

   - body : 

     ``` json
     { 
       "name" : name, // string
       "password" : md5(passowrd) // md5加密, string
       "email" : email // string
     }
     ```

   - Response : 

     ```json
     /* error */
     {
       "code" : ErrCode, // number
      	"msg" : msg // string 
     }
     
     /* success */
     { 
     	"code" : ErrCode, // number
       "msg" : msg, // string
       "data" : { 
       	"token" : token // string
       }
     }
     ```
   
1. **登陆**

   - URL : `/api/login`

   - body :

     ```json 
     { 
     	"name" : name, // string 
       "password" : md5(password) // string
       "email" : email // string 
     }
     ```

   - Response : 

     ``` json
     /* error */
     {
       "code" : ErrCode, // number
      	"msg" : msg // string 
     }
     
     /* success */
     { 
     	"code" : ErrCode, // number
       "msg" : msg, // string
       "data" : { 
       	"token" : token // string
       }
     }
     ```
   
1. **更新邮箱**

   - URL : `/api/update_email`

   - body ：

     ```json
     { 
     	"email" : email, // string
       "token" : token // string
     }
     ```
   
   - Response : 
   
     ```json
     { 
     	"code" : ErrCode, // number
       "msg" : msg // string
     }
     ```
   
1. **更新密码**

   - URL : `/api/update_password`

   - body : 

     ```json
     { 
     	"password" : md5(password), // string
       "token" : token, // string
     }
     ```
   
   - Response : 
   
     ```json
     { 
     	"code" : ErrCode, // number
       "msg" : msg // string
     }
     ```

---

### 2. WebSocket api

#### c2s : 

1. **lobby**
   - `['login_req', token]`
   - `['logout_req']`
   - `['create_role_req', nick]`

2. **room**
	- `['create_room_req', title, player_count]`
	- `['enter_room_req', room_id, re_enter]`
	- `['leave_room_req', room_id]`
	- `['get_ready_req', room_id]`
	- `['shuffle_room_seats_req', room_id]`
	- `['room_use_voice_req', room_id, voice_id]`
	- `['room_kickout_player_req', room_id, be_kicked]`
	- `['start_game_req', room_id]`
	- `['game_play_card_req', room_id]`
	- `['game_deal_card_req', room_id]`
	- `['game_report_no_uno_req', room_id]`
	- `['get_room_list_req']`

#### s2c: 

1. **lobby**
	- `['login_rsp', ErrCode]`
	- `['create_role_rsp', ErrCode]`
	- `['create_role_ntf']`

2. **room**
	- `['create_room_rsp', ErrCode, room_snapshot]`
	- `['login_rsp', ErrCode, token, base_data]`
	- `['room_event_ntf', ErrCode, room_id, sender, event, ...]`
	- `['enter_room_rsp', ErrCode, room_snapshot]`
	- `['leave_room_rsp', ErrCode]`
	- `['get_ready_rsp', ErrCode]`
	- `['shuffle_room_seats_rsp', ErrCode]`
	- `['room_use_voice_rsp', ErrCode]`
	- `['room_kickout_player_rsp', ErrCode]`
	- `['start_game_rsp', ErrCode]`
	- `['game_play_card_rsp', ErrCode]`
	- `['game_deal_card_rsp', ErrCode]`
	- `['game_report_no_uno_rsp', ErrCode]`
	- `['get_room_list_rsp', room_list, session_count]`
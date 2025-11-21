cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(UnoServer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(third-party/sqlite3)
add_subdirectory(third-party/MyWebsocket)

add_library(UnoServer
        src/core/ssmgr.cpp
        src/core/db.cpp
        src/core/server.cpp
        src/core/router.cpp
        src/core/background.cpp
        src/core/config.h
        src/core/config.cpp


        src/game/card.h
        src/game/constants.h
        src/game/dbagent.h
        src/game/helper.h
        src/game/lobby.h
        src/game/lobby.cpp
        src/game/stater.h
        src/game/stater.cpp
        src/game/room.h
        src/game/room.cpp
)



target_link_libraries(UnoServer
        sqlite3
        SQLiteCpp
        websocket
        leptjson
)

target_include_directories(UnoServer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

add_executable(main
    main.cpp
)

target_link_libraries(main
    UnoServer
)

# test sqlite
add_executable(test_sqlite test/test_db/test_sqlite.cpp)
target_link_libraries(test_sqlite
    sqlite3
    SQLiteCpp
)

#test background
add_executable(test_background test/test_background.cpp)
target_link_libraries(test_background
    websocket
    UnoServer
)

#test db
add_executable(test_db test/test_db/test_db.cpp)
target_link_libraries(test_db
    sqlite3
    SQLiteCpp
    UnoServer
    websocket
)

#test router
add_executable(test_router test/test_router.cpp)
target_link_libraries(test_router
    UnoServer
)

#test helper
add_executable(test_helper test/test_helper.cpp)
target_link_libraries(test_helper
    UnoServer
)
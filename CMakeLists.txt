cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(UnoServer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third-party/MyWebSocket)

# --- MySQL Connector/C++ (X DevAPI via Homebrew) Configuration ---
set(MYSQL_CONNECTOR_DIR "/opt/homebrew/opt/mysql-connector-c++")

find_path(MYSQL_CONNECTOR_INCLUDE_DIR mysqlx/xdevapi.h
    HINTS ${MYSQL_CONNECTOR_DIR}/include
)

find_library(MYSQL_CONNECTOR_LIBRARY NAMES mysqlcppconnx
    HINTS ${MYSQL_CONNECTOR_DIR}/lib
)

if (MYSQL_CONNECTOR_INCLUDE_DIR AND MYSQL_CONNECTOR_LIBRARY)
    message(STATUS "Found MySQL Connector/C++ (X DevAPI) Include: ${MYSQL_CONNECTOR_INCLUDE_DIR}")
    message(STATUS "Found MySQL Connector/C++ (X DevAPI) Library: ${MYSQL_CONNECTOR_LIBRARY}")
else()
    message(WARNING "MySQL Connector/C++ (X DevAPI) not found via Homebrew.")
endif()
# ------------------------------------------------------

add_library(UnoServer
        src/core/ssmgr.cpp
        src/core/db.cpp
        src/core/server.cpp
        src/core/router.cpp
        src/core/background.cpp
        src/core/config.h
        src/core/config.cpp
        src/core/EventEmitter.h

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

        src/util/ssl.h
)



target_link_libraries(UnoServer
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

#test background
add_executable(test_background test/test_background.cpp)
target_link_libraries(test_background
    websocket
    UnoServer
)

#test db
add_executable(test_db test/test_db/test_db.cpp)
target_link_libraries(test_db
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

#test eventemitter
add_executable(test_event_emitter test/test_event_emitter.cpp)
target_link_libraries(test_event_emitter
    UnoServer
)

#test api
add_executable(test_api test/test_example/test.cpp)
target_link_libraries(test_api
    UnoServer
)

#test mysql
add_executable(test_mysql test/test_db/test_mysql.cpp)
target_link_libraries(test_mysql
    ${MYSQL_CONNECTOR_LIBRARY}
)
target_include_directories(test_mysql PRIVATE
        ${MYSQL_CONNECTOR_INCLUDE_DIR}
)

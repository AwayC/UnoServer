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
        src/core/session.cpp
        src/core/db.cpp
        src/core/errc.cpp
        src/core/server.cpp
        src/core/router.cpp
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

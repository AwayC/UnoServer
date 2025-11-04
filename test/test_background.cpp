//
// Created by AWAY on 25-11-4.
//

#include "core/background.h"
#include "uv.h"
#include <chrono>

uv_loop_t *g_loop;
uno::Background* g_bg;
uv_async_t g_async;
uno::ThreadQue<uno::BackCallback> g_que;

void background_handler(uv_async_t* handle)
{

}

int main()
{
    g_loop = uv_default_loop();
    uv_async_init(g_loop, &g_async, background_handler);



    return 0;
}

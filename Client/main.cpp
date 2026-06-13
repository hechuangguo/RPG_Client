/**
 * @file    main.cpp
 * @brief   RPG Client 程序入口
 *
 * 初始化 ClientLogger 并启动 GameApp 主循环。
 */

#include "app/GameApp.h"
#include "log/ClientLogger.h"

int main()
{
    if (!ClientLogger::instance().init(false))
    {
        return 1;
    }

    ClientLogger::instance().info("RPGClient starting");

    GameApp app;
    const int code = app.run();

    ClientLogger::instance().info("RPGClient exit code=%d", code);
    ClientLogger::instance().flush();
    return code;
}

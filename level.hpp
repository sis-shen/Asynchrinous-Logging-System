#pragma once

namespace suplog{
    class LogLevel
    {
    public:
        enum class Level{
            DEBUG = 1,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };

        static const char* toString(LogLevel::Level lv)
        {
            switch(lv)
            {
                //在内部定义一个宏函数,把变量名name变成字符串
                #define TOSTRING(name) #name
                case LogLevel::Level::DEBUG: return TOSTRING(DEBUG);//直接返回，不加用break
                case LogLevel::Level::INFO: return TOSTRING(INFO);
                case LogLevel::Level::WARN: return TOSTRING(WARN);
                case LogLevel::Level::ERROR: return TOSTRING(ERROR);
                case LogLevel::Level::FATAL: return TOSTRING(FATAL);
                case LogLevel::Level::OFF: return TOSTRING(OFF);
                #undef TOSTRING //取消宏函数
                default: return "UNKOWN";

            }

            return "UNKNOWN";//防止编译报错
        }

    };
}
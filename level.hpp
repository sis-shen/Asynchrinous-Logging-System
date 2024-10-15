#pragma once

//提供日志等级服务
//提供日志等级枚举
//提供日志等级转字符串

namespace suplog{
    class LogLevel
    {
    public:
        //枚举所有的日志等级
        enum class Level
        {
            DEBUG = 1,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };

        //提供日志等级转换成字符串的静态成员函数
        static const char* toString(LogLevel::Level lv)
        {
            switch(lv)
            {
                //在内部定义一个宏函数,把变量名name变成字符串
                //宏函数能较少函数调用，提高运行效率
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

            //提供默认返回值
            return "UNKNOWN";//防止编译报错
        }

    };
}
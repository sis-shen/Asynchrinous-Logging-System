#pragma once

#include "logger.hpp"

namespace suplog
{
    Logger::ptr getLogger(const std::string& name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }

    Logger::ptr rootLogger()
    {
        return LoggerManager::getInstance().rootLogger();
    }

    bool hasLogger(const std::string& name)
    {
        return LoggerManager::getInstance().hasLogger(name);
    }

    #define WOW 6666

    //开始定义宏函数
    #define debug(fmt,...)  debug(__FILE__,__LINE__,fmt,##__VA_ARGS__)//利用宏，自动传入文件路径和行号
    #define info(fmt,...)  info(__FILE__,__LINE__,fmt,##__VA_ARGS__)
    #define warn(fmt,...)  warn(__FILE__,__LINE__,fmt,##__VA_ARGS__)
    #define error(fmt,...)  error(__FILE__,__LINE__,fmt,##__VA_ARGS__)
    #define fatal(fmt,...)  fatal(__FILE__,__LINE__,fmt,##__VA_ARGS__)

    //封装真正能用的宏函数
    #define LOG_DEBUG(logger,fmt,...) (logger)->debug(fmt,##__VA_ARGS__)//这里就用了上面的宏函数
    #define LOG_INFO(logger,fmt,...) (logger)->info(fmt,##__VA_ARGS__)
    #define LOG_WARN(logger,fmt,...) (logger)->warn(fmt,##__VA_ARGS__)
    #define LOG_ERROR(logger,fmt,...) (logger)->error(fmt,##__VA_ARGS__)
    #define LOG_FATAL(logger,fmt,...) (logger)->fatal(fmt,##__VA_ARGS__)

    //封装主函数的调用
    #define LOGD(fmt,...) LOG_DEBUG(suplog::rootLogger(),fmt,##__VA_ARGS__)
    #define LOGI(fmt,...) LOG_INFO(suplog::rootLogger(),fmt,##__VA_ARGS__)
    #define LOGW(fmt,...) LOG_WARN(suplog::rootLogger(),fmt,##__VA_ARGS__)
    #define LOGE(fmt,...) LOG_ERROR(suplog::rootLogger(),fmt,##__VA_ARGS__)
    #define LOGF(fmt,...) LOG_FATAL(suplog::rootLogger(),fmt,##__VA_ARGS__)
}

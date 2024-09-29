#pragma once

#include "util.hpp"
#include "level.hpp"
#include <thread>//c++多线程库
#include <memory>
#include <string>

namespace suplog{
    struct LogMsg
    {
        //声明一个全局且唯一的指针
        using ptr = std::shared_ptr<LogMsg>;
        std::string _name;  //日志器名称
        std::string _file;  //文件名
        size_t _line;       //行号
        std::string _payload;//日志消息
        size_t _ctime;      //时间-时间戳
        std::thread::id _tid;//线程id
        LogLevel::Level _level;//日志等级
        //默认构造函数
        LogMsg(){}
        //构造函数
        LogMsg(
            std::string name,
            std::string file,
            size_t line,
            std::string payload,
            LogLevel::Level level
        ):
        _name(name),
        _file(file),
        _payload(payload),
        _level(level),
        _line(line),
        _ctime(util::date::now()),
        _tid(std::this_thread::get_id()){}
    };
}
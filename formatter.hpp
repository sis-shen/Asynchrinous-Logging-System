#pragma once

#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include <memory>
#include <vector>
#include <tuple> //提供一种存储一组不同类型变量的容器


namespace suplog{
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual ~FormatItem(){}
        virtual void format(std::ostream& os,const LogMsg& msg) = 0;
    };

    class MsgFormatItem:public FormatItem
    {
    public:
        MsgFormatItem(const std::string& str = ""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<msg._payload;
        }
    };

    class LevelFormatItem:public FormatItem{
    public:
        LevelFormatItem(const std::string& str = ""){};
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<LogLevel::toString(msg._level);
        }
    };

    class NameFormatItem:public FormatItem{
    public:
        NameFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<msg._name;
        }
    };

    class ThreadFormatItem:public FormatItem{
    public:
        ThreadFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<msg._tid;
        }
    };

    class TimeFormatItem:public FormatItem{
    private:
        std::string _format;
    public:
        TimeFormatItem(const std::string& format="%H:%M:%S"):_format(format){
            if(format.empty()) _format = {"%H:%M:%S"};
        }
        virtual void format(std::ostream& os,const LogMsg& msg){
            time_t t = msg._ctime;
            struct tm lt;
            localtime_r(&t,&lt);//从时间戳t中提取时间信息到结构体lt中
            char tmp[128];
            strftime(tmp,127,_format.c_str(),&lt);//格式化日期信息到字符串
            os<<tmp;
        }
    };

    class CFileFormatItem:public FormatItem{
    public:
        CFileFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<msg._file;
        }
    };

    class CLineFormatItem:public FormatItem{
    public:
        CLineFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<msg._line;
        }
    };

    class TabFormatItem:public FormatItem{
    public:
        TabFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<'\t';
        }
    };        

    class NLineFormatItem:public FormatItem{
    public:
        NLineFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<'\n';
        }
    };

    class OtherFormatItem:public FormatItem{
    private:
        std::string _str;
    public:
        OtherFormatItem(const std::string &str=""):_str(str){}
        
    }
}
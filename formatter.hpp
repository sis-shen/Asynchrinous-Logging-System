#pragma once

#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include <memory>
#include <vector>
#include <tuple> //提供一种存储一组不同类型变量的容器


namespace suplog{

    /*=========FormatItem及其派生类===========*/
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
        virtual void format(std::ostream& os,const LogMsg& msg){
            os<<_str;
        }
    };

    /*===============END==============*/

    class Formatter
    {
    public:
        using ptr = std::shared_ptr<Formatter>;
        /*
            %d ⽇期
            %T 缩进
            %t 线程id
            %p ⽇志级别
            %c ⽇志器名称
            %f ⽂件名
            %l ⾏号
            %m ⽇志消息
            %n 换⾏
            */

        Formatter(const std::string& pattern = "[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n")
        :_pattern(pattern)
        {
            assert(parsePattern());//调用成员函数，后面声明
        }

        //简单的查看pattern的接口
        const std::string pattern() {return _pattern;}
        //最后实现，但是是最常用的接口，故写在前面
        std::string format(const LogMsg& msg)
        {
            std::stringstream ss;
            for(auto& it: _items)
            {
                it->format(ss,msg);
            }
            return ss.str();
        }

        //创建FormatItem(给调用者使用多态)
        FormatItem::ptr createItem(const std::string& fc,const std::string& subfmt)
        {
            if(fc == "m") return FormatItem::ptr(new MsgFormatItem(subfmt));
            if(fc == "p") return FormatItem::ptr(new LevelFormatItem(subfmt));
            if(fc == "c") return FormatItem::ptr(new NameFormatItem(subfmt));
            if(fc == "t") return FormatItem::ptr(new ThreadFormatItem(subfmt));
            if(fc == "n") return FormatItem::ptr(new NLineFormatItem(subfmt));
            if(fc == "d") return FormatItem::ptr(new TimeFormatItem(subfmt));
            if(fc == "f") return FormatItem::ptr(new CFileFormatItem(subfmt));
            if(fc == "l") return FormatItem::ptr(new CLineFormatItem(subfmt));
            if(fc == "T") return FormatItem::ptr(new TabFormatItem(subfmt));
            return FormatItem::ptr();//未知fc
        }
        //pattern解析

        //每个要素分为三部分：
        // 格式化字符 : %d %T %p...
        // 对应的输出⼦格式 ： {%H:%M:%S}
        // 对应数据的类型 ： 0-表⽰原始字符串，也就是⾮格式化字符，1-表⽰格式化数据类型

        //默认格式"[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n"
        bool parsePattern()
        {
            //储存分割好的字符串,是否为原始字符串由int指示
            std::vector<std::tuple<std::string,std::string,int>> arry;
            std::string format_key;//存放%之后的格式化字符
            std::string format_val;//存放格式化字符串后边{}中的子格式化字符串
            std::string string_row;//存放原始的非格式化字符串
            bool sub_format_error = false;
            int pos = 0;
            while(pos < _pattern.size())
            {
                if(_pattern[pos] != '%')
                {
                    string_row.push_back(_pattern[pos++]);
                    continue;
                }
                if(pos+1<_pattern.size() && _pattern[pos+1] == '%')
                {
                    //规定%%格式化为原生的'%'字符
                    string_row.push_back('%');
                    pos+=2;
                    continue;
                }
                if(string_row.empty() == false)//原生字符串不为空
                {
                    arry.push_back(std::make_tuple(string_row,"",0));
                    string_row.clear();
                }

                //当前位置指向%字符
                pos++;//pos指向格式化字符位置
                if(pos<_pattern.size() && isalpha(_pattern[pos]))//判断是不是字符
                {
                    format_key = _pattern[pos];
                }
                else 
                {
                    std::cout<< &_pattern[pos-1]<<"位置附近格式错误！\n";
                    return false;
                }
                //下一步使pos指向格式化字符的下个位置，判断是否包含有子格式,例如在%d{%Y-%m-%d}中
                pos++;
                if(pos+1 < _pattern.size() && _pattern[pos] == '{')
                {
                    sub_format_error = true;
                    pos++;//pos指向花括号下一个字符
                    while (pos<_pattern.size())
                    {
                        if(_pattern[pos] == '}')
                        {
                            //循环出口
                            sub_format_error = false;
                            pos++;
                            break;
                        }
                        //在循环中时
                        format_val.push_back(_pattern[pos++]);
                    }
                    if(sub_format_error)
                    {
                        std::cout<<"{}对应出错\n";
                        return false;
                    }
                }
                arry.push_back(std::make_tuple(format_key,format_val,1));
                format_key.clear();
                format_val.clear();
            }//结束循环

            if(string_row.empty() == false)//原生字符串不为空
                arry.push_back(std::make_tuple(string_row,"",0));
            if(format_key.empty() == false)//格式化字符不为空，注，上下顺序不能换,要和循环内一致
                arry.push_back(std::make_tuple(format_key,format_val,1));

            //开始拼接item列表
            if(_items.empty() == false)_items.clear();//清理_items
            for(auto& it:arry)
            {
                if(std::get<2>(it) == 0)//获取第三个元素
                {
                    FormatItem::ptr fi(new OtherFormatItem(std::get<0>(it)));
                    _items.push_back(fi);
                }
                else 
                {
                    FormatItem::ptr fi = createItem(std::get<0>(it),
                                                    std::get<1>(it));
                    if(fi.get() == nullptr)
                    {
                        std::cout<<"没有对应的格式化字符串： %"
                                 <<std::get<0>(it)
                                 <<std::endl;
                        return false;
                    }
                    _items.push_back(fi);
                }
            }//完成拼接
            return true;
        }

    private:
        std::string _pattern;
        std::vector<FormatItem::ptr> _items;
    };

}
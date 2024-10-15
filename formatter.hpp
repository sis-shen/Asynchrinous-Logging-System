#pragma once

#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include <memory>
#include <vector>
#include <tuple> //提供一种存储一组不同类型变量的容器

//提供日志信息嵌入格式化字符串的类实现
//FormatItem抽象类

namespace suplog{

    /*=========FormatItem及其派生类===========*/
    class FormatItem
    {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual ~FormatItem(){}
        //规定统一实现format接口
        virtual void format(std::ostream& os,const LogMsg& msg) = 0;
    };

    //日志消息输出
    class MsgFormatItem:public FormatItem
    {
    public:
        MsgFormatItem(const std::string& str = ""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<msg._payload;
            return;//没有实际意义，只是标记函数结束
        }
    };

    //日志等级输出
    class LevelFormatItem:public FormatItem{
    public:
        LevelFormatItem(const std::string& str = ""){};
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<LogLevel::toString(msg._level);
            return;
        }
    };

    //日志器名称输出
    class NameFormatItem:public FormatItem{
    public:
        NameFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<msg._name;
            return;
        }
    };

    //线程td输出
    class ThreadFormatItem:public FormatItem{
    public:
        ThreadFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<msg._tid;
            return;
        }
    };

    //格式化时间输出
    class TimeFormatItem:public FormatItem{
    public://共有接口写在最前面
        TimeFormatItem(const std::string& format="%H:%M:%S"):_format(format)
        {
            if(format.empty()) _format = {"%H:%M:%S"};//这里要获取格式串
        }
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            time_t t = msg._ctime;
            struct tm lt;
            localtime_r(&t,&lt);//从时间戳t中提取时间信息到结构体lt中
            char tmp[128];
            strftime(tmp,127,_format.c_str(),&lt);//格式化日期信息到字符串
            os<<tmp;//最后输出
            return;
        }
    private://私有接口写在后面
        std::string _format;
    };

    //日志调用者的文件名输出
    class CFileFormatItem:public FormatItem{
    public:
        CFileFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<msg._file;
            return;
        }
    };

    //日志调用行号输出
    class CLineFormatItem:public FormatItem{
    public:
        CLineFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<msg._line;
            return;
        }
    };

    //Tab输出
    class TabFormatItem:public FormatItem{
    public:
        TabFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<'\t';
            return;
        }
    };        

    //换行符输出
    class NLineFormatItem:public FormatItem{
    public:
        NLineFormatItem(const std::string& str=""){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<'\n';
            return;
        }
    };

    //其它日志信息输出
    class OtherFormatItem:public FormatItem{
    public://公共接口写在前面
        OtherFormatItem(const std::string &str=""):_str(str){}
        virtual void format(std::ostream& os,const LogMsg& msg) override
        {
            os<<_str;
            return;
        }
    private:
        std::string _str;
    };

    /*====End==Of==FormatItem及其派生类===========*/

    class Formatter
    {
    public:
        using ptr = std::shared_ptr<Formatter>;//内置一个指针类
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
        
        //使用格式串初始化,规则如上
        Formatter(const std::string& pattern = "[%d{%H:%M:%S}][%t][%p][%c][%f:%l] %m%n")
        :_pattern(pattern)
        {
            assert(parsePattern());//调用成员函数，后面声明
        }

        //简单的查看pattern的接口
        const std::string pattern() { return _pattern; }
        //最后实现，但是是最常用的接口，故写在前面
        //使用LogMsg对象生成日志字符串
        std::string format(const LogMsg& msg)
        {
            std::stringstream ss;//创建string流
            //按格式串生成日志信息
            for(auto& it: _items)
            {
                it->format(ss,msg);
            }
            return ss.str();
        }

        //创建FormatItem(给调用者使用多态)
        FormatItem::ptr createItem(const std::string& fc,const std::string& subfmt)
        {
            //根据传入的format character选择创建的FormatItem类

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
            std::vector<std::tuple<std::string,std::string,int>> arry;//储存分析后的原素材
            std::string format_key;//存放%之后的格式化字符
            std::string format_val;//存放格式化字符串后边{}中的子格式化字符串
            std::string string_row;//存放原始的非格式化字符串
            bool sub_format_error = false;
            int pos = 0;
            while(pos < _pattern.size())//开始扫描格式串
            {
                //储存原始字符
                if(_pattern[pos] != '%')
                {
                    string_row.push_back(_pattern[pos++]);
                    continue;//跳过循环的后半部分,创建尽可能长的原始字符串
                }
                if(pos+1<_pattern.size() && _pattern[pos+1] == '%')
                {
                    //规定%%格式化为原生的'%'字符
                    string_row.push_back('%');
                    pos+=2;
                    continue;//跳过循环的后半部分,创建尽可能长的原始字符串
                }
                if(string_row.empty() == false)//原生字符串不为空
                {
                    //准备好了一段素材
                    arry.push_back(std::make_tuple(string_row,"",0));
                    //清空容器
                    string_row.clear();
                }

                //当前位置指向%字符
                pos++;//pos指向格式化字符位置
                if(pos<_pattern.size() && isalpha(_pattern[pos]))//判断是不是字符
                {
                    //储存fc字符
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
                        //在循环中时,存储子格式
                        format_val.push_back(_pattern[pos++]);
                    }
                    if(sub_format_error)
                    {
                        std::cout<<"{}对应出错\n";
                        return false;
                    }
                }
                //储存格式字符创建的原素材
                arry.push_back(std::make_tuple(format_key,format_val,1));
                format_key.clear();
                format_val.clear();
            }//结束循环

            //继续处理未清空的容器/缓存
            if(string_row.empty() == false)//原生字符串不为空
                arry.push_back(std::make_tuple(string_row,"",0));
            if(format_key.empty() == false)//格式化字符不为空，注，上下顺序不能换,要和循环内一致
                arry.push_back(std::make_tuple(format_key,format_val,1));

            //开始产生item列表
            if(_items.empty() == false)_items.clear();//清理_items
            for(auto& it:arry)
            {
                if(std::get<2>(it) == 0)//获取第三个元素
                {
                    //生成非格式原始字符串
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
        std::string _pattern;//储存格式串
        std::vector<FormatItem::ptr> _items;//产生格式化字符串的生成器列表
    };

}
#pragma once

#include "util.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include <unistd.h>
#include <iostream>
#include <memory>
#include <mutex>

namespace suplog{
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink(){}
        virtual ~LogSink() =default;
        virtual void log(const char*data,size_t len) = 0;
    };

    class StdoutSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<StdoutSink>;
        StdoutSink()=default;
        
        void log(const char*data,size_t len) override{
            std::cout.write(data,len);
        }
    };

    class FileSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<FileSink>;
        FileSink(const std::string& filename):_filename(filename){
            //创建目录
            util::file::create_directory(util::file::path(filename));
            _ofs.open(_filename,std::ios::binary|std::ios::app);//二进制方式写入
            assert(_ofs.is_open());
        }

        //提供给外界获取文件名
        const std::string &file(){return _filename;}

        void log(const char*data,size_t len) override {
            _ofs.write((const char*)data,len);
            if(_ofs.good() == false){
                std::cout<<"日志输出文件失败!\n";
            }
        }
    private:
        std::string _filename;
        std::ofstream _ofs;
    };

    class RollSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<RollSink>;
        RollSink(const std::string& basename,size_t max_size)
        :_basename(basename),_max_fsize(max_size),_cur_fsize(0)
        {
            //创建目录
            util::file::create_directory(util::file::path(_basename));
        }

        void log(const char*data,size_t len) override {
            initLogFile();
            _ofs.write(data,len);
            if(_ofs.good() == false){
                std::cout<<"日志输出文件失败!\n";
            }
            _cur_fsize += len;
        }

    private:
        void initLogFile(){
            if(_ofs.is_open() == false || _cur_fsize >=_max_fsize)
            {
                _ofs.close();
                if(_cur_fsize >=_max_fsize) sleep(1);//防止同一秒有过多日志消息要打印，导致打开同一个日志文件
                std::string name = createFilename();
                _ofs.open(name,std::ios::binary | std::ios::app);
                assert(_ofs.is_open());
                _cur_fsize = 0;
                return;
            }
            return;
        }

        std::string createFilename()
        {
            time_t t = time(NULL);
            struct tm lt;
            localtime_r(&t,&lt);
            std::stringstream ss;
            //获取详细时间信息
            ss << _basename;
            ss<< lt.tm_year + 1900;
            ss <<lt.tm_mon + 1;
            ss << lt.tm_mday;
            ss << lt.tm_hour;
            ss << lt.tm_min;
            ss << lt.tm_sec;
            ss << ".log";
            return ss.str();
        }
    private:
        std::string _basename;
        std::ofstream _ofs;
        size_t _max_fsize;
        size_t _cur_fsize;
    };

    //创建简单工厂
    class SinkFactory{
    public:
        template<typename SinkType,typename ...Args>
        static LogSink::ptr create(Args&&...args){
            //返回shared_ptr,传的是构造函数参数列表
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
}
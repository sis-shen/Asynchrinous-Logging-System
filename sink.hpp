#pragma once

#include "util.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "logexception.hpp"
#include "DBUserConfig.hpp"
#include "looper.hpp"
#include <unistd.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>

namespace suplog{
    //声明抽象父类LogSink
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink(){}
        virtual ~LogSink() =default;
        virtual void log(const char*data,size_t len) = 0;//声明统一的log接口
    };

    //数据库落地
    class DatabaseSink:public LogSink
    {
        static const int QUEUE_MAX_SIZE = 1024;//默认设置消息队列的最大长度
    public:
        using ptr = std::shared_ptr<DatabaseSink>;
        DatabaseSink()
        :_running(true)
        ,_t(&DatabaseSink::consumer,this)
        {}

        //生产者
        void log(const char*data,size_t len)override
        {
            std::unique_lock<std::mutex>lock(_mutex);//加锁
            if(_msgQueue.size() >= QUEUE_MAX_SIZE)
            {
                _push_con.wait(lock,[&]{
                    return _msgQueue.size()<DatabaseSink::QUEUE_MAX_SIZE;
                });
            }
            //DEBUG
            std::cout<<"插入数据"<<std::string(data,len)<<std::endl;
            _msgQueue.push(std::string(data,len));
            lock.unlock();
            _pop_con.notify_all();
        }

        static void consumer(void* arg)
        {
            DatabaseSink* ds = (DatabaseSink*)arg;

            DBUserConfig::ptr user(DBUserConfig::getInstance());

            sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
            sql::Connection* conn = driver->connect(user->Address(),user->User(),user->Password());
            conn->setSchema(user->DataBase());
            sql::Statement* stm = conn->createStatement();//创建statement用于执行SQL语句

            while(true)
            {
                std::string sql_str;
                {
                    std::unique_lock<std::mutex> lock(ds->_mutex);
                    //这里顺序不能乱， 必须先等待，然后做判断
                    if(ds->_running)
                        ds->_pop_con.wait(lock,[&]{
                            return !ds->_msgQueue.empty() || !ds->_running;
                        });
                    if(ds->_running == false && ds->_msgQueue.empty())
                    {
                        // std::cout<<"子线程退出\n";
                        exit(0);//线程退出
                    }
                    sql_str = ds->_msgQueue.front();
                    ds->_msgQueue.pop();
                    ds->_push_con.notify_all();
                    //其次，这里尽量不要对lock进行unlock，用花括号先定一个作用域就行
                }
                //执行mysql语句
                stm->execute(sql_str);
            }
        }

        ~DatabaseSink()
        {
            //关闭连接
            _running = false;
            _pop_con.notify_all();
            _push_con.notify_all();
            _t.join();
        }

    private:
        std::mutex _mutex;
        std::condition_variable _push_con;
        std::condition_variable _pop_con;
        int _reconnect_cnt;
        std::atomic<bool> _running;
        std::queue<std::string> _msgQueue;
        std::thread _t;
    };

    //标准输出落地
    class StdoutSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<StdoutSink>;
        StdoutSink()=default;
        
        void log(const char*data,size_t len) override
        {
            std::cout.write(data,len);
            return;//标记函数结尾
        }
    };

    //文件落地类
    class FileSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<FileSink>;
        FileSink(const std::string& filename):_filename(filename)
        {
            //创建目录
            util::file::create_directory(util::file::path(filename));
            _ofs.open(_filename,std::ios::binary|std::ios::app);//二进制方式写入
            if(_ofs.is_open() == false)
            {
                throw LogException("FileSink: open file failed");//文件打开失败
            }
        }

        //提供给外界获取文件名
        const std::string &file(){ return _filename; }

        void log(const char*data,size_t len) override 
        {
            //写入数据
            _ofs.write((const char*)data,len);
            if(_ofs.good() == false)
            {
                //文件流状态异常
                throw LogException("FileSink:日志文件输出失败！");
            }
            return;
        }
    private:
        std::string _filename;//文件路径
        std::ofstream _ofs;//文件输出流
    };

    //滚动文件落地类
    class RollSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<RollSink>;
        //实际文件名 = bsename + 可变部分
        RollSink(const std::string& basename,size_t max_size)
        :_basename(basename),_max_fsize(max_size),_cur_fsize(0)
        {
            //创建目录
            util::file::create_directory(util::file::path(_basename));
        }

        void log(const char*data,size_t len) override 
        {
            initLogFile();//初始化输出文件和文件输出流
            _ofs.write(data,len);//写入数据
            if(_ofs.good() == false)
            {
                throw LogException("RollSink: 日志文件输出失败！");
            }
            _cur_fsize += len;//更新文件大小
            return;
        }

    private:
        void initLogFile(){
            //如果满足条件，触发创建新文件的条件
            if(_ofs.is_open() == false || _cur_fsize >=_max_fsize)
            {
                _ofs.close();//先关闭原有文件流
                if(_cur_fsize >=_max_fsize) sleep(1);//防止同一秒有过多日志消息要打印，导致打开同一个日志文件
                std::string name = createFilename();
                _ofs.open(name,std::ios::binary | std::ios::app);
                if(_ofs.is_open() == false)
                {
                    throw LogException("RollSink: init LogFile failed");
                }
                _cur_fsize = 0;//创建新文件后，重置文件大小
                return;
            }
            return;
        }
        
        //封装产生文件名的函数
        std::string createFilename()
        {
            //这里采用的格式为
            // basename + 年月日 + .log
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
        std::string _basename;//基础文件名
        std::ofstream _ofs;//输出用的文件输出流
        size_t _max_fsize;//文件最大容量
        size_t _cur_fsize;//当前输出文件的使用量
    };

    //创建简单工厂
    class SinkFactory{
    public:
        //可变参数列表里储存了构造函数所需的所有参数
        template<typename SinkType,typename ...Args>
        static LogSink::ptr create(Args&&...args){
            //返回shared_ptr,传的是构造函数参数列表
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
}
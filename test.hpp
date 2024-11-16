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
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>

namespace test
{

    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink(){}
        virtual ~LogSink() =default;
        virtual void log(const char*data,size_t len) = 0;//声明统一的log接口
    };

        class DatabaseSink:public LogSink
    {
    public:
        using ptr = std::shared_ptr<DatabaseSink>;
        DatabaseSink()
        :_driver(nullptr)
        ,_con(nullptr)
        ,_reconnect_cnt(5)
        {
            _driver = sql::mysql::get_mysql_driver_instance();
        }

        void log(const char*data,size_t len)override
        {
            std::string header_str = suplog::util::header::addHeader(std::string(data,len));
        }

        static void work_loop(void* arg)
        {
            DatabaseSink* ds = (DatabaseSink*)arg;
        }

        ~DatabaseSink()
        {
            //关闭连接
            if(_con && !_con->isClosed())
                _con->close();
        }

    private:
        int _reconnect_cnt;
        sql::mysql::MySQL_Driver* _driver;
        sql::Connection* _con;
    };

    // class DatabaseSink: public LogSink
    // {
    // public:
    //     using ptr = std::shared_ptr<DatabaseSink>;
    //     DatabaseSink()
    //     :_driver(nullptr)
    //     ,_con(nullptr)
    //     ,_reconnect_cnt(5)
    //     {
    //         try{
    //             _driver = sql::mysql::get_mysql_driver_instance();
    //             DBUserConfig::ptr user(DBUserConfig::getInstance());
    //             //DEBUG
    //             std::cout<<user->Address()<<" "<<user->User() << " "<<user->Password()<<std::endl;
    //             //ENDEBUG
    //             _con = std::unique_ptr<sql::Connection>(_driver->connect(user->Address(),user->User(),user->Password()));
    //             if(_con->isClosed())
    //             {
    //                 std::cout<<"初次连接失败"<<std::endl;
    //                 reconnect();
    //             }
    //             _con->setSchema(user->DataBase());
    //             std::cout<<"设置数据库成功";
    //         }
    //         catch (sql::SQLException &e)
    //         {
    //             std::cerr<<"数据库连接失败: "<<e.what()<<std::endl;
    //             std::cout<<"即将发起重连..."<<std::endl;
    //             reconnect();
    //         }
    //     }

    //     void log(const char*data,size_t len)
    //     {
    //         std::string header_str = suplog::util::header::addHeader(std::string(data,len));
    //     }

    //     void reconnect()
    //     {
    //         int cnt = _reconnect_cnt;
    //         int interval = 1;
    //         while(cnt)
    //         {
    //             std::cout<<"发起重连..."<<std::endl;
    //             try{
    //                 _con->reconnect();
    //                 if(!_con->isClosed())
    //                 {
    //                     _con->setSchema(DBUserConfig::getInstance()->DataBase());
    //                     std::cout<<"成功重连"<<std::endl;
    //                     return;
    //                 }
    //             }
    //             catch (sql::SQLException &e)
    //             {
    //                 std::cerr<<"数据库重连失败: "<<e.what()<<std::endl;
    //             }

    //             cnt--;
    //             sleep(interval++);//每次重连间隔都会变长
    //         }
    //         //五次重连失败，终止程序
    //         throw sql::SQLException("5次重连失败");
    //     }


    //     ~DatabaseSink()
    //     {
    //         //关闭连接
    //         _con->close();
    //         _con.reset();
    //     }

    // private:
    //     int _reconnect_cnt;
    //     sql::mysql::MySQL_Driver* _driver;
    //     std::unique_ptr<sql::Connection> _con;
    // };
}
#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "sink.hpp"
#include "looper.hpp"//不认识？没关系
                    //这个头文件后面才实现,给异步日志器的实现用的
#include <vector>
#include <list>
#include <atomic>
#include <unordered_map>
#include <cstdarg>
#include <type_traits>

namespace suplog{
    class Logger{
    public:
        enum class Type{
            LOGGER_SYNC = 0,
            LOGGER_ASYNC
        };
        using ptr = std::shared_ptr<Logger>;

        Logger(const std::string& name,
            Formatter::ptr formatter,
            std::vector<LogSink::ptr> &sinks,
            LogLevel::Level level = LogLevel::Level::DEBUG
        ):_name(name),_level(level),_formatter(formatter),
        _sinks(sinks.begin(),sinks.end())
        {}

        std::string loggerName(){return _name;}
        LogLevel::Level loggerLevel(){return _level;}

        //使用C语言风格的不定参数
        //=========start========
        void debug(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::DEBUG) == false)
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::DEBUG,file,line,fmt,al);//日志输出
            va_end(al);//结束可变参数列表
        }

        void info(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::INFO) == false)
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::INFO,file,line,fmt,al);//日志输出
            va_end(al);//结束可变参数列表
        }

        void warn(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::WARN) == false)
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::WARN,file,line,fmt,al);//日志输出
            va_end(al);//结束可变参数列表
        }

        void error(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::ERROR) == false)
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::WARN,file,line,fmt,al);//日志输出
            va_end(al);//结束可变参数列表
        }

        void fatal(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::FATAL) == false)
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::FATAL,file,line,fmt,al);//日志输出
            va_end(al);//结束可变参数列表
        }

        //===============end===============

    public:
        //在内部声明一个建造者抽象类
        class Builder
        {
        public:
            using ptr = std::shared_ptr<Builder>;

            Builder():_level(LogLevel::Level::DEBUG),
                _logger_type(Logger::Type::LOGGER_SYNC)
            {}

            void buildLoggerName(const std::string& name){_logger_name = name;}

            void buildLoggerLevel(LogLevel::Level level){_level = level;}

            void buildLoggerType(Logger::Type type){_logger_type = type;}

            void buidFormatter(const Formatter::ptr& formatter){
                _formatter = formatter;
            }

            //C++风格不定参数
            template<typename SinkType,typename ...Args>
            void buildSink(Args &&...args){
                auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
                _sinks.push_back(sink);
            }

            virtual Logger::ptr build() = 0;
        protected:
            Logger::Type _logger_type;
            std::string _logger_name;
            LogLevel::Level _level;
            Formatter::ptr _formatter;
            std::vector<LogSink::ptr> _sinks;
        };

    protected:
    bool shouldLog(LogLevel::Level level){return level>= _level;}

    void log(LogLevel::Level level,const char*file,
        size_t line,const char*fmt,va_list al)
    {
        char *buf;//可以不初始化
        std::string msg;
        int len = vasprintf(&buf,fmt,al);//自动在堆区申请内存
        if(len < 0)
            msg = "格式化日志消息失败!!";
        else 
        {
            msg.assign(buf,len);
            free(buf);//释放空间
        }

        //LogMsg(name, file, line, payload, level)
        LogMsg logmsg(_name,file,line,std::move(msg),level);
        std::string str;
        str = _formatter->format(logmsg);
        logIt(std::move(str));
    }
    virtual void logIt(const std::string &msg) = 0;
    protected:
        std::mutex _mutex;
        std::string _name;
        Formatter::ptr _formatter;
        std::atomic<LogLevel::Level> _level;
        std::vector<LogSink::ptr> _sinks; 
    };

    //同步日志器比较简单，所以直接实现了
    class SyncLogger:public Logger
    {
    public:
        using ptr = std::shared_ptr<SyncLogger>;

        SyncLogger(const std::string& name,
            Formatter::ptr formatter,
            std::vector<LogSink::ptr>&sinks,
            LogLevel::Level level = LogLevel::Level::DEBUG)
            :Logger(name,formatter,sinks,level){
                std::cout << LogLevel::toString(level)<<"同步日志器创建成功...\n";
            }

        private:
            virtual void logIt(const std::string& msg_str)override{
                 // lock 的析构函数在离开作用域时自动释放互斥锁
                std::unique_lock<std::mutex> lock(_mutex);
                if(_sinks.empty()) {return;}
                for(auto &it:_sinks)
                    it->log(msg_str.c_str(),msg_str.size());
            }
    };

    class AsyncLogger:public Logger
    {
    public:
        using ptr = std::shared_ptr<AsyncLogger>;

        AsyncLogger(const std::string& name,
            Formatter::ptr formatter,
            std::vector<LogSink::ptr>&sinks,
            LogLevel::Level level = LogLevel::Level::DEBUG)
            :Logger(name,formatter,sinks,level)
            ,_looper(std::make_shared<AsyncLooper>(
                std::bind(&AsyncLogger::readLog,this,std::placeholders::_1)))
                //传一个this,使包装器里的函数能够是成员函数,this后面的才是包装器指定的参数
            {
                std::cout << LogLevel::toString(level)<<"异步日志器创建成功...\n";
            }

    private:
        virtual void logIt(const std::string &msg)
        {
            _looper->push(msg);//推送消息
        }

        void readLog(Buffer& msg)
        {
            if(_sinks.empty()){return;}//判空

            for(auto &it:_sinks)
            {
                //调用落地功能
                it->log(msg.begin(),msg.readAbleSize());
            }
        }
    protected:
        AsyncLooper::ptr _looper;
    
    };

    class LocalLoggerBuilder:public Logger::Builder
    {
    public:
        virtual Logger::ptr build()
        {
            if(_logger_name.empty()){
                std::cout<<"日志器名称不能为空！！";
                abort();
            }
            if(_formatter.get() == nullptr){
                std::cout<<"当前日志器： "<<_logger_name;
                std::cout<<" 未检测到⽇志格式,默认设置为: ";
                std::cout<<" %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n\n";
                _formatter = std::make_shared<Formatter>();
            }
            if(_sinks.empty())
            {
                std::cout<<"当前日志器: "<<_logger_name<<"问检测到落地方向，默认为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }

            Logger::ptr lp;
            if(_logger_type == Logger::Type::LOGGER_ASYNC)
            {
                lp = std::make_shared<AsyncLogger>(_logger_name,_formatter,_sinks,_level);
            }
            else 
            {
                lp = std::make_shared<SyncLogger>(_logger_name,_formatter,_sinks,_level);
            }
            return lp;
        }
    };

    class LoggerManager
    {
    public:
        static LoggerManager& getInstance()
        {
            static LoggerManager lm;//饿汉模式创建全局对象
            return lm;
        }

        bool hasLogger(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if(it == _loggers.end())
                return false;
            return true;
        }

        void addLogger(const std::string& name,
            const Logger::ptr logger)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(name,logger));
        }

        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if(hasLogger(name))
            {
                auto it = _loggers.find(name);
                if(it!= _loggers.end())
                    return it->second;//找到了，返回指针
            }
            //找不到
            return Logger::ptr();
        }

        Logger::ptr rootLogger()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _root_logger;
        }

    private:
        LoggerManager()//私有化防止外部调用，破坏单例模式
        {
            //调用本地建造者
            std::unique_ptr<LocalLoggerBuilder> slb(new LocalLoggerBuilder());
            slb->buildLoggerName("root");
            slb->buildLoggerType(Logger::Type::LOGGER_SYNC);
            //格式使用默认
            //采用默认落地方向
            _root_logger = slb->build();
            _loggers.insert(std::make_pair("root",_root_logger));
        }

        LoggerManager(const LoggerManager&) = delete;//删除拷贝构造
        LoggerManager& operator=(const LoggerManager&) = delete;//删除重载
    private:
        std::mutex _mutex;//互斥量
        Logger::ptr _root_logger;//至少有一个日志器
        std::unordered_map<std::string,Logger::ptr> _loggers;
        //使用父类指针统一管理，使用多态
    };

    class GlobalLoggerBuilder:public Logger::Builder
    {
    public:
        virtual Logger::ptr build() override
        {
            if(_logger_name.empty())
            {
                std::cout<<"日志器名称不能为空!!";
                abort();
            }

            //不能有重名的日志器
            assert(LoggerManager::getInstance().hasLogger(_logger_name) == false);

            if(_formatter.get() == nullptr){
                std::cout<<"当前日志器： "<<_logger_name;
                std::cout<<" 未检测到⽇志格式,默认设置为: ";
                std::cout<<" %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n\n";
                _formatter = std::make_shared<Formatter>();
            }
            if(_sinks.empty())
            {
                std::cout<<"当前日志器: "<<_logger_name<<"问检测到落地方向，默认为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }

            Logger::ptr lp;
            if(_logger_type == Logger::Type::LOGGER_SYNC)
            {
                lp = std::make_shared<SyncLogger>(_logger_name,_formatter,_sinks,_level);
            }
            else 
            {
                lp = std::make_shared<AsyncLogger>(_logger_name,_formatter,_sinks,_level);
            }

            //加入全局日志器管理类中
            LoggerManager::getInstance().addLogger(_logger_name,lp);

            return lp;
        }
    };
}


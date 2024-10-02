#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "sink.hpp"
#include "looper.hpp"//不认识？没关系，这个头文件后面才实现
//looper.h提供了异步日志器类
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
            void buildSink(Args &&...arfs){
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
            virtual void logIt(const std::string& msg_str){
                std::unique_lock<std::mutex> lock(_mutex);//函数结束后自动释放锁
                if(_sinks.empty()) {return;}
                for(auto &it:_sinks)
                    it->log(msg_str.c_str(),msg_str.size());
            }
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
        }
    };
}
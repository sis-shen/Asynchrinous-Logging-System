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
        //类内枚举日志器的类型
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

        std::string loggerName(){ return _name; }//获取日志器名称
        LogLevel::Level loggerLevel(){ return _level; }//获取日志器等级

        //使用C语言风格的不定参数输出日志
        //接口log位于protected访问限定符下
        //=========start========
        //各个接口的主要差别在于日志等级不同
        void debug(const char*file,size_t line,const char*fmt,...)
        {
            if(shouldLog(LogLevel::Level::DEBUG) == false)//如果等级不满足日志器最低要求
                return;

            va_list al;
            va_start(al,fmt);//依据fmt从内存中提取可变参数列表
            log(LogLevel::Level::DEBUG,file,line,fmt,al);//日志输出,al向函数传递可变参数列表
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
            
            //默认支持以下接口
            //======start======//

            //设置日志器名称
            void buildLoggerName(const std::string& name){ 
                _logger_name = name;
                return;
            }

            //设置日志器等级
            void buildLoggerLevel(LogLevel::Level level){ 
                _level = level;
                return;
            }

            //设置日志器类型
            void buildLoggerType(Logger::Type type){
                _logger_type = type;
                return;
            }

            //设置日志器格式串
            void buidFormatter(const Formatter::ptr& formatter){
                _formatter = formatter;
                return;
            }

            //设置日志器格式串-函数重载
            void buidFormatter(const std::string& formatStr){
                auto formatter = std::make_shared<suplog::Formatter>(formatStr);
                _formatter = formatter;
                return;
            }

            //C++风格不定参数
            //增加落地类
            template<typename SinkType,typename ...Args>
            void buildSink(Args &&...args){
                auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
                _sinks.push_back(sink);
                return;
            }

            //清空落地类列表
            void clearSink(){
                _sinks.clear();
                return;
            }
            //======end======//

            //声明抽象接口build，具体实现交给派生类
            virtual Logger::ptr build() = 0;
        protected:
            Logger::Type _logger_type;
            std::string _logger_name;
            LogLevel::Level _level;
            Formatter::ptr _formatter;
            std::vector<LogSink::ptr> _sinks;
        };//=====Build类声明定义结束=======

    //回到Logger类
    protected:
    //根据日志等级，判断是否应该日志输出
    bool shouldLog(LogLevel::Level level){ return level >= _level; }

    void log(LogLevel::Level level,const char*file,
        size_t line,const char*fmt,va_list al)
    {
        char *buf;//可以不初始化
        std::string msg;
        //将格式化串和可变参数列表生成字符串，并存入buf指向的内存
        int len = vasprintf(&buf,fmt,al);//自动在堆区申请内存
        if(len < 0)
            msg = "格式化日志消息失败!!";
        else 
        {
            msg.assign(buf,len);//转存到msg对象中
            free(buf);//释放空间
        }

        //LogMsg(name, file, line, payload, level)
        LogMsg logmsg(_name,file,line,std::move(msg),level);//使用了拷贝构造
        std::string str;
        //使用logmsg获取输出字符串/输出任务
        str = _formatter->format(logmsg);
        logIt(std::move(str));//真正开始执行输出任务。如何调用落地类由派生类具体实现
    }

    //交给派生类实现
    virtual void logIt(const std::string &msg) = 0;

    protected:
        std::mutex _mutex;//为多线程环境提前准互斥量
        std::string _name;
        Formatter::ptr _formatter;
        std::atomic<LogLevel::Level> _level;//使用atomic使_level的修改操作原子化
        std::vector<LogSink::ptr> _sinks; 
    };

    //同步日志器
    class SyncLogger:public Logger
    {
    public:
        using ptr = std::shared_ptr<SyncLogger>;

        SyncLogger(const std::string& name,
            Formatter::ptr formatter,
            std::vector<LogSink::ptr>&sinks,
            LogLevel::Level level = LogLevel::Level::DEBUG)
            :Logger(name,formatter,sinks,level)
            {
                std::cout << LogLevel::toString(level)<<"同步日志器创建成功...\n";
            }

        private:
            virtual void logIt(const std::string& msg_str)override
            {
                 // lock 的析构函数在离开作用域时自动释放互斥锁
                std::unique_lock<std::mutex> lock(_mutex);
                if(_sinks.empty()) { return; }//没有落地方向

                //每个落地方向都输出一次
                for(auto &it:_sinks)
                    it->log(msg_str.c_str(),msg_str.size());

                return;
            }
    };

    //异步日志器
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
        virtual void logIt(const std::string &msg) override
        {
            //v1.2增加报头封装
            std::string header_str = suplog::util::header::addHeader(msg);
            _looper->push(header_str);//推送消息
            return;
        }

        //_looper所用的回调函数
        void readLog(Buffer& msg_line)
        {
            if(_sinks.empty()){ return; }//判空

            int cur = 0;
            while(cur < msg_line.readAbleSize())
            {
                //v1.2增加报头解析和字符串拆分
                int header_len = suplog::util::header::HEADER_LEN;
                int len = suplog::util::header::readHeader(msg_line.begin()+cur,header_len);//读取长度
                cur+=header_len;//跳过报头
                std::string msg(msg_line.begin()+cur,len);//提取报文
                cur+=len;//跳过报文

                for(auto &it:_sinks)
                {
                    //调用落地功能
                    it->log(msg.c_str(),msg.size());//直接一次性输出所有缓存的日志
                }
            }

            return;
        }
    protected:
        AsyncLooper::ptr _looper;
    };

    //本地日志器建造者
    class LocalLoggerBuilder:public Logger::Builder
    {
    public:
        virtual Logger::ptr build() override
        {
            //检测名称是否存在
            if(_logger_name.empty())
            {
                throw LogException("日志器名称不能为空！！");
            }
            //检测格式串
            if(_formatter.get() == nullptr){
                std::cout<<"当前日志器： "<<_logger_name;
                std::cout<<" 未检测到⽇志格式,默认设置为: ";
                std::cout<<" %d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n\n";
                _formatter = std::make_shared<Formatter>();
            }
            //检测是否存在落地方向
            if(_sinks.empty())
            {
                std::cout<<"当前日志器: "<<_logger_name<<"问检测到落地方向，默认为标准输出!\n";
                _sinks.push_back(std::make_shared<StdoutSink>());
            }

            Logger::ptr lp;//使用多态
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

        //按名字查询某一日志器
        bool hasLogger(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if(it == _loggers.end())//找不到
                return false;
            //找到了
            return true;
        }

        //新增日志器
        void addLogger(const std::string& name,
            const Logger::ptr logger)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loggers.insert(std::make_pair(name,logger));
        }

        //按名字获取日志器的指针
        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if(it == _loggers.end())
                return Logger::ptr();//找不到

            return it->second;//找到了，返回指针
        }

        //获取根日志器
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

        //为单例模式做准备
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
                throw LogException("日志器名称不能为空!!");
            }

            //不能有重名的日志器
            if(LoggerManager::getInstance().hasLogger(_logger_name) == true)
            {
                throw LogException("GlobalLoggerBuilder:不能有重名日志器!");
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

            //创建日志器
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
#include "suplog.hpp"
#include "bench.hpp"
#include <iostream>

using namespace std;

//性能测试
void sync_bench_thread_log(size_t thread_count,
    size_t msg_count,size_t msg_len)
{
    static int num = 1;//记录函数被调用的次数
    std::string logger_name = "sync_bench_logger" + std::to_string(num++);
    LOGI("*****************************************************");
    LOGI("同步日志测试：%u threads,%u messages",thread_count,msg_count);

    suplog::GlobalLoggerBuilder::ptr glb(new suplog::GlobalLoggerBuilder);
    glb->buildLoggerName(logger_name);
    glb->buidFormatter("%m%n");
    std::string path = std::string("./testdir/sync")+std::to_string(num)+std::string(".log");
    glb->buildSink<suplog::FileSink>(path);
    glb->buildLoggerType(suplog::Logger::Type::LOGGER_SYNC);
    glb->build();
    
    suplog::bench(logger_name,thread_count,msg_len,msg_count);
    LOGI("*****************************************************");

}

void async_bench_thread_log(size_t thread_count,
    size_t msg_count,size_t msg_len)
{
    static int num = 1;
    std::string logger_name = "async_bench_logger"+std::to_string(num++);
    LOGI("*****************************************************");
    LOGI("异步日志测试：%u threads,%u messages",thread_count,msg_count);

    suplog::GlobalLoggerBuilder::ptr glb(new suplog::GlobalLoggerBuilder);
    glb->buildLoggerName(logger_name);
    glb->buidFormatter("%m%n");
    std::string path = std::string("./testdir/async")+std::to_string(num)+std::string(".log");
    glb->buildSink<suplog::FileSink>(path);
    glb->buildLoggerType(suplog::Logger::Type::LOGGER_ASYNC);
    glb->build();
    
    suplog::bench(logger_name,thread_count,msg_len,msg_count);
    LOGI("*****************************************************");

}

void bench_test()
{
    //同步写日志
    sync_bench_thread_log(1,1000000,100);
    sync_bench_thread_log(5,1000000,100);
    //异步日志输出，为了避免因为等待落地影响时间，所以日志数量降低为小于双缓冲区大小进行测试
    async_bench_thread_log(1,100000,100);
    async_bench_thread_log(5,100000,100);
}

int main()
{
    bench_test();
    return 0;
}


// //功能测试

// void loggerTest(const std::string& logger_name)
// {
//     suplog::Logger::ptr lp = suplog::getLogger(logger_name);

//     assert(lp.get());//防止拿到空指针
//     LOGD("------------example---------------");
//     //测试原生日志器
//     lp->debug("%s", "logger->debug");
//     lp->info("%s", "logger->info");
//     lp->warn("%s", "logger->warn");
//     lp->error("%s", "logger->error");
//     lp->fatal("%s", "logger->fatal");
//     //测试代理模式
//     LOG_DEBUG(lp, "%s", "LOG_DEBUG");
//     LOG_INFO(lp, "%s", "LOG_INFO");
//     LOG_WARN(lp, "%s", "LOG_WARN");
//     LOG_ERROR(lp, "%s", "LOG_ERROR");
//     LOG_FATAL(lp, "%s", "LOG_FATAL");
//     LOGD("-----------------------------------");

//     std::string log_msg = "hello supdriver test log msg -";
//     size_t count = 0;
//     while(count < 1000000)//输出一百万条日志
//     {
//         std::string msg = log_msg + std::to_string(count++);
//         lp->error("%s",msg.c_str());
//     }

// }

// int main()
// {
//     suplog::GlobalLoggerBuilder::ptr glb(new suplog::GlobalLoggerBuilder());
//     glb->buildLoggerName("sync-logger");//设置日志器名称
//     // glb->buidFormatter("[%d][%c][%f:%l][%p] %m%n");//设置日志输出格式
//     glb->buildLoggerLevel(suplog::LogLevel::Level::DEBUG);
//     glb->buildSink<suplog::StdoutSink>();//创建标准输出落地方向
//     glb->buildSink<suplog::FileSink>("./testdir/logs/sync.log");//创建文件落地方向
//     glb->buildSink<suplog::RollSink>("./testdir/roll_logs/roll-",10*1024*1024);
//     glb->buildLoggerType(suplog::Logger::Type::LOGGER_SYNC);

//     glb->build();//建造同步日志器

//     glb->buildLoggerName("async-logger");
//     glb->buildSink<suplog::FileSink>("./testdir/logs/async.log");//创建文件落地方向
//     glb->buildSink<suplog::RollSink>("./testdir/async-roll/roll-",10*1024*1024);
//     glb->buildLoggerType(suplog::Logger::Type::LOGGER_ASYNC);

//     glb->build();//建造异步日志器

//     loggerTest("sync-logger");
//     loggerTest("async-logger");

//     return 0;
// }







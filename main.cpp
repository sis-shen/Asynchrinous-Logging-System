#include "suplog.hpp"
#include <iostream>

using namespace std;

int main()
{
    std::string path = "./test/rollsink";
    suplog::LoggerManager& lm = suplog::LoggerManager::getInstance();

    suplog::GlobalLoggerBuilder glb;
    glb.buildLoggerName("test");
    glb.buildSink<suplog::RollSink>("./test/rollsink",10);
    glb.buildLoggerType(suplog::Logger::Type::LOGGER_ASYNC);

    suplog::Logger::ptr alogger= glb.build();
    suplog::Logger::ptr slogger = lm.rootLogger();

    LOGD("测试测试测试");
    return 0;
}

// int main()
// {
//     cout<<"测试字符串转换 FATAL:";
//     cout<<suplog::LogLevel::toString(suplog::LogLevel::Level::FATAL)<<endl;
//     return 0;
// }

// int main()
// {
//     cout<<"测试时间: "<<suplog::util::date::now()<<endl;
//     cout<<"测试文件"<<endl;
//     cout<<"测试path()，获取到的path为： " << suplog::util::file::path("/home/supdriver/code/file.txt");
//     cout<<"测试创建目录:... ";
//     suplog::util::file::create_directory("./testdir/dir1/dir2");
//     cout<<"创建成功"<<endl;
//     cout<<"测试exists(), 测试路径:\"./testdir/dir1/dir2\",返回结果"<<suplog::util::file::exists("./testdir/dir1/dir2")<<endl;
//     return 0;
// }






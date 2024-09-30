#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "sink.hpp"
#include <iostream>

using namespace std;

int main()
{
    suplog::Formatter fmt;
    std::string pattern = "[%d{%H:%M:%S}] %m%n";
    suplog::LogMsg msg={
        "logger",       //名字
        "main.cpp",     //文件名
        22,             //行数
        "创建套接字失败",//正文
        suplog::LogLevel::Level::ERROR   //等级
    };
    std::string str = fmt.format(msg);

    std::shared_ptr<suplog::LogSink> lsptr;

    lsptr.reset(new suplog::StdoutSink());
    lsptr->log(str.c_str(),str.size());

    lsptr.reset(new suplog::FileSink("./testdir/log.log"));
    lsptr->log(str.c_str(),str.size());

    lsptr.reset(new suplog::RollSink("./testdir/rollsink/log",10));//故意设置小，查看滚动效果
    string msg1=string(str).append("msg1");
    string msg2=string(str).append("msg2");

    lsptr->log(msg1.c_str(),msg1.size());
    lsptr->log(msg2.c_str(),msg2.size());

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






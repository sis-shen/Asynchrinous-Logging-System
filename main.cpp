#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
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
    cout<<fmt.format(msg);
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






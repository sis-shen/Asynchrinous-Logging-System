#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cassert>
#include <sys/stat.h>

namespace suplog{
    namespace util{
        class date
        {
        public:
            static size_t now(){return (size_t)time(nullptr);}
        };

        class file{
        public:
            static bool exists(const std::string&name)
            {
                //struct stat配合stat函数作为输出型参数
                struct stat st;
                //stat函数是系统调用接口
                return stat(name.c_str(),&st) == 0;
            }
            static std::string path(const std::string &name)
            {
                //规定找不到就返回".""
                if(name.empty()) return ".";

                size_t pos = name.find_last_of("/\\");//找到最后一个"/"或"\\"
                return name.substr(0,pos+1);//取得目录的路径
            }
            static void create_directory(const std::string &path)
            {
                if(path.empty()) return;//路径为空
                if(exists(path)) return;//路径不存在-使用系统调用接口
                size_t pos,index = 0;
                while(index<path.size())
                {
                    //找到index及以后的"/"
                    pos = path.find_first_of("/\\",index);
                    if(pos == std::string::npos)
                    {
                        //创建最后一层目录
                        mkdir(path.c_str(),0755);
                        return;
                    }
                    //二者重叠时，跳到下一次循环
                    if(pos ==index) {index = pos+1;continue;}
                    std::string subdir = path.substr(0,pos);
                    //文件夹已存在，不用创建，跳到下一段即可
                    if(subdir == "." || subdir == "..")
                        {index = pos + 1;continue;}
                    if(exists(subdir))//理由同上
                        {index = pos + 1;continue;}
                    //逐级创建目录，权限为0755
                    mkdir(subdir.c_str(),0755);
                    index = pos + 1;
                }
            }
        };
    }
}


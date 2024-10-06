#pragma once
#include "suplog.hpp"
#include <chrono>//处理时间


namespace suplog
{
void bench(const std::string& logger_name,size_t thread_num,
    size_t msglen,size_t msg_count)
{
    Logger::ptr lp = getLogger(logger_name);
    if(lp.get() ==nullptr) return;
    std::string msg(msglen,'1');//用字符1补全长度
    size_t msg_count_per_thread = msg_count/thread_num;
    std::vector<double> cost_time(thread_num);
    std::vector<std::thread>threads;
    std::cout<<"输入线程数量"<<thread_num<<std::endl;
    std::cout<<"输出日志数量"<<msg_count <<std::endl;
    std::cout << "输出⽇志⼤⼩: " << msglen * msg_count / 1024 << "KB" <<std::endl;

    for(int i =0;i<thread_num;++i)
    {
        //empalce_back直接构造对象，即新增新的任务线程
        threads.emplace_back([&,i](){
            auto start = std::chrono::high_resolution_clock::now();
            for(size_t j = 0;j<msg_count_per_thread;++j)
            {
                lp->fatal("%s",msg.c_str());//输出日志
            }
            auto end = std::chrono::high_resolution_clock::now();
            //计算时间差
            auto cost = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
            cost_time[i] = cost.count();
            auto avg = msg_count_per_thread/cost_time[i];//每个线程中每秒打印多少日志
            std::cout<<"线程"<<i<<"耗时："<<cost_time[i]<<"s ";
            std::cout<<"平均：： "<<(size_t)avg<<"/s\n";
        });
    }

    for(auto& thr:threads)
    {
        thr.join();//回收子线程
    }

    double max_cost = 0;
    for(auto cost:cost_time) max_cost = max_cost <cost?cost:max_cost;
    std::cout<<"总消耗时间： "<<max_cost<<std::endl;
    std::cout<<"平均每秒输出: "<<(size_t)(msg_count/max_cost)<<std::endl;

}



}



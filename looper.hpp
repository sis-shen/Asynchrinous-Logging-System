#pragma once

#include "util.hpp"
#include "buffer.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>

namespace suplog
{
    class AsyncLooper
    {
    public:
        using ptr = std::shared_ptr<AsyncLooper>;
        using Functor = std::function<void(Buffer &buffer)>;

        AsyncLooper(const Functor&cb)
        : _running(true),
        _looper_callback(cb),
        _thread(std::thread(&AsyncLooper::worker_loop,this))//在构造函数中启动线程
        {}

        ~AsyncLooper(){ stop(); }

        void stop()
        {
            _running =false;
            _pop_cond.notify_all();//将所有线程的条件变量等待唤醒
            _thread.join();//接收子进程
        }

        //推送任务
        void push(const std::string&msg)
        {
            if(_running == false) return;

            std::unique_lock<std::mutex> lock(_mutex);//加锁

            //等待条件
            _push_cond.wait(lock,[&]{
                return _tasks_push.writeAbleSize() >= msg.size();
            });//防止消息过大

            _tasks_push.push(msg.c_str(),msg.size());//完成消息任务推送

            _pop_cond.notify_all();//唤醒所有消费者
        }

    private:
        //子线程的入口函数
        static void worker_loop(void* arg)
        {
            //多线程执行函数得靠arg传递this指针
            AsyncLooper* alooper = (AsyncLooper* )arg;
            while(1)
            {
                 // lock 的析构函数在离开作用域时(完成一趟循环)自动释放互斥锁
                std::unique_lock<std::mutex> lock(alooper->_mutex);
                //线程出口,为空或关闭时退出循环
                if(alooper->_running == false && alooper->_tasks_push.empty())
                    return;
                
                //生产者缓冲不为空或者停止运行时才会被唤醒
                alooper->_pop_cond.wait(lock,[&]{
                    return !alooper->_tasks_push.empty() || !alooper->_running;
                });
                alooper->_tasks_push.swap(alooper->_tasks_pop);//交换缓冲区

                alooper->_push_cond.notify_all();//唤醒所有生产者缓冲区
                alooper->_looper_callback(alooper->_tasks_pop);//调用回调函数，输出消费者缓冲区
                //输出完成，清空缓冲区
                alooper->_tasks_pop.reset();
            }
        }
    
    private:
        Functor _looper_callback;//输出缓冲区内容的回调函数
    private:
        std::mutex _mutex;//互斥锁
        std::atomic<bool> _running;//能够原子地赋值状态指示
        std::condition_variable _push_cond;//生产者条件变量
        std::condition_variable _pop_cond;//消费者条件变量
        Buffer _tasks_push;//双缓冲区
        Buffer _tasks_pop;
        std::thread _thread;//主线程管理子线程类用的
    };
}
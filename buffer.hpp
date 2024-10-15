#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cassert>

namespace suplog
{

// 设置Buffer的一些基本参数
#define BUFFER_DEFUALT_SIZE (1*1024*1024)      //缓冲区默认大小
#define BUFFER_INCREMENT_SIZE (1*1024*1024)    //缓冲区默认扩展大小
#define BUFFER_THRESHOLD_SIZE (10*1024*1024)   //缓冲区最大值

class Buffer{
public:
    Buffer():_reader_idx(0),_writer_idx(0),_v(BUFFER_DEFUALT_SIZE){}

    bool empty(){ return _reader_idx == _writer_idx; }        //判空
    size_t readAbleSize(){ return _writer_idx - _reader_idx; }//是否可读
    size_t writeAbleSize(){ return _v.size() - _writer_idx; } //是否可写
    void reset(){ _reader_idx = _writer_idx = 0; }  //重置读写指针
    //交换缓冲区
    void swap(Buffer& buf)
    {
        _v.swap(buf._v);
        std::swap(_reader_idx,buf._reader_idx);
        std::swap(_writer_idx,buf._writer_idx);
    }
    
    //向缓冲区推送数据
    void push(const char*data,size_t len)
    {
        ensureEnoughSpace(len);
        assert(len<=writeAbleSize());
        std::copy(data,data+len,&_v[_writer_idx]);
        _writer_idx+=len;
    }

    //缓冲区删除定长数据
    void pop(size_t len)
    {
        _reader_idx +=len;
        assert(_reader_idx <=_writer_idx);
    }

    const char* begin() 
    {
        //获取起始地址 
        return &_v[_reader_idx]; 
    }

protected:
    void ensureEnoughSpace(size_t len)
    {
        //如果写的下,则退出
        if(len <=writeAbleSize())return;

        size_t new_capacity;
        //每次增加1M的大小
        if(_v.size() < BUFFER_THRESHOLD_SIZE)
            new_capacity = _v.size()*2+len;
        else 
            new_capacity = _v.size() + BUFFER_INCREMENT_SIZE + len;

        _v.resize(new_capacity);
        return;
    }

private:
    size_t _reader_idx;
    size_t _writer_idx;
    std::vector<char> _v;
};

}
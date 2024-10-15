# Asynchrinous-Logging-System
c++基于多种设计模式的异步日志系统

# V1.1版本使用手册

[戳我去博客🔗](https://www.supdriver.top/2024/09/24/muiltiDesignPatternsLogSystem/)

## 介绍
**c++基于多种设计模式的异步日志系统V1.1** 是基于C++文件操作、多线程等实现的，提供日志服务的项目。所包含的业务点有

+ 多级别日志消息
+ 支持同步日志和异步日志
+ 支持**可靠写入**到**多种落地方向**:
    + 控制台标准输出
    + 单个日志文件输出
    + 滚动文件输出
+ 支持**多线程程序并发写日志**
+ 支持**扩展**不同的日志目标落地

可以帮助自动定位调用日志的文件和行号，且能够方便地通过接口选择日志等级

## 如何使用

### 头文件引入
由于C++头文件具有`间接包含`的特点，所以只需要包含一个`suplog.hpp`头文件即可

```C++
#include suplog.hpp
```

### 全局的日志器管理类
本项目使用了`单例模式`，提供了一个全局可用的`suplog::LoggerManager`类,可用于管理全局可用的日志，并提供了`suplog::LoggerManager::getInstance()`接口获取单例。**但一般使用并不需要获取单例**

### 建造全局日志器
本项目提供了`suplog::GlobalLoggerBuilder`类用于建造全局日志器，并且会**自动添加到全局的日志器管理类单例**中

建造日志器所需参数:

| 参数名称 | 接口 | 描述 |
| -------- | ---- | ---- |
| 日志器名称 | `buildLoggerName` | 不可为空，不可重复 |
| 日志器类型 | `buildLoggerType` |`suplog::Logger::Type::LOGGER_SYNC`(**默认**) 或 `suplog::Logger::Type::LOGGER_ASYNC` |
| 日志器等级 | `buildLoggerLevel` | **默认**为`suplog::LogLevel::Level::DEBUG`,也可以是`suplog::LogLevel::Level`中的其它枚举 |
| 日志器落地方向 | `buildSink` | `suplog::StdoutSink`(标准输出)(**默认**)、`suplog::FileSink`(文件输出)、`suplog::RollSink`(滚动文件输出) 中选择，可添加多个。 |
| 日志格式串 | `buidFormatter` | 规定日志输出的格式，**默认**为 `%d{%H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n\n` |

格式串介绍如下
  + `%d{%H:%M:%S}`：表⽰⽇期时间，花括号中的内容表⽰⽇期时间的格式。
  + `%T`：表⽰制表符缩进。
  + `%t`：表⽰线程ID
  + `%p`：表⽰⽇志级别
  + `%c`：表⽰⽇志器名称，不同的开发组可以创建⾃⼰的⽇志器进⾏⽇志输出，⼩组之间互不影响。
  + `%f`：表⽰⽇志输出时的源代码⽂件名。
  + `%l`：表⽰⽇志输出时的源代码⾏号。
  + `%m`：表⽰给与的⽇志有效载荷数据
  + `%n`：表⽰换⾏


接口声明如下：
+ `void buildLoggerName(const std::string& name)`  设置日志器名称
+ `void buildLoggerType(Logger::Type type)`         设置日志器类型
+ `void buildLoggerLevel(LogLevel::Level level)`    设置日志器等级
+ `template<typename SinkType,typename ...Args>void buildSink(Args &&...args)` **添加**落地方向
+ `void buidFormatter(const std::string& formatStr)`    设置格式串
+ `void buidFormatter(const Formatter::ptr& formatter)` 上一个函数的重载函数
+ `virtual Logger::ptr build() override` **建造/产生一个日志器，并返回它的指针**

使用示例如下

```C++
suplog::GlobalLoggerBuilder::ptr glb(new suplog::GlobalLoggerBuilder());
glb->buildLoggerName("sync-logger");//设置日志器名称
// glb->buidFormatter("[%d][%c][%f:%l][%p] %m%n");//设置日志输出格式
glb->buildLoggerLevel(suplog::LogLevel::Level::DEBUG);
glb->buildSink<suplog::StdoutSink>();//创建标准输出落地方向
glb->buildSink<suplog::FileSink>("./testdir/logs/sync.log");//创建文件落地方向
glb->buildSink<suplog::RollSink>("./testdir/roll_logs/roll-",10*1024*1024);
glb->buildLoggerType(suplog::Logger::Type::LOGGER_SYNC); //设置日志器类型

glb->build();//建造同步日志器,已自动添加到全局
```

### 建造局部日志器
局部日志器的作用范围只存在于某一作用域（局部）。

除了`build()`函数有重写过，与全局的互为多态关系，剩余接口都是一样的，这里不多赘述了

所以最大的不同是需要有变量承接`build()`的返回值

```C++
//调用本地建造者
std::unique_ptr<LocalLoggerBuilder> slb(new suplog::LocalLoggerBuilder());
slb->buildLoggerName("root");
slb->buildLoggerType(suplog::Logger::Type::LOGGER_SYNC);
//格式使用默认
//采用默认落地方向
suplog::Logger::ptr _root_logger = slb->build();
```

### 获取全局日志器
`suplog.hpp`提供了全局函数用于获取全局的日志器

+ `Logger::ptr rootLogger()`获取根日志器
+ `bool hasLogger(const std::string& name)` 按名字查询是否存在该日志器
+ `Logger::ptr getLogger(const std::string& name)` 按名字获取日志器

### 使用日志器

#### 全局函数/宏函数
`suplog.hpp`封装了全局函数用于输出日志

一般日志器可用宏函数:
+ `LOG_DEBUG(logger,fmt,...)`
+ `LOG_INFO(logger,fmt,...)`
+ `LOG_WARN(logger,fmt,...)`
+ `LOG_ERROR(logger,fmt,...)`
+ `LOG_FATAL(logger,fmt,...)`

使用**根日志器**直接标准输出可直接调用宏函数:
+ `LOGD(fmt,...)`   //*Debug*
+ `LOGI(fmt,...)`   //*Info*
+ `LOGW(fmt,...)`   //*Warn*
+ `LOGE(fmt,...)`   //*Error*
+ `LOGF(fmt,...)`   //*Fatal*

#### 日志器指针
使用日志器指针可以调用成员函数来输出日志,可用的函数如下:
+ debug
+ info
+ warn
+ error
+ fatal

以`debug`为例,函数声明为`void debug(const char*file,size_t line,const char*fmt,...)`

使用样例如下

```C++
#include "logger.hpp"
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

    alogger->warn("main.cpp",19,"这是一条警告测试信息，测试码:%d",0);
    alogger->warn("main.cpp",20,"这是一条警告测试信息，测试码:%d",1);

    slogger->debug("main.cpp",22,"这是一条标准输出测试信息");

    return 0;
}
```


# learnlog


learnlog 是参照 [spdlog](https://github.com/gabime/spdlog) 设计的一个 C++11 日志库，同时在`异步模式`下加入了 [concurrentqueue](https://github.com/cameron314/concurrentqueue) 作为缓冲队列，在处理速度上有 [大幅提升](#benchmarks)。


## 编译方法

```console
$ git clone https://github.com/def-a-name/learnlog.git
$ cd learnlog && mkdir build && cd build
$ cmake .. && cmake --build .
```

## 特点

- 异步模式下，3 个线程池各有特点：

|名称|内存占用|线程内日志有序性|速度|
|:---|:---|:---|:---|
|lock|小|特殊情况|慢|
|lockfree|较小|特殊情况|较快|
|lockfree_concurrent|较大|所有情况|快|

> 内存占用：日志缓冲队列使用的内存空间大小。  
> `lock` 使用 `block_queue`，初始化后不额外申请内存；`lockfree` 使用 `BlockingConcurrentQueue`，初始化后也不额外申请内存；`lockfree_concurrent` 使用 `ConcurrentQueue`，初始化后可能会请求扩容。
>
> 
> 线程内日志有序性：多个线程并发写日志时，在每个线程内，每条日志的实际输出次序与代码次序相同。  
> `lock` 和 `lockfree` 只能在线程池中仅单个后台线程时保证这一点，而 `lockfree_concurrent` 凭借队列设计和特殊的入队/出队方法，对于任意数量的后台线程，都能实现线程内日志的有序。

- 除异步模式外，learnlog 模仿了 spdlog 的优秀设计，整体操作逻辑大致相同，在若干细节实现上有所改动；
- 日志信息可以输出到：
  * 自动编号的一系列文件；
  * 一个普通文件；
  * 控制台（支持上色）；
  * 标准输出流`（std::ostream）`；
- 跨平台，在 Linux 和 Windows 下均充分测试；
- 完善了在 Windows 下对中文字符串`（std::wstring）`的支持，包括中文日志、中文路径、终端有色中文字符等；
- 有异常处理类，运行时不会因抛出异常而终止，而是捕获异常，并在控制台输出异常的发生位置与错误信息；
- 为大多数组件编写了单元测试，测试包括了多线程下的表现；
- 有较为明晰的中文注释；

## Example

[example.cpp](example/example.cpp) 中展示了 learnlog 的主要使用方法，包括：

1. 直接调用默认`logger`记录日志；
2. 创建自定义的`logger`并记录日志；
3. 创建`sink`来指派日志的输出位置；
4. 创建`async_logger`以安排异步任务，将日志记录请求和实际输出操作分离；

`example.cpp`默认开启编译，编译出的可执行文件位于 `build/example/example`。

## Benchmarks

编译方法：

```console
$ cd build
$ cmake -DLEARNLOG_BUILD_BENCH=ON ..
$ cmake --build .  # 或者 make -j
```

在生成的可执行文件中，`async_thread_pool_bench` 是对 3 个线程池（[lock](base/lock_thread_pool.h)、[lockfree](base/lockfree_thread_pool.h)、[lockfree_concurrent](base/lockfree_concurrent_thread_pool.h)）将日志写入文件的速度对比测试。依次改变 `日志总数`、`前台写日志线程数`、`后台输出日志线程数`，统计日志处理速度的变化：

```console
$ cd build/bench
$ ./async_thread_pool_bench  # ./async_thread_pool_bench <queue_size> <iter_times> <msg_num> <pthread_num> <cthread_num>
```

> 测试结果表明：
> - 在并发量大，资源争用明显的情况下`（写线程多于输出线程，短时间内日志入队请求大于队列总容量）`，`lockfree` 和 `lockfree_concurrent` 的处理速度是 `lock` 的 **5~10 倍**；  
> - 而随着 `后台输出线程` 的增加，`lockfree_concurrent` 在保证了 `线程内日志有序性` 的同时，处理速度稳步增长，显著快于 `lock` 和 `lockfree_concurrent`。

`async_queue_bench` 测试了 [ConcurrentQueue](base/concurrentqueue/concurrentqueue.h) 和 [BlockingConcurrentQueue](base/concurrentqueue/blockingconcurrentqueue.h) 在不同的 入队/出队 方法组合下的理论速度（仅将空日志入队/出队，不做中间处理），同时也加入了 [block_queue](base/concurrentqueue/block_queue.h) 进行对比：

```console
$ cd build/bench
$ ./async_queue_bench  # ./async_queue_bench <queue_size>
```

## 文档

开发过程中记录的部分笔记： [https://doc.def-a-name.top:2404/learnlog-note.html](https://doc.def-a-name.top:2404/learnlog-note.html)

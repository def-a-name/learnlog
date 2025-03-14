# learnlog


learnlog 是参照 [spdlog](https://github.com/gabime/spdlog) 设计的一个 C++ 日志库，同时在`异步模式`下加入了 [concurrentqueue](https://github.com/cameron314/concurrentqueue) 作为缓冲队列，在处理速度上有 **[大幅提升](#benchmarks)**。


## 编译方法

```console
$ git clone https://github.com/def-a-name/learnlog.git
$ cd learnlog && mkdir build && cd build
$ cmake .. && cmake --build .
```

## 特点

## Example

## Benchmarks

编译方法：

```console
$ cd build
$ cmake -DLEARNLOG_BUILD_BENCH=ON ..
$ cmake --build .  # 或者 make -j
```

在生成的可执行文件中，`async_thread_pool_bench` 是对 3 个线程池（[lock](base/lock_thread_pool.h)、[lockfree](base/lockfree_thread_pool.h)、[lockfree_concurrent](base/lockfree_concurrent_thread_pool.h)）将日志写入文件的速度对比测试。依次改变 `日志总数`、`前端写日志线程数`、`后端输出日志线程数`，统计日志处理速度的变化：

```console
$ cd build/bench
$ ./async_thread_pool_bench  # ./async_thread_pool_bench <queue_size> <iter_times> <msg_num> <pthread_num> <cthread_num>
```

`async_queue_bench` 测试了 [ConcurrentQueue](base/concurrentqueue/concurrentqueue.h) 和 [BlockingConcurrentQueue](base/concurrentqueue/blockingconcurrentqueue.h)）在不同的 入队/出队 方法组合下的理论速度（仅将空日志入队/出队，不做中间处理），同时也加入了 [block_queue](base/concurrentqueue/block_queue.h) 进行对比：

```console
$ cd build/bench
$ ./async_queue_bench  # ./async_queue_bench <queue_size>
```

## 文档

开发过程中记录的部分笔记： [https://doc.def-a-name.top:2404/learnlog-note.html](https://doc.def-a-name.top:2404/learnlog-note.html)

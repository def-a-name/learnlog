// #include <iostream>
// #include <atomic>
// #include <thread>
// #include <vector>
// #include <cassert>

// template <typename T>
// class MPMCQueue {
// private:
//     std::vector<T> buffer; // 环形缓冲区
//     std::atomic<size_t> write_index; // 写指针
//     std::atomic<size_t> read_index; // 读指针
//     size_t capacity; // 缓冲区容量

// public:
//     MPMCQueue(size_t size) : buffer(size), write_index(0), read_index(0), capacity(size) {}

//     bool enqueue(const T& item) {
//         size_t write_pos = write_index.load(std::memory_order_relaxed);
//         size_t next_write_pos = (write_pos + 1) % capacity;

//         // 检查队列是否满
//         if (next_write_pos == read_index.load(std::memory_order_acquire)) {
//             return false; // 队列满，无法入队
//         }

//         // 使用 CAS 原子操作更新写指针
//         if (write_index.compare_exchange_strong(write_pos, next_write_pos, std::memory_order_release, std::memory_order_relaxed)) {
//             buffer[write_pos] = item; // 将数据写入队列
//             return true;
//         }
//         return false; // 重试
//     }

//     bool dequeue(T& item) {
//         size_t read_pos = read_index.load(std::memory_order_relaxed);
        
//         // 检查队列是否空
//         if (read_pos == write_index.load(std::memory_order_acquire)) {
//             return false; // 队列空，无法出队
//         }

//         // 使用 CAS 原子操作更新读指针
//         size_t next_read_pos = (read_pos + 1) % capacity;
//         if (read_index.compare_exchange_strong(read_pos, next_read_pos, std::memory_order_release, std::memory_order_relaxed)) {
//             item = buffer[read_pos]; // 读取数据
//             return true;
//         }
//         return false; // 重试
//     }

//     bool is_empty() const {
//         return read_index.load(std::memory_order_acquire) == write_index.load(std::memory_order_acquire);
//     }

//     bool is_full() const {
//         return (write_index.load(std::memory_order_acquire) + 1) % capacity == read_index.load(std::memory_order_acquire);
//     }
// };

// void producer(MPMCQueue<int>& queue) {
//     for (int i = 0; i < 10; ++i) {
//         while (!queue.enqueue(i)) {
//             // 队列满时，生产者重试
//             std::this_thread::yield();
//         }
//         std::cout << "Produced: " << i << std::endl;
//     }
// }

// void consumer(MPMCQueue<int>& queue) {
//     int item;
//     for (int i = 0; i < 10; ++i) {
//         while (!queue.dequeue(item)) {
//             // 队列空时，消费者重试
//             std::this_thread::yield();
//         }
//         std::cout << "Consumed: " << item << std::endl;
//     }
// }

// int main() {
//     MPMCQueue<int> queue(5); // 容量为5的环形缓冲区

//     std::thread producer_thread(producer, std::ref(queue));
//     std::thread consumer_thread(consumer, std::ref(queue));

//     producer_thread.join();
//     consumer_thread.join();

//     return 0;
// }

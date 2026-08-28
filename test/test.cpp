// Synchronous logging must not turn the MPSC queue into a multi-consumer queue.
#define ALGLOG_CONTAINER_MPSC_RINGBUFFER
#include <alglog-project-logger-template.h>
#include <atomic>
#include <cstddef>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "test_multi_include.h"

namespace {

class counting_sink : public alglog::sink {
public:
    std::atomic<int> count{0};
    std::atomic<std::size_t> total_bytes{0};

    counting_sink() {
        valve = alglog::builtin::valve::always_open;
    }

    void output(const alglog::log_t& log) override {
        const auto message = log.msg;
        total_bytes.fetch_add(message.size());
        count.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
};

bool concurrent_sync_logging_preserves_all_messages() {
    auto sink = std::make_shared<counting_sink>();
    auto logger = std::make_shared<alglog::logger>();
    logger->connect_sink(sink);

    constexpr int num_threads = 8;
    constexpr int num_iterations = 1000;
    std::atomic<int> ready{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&] {
            ready.fetch_add(1);
            while (!start.load()) {
                std::this_thread::yield();
            }
            for (int j = 0; j < num_iterations; ++j) {
                logger->info("01234567890123456789012345678901234567890123456789 {}", j);
            }
        });
    }
    while (ready.load() != num_threads) {
        std::this_thread::yield();
    }
    start.store(true);
    for (auto& thread : threads) {
        thread.join();
    }
    constexpr std::size_t prefix_length = 51;
    const auto expected_bytes = num_threads * (10 * (prefix_length + 1) + 90 * (prefix_length + 2) + 900 * (prefix_length + 3));
    return sink->count.load() == num_threads * num_iterations && sink->total_bytes.load() == expected_bytes;
}

}  // namespace

int main(){

    if (!concurrent_sync_logging_preserves_all_messages()) {
        std::cerr << "synchronous logging lost or duplicated messages" << std::endl;
        return 1;
    }

    MyLogDebug("Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    MyLogDebug("ed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
    MyLogDebug("Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.");
    MyLogDebug("exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? ");
    MyLogDebug("vel illum, qui dolorem eum fugiat, quo voluptas nulla pariatur?");

    // level
    MyLogError("log");
    MyLogAlert("log");
    MyLogInfo("log");
    MyLogCritical("log");
    MyLogWarn("log");
    MyLogDebug("log");
    MyLogTrace("log");

    // fmt test
    MyLogDebug("The answer is {}.", 42);
    std::vector<int> vec = {1,2,3,4,5};
    MyLogTrace("vector =  {}", vec);

    // mt test
    {
        std::vector<std::shared_ptr<std::thread>> t_vec;
        for (int j=0; j<10; ++j){
            auto th = std::make_shared<std::thread>(
                [&](int tnum){
                    std::mt19937 mt;
                    std::random_device rnd;
                    mt.seed(rnd());
                    for(int i=0; i<10; ++i){
                        MyLogTrace("[ thread {} ] val = {}", tnum, i);
                        std::this_thread::sleep_for(std::chrono::milliseconds(mt() % 10));
                    }
                }, j
            );
            t_vec.push_back( th );
        }
        for (size_t j=0; j<t_vec.size(); ++j){
            t_vec[j]->join();
        }
    }

    // mt stress test
    {
        int num_threads = 10;
        int num_iterations = 1000;
        std::vector<std::shared_ptr<std::thread>> t_vec;
        std::atomic<int> error_count{0};

        for (int j=0; j<num_threads; ++j){
            auto th = std::make_shared<std::thread>(
                [&](int tnum){
                    std::mt19937 mt;
                    std::random_device rnd;
                    mt.seed(rnd());
                    for(int i=0; i<num_iterations; ++i){
                        auto start = std::chrono::high_resolution_clock::now();
                        MyLogInfo("[ thread {} ] stress val = {}", tnum, i);
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                        if (duration.count() > 100) {
                            MyLogError("[ thread {} ] Write time exceeded 100us: {}us", tnum, duration.count());
                            error_count++;
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(mt() % 50));
                    }
                }, j
            );
            t_vec.push_back( th );
        }
        for (size_t j=0; j<t_vec.size(); ++j){
            t_vec[j]->join();
        }
        if (error_count > 0) {
            MyLogInfo("MT stress test failed with {} errors({} %)", error_count, (100.0 * error_count / (num_threads * num_iterations)));
        } else {
            MyLogInfo("MT stress test passed.");
        }
    }


    // multi include test
    call_from_another_source(39);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));



    // speed test
    {
        const int num_logs = 1024*8;

        auto print_last_line = [](const std::string& filename){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::ifstream file(filename);
            std::string last_line;
            std::string line;
            while (std::getline(file, line)) {
                last_line = line;
            }
            std::cout << last_line << std::endl;
        };

        {
            auto l_sync = std::make_shared<alglog::logger>();
            l_sync->connect_sink( std::make_shared<alglog::builtin::file_sink>("time_count_sync.log") );
            auto t = alglog::time_counter(l_sync, "sync mode");
            for(int i=0; i<num_logs; ++i){
                l_sync->trace("log #{} {} {}", i,i,i);
            }
        }
        print_last_line("time_count_sync.log");

        {
            auto l_async = std::make_shared<alglog::logger>(true);
            l_async->connect_sink( std::make_shared<alglog::builtin::file_sink>("time_count_async.log") );
            auto t = alglog::time_counter(l_async, "async mode (no timer flush)");
            for(int i=0; i<num_logs; ++i){
                l_async->trace("log #{} {} {}", i,i,i);
            }
            l_async->flush();
        }
        print_last_line("time_count_async.log");
    }

    std::cout << "end" << std::endl;
}
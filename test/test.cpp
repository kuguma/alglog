#include <alglog-project-logger-template.h>
#include <iostream>
#include <random>
#include <thread>

#include "test_multi_include.h"


int main(){

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

    // multi include test
    call_from_another_source(39);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // speed test
    {
        auto l_sync = std::make_shared<alglog::logger>();
        l_sync->connect_sink( std::make_shared<alglog::builtin::file_sink>("time_count_sync.log") );
        {
            auto t = alglog::time_counter(l_sync, "sync mode");
            for(int i=0; i<100000; ++i){
                l_sync->trace("log #{} {} {}", i,i,i);
            }
        }
        auto l_async = std::make_shared<alglog::logger>(true);
        l_async->connect_sink( std::make_shared<alglog::builtin::file_sink>("time_count_async.log") );
        {
            auto t = alglog::time_counter(l_async, "async mode");
            for(int i=0; i<100000; ++i){
                l_async->trace("log #{} {} {}", i,i,i);
            }
            l_async->flush();
        }
        auto l_async_flush = std::make_shared<alglog::logger>(true);
        l_async_flush->connect_sink( std::make_shared<alglog::builtin::file_sink>("time_count_async_flush.log") );
        {
            auto f = std::make_unique<alglog::flusher>(l_async_flush);
            f->start(500);
            auto t = alglog::time_counter(l_async_flush, "async mode (flush)");
            for(int i=0; i<100000; ++i){
                l_async_flush->trace("log #{} {} {}", i,i,i);
            }
        }
    }

    // // error test
    // {
    //     auto l = alglog::builtin::get_default_logger();
    //     for(int i=0; i<1000; ++i){
    //         l->debug("log #{}", i);
    //         if (i == 500){
    //             vec.clear();
    //             l->debug("for error {}", i/0);
    //             throw;
    //         }
    //     }
    // }

    std::cout << "end" << std::endl;
}
#include <alglog.h>
#include <iostream>
#include <random>
#include <thread>

#include "test_mylogger.h"

#include "test_multi_include.h"

int main(){
    auto logger = alglog::get_default_logger("main");
    auto snk = std::make_shared<alglog::builtin::file_sink>("test.log");
    logger->connect_sink(snk);

    // level test
    logger->release("Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    logger->critical("ed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
    logger->warn("Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.");
    logger->debug("exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? ");
    logger->trace("vel illum, qui dolorem eum fugiat, quo voluptas nulla pariatur?");

    // fmt test
    logger->debug("The answer is {}.", 42);
    std::vector<int> vec = {1,2,3,4,5};
    logger->trace("vector =  {}", vec);

    // mt test
    std::vector<std::shared_ptr<std::thread>> t_vec;
    for (int j=0; j<10; ++j){
        auto th = std::make_shared<std::thread>(
            [&](int tnum){
                std::mt19937 mt;
                std::random_device rnd;
                mt.seed(rnd());
                for(int i=0; i<100; ++i){
                    logger->debug("[ thread {} ] val = {}", tnum, i);
                    std::this_thread::sleep_for(std::chrono::milliseconds(mt() % 5));
                }
            }, j
        );
        t_vec.push_back( th );
    }
    for (int j=0; j<t_vec.size(); ++j){
        t_vec[j]->join();
    }

    // multi project test
    // すべてのロガーは独立に動くので、出力の整合性は保たれない（まぁ妥当な挙動かと思う）
    // namespaceを切るなど、他プロジェクトに配慮した使い方は必要
    LogDebugPrj1("prj1");
    LogDebugPrj1("prj1 {}", 3939);
    LogDebugPrj2("prj2");
    LogDebugPrj2("prj2 {}", 3939);

    // multi include test
    call_from_another_source(39);
}
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
    MyLogAlart("log");
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
                    MyLogDebug("[ thread {} ] val = {}", tnum, i);
                    std::this_thread::sleep_for(std::chrono::milliseconds(mt() % 10));
                }
            }, j
        );
        t_vec.push_back( th );
    }
    for (int j=0; j<t_vec.size(); ++j){
        t_vec[j]->join();
    }

    // multi include test
    call_from_another_source(39);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

}
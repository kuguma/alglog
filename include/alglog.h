#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <fstream>
#include <atomic>
#include <mutex>


// switch
#if defined(NDEBUG) || (!defined(DEBUG) && !defined(_DEBUG))
    #define ALGLOG_CRITITAL_OFF
    #define ALGLOG_WARN_OFF
    #define ALGLOG_DEBUG_OFF
    #ifndef ALGLOG_TRACE
        #define ALGLOG_TRACE_OFF
    #endif
#endif

// #define ALGLOG_RELEASE_OFF
// #define ALGLOG_ALL_OFF


namespace alglog{

/* ----------------------------------------------------------------------------

    [ alglog ]

---------------------------------------------------------------------------- */


enum class level{
    release = 0, // このログはリリースにも含まれる。ALGLOG_RELEASE_OFFを定義すれば無効化できる。
// -------------------------------------------------------------------------------------------------- ↓デバッグビルドに含まれる
    critical, // 一応作っておくが、assertを使うべき。もしくはエラーを詳細化すべき。
    warn, // assertを掛けるまでではないが、なんか嫌な感じのことが起こってるときに出す。
    debug, // 理想的には、このログを眺めるだけでプログラムの挙動の全体の流れを理解できるようになっていると良い。
// -------------------------------------------------------------------------------------------------- ↓デバッグビルドかつALGLOG_TRACEのときに含まれる
    trace // 挙動を追うときに使う詳細なログ。機能開発中や、込み入ったバグを追いかけるときに使う。
};

// ログクラス
struct log_t{
    std::chrono::time_point<std::chrono::system_clock> time;
    std::thread::id thread_id;
    std::string msg;
    level lvl;
    std::string get_level_str() const {
        if (lvl == level::release){
            return "REL";
        }
        if (lvl == level::critical){
            return "CRT";
        }
        if (lvl == level::warn){
            return "WRN";
        }
        if (lvl == level::debug){
            return "DBG";
        }
        if (lvl == level::trace){
            return "TRC";
        }
        return "---";
    }
};


namespace builtin{
    const auto formatter = [](const log_t& l) -> std::string {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(l.time.time_since_epoch()).count() % 1000;
        return fmt::format("[{:%Y-%m-%d %H:%M:%S}.{:04}] [{}] [t {:>8}] | {}", l.time, ms, l.get_level_str(), l.thread_id, l.msg );
    };
    const auto always_output = [](const log_t& l){return true;};
    const auto trace_level_output = always_output;
    const auto debug_level_output = [](const log_t& l){if (l.lvl != level::trace) {true;} else {false;}}; // 出力する一番低いレベルがdebugということ。
    const auto release_level_output = [](const log_t& l){if (l.lvl == level::release) {true;} else {false;}}; // このラムダでリリースレベルにしても、出力が抑制されるだけでバイナリから消えるわけではない
}


struct sink{
    std::function<bool(const log_t&)> cond = builtin::always_output; // データを出力するかを判断する関数
    std::function<std::string(const log_t&)> formatter = builtin::formatter; // sinkはformatterを持ち、出力の際に利用する。
    virtual void output(const log_t&) = 0; // ログ出力のタイミングで接続されているloggerからこのoutputが呼び出される。
    void _cond_output(const log_t& l){
        if (cond(l)){
            output(l);
        }
    }
};


namespace builtin{
    struct file_sink : public sink{
        std::unique_ptr<std::ofstream> ofs;
        file_sink(const std::string& file_name) : ofs(std::make_unique<std::ofstream>(file_name)){}
        void output(const log_t& l) override {
            (*ofs.get()) << formatter(l) << std::endl;
        }
    };

    struct print_sink : public sink{
        void output(const log_t& l) override {
            std::cout << formatter(l) << std::endl;
        }
    };
}


class logger{
private:
    std::string name;
    std::vector<log_t> logs; // TODO : 独自クラスを作って高速化する
    std::vector<std::shared_ptr<sink>> sinks; // loggerは自分が持っているsink全てに入力されたlogを受け渡す。
    std::unique_ptr<std::thread> flusher_thread = nullptr;
    std::atomic<bool> flusher_thread_run;
    std::mutex logs_mtx;
    std::mutex sinks_mtx;
public:
    logger(const std::string& name) : name(name) {}
    ~logger(){
        if(flusher_thread){
            flusher_thread_run = false;
            flusher_thread->join(); // 終了まで待機
        }
        flush(); // 終了時に必ずフラッシュする
    }

    void connect_sink(std::shared_ptr<sink> s){
        std::lock_guard<std::mutex> lock(sinks_mtx);
        sinks.push_back(s);
    }

    // 保管されているログを出力する。
    void flush(){
        std::lock_guard<std::mutex> lock(logs_mtx);
        for(auto& log : logs){
            for(auto& s : sinks){
                s->_cond_output(log);
            }
        }
        logs.clear();
    }

    // ログを保管する。直ちに出力はしない。
    void store(const std::string& msg, const level lvl){
        #ifndef ALGLOG_ALL_OFF
            log_t log = {std::chrono::system_clock::now(), std::this_thread::get_id(), msg, lvl};
            std::lock_guard<std::mutex> lock(logs_mtx);
            logs.push_back(log);
        #endif
    }

// ------------------------------------


    template <class ... T>
    void release(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_RELEASE_OFF
            store(fmt::format(fmt, std::forward<T>(args)...), level::release);
        #endif
    }

    template <class ... T>
    void critical(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_CRITICAL_OFF
            store(fmt::format(fmt, std::forward<T>(args)...), level::critical);
        #endif
    }

    template <class ... T>
    void warn(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_WARN_OFF
            store(fmt::format(fmt, std::forward<T>(args)...), level::warn);
        #endif
    }

    template <class ... T>
    void debug(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_DEBUG_OFF
            store(fmt::format(fmt, std::forward<T>(args)...), level::debug);
        #endif
    }

    template <class ... T>
    void trace(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_TRACE_OFF
            store(fmt::format(fmt, std::forward<T>(args)...), level::trace);
        #endif
    }

// ------------------------------------

    // intervalミリ秒ごとにloggerをフラッシュする。
    // intervalを短くしすぎるとメインプログラムの挙動に影響が出る可能性があります。
    // TODO スレッド優先度を最低にする
    void flush_every(std::chrono::milliseconds interval = std::chrono::milliseconds(500)){
        flusher_thread = std::make_unique<std::thread>([&,interval]{
            while(flusher_thread_run){
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                this->flush();
            }
        });
    }
};

// 標準出力に対して500msごとにflushするロガーを取得する
inline std::shared_ptr<logger> get_default_logger(const std::string& name){
    auto lgr = std::make_shared<logger>(name);
    auto snk = std::make_shared<builtin::print_sink>();
    lgr->connect_sink(snk);
    lgr->flush_every(std::chrono::milliseconds(500));
    return lgr;
}

// __FILE__, __LINE__, __FUNC__

} // end namespace alglog
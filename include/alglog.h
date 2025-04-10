// Copyright(c) 2023-present, Kai Aoki
// Under MIT license, but binary embeddable without copyright notice.
// https://github.com/kuguma/alglog

#pragma once

#ifndef ALGLOG_DIRECT_INCLUDE_GUARD
    #error "Direct inclusion of alglog.h is prohibited. Create project logger and define ALGLOG_DIRECT_INCLUDE_GUARD before include this. (see readme.md)"
#endif

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <fmt/std.h>
#include <fmt/color.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <list>
#include <array>
#include <functional>
#include <string>
#include <memory>
#include <fstream>
#include <atomic>
#include <mutex>
#include <cassert>
#include "mpsc_ring_buffer.h"


/* ----------------------------------------------------------------------------

    [ alglog ]

---------------------------------------------------------------------------- */

/*
    < usage >
    次のケースでの利用を推奨する。

    1. alglog-project-logger-template.h を参考に、プロジェクトロガーのヘッダソースを作成する。
        必要に応じて、Loggerのコンストラクタでsinkやフォーマッタを設定する。
    2. プロジェクトの各ソースからは、alglog.hではなく先ほど作成したプロジェクトロガーをincludeするようにする。
*/


// compile switch -------------------------------------------------

#if defined(NDEBUG) || ( defined(_MSC_VER) && (!defined(_DEBUG)) )
    #define ALGLOG_RELEASE_BUILD
#else
    #define ALGLOG_DEBUG_BUILD
#endif


#ifdef ALGLOG_DEFAULT_LOG_SWITCH
#ifdef ALGLOG_RELEASE_BUILD
    #define ALGLOG_ERROR_ON
    #define ALGLOG_ALERT_ON
    #define ALGLOG_INFO_ON
#endif

#ifdef ALGLOG_DEBUG_BUILD
    #define ALGLOG_ERROR_ON
    #define ALGLOG_ALERT_ON
    #define ALGLOG_INFO_ON
    #define ALGLOG_CRITICAL_ON
    #define ALGLOG_WARN_ON
    #define ALGLOG_DEBUG_ON
    #define ALGLOG_TRACE_ON
    #define ALGLOG_INTERNAL_ON
#endif
#endif


// システムコールでプロセスIDを取得する。もしくは機能を利用しない。
#if defined(ALGLOG_GETPID) && (defined(_WIN32) || defined(_WIN64))
    #include <windows.h>
    inline uint32_t get_process_id(){
        return static_cast<uint32_t>(GetCurrentProcessId());
    }
#elif defined(ALGLOG_GETPID)
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/syscall.h>
    inline uint32_t get_process_id(){
        return static_cast<uint32_t>(getpid());
    }
#else
    inline uint32_t get_process_id(){
        return 0;
    }
#endif

// スレッドIDを取得する。もしくは機能を利用しない。
#ifdef ALGLOG_GETTID
    inline std::thread::id get_thread_id(){
        return std::this_thread::get_id();
    }
#else
    inline std::thread::id get_thread_id(){
        return std::thread::id();
    }
#endif

// flusher thread の優先度を自動設定する。
#if defined(ALGLOG_AUTO_THREAD_PRIORITY) && (defined(_WIN32) || defined(_WIN64))
    #include <windows.h> // Required for SetThreadPriority
    inline void set_thread_priority_lowest(){
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    }
#elif defined(ALGLOG_AUTO_THREAD_PRIORITY)
    #include <pthread.h> // Required for pthread functions
    #include <sched.h>   // Required for sched functions
    inline void set_thread_priority_lowest(){
        sched_param sch;
        int policy;
        pthread_getschedparam(pthread_self(), &policy, &sch);
        sch.sched_priority = sched_get_priority_min(policy);
        pthread_setschedparam(pthread_self(), policy, &sch);
    }
#else
    inline void set_thread_priority_lowest(){
        // do nothing
    }
#endif

// ファイル名のベースネームを取得する。
// 新しいgcc, clangでは__FILE_NAME__が使える。
// 使えない場合は再帰テンプレートによりベースネームを抽出する。
// NOTE : C++20からはstd::source_locationが使える。
// TODO : 対応
#if (__GNUC__ >= 12)
    #define __ALGLOG_FNAME__ __FILE_NAME__
#elif (__clang_major__ >= 10)
    #define __ALGLOG_FNAME__ __FILE_NAME__
#else
    template <typename T, size_t S>
    inline constexpr size_t _alglog_fname_offset(const T (& str)[S], size_t i = S - 1)
    {
        return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? _alglog_fname_offset(str, i - 1) : 0);
    }

    template <typename T>
    inline constexpr size_t _alglog_fname_offset(T (& str)[1])
    {
        return 0;
    }

    #define __ALGLOG_FNAME__ ((const char*)__FILE__ + _alglog_fname_offset(__FILE__))
#endif

// -------------------------------------------------------


namespace alglog{

enum class level{
// -------------------------------------------------------------------------------------------------- ↓リリースビルドに含まれる
    error = 0, // ユーザー向けエラー情報ログ：APIの投げた例外を補足する形などを想定。
    alert, // ユーザー向け警告ログ：ユーザーの意図しないフォールバック等が行われた場合の出力利用を想定。
    info, // ユーザー向け情報提供ログ：API呼び出し履歴などを想定。
// -------------------------------------------------------------------------------------------------- ↓デバッグビルドに含まれる
    critical, // 致命的な内部エラー：assertと組み合わせて使うと効果的。
    warn, // assertを掛けるまでではないが、何か嫌な感じのことが起こってるときに出す。
    debug, // 理想的には、このログを眺めるだけでプログラムの挙動の全体の流れを理解できるようになっていると良い。
// -------------------------------------------------------------------------------------------------- ↓デバッグビルドかつALGLOG_TRACEのときに含まれる
    trace // 挙動を追うときに使う詳細なログ。機能開発中や、込み入ったバグを追いかけるときに使う。
};


// ソース位置（マクロを利用して流し込む）
struct source_location {
    const char* file = "";
    int line = 0;
    const char* func = "";
    constexpr source_location() = default;
    constexpr source_location(const char* file, int line, const char* func)
        : file(file), line(line), func(func) {}
};

// ログクラス
struct log_t{
    std::string msg;
    level lvl;
    std::chrono::time_point<std::chrono::system_clock> time;
    uint32_t pid;
    std::thread::id tid;
    source_location loc;
    std::string get_level_str() const {
        if (lvl == level::error){
            return " ERR";
        }
        if (lvl == level::alert){
            return "ALRT";
        }
        if (lvl == level::info){
            return "INFO";
        }
        if (lvl == level::critical){
            return "CRIT";
        }
        if (lvl == level::warn){
            return "WARN";
        }
        if (lvl == level::debug){
            return " DBG";
        }
        if (lvl == level::trace){
            return "TRCE";
        }
        return "----";
    }
};


// ------------------------------------
// Log収集用コンテナ

// alglogで用いるログコンテナのインターフェース。
// 任意の実装を利用可能だが、スレッドセーフ or ロックフリーの実装を推奨する。
class log_container_interface{
public:
    // ログを追加する。追加に成功したらtrueを返す。
    // ブロック不可。
    virtual bool push(const log_t&) = 0;

    // ログを取り出す。取り出しに成功したらtrueを返す。
    // ブロック不可。
    virtual bool pop(log_t&) = 0;
};


// std::listとmtxを使った簡易スレッドセーフ実装。
// ほぼ無限に書き込めるが、ロックが長く低速。
class log_container_std_list : public log_container_interface{
private:
    std::list<log_t> c;
    mutable std::mutex mtx;
public:
    bool push(const log_t& l) override {
        std::lock_guard<std::mutex> lock(mtx);
        c.push_back(l);
        return true;
    }
    bool pop(log_t& l) override {
        std::lock_guard<std::mutex> lock(mtx);
        if (c.empty()){
            return false;
        }
        l = c.front();
        c.pop_front();
        return true;
    }
};

// multi producer / single consumer のリングバッファ。
// 容量に制限があり、内容量に関わらずメモリを消費する。
// 制限を超える分は書込みに失敗するが、ロックフリーで比較的高速。
template <size_t N>
class log_container_mpsc : public log_container_interface{
private:
    mpsc_ring_buffer<log_t, N> c;
public:
    bool push(const log_t& l) override {
        return c.push(l);
    }
    bool pop(log_t& l) override {
        return c.pop(l);
    }
};

#if defined(ALGLOG_CONTAINER_STD_LIST) && defined(ALGLOG_CONTAINER_MPSC_RINGBUFFER)
    #error "ALGLOG_CONTAINER_STD_LIST and ALGLOG_CONTAINER_MPSC_RINGBUFFER are mutually exclusive. Please define only one."
#endif

#if defined(ALGLOG_CONTAINER_STD_LIST)
    using log_container_t = log_container_std_list;
#elif defined(ALGLOG_CONTAINER_MPSC_RINGBUFFER)
    #if defined (ALGLOG_MPSC_RINGBUFFER_SIZE)
        using log_container_t = log_container_mpsc<ALGLOG_MPSC_RINGBUFFER_SIZE>;
    #else
        using log_container_t = log_container_mpsc<1024 * 16>;
    #endif
#else
    // default
    using log_container_t = log_container_std_list;
#endif

// ------------------------------------
// Core

struct sink{
    std::function<bool(const log_t&)> valve = nullptr; // データを出力するかを判断する関数
    std::function<std::string(const log_t&)> formatter = nullptr; // sinkはformatterを持ち、出力の際に利用する。
    virtual void output(const log_t&) = 0; // ログ出力のタイミングで接続されているloggerからこのoutputが呼び出される。
    void _cond_output(log_t& l){
        if (valve(l)){
            output(l);
        }
    }
    virtual ~sink(){}
};

class logger{
private:
    log_container_t logs;
    std::vector<std::shared_ptr<sink>> sinks; // loggerは自分が持っているsink全てに入力されたlogを受け渡す。
    std::mutex sinks_mtx;
public:
    const bool async_mode; // 非同期モードフラグ。非同期モードでは手動でflushする必要がある。同期モードではログ記録と同時に自動的にflush()が呼ばれる。
    logger(bool async_mode = false) : async_mode(async_mode) {}
    ~logger(){
        flush(); // 終了時に必ずフラッシュする
    }

    void connect_sink(std::shared_ptr<sink> s){
        std::lock_guard<std::mutex> lock(sinks_mtx);
        sinks.push_back(s);
    }

    // 保管されているログを全て出力する。
    void flush(){
        while(true){
            log_t l;
            auto ret = logs.pop(l);
            if (!ret){
                break;
            }
            for(auto& s : sinks){
                s->_cond_output(l);
            }
        }
    }

    // ------------------------------------
    // ログ保管

    // ログを保管する。
    // すべてのログ出力はこのraw_storeを通る。
    void raw_store(source_location loc, const level lvl, const std::string& msg){
        log_t log = {
            msg,
            lvl,
            std::chrono::system_clock::now(),
            get_process_id(),
            get_thread_id(),
            loc
        };
        logs.push(log);
        if (!async_mode){
            flush();
        }
    }

    void raw_store(const level lvl, const std::string& msg){
        raw_store(source_location{}, lvl, msg);
    }

    template <class ... T>
    void fmt_store(source_location loc, const level lvl, fmt::format_string<T...> fmt, T&&... args){
        // TODO : C++17以降であればconstexpr ifを使える
        switch(lvl){
            case level::error:
                #ifdef ALGLOG_ERROR_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::alert:
                #ifdef ALGLOG_ALERT_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::info:
                #ifdef ALGLOG_INFO_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::critical:
                #ifdef ALGLOG_CRITICAL_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::warn:
                #ifdef ALGLOG_WARN_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::debug:
                #ifdef ALGLOG_DEBUG_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            case level::trace:
                #ifdef ALGLOG_TRACE_ON
                    raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
                #endif
                break;
            default:
                break;
        }
    }

    template <class ... T>
    void fmt_store(const level lvl, fmt::format_string<T...> fmt, T&&... args){
        fmt_store(source_location{}, lvl, fmt, std::forward<T>(args)...);
    }

    // ----------------------------------------------

    template <class ... T>
    void error(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_ERROR_ON
            raw_store(level::error, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void alert(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_ALERT_ON
            raw_store(level::alert, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void info(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_INFO_ON
            raw_store(level::info, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void critical(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_CRITICAL_ON
            raw_store(loc, level::critical, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void critical(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_CRITICAL_ON
            raw_store(level::critical, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }


    template <class ... T>
    void warn(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_WARN_ON
            raw_store(loc, level::warn, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void warn(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_WARN_ON
            raw_store(level::warn, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void debug(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_DEBUG_ON
            raw_store(loc, level::debug, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void debug(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_DEBUG_ON
            raw_store(level::debug, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void trace(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_TRACE_ON
            raw_store(loc, level::trace, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void trace(fmt::format_string<T...> fmt, T&&... args){
        #ifdef ALGLOG_TRACE_ON
            raw_store(level::trace, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
};

// 定期的にロガーをフラッシュしたい場合に使えるヘルパークラス
class flusher{
private:
    std::weak_ptr<logger> lgr;
    std::atomic<bool> flusher_thread_run;
    std::unique_ptr<std::thread> flusher_thread = nullptr;
public:
    flusher(std::weak_ptr<logger> logger_weak_ptr) : lgr(logger_weak_ptr), flusher_thread_run(false) {
        if (auto l = lgr.lock()){
            assert(l->async_mode);
        }
    }
    ~flusher(){
        if(flusher_thread){
            flusher_thread_run = false;
            flusher_thread->join(); // 終了まで待機
        }
    }

    // intervalミリ秒ごとにloggerをフラッシュする。
    // intervalを短くしすぎるとメインプログラムの挙動に影響が出る可能性がある。
    void start(int interval_ms = 500){
        auto interval = std::chrono::milliseconds(interval_ms);
        flusher_thread_run = true;
        flusher_thread = std::make_unique<std::thread>([&,interval]{
            #ifdef ALGLOG_INTERNAL_ON
                if (auto l = lgr.lock()){
                    l->raw_store(level::debug, "[alglog] start periodic flashing");
                }
            #endif
            set_thread_priority_lowest();

            while(flusher_thread_run){
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if (auto l = lgr.lock()){
                    l->flush();
                }else{
                    break; // loggerが解放された場合、flusherのスレッドも終了する
                }
            }
        });
    }
    void stop(){
        flusher_thread_run = false;
    }
};

// ------------------------------------


namespace builtin{

    namespace formatter{
        // リリース時コンソール出力向けのフォーマッタ
        const auto simple = [](const log_t& l) -> std::string {
            return fmt::format("[{:%F %T}] [{}] | {}",
                l.time, l.get_level_str(), l.msg );
        };
        // デバッグ時ファイル出力向けのフォーマッタ。全てのパラメータを出力する
        const auto full = [](const log_t& l) -> std::string {
            return fmt::format("[{:%F %T}] [{}] [process {:>8}] [thread {:>8}] [{:>24}:{:<4}({:>24})] | {}",
                l.time, l.get_level_str(), l.pid, l.tid, l.loc.file, l.loc.line, l.loc.func, l.msg );
            // ref : https://cpprefjp.github.io/reference/chrono/format.html
        };
        // デバッグ時コンソール出力向けのフォーマッタ
        const auto console = [](const log_t& l) -> std::string {
            return fmt::format("[{:%T}] [{}] [{:>24}: {:<4}({:>24})] | {}",
                l.time, l.get_level_str(), l.loc.file, l.loc.line, l.loc.func, l.msg );
        };

    }

// valve
    namespace valve{
        const auto always_open = [](const log_t&){return true;};
        const auto except_trace = [](const log_t& l){if (l.lvl != level::trace) {return true;} else {return false;}};
        const auto release_only = [](const log_t& l){if (static_cast<int>(l.lvl) <= static_cast<int>(level::info)) {return true;} else {return false;}}; // リリースモードのシミュレートをするだけでバイナリからは消えない。
        const auto debug_only = [](const log_t& l){if (static_cast<int>(level::critical) <= static_cast<int>(l.lvl)) {return true;} else {return false;}}; // デバッグモードのシミュレートをするだけでバイナリからは消えない。
    }

// sink
    struct file_sink : public sink{
        std::unique_ptr<std::ofstream> ofs;
        file_sink(const std::string& file_name) : ofs(std::make_unique<std::ofstream>(file_name)) {
            this->valve = valve::always_open;
            this->formatter = formatter::full;
        }
        void output(const log_t& l) override {
            (*ofs.get()) << formatter(l) << std::endl;
        }
        virtual ~file_sink() {
            if (ofs){
                ofs->flush();
            }
        }
    };

    struct print_sink : public sink{
        print_sink(){
            this->valve = valve::always_open;
            this->formatter = formatter::console;
        }
        void output(const log_t& l) override {
            std::cout << formatter(l) << std::endl;
        }
    };

    namespace color{
        // https://www.schemecolor.com/time-plan.php
        static constexpr uint32_t watermelon_red = 0xC3443D;
        static constexpr uint32_t mellow_apricot = 0xFABA76;
        static constexpr uint32_t caramel = 0xFADE9B;
        static constexpr uint32_t pearl_aqua = 0x85D6B2;
        static constexpr uint32_t moonstone = 0x3F9EBD;
        static constexpr uint32_t stpatricks_blue = 0x2B2D7C;
    }

    struct color_print_sink : public print_sink{
        void output(const log_t& l) override {
            auto fg_color = fmt::color::white;
            switch(l.lvl){
                case level::error:
                    fg_color = static_cast<fmt::color>(color::watermelon_red);
                    break;
                case level::alert:
                    fg_color = static_cast<fmt::color>(color::mellow_apricot);
                    break;
                case level::info:
                    fg_color = static_cast<fmt::color>(color::pearl_aqua);
                    break;
                case level::critical:
                    fg_color = static_cast<fmt::color>(color::watermelon_red);
                    break;
                case level::warn:
                    fg_color = static_cast<fmt::color>(color::caramel);
                    break;
                case level::debug:
                    fg_color = static_cast<fmt::color>(color::moonstone);
                    break;
                case level::trace:
                    fg_color = fmt::color::light_slate_gray;
                    break;
                default:
                    break; 
            }
            fmt::print(fg(fg_color), formatter(l) + "\n");
        }
    };

    // 標準出力に対して出力する同期ロガーを取得する
    inline std::shared_ptr<logger> get_default_logger(){
        auto lgr = std::make_shared<logger>();
        auto snk = std::make_shared<color_print_sink>();
        lgr->connect_sink(snk);
        return lgr;
    }
}

class time_counter{
private:
    std::weak_ptr<logger> lgr;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::string title;
    level lvl;

public:
    time_counter(std::shared_ptr<logger> logger_weak_ptr, const std::string& title, level lvl = level::debug) : lgr(logger_weak_ptr), title(title), lvl(lvl) {
        if(auto l = lgr.lock()){
            l->fmt_store(lvl, "[{}] start time count", title);
        }
        start_time = std::chrono::high_resolution_clock::now();
    }

    ~time_counter() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000>>>(end_time - start_time);
        if(auto l = lgr.lock()){
            l->fmt_store(lvl, "[{}] {}", title, elapsed_time);
        }
    }
};


// -------------------------------------------------------

#define ALGLOG_SR alglog::source_location{__ALGLOG_FNAME__, __LINE__, __func__}


} // end namespace alglog

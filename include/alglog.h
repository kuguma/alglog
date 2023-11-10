// Copyright(c) 2023-present, Kai Aoki
// Under MIT license, but binary embeddable without copyright notice.

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <fmt/std.h>
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


/* ----------------------------------------------------------------------------

    [ alglog ]

---------------------------------------------------------------------------- */

/*
    < usage >
    次のケースでの利用を推奨する。

    1. alglog-project-logger-template.h を参考に、プロジェクトロガーのヘッダソースを作成する。
        Loggerのコンストラクタでsinkやフォーマッタを設定する。
        AlgLogXXXをラップした自作マクロを定義する。
    2. プロジェクトの各ソースからは、alglog.hではなく先ほど作成したプロジェクトロガーをincludeするようにする。
*/


// compile switch -------------------------------------------------

// デフォルト動作では、ERROR, ALART, INFOのみがリリースビルドで残る。
#ifdef ALGLOG_ALL_OFF
    #define ALGLOG_ERROR_OFF
    #define ALGLOG_ALART_OFF
    #define ALGLOG_INFO_OFF
    #define ALGLOG_CRITICAL_OFF
    #define ALGLOG_WARN_OFF
    #define ALGLOG_DEBUG_OFF
    #define ALGLOG_TRACE_OFF
    #define ALGLOG_INTERNAL_OFF
#elif defined(NDEBUG) || ( defined(_MSC_VER) && (!defined(_DEBUG)) )
    #define ALGLOG_CRITICAL_OFF
    #define ALGLOG_WARN_OFF
    #define ALGLOG_DEBUG_OFF
    #define ALGLOG_TRACE_OFF
    #define ALGLOG_INTERNAL_OFF
#endif

// システムコールでプロセスIDを取得する。もしくは機能を利用しない。
#ifdef ALGLOG_GETPID_OFF
    inline uint32_t get_process_id(){
        return 0;
    }
#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    inline uint32_t get_process_id(){
        return static_cast<uint32_t>(GetCurrentProcessId());
    }
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/syscall.h>
    inline uint32_t get_process_id(){
        return static_cast<uint32_t>(getpid());
    }
#endif

// スレッドIDを取得する。もしくは機能を利用しない。
#ifdef ALGLOG_GETTID_OFF
    inline std::thread::id get_thread_id(){
        return std::thread::id(0);
    }
#else
    inline std::thread::id get_thread_id(){
        return std::this_thread::get_id();
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

    #define __ALGLOG_FNAME__ (__FILE__ + _alglog_fname_offset(__FILE__))
#endif

// -------------------------------------------------------


namespace alglog{

enum class level{
// -------------------------------------------------------------------------------------------------- ↓リリースビルドに含まれる
    error = 0, // ユーザー向けエラー情報ログ：APIの投げた例外を補足する形などを想定。
    alart, // ユーザー向け警告ログ：ユーザーの意図しないフォールバック等が行われた場合の出力利用を想定。
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
        if (lvl == level::alart){
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

// Core

#ifdef ALGLOG_USE_ARRAY_CONTAINER // 試験的な機能。デフォルトはオフ
    template <class T, int N>
    struct veclike_array{
        std::array<T,N> arr;
        long long cnt = 0;

        void push_back(T& val){
            arr.at(cnt) = val;
            ++cnt;
        }
        void clear() {
            cnt = 0;
        }
        long long size() const {
            return cnt;
        }
        bool is_full() const {
            return cnt == N-1;
        }
        const T* begin() const {
            return &arr[0];
        }
        const T* end() const {
            return &arr[cnt];
        }
    };
    using log_container_t = std::veclike_array<log_t. 4096>;
#else
    using log_container_t = std::list<log_t>;
#endif


struct sink{
    std::function<bool(const log_t&)> valve = nullptr; // データを出力するかを判断する関数
    std::function<std::string(const log_t&)> formatter = nullptr; // sinkはformatterを持ち、出力の際に利用する。
    virtual void output(const log_t&) = 0; // ログ出力のタイミングで接続されているloggerからこのoutputが呼び出される。
    void _cond_output(const log_container_t& logs){
        for(const auto& l : logs){
            if (valve(l)){
                output(l);
            }
        }
    }
};

class logger{
private:
    log_container_t logs;
    std::vector<std::shared_ptr<sink>> sinks; // loggerは自分が持っているsink全てに入力されたlogを受け渡す。
    std::mutex logs_mtx;
    std::mutex sinks_mtx;
public:
    const bool async_mode;
    logger(bool async_mode = false) : async_mode(async_mode) {}
    ~logger(){
        flush(); // 終了時に必ずフラッシュする
    }

    void connect_sink(std::shared_ptr<sink> s){
        std::lock_guard<std::mutex> lock(sinks_mtx);
        sinks.push_back(s);
    }

    // 保管されているログを出力する。
    void flush(){
        std::lock_guard<std::mutex> lock(logs_mtx);
        flush_no_lock();
    }

    void flush_no_lock(){
        for(auto& s : sinks){
            s->_cond_output(logs);
        }
        logs.clear();
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
        if (!async_mode){
            logs.push_back(log);
            flush_no_lock();
        }else{
            std::lock_guard<std::mutex> lock(logs_mtx);
            logs.push_back(log);
            #ifdef ALGLOG_USE_ARRAY_CONTAINER
                if (logs.is_full()){
                    flush_no_lock();
                }
            #endif
        }
    }

    void raw_store(const level lvl, const std::string& msg){
        raw_store(source_location{}, lvl, msg);
    }

    template <class ... T>
    void fmt_store(source_location loc, const level lvl, fmt::format_string<T...> fmt, T&&... args){
        raw_store(loc, lvl, fmt::format(fmt, std::forward<T>(args)...));
    }

    template <class ... T>
    void fmt_store(const level lvl, fmt::format_string<T...> fmt, T&&... args){
        raw_store(lvl, fmt::format(fmt, std::forward<T>(args)...));
    }

    // ----------------------------------------------

    template <class ... T>
    void error(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_ERROR_OFF
            raw_store(level::error, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void alart(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_ALART_OFF
            raw_store(level::alart, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void info(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_INFO_OFF
            raw_store(level::info, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void critical(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_CRITICAL_OFF
            raw_store(loc, level::critical, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void critical(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_CRITICAL_OFF
            raw_store(level::critical, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }


    template <class ... T>
    void warn(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_WARN_OFF
            raw_store(loc, level::warn, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void warn(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_WARN_OFF
            raw_store(level::warn, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void debug(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_DEBUG_OFF
            raw_store(loc, level::debug, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void debug(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_DEBUG_OFF
            raw_store(level::debug, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }

    template <class ... T>
    void trace(source_location loc, fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_TRACE_OFF
            raw_store(loc, level::trace, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
    template <class ... T>
    void trace(fmt::format_string<T...> fmt, T&&... args){
        #ifndef ALGLOG_TRACE_OFF
            raw_store(level::trace, fmt::format(fmt, std::forward<T>(args)...));
        #endif
    }
};


// #ifndef ALGLOG_INTERNAL_OFF
//     #define ALGLOG_INTERNAL_STORE(...) lgr->raw_store(level::debug, __VA_ARGS__)
// #else
//     #define ALGLOG_INTERNAL_STORE(...) ((void)0)
// #endif

// 定期的にロガーをフラッシュしたい場合に使えるヘルパークラス
class flusher{
private:
    std::atomic<bool> flusher_thread_run;
    std::unique_ptr<std::thread> flusher_thread = nullptr;
    std::weak_ptr<logger> lgr;
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
    // TODO スレッド優先度を最低にする
    void start(int interval_ms = 500){
        auto interval = std::chrono::milliseconds(interval_ms);
        flusher_thread_run = true;
        flusher_thread = std::make_unique<std::thread>([&,interval]{
            #ifndef ALGLOG_INTERNAL_OFF
                if (auto l = lgr.lock()){
                    l->raw_store(level::debug, "[alglog] start periodic flashing");
                }
            #endif
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
        const auto always_open = [](const log_t& l){return true;};
        const auto except_trace = [](const log_t& l){if (l.lvl != level::trace) {return true;} else {return false;}};
        const auto release_mode = [](const log_t& l){if (static_cast<int>(l.lvl) <= static_cast<int>(level::info)) {return true;} else {return false;}}; // リリースモードのシミュレートをするだけでバイナリからは消えない。
        const auto debug_mode = [](const log_t& l){if (static_cast<int>(level::critical) <= static_cast<int>(l.lvl)) {return true;} else {return false;}}; // リリースモードのシミュレートをするだけでバイナリからは消えない。
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

    // 標準出力に対して出力する同期ロガーを取得する
    inline std::shared_ptr<logger> get_default_logger(){
        auto lgr = std::make_shared<logger>();
        auto snk = std::make_shared<print_sink>();
        lgr->connect_sink(snk);
        return lgr;
    }
}

class time_counter{
private:
    std::weak_ptr<logger> lgr;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    level lvl;
    std::string title;

public:
    time_counter(std::shared_ptr<logger> logger_weak_ptr, const std::string& title, level lvl = level::debug) : lgr(logger_weak_ptr), lvl(lvl), title(title) {
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

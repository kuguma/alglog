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


/* ----------------------------------------------------------------------------

    [ alglog ]

---------------------------------------------------------------------------- */


// compile switch -------------------------------------------------

// リリースビルドではlevel::release以外は無効化される。
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

#pragma C

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
#elif defined(__linux__) || defined (__apple__)
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
    error = 0, // ユーザー向けエラー情報ログ：APIの発射する例外を補足する形を想定。
    alart, // ユーザー向け警告ログ：ユーザーの意図しないフォールバック等が行われた場合の出力利用を想定。
    info, // ユーザー向け情報提供ログ：API呼び出し履歴等を想定。
// -------------------------------------------------------------------------------------------------- ↓デバッグビルドに含まれる
    critical, // 致命的な内部エラー：assertと組み合わせて使うと効果的。
    warn, // assertを掛けるまでではないが、なんか嫌な感じのことが起こってるときに出す。
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
        return "---";
    }
};


// ------------------------------------

// Core

#ifndef ALGLOG_INTERNAL_OFF
    #define ALGLOG_INTERNAL_STORE(...) this->raw_store(level::debug, __VA_ARGS__)
#else
    #define ALGLOG_INTERNAL_STORE(...) ((void)0)
#endif

struct sink{
    std::function<bool(const log_t&)> valve = nullptr; // データを出力するかを判断する関数
    std::function<std::string(const log_t&)> formatter = nullptr; // sinkはformatterを持ち、出力の際に利用する。
    virtual void output(const log_t&) = 0; // ログ出力のタイミングで接続されているloggerからこのoutputが呼び出される。
    void _cond_output(const log_t& l){
        if (valve(l)){
            output(l);
        }
    }
};


class logger{
private:
    std::vector<log_t> logs; // TODO : 独自クラスを作って高速化する
    std::vector<std::shared_ptr<sink>> sinks; // loggerは自分が持っているsink全てに入力されたlogを受け渡す。
    std::unique_ptr<std::thread> flusher_thread = nullptr;
    std::atomic<bool> flusher_thread_run;
    std::mutex logs_mtx;
    std::mutex sinks_mtx;
public:
    logger() {}
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

    // ------------------------------------
    // ログ保管。基本的にはマクロを経由して呼び出す前提の設計

    // ログを保管する。直ちに出力はしない。
    void raw_store(source_location loc, const level lvl, const std::string& msg){
        log_t log = {
            msg,
            lvl,
            std::chrono::system_clock::now(),
            get_process_id(),
            get_thread_id(),
            loc
        };
        std::lock_guard<std::mutex> lock(logs_mtx);
        logs.push_back(log);
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

    // ------------------------------------

    // intervalミリ秒ごとにloggerをフラッシュする。
    // intervalを短くしすぎるとメインプログラムの挙動に影響が出る可能性があります。
    // TODO スレッド優先度を最低にする
    void flush_every(std::chrono::milliseconds interval = std::chrono::milliseconds(500)){
        if (flusher_thread_run) {
            ALGLOG_INTERNAL_STORE("[alglog] already periodic flashing");
            return;
        }
        flusher_thread_run = true;
        flusher_thread = std::make_unique<std::thread>([&,interval]{
            ALGLOG_INTERNAL_STORE("[alglog] start periodic flashing");
            while(flusher_thread_run){
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                this->flush();
            }
        });
    }
};

// ------------------------------------


namespace builtin{

    namespace formatter{
        // デバッグ時ファイル出力向けのフォーマッタ。全てのパラメータを出力する
        const auto full = [](const log_t& l) -> std::string {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(l.time.time_since_epoch()).count() % 1000;
            return fmt::format("[{:%F %T}.{:04}] [{}] [process {:>8}] [thread {:>8}] [{:>24}:{:<4}({:>24})] | {}",
                l.time, ms, l.get_level_str(), l.pid, l.tid, l.loc.file, l.loc.line, l.loc.func, l.msg );
            // ref : https://cpprefjp.github.io/reference/chrono/format.html
        };
        // デバッグ時コンソール出力向けのフォーマッタ
        const auto console = [](const log_t& l) -> std::string {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(l.time.time_since_epoch()).count() % 1000;
            return fmt::format("[{:%T}.{:04}] [{}] [{:>24}: {:<4}({:>24})] | {}",
                l.time, ms, l.get_level_str(), l.loc.file, l.loc.line, l.loc.func, l.msg );
        };
        // リリース時コンソール出力向けのフォーマッタ
        const auto simple = [](const log_t& l) -> std::string {
            return fmt::format("[{:%F %T}] [{}] | {}",
                l.time, l.get_level_str(), l.msg );
        };
    }

// valve
    namespace valve{
        const auto always_open = [](const log_t& l){return true;};
        const auto except_trace = [](const log_t& l){if (l.lvl != level::trace) {true;} else {false;}};
        const auto release_mode = [](const log_t& l){if (static_cast<int>(l.lvl) <= static_cast<int>(level::info)) {true;} else {false;}}; // リリースモードのシミュレートをするだけでバイナリからは消えない。
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

    // 標準出力に対して500msごとにflushするロガーを取得する
    inline std::unique_ptr<logger> get_default_logger(){
        auto lgr = std::make_unique<logger>();
        auto snk = std::make_shared<print_sink>();
        lgr->connect_sink(snk);
        lgr->flush_every(std::chrono::milliseconds(500));
        return lgr;
    }
}





// -------------------------------------------------------

/*
    ・C++20より前では、マクロを使わないとソース情報を取得できない。
    ・デバッグにおいては、ソース情報は必須である。
    ・マクロにはnamespaceがない。
    ・nemspaceなしで同じキーワードを使えば、当然衝突する。

    コンセプト：
        ・ロガーマクロは全てのソース内で同一（衝突してもよい
        ・ロガーの出力先は、メインプロジェクトが制御する（サブプロジェクトの出力も奪う）
        ・初めてincludeされた方でAlgLogXXXはdefineされる。
        ・つまり全てのPRJ1ソースで、alglog.hを最初にincludeするルールを導入する。
            ・子プロジェクトはALGLOG_LOGGER_ACCESORが定義されているので


    デフォルトでは、グローバルロガーはalglogが提供するものになる。
    小規模なプロジェクトでは、これで十分。

    複数のプロジェクトを跨ぐ場合は、include順に気を付ける。
    ・loggerは一番最後にincludeする。



*/ 

class Global {
private:
    Global(){};
    ~Global() = default;

public:
    std::unique_ptr<alglog::logger> lgr = nullptr;
    Global(const Global&) = delete;
    Global& operator=(const Global&) = delete;
    Global(Global&&) = delete;
    Global& operator=(Global&&) = delete;

    static Global& get() {
        static Global instance;
        return instance;
    }

    bool initialized(){
        return static_cast<bool>(lgr);
    }
    void init_logger(std::unique_ptr<alglog::logger>&& logger){
        if (!lgr){
            lgr = std::move(logger);
        }else{
            throw std::runtime_error("It is not possible to initialize a logger twice");
        }
    }
    void release_logger(){
        lgr.reset();
    }
};

// コンパイラにこのフラグを設定し、プロジェクトのロガーヘッダでundefを行ようにする。
// 直接alglogをincludeした場合（つまりロガーの設定を行わないビルド経路が存在した場合に）エラーになる。

#ifdef ALGLOG_DIRECT_INCLUDE_GUARD
    #pragma error "Direct inclusion of alglog.h is prohibited. Please include the logger header in your project definition."
#endif

#define ALGLOG_SR alglog::source_location{__ALGLOG_FNAME__, __LINE__, __func__}

#ifndef ALGLOG_ERROR_OFF
    #define AlgLogError(...) alglog::Global::get().lgr->fmt_store(alglog::level::error, __VA_ARGS__)
#else
    #define AlgLogError(...) ((void)0)
#endif

#ifndef ALGLOG_ALART_OFF
    #define AlgLogAlart(...) alglog::Global::get().lgr->fmt_store(alglog::level::alart, __VA_ARGS__)
#else
    #define AlgLogAlart(...) ((void)0)
#endif

#ifndef ALGLOG_INFO_OFF
    #define AlgLogInfo(...) alglog::Global::get().lgr->fmt_store(alglog::level::info, __VA_ARGS__)
#else
    #define AlgLogInfo(...) ((void)0)
#endif

#ifndef ALGLOG_CRITICAL_OFF
    #define AlgLogCritical(...) alglog::Global::get().lgr->fmt_store(ALGLOG_SR, alglog::level::critical, __VA_ARGS__)
#else
    #define AlgLogCritical(...) ((void)0)
#endif

#ifndef ALGLOG_WARN_OFF
    #define AlgLogWarn(...) alglog::Global::get().lgr->fmt_store(ALGLOG_SR, alglog::level::warn, __VA_ARGS__)
#else
    #define AlgLogWarn(...) ((void)0)
#endif

#ifndef ALGLOG_DEBUG_OFF
    #define AlgLogDebug(...) alglog::Global::get().lgr->fmt_store(ALGLOG_SR, alglog::level::debug, __VA_ARGS__)
#else
    #define AlgLogDebug(...) ((void)0)
#endif

#ifndef ALGLOG_TRACE_OFF
    #define AlgLogTrace(...) alglog::Global::get().lgr->fmt_store(ALGLOG_SR, alglog::level::trace, __VA_ARGS__)
#else
    #define AlgLogTrace(...) ((void)0)
#endif

#define AlgLogFlush(...) alglog::Global::get().lgr->flush();
#define AlgLogInitGlobalLogger(S) alglog::Global::get().init_logger(S) // プログラム中で1度のみ呼び出すことができます。
#define AlgLogReleaseGlobalLogger(S) alglog::Global::get().release_logger(S)
#define AlgLogInitDefaultGlobalLogger() alglog::Global::get().init_logger(alglog::builtin::get_default_logger());

#undef ALGLOG_LOGGER_ACCESOR

} // end namespace alglog
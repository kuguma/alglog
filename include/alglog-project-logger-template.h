#pragma once

#include <alglog.h>
#include <mutex>



namespace my_project{

    class Logger {
    private:
        Logger() : logger(std::make_unique<alglog::logger>("my_project"))
        {
            // modify this
            logger->connect_sink( std::make_shared<alglog::builtin::print_sink>() );
            logger->connect_sink( std::make_shared<alglog::builtin::file_sink>("my_project.log") );
            logger->flush_every(std::chrono::milliseconds(500));
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& get() {
            static Logger instance;
            return instance;
        }
    };
}


#define mpjLogRelease(...) AlgLogRelease(mpj::Logger::get().logger, __VA_ARGS__)
#define mpjLogCritical(...) AlgLogCritical(mpj::Logger::get().logger, __VA_ARGS__)
#define mpjLogWarn(...) AlgLogWarn(mpj::Logger::get().logger, __VA_ARGS__)
#define mpjLogDebug(...) AlgLogDebug(mpj::Logger::get().logger, __VA_ARGS__)
#define mpjLogTrace(...) AlgLogTrace(mpj::Logger::get().logger, __VA_ARGS__)


/*
TODO 

グローバルなロガーの管理、他プロジェクトとの兼ね合い

他プロジェクトがある場合、
    マスター　デバッグ　リリース
    スレーブ　デバッグ

    みたいな感じになると思う。
    mapを使えば簡単だが、毎回呼び出すコストが大きいのであまりやりたくない。。

シングルトンのget()のコストはそこまで高くない。
テンプレートを使う？他プロジェクトがソースからビルドか、ライブラリリンクかによっても変わってくる。
    やっぱりグローバルなものをlogger側で持つのはやめて独立させた方が良いか？

デバッグでは結局マクロを経由して呼ばないといけない。だったら最初からインターフェースはすべてマクロを標準にした方が良いのではないか？
alglog＋カスタム用ヘッダの組み合わせが安定か？

リリース版はどうする。同期出力をサポートすべきか？

時間計測機能をつけてもいいかも。

あるエリアにTraceを埋め込んだけどもう見る必要ないみたいなのは割とでてきそう。タグ機能をつけるべきか？



*/
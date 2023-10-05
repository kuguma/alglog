# alglog

## Install 

### with cmake

```cmake
FetchContent_Declare(
    ALGLOG
    GIT_REPOSITORY https://github.com/kuguma/alglog.git
    GIT_TAG     0.0.1
)
FetchContent_MakeAvailable(ALGLOG)
```

## Features

- 軽量、高速でシンプルなロガー
- 最低限の機能（たった200行！）
- クロスプラットフォーム（WIP）
- スレッドセーフ, 非同期出力
- `fmtlib`を利用した便利なフォーマット
- C++11から利用可能
- MIT License（ただしバイナリ配布は権利表記義務なし）
- 日本語のドキュメント😊

## Highlights

- spdlogに影響を受けた設計
- グローバルなロガーなし：コンポーネントごとに好き勝手に利用しても衝突の心配なし
- 非同期出力：実行速度を重視する商用ライブラリで採用可能

## 開発経緯

- このライブラリは、ビジネス要件により権利表記を避けたいが、どうにかまともなロガーが欲しいという声により開発されました。
- 権利表記を行うことができる場合は、より洗練されたOSSロガー（例えば`spdlog`）の採用を強く推奨します。

## 概要

ロガーは`logger`, `sink`, `formatter`から構成される。

1. ロガーにログを書き込むと、まずログは`logger`内に蓄積される。

    蓄積されたログは、`logger.flush()`を呼び出して手動でflushするか、`logger.flush_every(ms)`を呼び出して定期的に出力することができる。（スレッドが裏で起動される。）

    内部のログは`logger`の解放時に自動的に`flush()`される。

2. `flush()`されたログは、`logger`が接続している`sink`を通り、出力される。`sink`が出力条件を持つ場合は（例えばログレベル）、その条件を満たす場合のみ出力が行われる。

    `sink`には、組み込みで

    - `alglog::builtin::file_sink`
    - `alglog::builtin::print_sink`
    
    などが存在する。
    
    もしくは、自分で`alglog::sink`クラスを継承して定義し、`logger.connect_sink()`で任意のロガーに出力することもできる。

3. `sink`から出力されるとき、`sink`は自分が持つ`formatter`を通して、ログを整形する。`sink.formatter`はpublicなラムダ変数なので、自分で作成して`sink`に設定することもできる。（もしくは自作sinkの場合は、formatterを無視してもよい。）


### サンプル

```C++
    auto lgr = std::make_shared<logger>("main_logger");
    auto snk = std::make_shared<builtin::print_sink>();
    lgr->connect_sink(snk);
    lgr->flush_every(std::chrono::milliseconds(500));
```


### ケーススタディ

とにかくすぐロガーが使いたい

```C++
    #include <alglog.h>

    auto logger = alglog::get_default_logger("main");
    logger->debug("hello world");
    logger->debug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    logger->trace("vector =  {}", vec);
```

シングルトンなロガーがほしい

```C++

// このライブラリでは、複数プロジェクトで利用した場合の競合を避けるため、
// シングルトン化したロガーは提供していません。
// シングルトン化は自分で行ってください。

// 以下はサンプルコードです。

#include "alglog.h"

namespace my_project{

    class Logger {
    private:
        Logger(){
            logger = alglog::get_default_logger("main");
            auto snk = std::make_shared<alglog::builtin::file_sink>("sample.log");
            logger->connect_sink(snk);
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& inst() {
            static Logger instance;
            return instance;
        }
    };
}

#define LogDebug(...) my_project::Logger::inst().logger->debug(__VA_ARGS__)

```



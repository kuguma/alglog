# alglog

alglogは、中規模のC++商用プロジェクトをターゲットとした、権利表記義務のないロガーライブラリです。

## Install 

### with cmake

```cmake
FetchContent_Declare(
    ALGLOG
    GIT_REPOSITORY https://github.com/kuguma/alglog.git
    GIT_TAG     0.1.0
)
FetchContent_MakeAvailable(ALGLOG)
```

## Introduction

商用ライブラリでは、ロガーに求めるいくつかの特殊な要件があります。

### 顧客用ログ

顧客用ログでは、出力が有効な場合（例えばverboseモード）に

- API呼び出し履歴
- 警告やエラー

などが出力されてほしいでしょう。これらのログは

- 高速な処理
- 出力が無効な場合、無駄な処理が行われない
- 内部処理を推測される恐れのある情報が含まれない

といった要件を持ちます。

### 開発用ログ

一方で開発用ログの要件は、

- ソース情報やスレッドIDなど、可能な限り多くの情報が付加される
- 強制終了時に消失しない
- 複数プロジェクトで併用した場合（例えばメインプロジェクトと共通プロジェクト）に競合しない

といったものになるでしょう。

これらの要件にあなたのプロジェクトが合致する場合、alglogは良い選択肢になります。

## Features

- 軽量、高速でシンプルなロガー
- 非同期かつスレッドセーフ
- クロスプラットフォーム
- `fmtlib`を利用した便利なフォーマット
- C++14から利用可能
- 日本語のドキュメント😊

## Highlights

- マクロAPIのみ：実用に振り切った設計
- 非同期出力：実行速度を重視する商用ライブラリで採用可能
- 商用利用可能：MIT Licenseだが、バイナリ配布は権利表記義務なし
- グローバルロガーなし：複数プロジェクトで採用しても競合しません

## 開発経緯

- このライブラリは、ビジネス要件により権利表記を避けたいが、どうにかまともなロガーが欲しいという声により開発されました。
- 権利表記を行うことができる場合は、より洗練されたOSSロガー（例えば`spdlog`や`quill`など ）の採用を強く推奨します。
    - 例えば、ロガーを利用するのがデバッグビルドのみであれば、問題なくOSSを採用できます。

## 内部設計

`alglog`は、次のように設計されています。

ロガーは`logger`、`sink`、`valve`、および`formatter`から構成されています。

1. ロガーにログを書き込むと、まずログは`logger`内に蓄積されます。

    デフォルトでは非同期モードが選択されており、蓄積されたログは手動で`logger.flush()`を呼び出すことでフラッシュする必要があります。定期的に出力したい場合、`alglog::flusher`を利用できます（この場合、スレッドが裏で起動されます）。

    `logger.sync_mode = true`とすることで、同期モードで実行できます。内部のログは、`logger`が解放されるか、内部のコンテナがいっぱいになった場合に自動的に`flush()`されます。

2. `flush()`されたログは、`logger`が接続している`sink`を通過し、出力されます。`sink`は`valve`と呼ばれる出力条件判定ラムダ関数を持ち、その条件を満たす場合のみ`log`は`sink`を通過します。

    組み込みで以下の`sink`が提供されています。

    - `alglog::builtin::file_sink`
    - `alglog::builtin::print_sink`
    
    また、自分で`alglog::sink`クラスを継承し、`logger.connect_sink()`を使用して任意のロガーに出力することもできます。

3. `sink`から出力されるとき、`sink`は自身が持つ`formatter`を介してログを整形します。`sink.formatter`はpublicなラムダ変数であり、自分で作成して`sink`に上書き設定することもできます（自作sinkの場合、formatterを無視してもかまいません）。

## API

alglogのロガーはそのまま使うこともできるが、ソースローケーションの埋め込みを行うためにはマクロを経由する必要がある。

`alglog-project-logger-template.h`をフォークして、プロジェクト用のロガーを作成することを推奨する。

```C++
    MyLogInfo("hello world");
    MyLogDebug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    MyLogTrace("vector =  {}", vec);

```


## ログレベル

alglogは、リリースビルド用に3段階、デバッグ用に4段階のログレベルを持ちます。

リリースビルド用の3段階はユーザー公開用のログになり、ソース情報等の細かい情報は含まれません。

デバッグ用の4段階は、リリースビルドではバイナリから消滅します。

```C++
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
```

## How to use / Q & A

とにかくすぐロガーが使いたい

```C++
    #include <alglog.h>

    auto logger = alglog::builtin::get_default_logger();

    logger->info("hello world");
    logger->debug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    logger->trace("vector =  {}", vec);
```

ロガーを手動で設定したい

```C++
    auto lgr = std::make_shared<alglog::logger>("my_logger");
    auto psink = std::make_shared<builtin::print_sink>();
    psink.valve = alglog::builtin::debug_level_output;
    lgr->connect_sink(psink);
    lgr->connect_sink( std::make_shared<builtin::file_sink>("my_logger.log") );
    auto flusher = std::make_unique<alglog::flusher>(lgr);
    flusher.start(500);

    // loggerの出力先は、alglog::sinkを継承した自作クラスを用いてカスタムできます。
    // あるsinkが出力を行うかどうかは、sink.valveにラムダを設定することで制御できます。
```

特定の機能をデバッグしている。その機能のログだけを表示したい

sinkにカスタムvalveを設定することで解決できます。


```C++
    const auto keyword_valve = [](const log_t& l){
        if (l.find("keyword")){
            return true;
        }
        return false;
    };
```


想定された運用方法

alglogでは、`alglog.h`をラップしたヘッダ（プロジェクトロガーヘッダ）を作成し、マクロ経由で呼び出してもらうことを想定しています。

`alglog-project-logger-template.h`を参照してください。

```C++

// ---------------- my_logger.h ----------------

#pragma once

#include <alglog.h>

namespace my_project{

    class Logger {
    private:
        Logger() : logger(std::make_shared<alglog::logger>()), flusher(std::make_unique<alglog::flusher>(logger))
        {
            // modify this
            logger->connect_sink( std::make_shared<alglog::builtin::print_sink>() );
            logger->connect_sink( std::make_shared<alglog::builtin::file_sink>("my_project.log") );
            flusher->start();
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        std::unique_ptr<alglog::flusher> flusher;
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

#define MyLogError(...) my_project::Logger::get().logger->error(__VA_ARGS__)
#define MyLogAlart(...) my_project::Logger::get().logger->alart(__VA_ARGS__)
#define MyLogInfo(...) my_project::Logger::get().logger->info(__VA_ARGS__)
#define MyLogCritical(...) my_project::Logger::get().logger->critical(ALGLOG_SR, __VA_ARGS__)
#define MyLogWarn(...) my_project::Logger::get().logger->warn(ALGLOG_SR, __VA_ARGS__)
#define MyLogDebug(...) my_project::Logger::get().logger->debug(ALGLOG_SR, __VA_ARGS__)
#define MyLogTrace(...) my_project::Logger::get().logger->trace(ALGLOG_SR, __VA_ARGS__)


//　---------------- something.cpp ----------------

    #include <mylogger.h>
    // ... other includes ... 

```

## コンパイルスイッチ

include前にこれらのキーワードを定義するか、コンパイラに引数として与えることで、ログ出力がバイナリに含まれるかを制御できます。
デフォルト挙動では、`alglog`はすべてのレベルのログを出力します。

```C++
#define ALGLOG_ALL_OFF // すべてのログ出力を無効化する。
#define ALGLOG_<XXX>_OFF // <XXX>出力を無効化する。
#define NDEBUG // (cmakeのReleaseビルド標準）error, alart, info出力以外を無効化する。

#define ALGLOG_GETPID_OFF // プロセスIDを取得しない。
#define ALGLOG_GETTID_OFF // スレッドIDを取得しない。
```

## 設計に関する考察

### なぜマクロAPIを推奨するのか

C++でロガーライブラリを構築する場合、以下のような制約と向き合う必要があります。

- C++20より前では、マクロを使わないとソース情報を取得できない。
- デバッグにおいては、ソース情報は必須である。
- マクロにはnamespaceがない。

従って、C++20未満の環境では、残念ながらマクロを使ったログ呼び出しを基本として設計を行う必要があります。

### 複数プロジェクトでの運用

同じロギングライブラリが複数プロジェクトで使用される場合、最も問題なのは衝突です。

あらゆるプロジェクトが、自分の都合の良いようにライブラリを使おうとすることを想定する必要があります。2つのプロジェクトが、お互いに自分だけの出力を有効化するように設定を行っていたらどうなるでしょうか？

このような問題を避けるため、alglogではグローバルロガーを実装していません。

グローバル化はユーザーがそれぞれの責任で作成します。（namespaceの利用を強く推奨します）
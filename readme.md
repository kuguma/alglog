# alglog

alglogは、中規模のC++商用プロジェクトをターゲットとした、シンプルなロガーライブラリです。

## Install 

### with cmake

```cmake
FetchContent_Declare(
    alglog
    GIT_REPOSITORY https://github.com/kuguma/alglog.git
    GIT_TAG     <version_tag>
)
FetchContent_MakeAvailable(alglog)
```

## Features

- 3種類の顧客用ログレベルと、4種類のデバッグ用ログレベル
- 使いやすい同期ロギングと、高速な非同期ロギングの双方をサポート
- クラスプラットフォーム
- カスタムMITライセンス：バイナリ配布の場合、権利表示の必要なし。
- C++17から利用可能
- `fmtlib`を利用した便利なフォーマット
- グローバルロガーなし：複数プロジェクトで採用しても競合しません
- 日本語のドキュメント😊

## Overview

#### alglogは、商用ライブラリを念頭に設計されています。

- 顧客に公開したいログ（例えばverboseモードでの出力）と、デバッグ用のログを1つのライブラリでサポートできます。
- 顧客ログには、ソース情報は含まれません。
- デバッグログはマクロを経由して呼び出すことで、ソース情報を含めることができます。ログ呼び出しはリリースビルドでは消滅します。
- もちろん、`ALGLOG_ALL_OFF`ですべてを無効化することもできます。

#### 速度を重視するプロジェクトのために、非同期ロギングをサポートしています。

- 同期モードでは、ログの記録と同時に出力を行いますが、標準出力やファイルへの書き込みは大きなオーバーヘッドになります。
- 非同期モードでは、ログの記録とは独立して、後ほど任意のタイミングで出力を行うことができます。これにより、ロギングのオーバーヘッドを最小限に抑えてプログラムを実行できます。

#### カスタムMITライセンスを採用しています。

- boostやfmtlibと同様に、バイナリ配布の場合は権利表記が必要ありません。
- プロジェクトのローンチ後でも、導入に際して顧客説明が必要なく、上司への説明が簡単です。これは開発者にとって嬉しい点です。
- 柔軟にOSSを採用できるプロジェクトでは、メンテナンスされており、より洗練されたロガー（例えば`spdlog`や`quill`など ）の採用を推奨します。

#### グローバルロガーを採用していません。

- 多くのロガーでは、includeと同時にグローバルロガーが定義されます。これは一見便利なように見えますが、複数のプロジェクトから利用するとよく衝突します。
- alglogでは、代わりにプロジェクトロガーのテンプレートを提供することで、便利かつ副作用の少ないライブラリを目指しています。

## 内部設計

`alglog`は、次のように設計されています。設計はspdlogに影響を受けています。

ロガーは`logger`、`sink`、`valve`、および`formatter`から構成されています。

1. ロガーにログを書き込むと、まずログは`logger`内に蓄積されます。

    デフォルトでは同期モードで、蓄積と同時に出力が行われます。

    非同期モードでalglogを利用するには、loggerのコンストラクタに`true`を与えます。この場合、蓄積されたログは手動で`logger.flush()`を呼び出してフラッシュする必要があります。定期的に出力したい場合、`alglog::flusher`を利用できます（定期的にflushを行うスレッドが起動します）。

2. `flush()`されたログは、`logger`が接続している`sink`を通過し、出力されます。`sink`は`valve`と呼ばれる出力条件判定ラムダ関数を持ち、その条件を満たす場合のみ`log`は`sink`を通過します。

    組み込みで以下の`sink`が提供されています。

    - `alglog::builtin::file_sink`
    - `alglog::builtin::print_sink`
    
    また、自分で`alglog::sink`クラスを継承し、`logger.connect_sink()`を使用して任意のロガーに出力することもできます。

3. `sink`から出力されるとき、`sink`は自身が持つ`formatter`を介してログを整形します。`sink.formatter`はpublicなラムダ変数であり、自分で作成して`sink`に上書き設定することもできます（自作sinkの場合、formatterを無視してもかまいません）。

## API

alglogのロガーはそのまま使うこともできますが、ソースローケーションの埋め込みを行うためにはマクロを経由する必要があります。

`alglog-project-logger-template.h`をフォークして、プロジェクト用のロガーを作成することを推奨します。

```C++
    MyLogInfo("hello world");
    MyLogDebug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    MyLogTrace("vector =  {}", vec);
```


## ログレベル

alglogは、顧客公開用に3段階、デバッグ用に4段階のログレベルを持ちます。

顧客公開用の3段階はリリースビルドでも消滅せず、ソース情報等の細かい情報は含まれません。

デバッグ用の4段階は、リリースビルドではバイナリから消滅します。

```C++
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
```

## コンパイラスイッチ

include前にこれらのキーワードを定義するか、コンパイラに引数として与えることで、ログ出力がバイナリに含まれるかを制御できます。
デフォルト挙動では、`alglog`はすべてのレベルのログを出力します。

```C++
#define ALGLOG_ALL_OFF // すべてのログ出力を無効化する。
#define ALGLOG_<LOG_LEVEL>_OFF // <LOG_LEVEL>出力を無効化する。
#define NDEBUG // (cmakeのReleaseビルド標準）error, alert, info出力以外を無効化する。

#define ALGLOG_GETPID_OFF // プロセスIDを取得しない。
#define ALGLOG_GETTID_OFF // スレッドIDを取得しない。
```

## How to use / Q & A

### とにかくすぐロガーが使いたい（非推奨）

```C++
    #include <alglog.h>

    auto logger = alglog::builtin::get_default_logger();

    logger->info("hello world");
    logger->debug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    logger->trace("vector =  {}", vec);
```


### プロジェクトロガーの作成（推奨）

alglogでは、`alglog.h`をラップしたヘッダ（プロジェクトロガーヘッダ）を作成し、マクロ経由で呼び出してもらうことを想定しています。

詳細は`alglog-project-logger-template.h`を参照してください。

```C++

// ---------------- mylogger.h ----------------

#pragma once

#define ALGLOG_DIRECT_INCLUDE_GUARD
#include <alglog.h>

namespace my_project{

    class Logger {
    private:
        Logger() : logger(std::make_shared<alglog::logger>(true)), flusher(std::make_unique<alglog::flusher>(logger))
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
#define MyLogAlert(...) my_project::Logger::get().logger->alert(__VA_ARGS__)
#define MyLogInfo(...) my_project::Logger::get().logger->info(__VA_ARGS__)
#define MyLogCritical(...) my_project::Logger::get().logger->critical(ALGLOG_SR, __VA_ARGS__)
#define MyLogWarn(...) my_project::Logger::get().logger->warn(ALGLOG_SR, __VA_ARGS__)
#define MyLogDebug(...) my_project::Logger::get().logger->debug(ALGLOG_SR, __VA_ARGS__)
#define MyLogTrace(...) my_project::Logger::get().logger->trace(ALGLOG_SR, __VA_ARGS__)


//　---------------- something.cpp ----------------

#define ALGLOG_DIRECT_INCLUDE_GUARD
#include <mylogger.h>
// ... other includes ... 

```

### ロガーを手動で設定したい

```C++
    auto lgr = std::make_shared<alglog::logger>(true);
    auto psink = std::make_shared<builtin::print_sink>();
    psink.valve = alglog::builtin::debug_level_output;
    lgr->connect_sink(psink);
    lgr->connect_sink( std::make_shared<builtin::file_sink>("my_logger.log") );
    auto flusher = std::make_unique<alglog::flusher>(lgr);
    flusher.start(500);

    // loggerの出力先は、alglog::sinkを継承した自作クラスを用いてカスタムできます。
    // あるsinkが出力を行うかどうかは、sink.valveにラムダを設定することで制御できます。
```

### 特定の機能のログだけを表示したい

sinkにカスタムvalveを設定することで解決できます。


```C++
    const auto keyword_valve = [](const log_t& l){
        if (l.find("keyword")){
            return true;
        }
        return false;
    };
```

## 設計に関する考察

### なぜマクロAPIを推奨するのか

C++でロガーライブラリを構築する場合、以下のような制約と向き合う必要があります。

- C++20より前では、マクロを使わないとソース情報を取得できない。
- デバッグにおいては、ソース情報は必須である。
- マクロにはnamespaceがない。

従って、C++20未満の環境では、残念ながらマクロを使ったログ呼び出しを基本として設計を行う必要があります。

### 複数プロジェクトでの運用

同じロギングライブラリが複数プロジェクトで使用される場合、よく問題になるのは設定の衝突です。

あらゆるプロジェクトが、自分の都合の良いようにライブラリを使おうとすることを想定する必要があります。2つのプロジェクトが、お互いに自分だけの出力を有効化するように設定を行っていたらどうなるでしょうか？

このような問題を避けるため、alglogではグローバルロガーを実装していません。

グローバル化はユーザーがそれぞれの責任で行います。（namespaceの利用を強く推奨します）

## Test

```shell
$ cd alglog
$ python test/main.py
```

テストの拡充を歓迎します！
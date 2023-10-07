# alglog

alglogは、中規模のC++商用プロジェクトをターゲットとした、権利表記義務のないロガーライブラリです。

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
- 最低限の機能（コア機能はたった200行！）
- クロスプラットフォーム
- `fmtlib`を利用した便利なフォーマット
- C++11から利用可能
- MIT License（ただしバイナリ配布は権利表記義務なし）
- 日本語のドキュメント😊

## Highlights

- マクロAPIのみ：実用に振り切った設計
- 非同期出力：実行速度を重視する商用ライブラリで採用可能
- 複数プロジェクトで採用しても競合しません

## 開発経緯

- このライブラリは、ビジネス要件により権利表記を避けたいが、どうにかまともなロガーが欲しいという声により開発されました。
- 権利表記を行うことができる場合は、より洗練されたOSSロガー（例えば`spdlog`や`quill`など ）の採用を強く推奨します。
    - 例えば、ロガーを利用するのがデバッグビルドのみであれば、問題なくOSSを採用できます。

## 内部設計

alglogは、以下のような設計になっている。

- バイナリ全体に存在するロガーは、グローバルなロガーただ一つ（を推奨）。
- ロギングの呼び出しはマクロのみ（を推奨）。

この設計を採用した理由は後述する。

<br>

ロガーは`logger`, `sink`, `formatter`から構成される。

1. ロガーにログを書き込むと、まずログは`logger`内に蓄積される。

    蓄積されたログは、`logger.flush()`を呼び出して手動でflushするか、`logger.flush_every(ms)`を呼び出して定期的に出力することができる。（スレッドが裏で起動される。）

    内部のログは`logger`の解放時に自動的に`flush()`される。

2. `flush()`されたログは、`logger`が接続している`sink`を通り、出力される。`sink`は`valve`という名の出力条件判定ラムダ関数を持ち、その条件を満たす場合のみ`log`は`sink`を通る。

    `sink`には、組み込みで

    - `alglog::builtin::file_sink`
    - `alglog::builtin::print_sink`
    
    などが存在する。
    
    もしくは、自分で`alglog::sink`クラスを継承して定義し、`logger.connect_sink()`で任意のロガーに出力することもできる。

3. `sink`から出力されるとき、`sink`は自分が持つ`formatter`を通して、ログを整形する。`sink.formatter`はpublicなラムダ変数なので、自分で作成して`sink`に設定することもできる。（もしくは自作sinkの場合は、formatterを無視してもよい。）

## API

alglogでは、グローバルなロガーにsink等の設定を行った後は、マクロを呼び出してログを記録するAPIになっている。

`AlgLogXXX()`のような形で呼び出しを行う。`()`内は`fmt`の機能が使える。

```C++
    AlgLogInfo("hello world");
    AlgLogDebug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    AlgLogTrace("vector =  {}", vec);

```


## ログレベル

alglogは、リリースビルド用に3段階、デバッグ用に4段階のログレベルを持つ。

リリースビルド用の3段階はユーザー公開用のログになり、ソース情報等の細かい情報は含まれない。

デバッグ用の4段階は、リリースビルドではバイナリから消滅する。

```C++
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
```

## How to use

とにかくすぐロガーが使いたい

```C++
    #include <alglog.h>

    AlgLogInitDefaultGlobalLogger();

    AlgLogDebug("hello world");
    AlgLogDebug("The answer is {}.", 42);

    std::vector<int> vec = {1,2,3,4,5};
    AlgLogTrace("vector =  {}", vec);
```

ロガーを手動で設定したい

```C++
    auto lgr = std::make_unique<alglog::logger>("my_logger");
    auto psink = std::make_shared<builtin::print_sink>();
    psink.valve = alglog::builtin::debug_level_output;
    lgr->connect_sink(psink);
    lgr->connect_sink( std::make_shared<builtin::file_sink>("my_logger.log") );
    lgr->flush_every(std::chrono::milliseconds(500));
    AlgLogSetGlobalLogger(std::move(lgr));

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


スレッドとプロセスIDの表示をオフにしたい

- これらの情報の取得はシステムコールを利用するため、オーバーヘッドが大きくなります。
- 問題を調査する時以外は、オフにしておくことができます。

```C++
#define ALGLOG_GETPID_OFF
#define ALGLOG_GETTID_OFF

```

想定された運用方法

alglogでは、`alglog.h`をラップしたヘッダ（プロジェクトロガーヘッダ）を作成してもらうことを想定しています。

そうして作成したヘッダは、必ずソースの一番最初に`include`するようにします。

```C++

// my_logger.h

    #pragma once
    #include <alglog.h>

    // これをAPI呼び出しやプログラムのエントリポイント後にすぐ呼び出す
    inline void setup_logger(){
        auto logger = alglog::builtin::get_default_logger("main");
        // ... setup code ...
        AlgLogSetGlobalLogger(std::move(logger));
    }

// something.cpp

    #include <mylogger.h>
    // ... other includes ... 

```

複数プロジェクトでalglogを採用すると、場合によってはloggerの設定がコンフリクトします。

それを避けるには、次のようにします。

- 想定された運用方法　を採用する。
- コンパイラフラグで`ALGLOG_DIRECT_INCLUDE_GUARD`を定義した上で、プロジェクトロガーヘッダの最後で `#undef ALGLOG_DIRECT_INCLUDE_GUARD`する。
    - これにより、プロジェクトロガーを経由しない直接includeを排除できます。



## コンパイルスイッチ

include前にこれらのキーワードを定義するか、コンパイラに引数として与えることで、ログ出力がバイナリに含まれるかを制御できる。
デフォルト挙動では、`alglog`はすべてのレベルのログを出力する。

```C++
#define ALGLOG_ALL_OFF // すべてのログ出力を無効化する。
#define ALGLOG_<XXX>_OFF // <XXX>出力を無効化する。
#define NDEBUG // (cmakeのReleaseビルド標準）error, alart, info出力以外を無効化する。

#define ALGLOG_GETPID_OFF // プロセスIDを取得しない。
#define ALGLOG_GETTID_OFF // スレッドIDを取得しない。
```

## 設計に関する考察

C++でロガーライブラリを構築する場合、以下のような制約と向き合う必要がある。

- C++20より前では、マクロを使わないとソース情報を取得できない。
- デバッグにおいては、ソース情報は必須である。
- マクロにはnamespaceがない。

ロギング用の関数はできるだけコンパクトであることが求められる。短いキーワードで記述できるようにすれば、当然複数プロジェクト間でマクロの衝突や上書きが発生する。

したがって、全体で1つのロガーのみを認め、ロガーの設定は最上位プロジェクトが制御するようにするのが最も合理的になる。

複数のプロジェクトに跨って複数のロガーを存在させるとそれぞれの入出力の調停を行う必要も出てくるので、単一のロガーを認める設計は、処理速度上でも有利になる。

したがってalglogでは単一のグローバルロガーを用い、複数プロジェクトでは運用を統一することで対処する形を採用した。

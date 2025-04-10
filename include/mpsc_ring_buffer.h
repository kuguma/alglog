#pragma once
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

// wroted by o1

#ifdef _MSC_VER
#pragma warning(disable:4324)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4389)
#endif

/**
 * @tparam T   要素型
 * @tparam N   バッファサイズ（2 のべき乗であること）
 *
 * Vyukov‑style のシーケンス番号アルゴリズムを
 * 「複数 Producer / 単一 Consumer」に絞って簡略化。
 *
 *  - push():  成功なら true、満杯なら false
 *  - pop():   成功なら true、空なら false
 *
 * 使い方例:
 *   mpsc_ring_buffer<int, 1024> q;
 *   q.push(42);
 *   int v;
 *   if (q.pop(v)) { ... }
 */
template<typename T, std::size_t N>
class mpsc_ring_buffer {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");

    struct Slot {
        std::atomic<std::size_t> seq;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };

    alignas(64) Slot  buf_[N];
    alignas(64) std::atomic<std::size_t> head_{0};   // producer 用
    alignas(64) std::size_t             tail_{0};    // consumer 専用

    static constexpr std::size_t mask_ = N - 1;

public:
    mpsc_ring_buffer() {
        for (std::size_t i = 0; i < N; ++i)
            buf_[i].seq.store(i, std::memory_order_relaxed);
    }

    ~mpsc_ring_buffer() {
        // 残っている要素を破棄
        T tmp;
        while (pop(tmp)) {}
    }

    /** Producer: エンキュー */
    bool push(const T& v) noexcept {
        return emplace(v);
    }
    bool push(T&& v) noexcept {
        return emplace(std::move(v));
    }

    /** Consumer: デキュー */
    bool pop(T& out) noexcept {
        Slot* s = &buf_[tail_ & mask_];
        std::size_t seq = s->seq.load(std::memory_order_acquire);

        // seq == tail_ + 1 ならデータが入っている
        if (seq != tail_ + 1)
            return false;               // 空

        // 取り出し
        out = std::move(*reinterpret_cast<T*>(&s->storage));
        reinterpret_cast<T*>(&s->storage)->~T();

        // 次の周回用にシーケンスを更新
        s->seq.store(tail_ + N, std::memory_order_release);
        ++tail_;
        return true;
    }

private:
    template<typename U>
    bool emplace(U&& v) noexcept {
        std::size_t pos = head_.load(std::memory_order_relaxed);

        for (;;) {
            Slot* s = &buf_[pos & mask_];
            std::size_t seq = s->seq.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) -
                                 static_cast<std::intptr_t>(pos);

            if (diff == 0) {
                // スロット確保を試みる
                if (head_.compare_exchange_weak(
                        pos, pos + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed))
                {
                    // 自分のスロットを確保できた
                    new (&s->storage) T(std::forward<U>(v));
                    s->seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
                // CAS に負けたので pos は更新されている。ループ続行
            } else if (diff < 0) {
                return false;            // バッファ満杯
            } else {
                // 他スレッドがまだ書き込み中 → pos を再読み込み
                pos = head_.load(std::memory_order_relaxed);
            }
        }
    }
};

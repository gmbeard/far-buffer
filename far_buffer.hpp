#ifndef GMB_FAR_BUFFER_HPP_INCLUDED
#define GMB_FAR_BUFFER_HPP_INCLUDED

#include <memory>
#include <limits>
#include <iterator>
#include <algorithm>
#include <cassert>

namespace gmb {
namespace utils {

template<typename T>
struct Span {

    template<typename U>
        friend struct Span;

    using value_type = T;
    using iterator = T*;
    using const_iterator = typename
        std::conditional<std::is_const<T>::value,
                         T,
                         T const>::type*;

    template<
        typename U,
        size_t N,
        typename std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr>
    explicit Span(U (&vals)[N]) noexcept :
        ptr_ { &vals[0] }
    ,   len_ { N }
    { }

    template<
        typename U,
        typename std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr>
    Span(Span<U> const& other) noexcept :
        ptr_ { other.ptr_ }
    ,   len_ { other.len_ }
    { }

    template<
        typename U,
        typename std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr>
    Span(U* first, U* last) noexcept :
        ptr_ { first }
    ,   len_ { static_cast<size_t>(last - first) }
    { 
        assert(first <= last &&
            "Invalid range given to span c'tor!");
    }

    template<
        typename U,
        typename std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr>
    Span(U* ptr, size_t len) noexcept :
        ptr_ { ptr }
    ,   len_ { len}
    { }

    friend auto swap(Span& lhs, Span& rhs) noexcept {
        using std::swap;
        swap(lhs.ptr_, rhs.ptr_);
        swap(lhs.len_, rhs.len_);
    }

    template<
        typename U,
        typename std::enable_if_t<std::is_convertible<U*, T*>::value>* = nullptr>
    auto operator=(Span<U> const& other) noexcept -> Span& {
        Span tmp { other };
        swap(*this, tmp);
        return *this;
    }

    auto size() const noexcept -> size_t {
        return len_;
    }

    auto begin() noexcept -> iterator { 
        return ptr_;
    }

    auto end() noexcept -> iterator {
        return ptr_ + len_;
    }

    auto begin() const noexcept -> const_iterator {
        return const_cast<Span*>(this)->begin();
    }

    auto end() const noexcept -> const_iterator {
        return const_cast<Span*>(this)->end();
    }

private:
    T* ptr_;
    size_t len_;
};

struct FarBufferBlock {
    size_t const no_of_blocks;
    size_t const length;
    size_t consumed;
    uint8_t* data;
};

template<typename Alloc>
using ToFarBufferBlockAlloc = typename 
    std::allocator_traits<Alloc>::template rebind_alloc<FarBufferBlock>;

inline auto consume_from_block(FarBufferBlock* block, size_t bytes) {
    assert(block && "block is null");
    assert(block->data && "block's data is null");
    assert(bytes <= block->consumed &&
        "Attempting to consume more bytes than in the buffer!");

    std::copy(block->data + bytes,
              block->data + block->consumed,
              block->data);
    block->consumed -= bytes;
}

template<typename Alloc>
auto allocate_far_buffer_block(size_t size, Alloc a) -> FarBufferBlock* {
    using std::allocator_traits;

    ToFarBufferBlockAlloc<Alloc> alloc { a };

    auto const n_items = 
        (size + (sizeof(FarBufferBlock) - 1)) / sizeof(FarBufferBlock);

    assert((n_items * sizeof(FarBufferBlock)) >= size &&
        "Didn't allocate enough buffer!");

    auto* far_buffer = 
        allocator_traits<ToFarBufferBlockAlloc<Alloc>>::allocate(
            alloc,
            n_items + 1
        );

    auto* buffer_start = reinterpret_cast<uint8_t*>(far_buffer + 1);

    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::construct(
        alloc,
        far_buffer,
        FarBufferBlock { 
            n_items + 1,
            size, 
            0, 
            buffer_start 
        }
    );

    return far_buffer;
}

template<typename Alloc>
auto deallocate_far_buffer_block(FarBufferBlock* fb, Alloc a) {
    using std::allocator_traits;

    auto const n_items = fb->no_of_blocks;

    ToFarBufferBlockAlloc<Alloc> alloc { a };
    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::destroy(alloc, fb);
    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::deallocate(alloc, fb, n_items);
}

template<typename Alloc>
struct BasicFarBuffer;

template<typename Alloc, bool IsConst>
struct BasicFarBufferIterator {
private:
    friend BasicFarBufferIterator<Alloc, true>;

public:
    using Parent = typename
        std::conditional<IsConst, 
                         BasicFarBuffer<Alloc> const,
                         BasicFarBuffer<Alloc>>::type;
    using value_type = typename
        std::conditional<IsConst,
                         typename Parent::value_type const,
                         typename Parent::value_type>::type;

    using pointer = value_type*;
    using reference = value_type&;
    using differenc_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    BasicFarBufferIterator(Parent* parent, size_t pos) noexcept :
        parent_ { parent }
    ,   pos_ { pos }
    { }

    BasicFarBufferIterator(BasicFarBufferIterator const&) noexcept = default;

    template<
        bool B,
        typename std::enable_if_t<!B && IsConst>* = nullptr>
    BasicFarBufferIterator(
        BasicFarBufferIterator<Alloc, B> const& other) noexcept :
        parent_ { other.parent_ }
    ,   pos_ { other.pos_ }
    { }

    friend auto swap(BasicFarBufferIterator& lhs,
                     BasicFarBufferIterator& rhs) noexcept
    {
        using std::swap;
        swap(lhs.parent_, rhs.parent_);
        swap(lhs.pos_, rhs.pos_);
    }

    auto operator=(BasicFarBufferIterator const&) noexcept -> 
        BasicFarBufferIterator& = default;

    template<
        bool B,
        typename std::enable_if_t<!B && IsConst>* = nullptr>
    auto operator=(BasicFarBufferIterator<Alloc, B> const& other) noexcept -> 
        BasicFarBufferIterator&
    {
        BasicFarBufferIterator tmp { other };
        swap(tmp, *this);
        return *this;
    }

    auto operator->() const noexcept -> pointer {
        assert((pos_ < parent_->size()) &&
            "Dereferencing iterator that is past the end!");
        return parent_->data() + pos_;
    }

    auto operator*() const noexcept -> reference {
        return *operator->();
    }

    auto operator++() noexcept -> BasicFarBufferIterator& {
        constexpr auto kMax = std::numeric_limits<decltype(pos_)>::max();
        assert((pos_ < kMax) &&
            "Incrementing iterator would overflow!");
        ++pos_;
        return *this;
    }

    auto operator++(int) noexcept -> BasicFarBufferIterator {
        BasicFarBufferIterator tmp { *this };
        operator++();
        return tmp;
    }

    auto operator+=(size_t n) noexcept -> BasicFarBufferIterator& {
        for(; n; n--) {
            operator++();
        }
        return *this;
    }

    auto operator+(size_t n) noexcept -> BasicFarBufferIterator {
        BasicFarBufferIterator tmp { *this };
        return tmp += n;
    }

    auto operator--() noexcept -> BasicFarBufferIterator& {
        assert(pos_ > 0 &&
            "Decrementing iterator would underflow!");
        --pos_;
        return *this;
    }

    auto operator--(int) noexcept -> BasicFarBufferIterator {
        BasicFarBufferIterator tmp { *this };
        operator--();
        return tmp;
    }

    auto operator-=(size_t n) noexcept -> BasicFarBufferIterator& {
        for(; n; n--) {
            operator--();
        }
        return *this;
    }

    auto operator-(size_t n) noexcept -> BasicFarBufferIterator {
        BasicFarBufferIterator tmp { *this };
        return tmp -= n;
    }

    friend auto operator==(BasicFarBufferIterator const& lhs,
                           BasicFarBufferIterator const& rhs) -> bool
    {
        return lhs.parent_ == rhs.parent_ && lhs.pos_ == rhs.pos_;
    }

    friend auto operator!=(BasicFarBufferIterator const& lhs,
                           BasicFarBufferIterator const& rhs) -> bool
    {
        return !(lhs == rhs);
    }

private:
    Parent* parent_;
    size_t  pos_;
};

template<typename Alloc = std::allocator<FarBufferBlock>>
struct BasicFarBuffer {
    template<typename Alloc, bool IsConst> 
        friend struct BasicFarBufferIterator;

    using value_type = uint8_t;
    using iterator = BasicFarBufferIterator<Alloc, false>;
    using const_iterator = BasicFarBufferIterator<Alloc, true>;

    explicit BasicFarBuffer(size_t size, Alloc alloc = Alloc { }) :
        block_ { allocate_far_buffer_block(size, alloc) }
    ,   alloc_ { alloc }
    { }

    BasicFarBuffer(BasicFarBuffer&& other) noexcept :
        block_ { other.block_ }
    ,   alloc_ { other.alloc_ }
    {
        other.block_ = nullptr;
    }

    BasicFarBuffer(BasicFarBuffer const&) = delete;

    ~BasicFarBuffer() {
        if (block_) {
            deallocate_far_buffer_block(block_, alloc_);
        }
    }

    friend auto swap(BasicFarBuffer& lhs, BasicFarBuffer& rhs) noexcept {
        using std::swap;
        swap(lhs.block_, rhs.block_);
        swap(lhs.alloc_, rhs.alloc_);
    }

    auto operator=(BasicFarBuffer const&) -> BasicFarBuffer& = delete;

    auto operator=(BasicFarBuffer&& other) noexcept -> BasicFarBuffer& {
        BasicFarBuffer tmp { std::move(other) };
        swap(tmp, *this);
        return *this;
    }

    auto control_block() noexcept -> FarBufferBlock* {
        assert(block_ && "block_ is null!");
        return block_;
    }

    auto control_block() const noexcept -> FarBufferBlock const* {
        return const_cast<BasicFarBuffer*>(this)->control_block();
    }

    auto data() noexcept -> value_type* { 
        assert(block_ && "block_ is null!");
        return block_->data; 
    }

    auto data() const noexcept -> value_type const* { 
        return const_cast<BasicFarBuffer*>(this)->data();
    }

    auto begin() noexcept -> iterator { 
        return { this, 0 };
    }

    auto end() noexcept -> iterator { 
        return { this, control_block()->consumed };
    }

    auto begin() const noexcept -> const_iterator {
        return { this, 0 };
    }

    auto end() const noexcept -> const_iterator {
        return { this, control_block()->consumed };
    }

    auto size() const noexcept -> size_t {
        return control_block()->consumed;
    }

    auto capacity() const noexcept -> size_t {
        return control_block()->length;
    }

    auto append(Span<uint8_t const> data) noexcept -> size_t {
        auto at = end();
        auto from = data.begin();

        auto n = 
            std::min(data.size(), capacity() - size());

        control_block()->consumed += n;
        for (auto i = n; i; --i) {
            *at++ = *from++;
        }
        
        return n;
    }

    auto fill(Span<uint8_t const> data) noexcept -> size_t {
        control_block()->consumed = 
            std::min(data.size(), capacity());

        auto in_first = data.begin();
        for (auto& b : *this) {
            b = *in_first++;
        }

        return in_first - data.begin();
    }

    auto consume(Span<uint8_t> target) noexcept -> size_t {
        
        auto out_it = target.begin();
        auto from_it = begin();
        while (out_it != target.end() && from_it != end()) {
            *out_it++ = *from_it++;
        }

        auto read = out_it - target.begin();
        consume_from_block(control_block(), read);
        return read;
    }

    auto clear() noexcept {
        consume_from_block(control_block(), size());
    }

private:
    FarBufferBlock* block_;
    Alloc alloc_;
};

using FarBuffer = BasicFarBuffer<std::allocator<FarBufferBlock>>;

}
}
#endif // GMB_FAR_BUFFER_HPP_INCLUDED

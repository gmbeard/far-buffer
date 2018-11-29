#ifndef GMB_FAR_BUFFER_HPP_INCLUDED
#define GMB_FAR_BUFFER_HPP_INCLUDED

#include <memory>
#include <cassert>

namespace gmb {
namespace utils {

struct FarBufferBlock {
    size_t length;
    size_t consumed;
    uint8_t* data;
};

template<typename Alloc>
using ToFarBufferBlockAlloc = typename 
    std::allocator_traits<Alloc>::template rebind_alloc<FarBufferBlock>;

template<typename Alloc>
auto allocate_far_buffer_block(size_t size, Alloc a) -> FarBufferBlock* {
    using std::allocator_traits;

    ToFarBufferBlockAlloc<Alloc> alloc { a };

    auto actual_size = 
        (sizeof(FarBufferBlock) * 2) + (size / sizeof(FarBufferBlock));

    auto* far_buffer = 
        allocator_traits<ToFarBufferBlockAlloc<Alloc>>::allocate(
            alloc,
            actual_size / sizeof(FarBufferBlock)
        );

    auto* buffer_start = reinterpret_cast<uint8_t*>(far_buffer + 1);

    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::construct(
        alloc,
        far_buffer,
        FarBufferBlock { 
            actual_size - sizeof(FarBufferBlock), 
            0, 
            buffer_start 
        }
    );

    return far_buffer;
}

template<typename Alloc>
auto deallocate_far_buffer_block(FarBufferBlock* fb, Alloc a) {
    using std::allocator_traits;

    auto n_items = (fb->length / sizeof(FarBufferBlock)) + 1;

    ToFarBufferBlockAlloc<Alloc> alloc { a };
    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::destroy(alloc, fb);
    allocator_traits<ToFarBufferBlockAlloc<Alloc>>::deallocate(alloc, fb, n_items);
}

template<typename Alloc = std::allocator<FarBufferBlock>>
struct BasicFarBuffer {
    using value_type = uint8_t;

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

    auto begin() noexcept -> uint8_t* { 
        return control_block()->data; 
    }

    auto end() noexcept -> uint8_t* { 
        return control_block()->data + control_block()->consumed; 
    }

    auto begin() const noexcept -> uint8_t const* {
        return const_cast<BasicFarBuffer*>(this)->begin(); 
    }

    auto end() const noexcept -> uint8_t const* {
        return const_cast<BasicFarBuffer*>(this)->end();
    }

    auto size() const noexcept -> size_t {
        return control_block()->consumed;
    }

    auto capacity() const noexcept -> size_t {
        return control_block()->length - size();
    }

private:
    FarBufferBlock* block_;
    Alloc alloc_;
};

using FarBuffer = BasicFarBuffer<std::allocator<FarBufferBlock>>;

}
}
#endif // GMB_FAR_BUFFER_HPP_INCLUDED

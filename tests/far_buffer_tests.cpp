#include "far_buffer.hpp"
#include <iostream>
#include <algorithm>
#include <vector>

using gmb::utils::FarBuffer;
using gmb::utils::Span;

auto print(FarBuffer const& buf) {
    for(auto const& b : buf) {
        std::cout << b;
    }
    std::cout << "\n";
}

auto main() -> int {

    auto buf = FarBuffer { 38 };
    assert(buf.capacity() == 38 && "Incorrect capacity reported!");

    auto vec = std::vector<uint8_t>(25, 'c');
    auto const pos = buf.fill({ &*vec.begin(), vec.size() });
    assert(pos == 25 && 
            "Incorrect number of bytes reportedly written!");
    assert(buf.size() == 25 && 
        "Incorrect number of bytes in buffer after write!");

    print(buf);

    return 0;
}

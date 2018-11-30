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

auto consume_buffer(FarBuffer& buf) {

    uint8_t n;
    size_t consumed;
    do {
        consumed = buf.consume({&n, &n + 1});
        print(buf);
    }
    while (consumed);
}

auto fill_buffer() {

    uint8_t c = 'A';
    size_t written;
    auto buf = FarBuffer { 52 };

    do {
        written = buf.append({ &c, &c + 1 });
        c += 1;
    }
    while (written);

    print(buf);
    consume_buffer(buf);
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

    fill_buffer();

    return 0;
}

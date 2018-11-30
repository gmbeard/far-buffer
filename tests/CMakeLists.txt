add_executable(
    far_buffer_test_prog
    far_buffer_tests.cpp
)

target_link_libraries(
    far_buffer_test_prog
    PRIVATE
        FarBuffer
)

target_compile_options(
    far_buffer_test_prog
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /analyze>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Werror -Wextra>
)

add_test(
    NAME FarBufferTests
    COMMAND far_buffer_test_prog
)

if( ECC_IMPL STREQUAL secp256k1 )
    add_executable( blind all_tests.cpp crypto/blind.cpp )
    target_link_libraries( blind fc )
endif()

add_executable( bloom_test all_tests.cpp bloom_test.cpp )
target_link_libraries( bloom_test fc )

add_executable( hmac_test hmac_test.cpp )
target_link_libraries( hmac_test fc )

add_executable( ecc_test crypto/ecc_test.cpp )
target_link_libraries( ecc_test fc )

add_executable( all_tests all_tests.cpp
                          compress/compress.cpp
                          crypto/aes_test.cpp
                          crypto/array_initialization_test.cpp
                          crypto/base_n_tests.cpp
                          crypto/bigint_test.cpp
                          crypto/blind.cpp
                          crypto/dh_test.cpp
                          crypto/rand_test.cpp
                          crypto/sha_tests.cpp
                          io/json_tests.cpp
                          io/stream_tests.cpp
                          io/tcp_test.cpp
                          io/varint_tests.cpp
                          network/http/websocket_test.cpp
                          thread/thread_tests.cpp
                          thread/parallel_tests.cpp
                          bloom_test.cpp
                          reflection_tests.cpp
                          serialization_test.cpp
                          stacktrace_test.cpp
                          time_test.cpp
                          utf8_test.cpp
                          variant_test.cpp
                          logging_tests.cpp
                          api_tests.cpp
                          )
target_link_libraries( all_tests fc )

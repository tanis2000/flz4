cmake_minimum_required(VERSION 3.15..4.0)
project(Uncompress
  VERSION 1.0
  LANGUAGES C)

add_library(lz4 ./deps/lz4/lz4.c ./deps/lz4/lz4file.c ./deps/lz4/lz4hc.c ./deps/lz4/xxhash.c ./deps/lz4/lz4frame.c
)
add_executable(uncompress uncompress.c)
target_include_directories(uncompress PUBLIC ./deps/lz4)
target_link_libraries(uncompress PRIVATE lz4)

cmake_minimum_required(VERSION 3.13)
project(MyQSPI_PSRAM_lib C CXX ASM)

add_library(MyQSPI_PSRAM_lib STATIC
    ${CMAKE_CURRENT_LIST_DIR}/headers/MyQSPI_PSRAM.h
    ${CMAKE_CURRENT_LIST_DIR}/headers/MyQSPI_PSRAM.hpp
)

file(GLOB_RECURSE PIO_FILES "${CMAKE_CURRENT_LIST_DIR}/pios/*.pio")

foreach(FILE_PATH ${PIO_FILES})
    pico_generate_pio_header(MyQSPI_PSRAM_lib ${FILE_PATH})
endforeach()

target_include_directories(MyQSPI_PSRAM_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/headers
)

target_link_libraries(MyQSPI_PSRAM_lib PUBLIC 
        hardware_dma
        hardware_pio
        hardware_sync
        pico_stdlib
)
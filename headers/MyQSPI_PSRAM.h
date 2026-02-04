#ifndef MY_QSPI_PSRAM_H
#define MY_QSPI_PSRAM_H

#include <iostream>
#include <string>

#include <cstdint>
#include <cassert>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"

// ---------- PIOS ----------

#include "qspi_rw.pio.h"
#include "spi_rw.pio.h"

// ---------- PIOS END ----------

#ifdef FREERTOS_CONFIG_H
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif

#ifndef PSRAM_CLOCK
    #define PSRAM_CLOCK 133000000
#endif

/// @brief Class for adding QSPI PSRAM functionality to rp2040
class MyQSPI_PSRAM{
    public:
        /// @brief Initialize the class with the first pin for vga, pin order: hsync, vsync, green, blue, red; First go the low order bits then high
        /// @param start_pin first of the pins (hsync pin)
        MyQSPI_PSRAM(uint8_t cs_sck_pins, uint8_t data_pins, uint8_t pio_num=0);

        /// @brief Initialize dma's, pio's and irq's, and start them. 
        /// @brief If using FreeRTOS call this inside a task, the interrupts will be pinned to that task. 
        void initPSRAM();

        void write16(uint32_t addr, uint16_t data);
        void write_block(uint32_t block_num, uint8_t* data, uint16_t data_len);

        uint16_t read16(uint32_t addr);
        uint8_t* read_block(uint32_t block_num);

    private: // private Variables
        uint8_t cs_sck_pins, data_pins;

        PIO _pio;
        uint32_t dma_chan_read, dma_chan_write;
        dma_channel_config dma_write_config, dma_read_config;

        uint qspi_sm;

        uint16_t clock_divider;

        static inline MyQSPI_PSRAM* self;
        uint8_t buffer[1032], cmd_read_buffer[10];
        
        
    private: // private Functions
        uint8_t find_clock_divisor();
};

#include "MyQSPI_PSRAM.hpp"

#endif // MY_QSPI_PSRAM_H
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
        /// @brief Initialize the class with the first pin for chip select and clock pins, data_pins, and choose a pio
        /// @param cs_sck_pins first of the chip select and clock pins
        /// @param data_pins first of the sio data pins
        /// @param pio_num choose pio(optional, default is 0)
        MyQSPI_PSRAM(uint8_t cs_sck_pins, uint8_t data_pins, uint8_t pio_num=0);

        /// @brief Initialize the psram, setup the qspi and dmas.
        void initPSRAM();

        /// @brief Write 2 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void write16(uint32_t addr, uint16_t data);

        /// @brief Write a whole page of data (1024 bytes).
        /// @param page_num Number of the page (0-8,191).
        /// @param data Pointer to the data buffer.
        /// @param data_len Length of the data buffer. 
        void write_page(uint32_t page_num, const uint8_t* data, const uint16_t data_len);

        uint16_t read16(uint32_t addr);

        const uint8_t* read_page(uint32_t page_num);

    private: // private Variables
        uint8_t cs_sck_pins, data_pins;

        PIO _pio;
        uint32_t dma_chan_read, dma_chan_write;
        dma_channel_config dma_write_config, dma_read_config;

        uint qspi_sm;

        uint16_t clock_divider;

        uint8_t buffer[1032], cmd_read_buffer[10];
        
    private: // private Functions
        uint8_t find_clock_divisor();
};

#include "MyQSPI_PSRAM.hpp"

#endif // MY_QSPI_PSRAM_H
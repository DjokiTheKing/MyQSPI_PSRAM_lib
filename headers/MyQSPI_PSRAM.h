#ifndef MY_QSPI_PSRAM_H
#define MY_QSPI_PSRAM_H

#include <iostream>
#include <string>

#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"

// ---------- PIOS ----------

#include "qspi_rw_2.pio.h"
#include "qspi_rw_4.pio.h"
#include "spi_rw.pio.h"

// ---------- PIOS END ----------

#ifndef PSRAM_MAX_CLOCK
    #define PSRAM_MAX_CLOCK 150000000
#endif

#ifndef PSRAM_MIN_CLOCK
    #define PSRAM_MIN_CLOCK 31250000
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
        void write2(uint32_t addr, uint16_t data);

        /// @brief Write 4 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void write4(uint32_t addr, uint32_t data);

        /// @brief Write 8 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void write8(uint32_t addr, uint64_t data);

        /// @brief Write 64 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void write64(uint32_t addr, const uint16_t* data);

        /// @brief Write a block of data .
        /// @param addr Write address.
        /// @param data Pointer to the data buffer.
        /// @param data_len Length of the data buffer. 
        void write(uint32_t addr, const uint16_t* data, const uint16_t data_len);

        /// @brief Read 2 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint16_t read2(uint32_t addr);

        /// @brief Read 4 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint32_t read4(uint32_t addr);

        /// @brief Read 8 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint64_t read8(uint32_t addr);

        /// @brief Read 64 bytes of data from the psram.
        /// @param addr Read address.
        /// @param data Pointer to the read buffer. Make sure it's at least 64 bytes of length
        void read64(uint32_t addr, uint16_t* data);

        /// @brief Read a block of data .
        /// @param addr Read address.
        /// @param data Pointer to the read buffer.
        /// @param data_len Length of the data to be read. 
        void read(uint32_t addr, uint16_t* data, const uint16_t data_len);

    private: // private Variables
        uint8_t cs_sck_pins, data_pins;

        PIO _pio;
        uint32_t dma_chan_read, dma_chan_write;
        dma_channel_config dma_write_config, dma_read_config;

        uint qspi_sm;

        uint16_t clock_divider;

        uint16_t buffer[300], cmd_read_buffer[5];

        /// @brief psram clocks
        uint16_t max_clocks_selected;

        /// @brief mcu clocks
        uint16_t min_clocks_deselected;
        
    private: // private Functions
        uint8_t find_clock_divisor();
};

#include "MyQSPI_PSRAM.hpp"

#endif // MY_QSPI_PSRAM_H
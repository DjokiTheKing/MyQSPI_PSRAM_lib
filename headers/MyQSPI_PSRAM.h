#ifndef MY_QSPI_PSRAM_H
#define MY_QSPI_PSRAM_H

#include <iostream>
#include <string>

#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/structs/bus_ctrl.h"

// ---------- PIOS ----------

#include "qspi_rw_2_nf.pio.h"
#include "qspi_rw_4_nf.pio.h"
#include "spi_rw.pio.h"

// ---------- PIOS END ----------

#ifndef PSRAM_MAX_CLOCK
    #define PSRAM_MAX_CLOCK 160000000
#endif

#ifndef PSRAM_MIN_CLOCK
    #define PSRAM_MIN_CLOCK 100000000
#endif

enum class MyQSPI_ERRORS : int8_t {
    PSRAM_OK = 0,
    PSRAM_ERROR_COULD_NOT_FIND_SUITABLE_CLOCK_DIV = 1,
    PSRAM_ERROR_COULD_NOT_DETECT_PSRAM = 2,
    PIO_ERROR_COULD_NOT_INITIALIZE = 3
};

#ifdef MYQSPI_PSRAM_RUN_FROM_PSRAM
    #define MYQSPI_PSRAM_FUNC_WRAPPER(x) __no_inline_not_in_flash_func(x)
#else
    #define MYQSPI_PSRAM_FUNC_WRAPPER(x) x
#endif

/// @brief Class for adding QSPI PSRAM functionality to rp2040
class MyQSPI_PSRAM{
    public:
        /// @brief Initialize the class with the first pin for chip select and clock pins, data_pins, and choose a pio
        /// @param cs_sck_pins first of the chip select and clock pins
        /// @param data_pins first of the sio data pins
        /// @param pio_num choose pio(optional, default is 0)
        MyQSPI_PSRAM(uint8_t cs_sck_pins, uint8_t data_pins, uint8_t pio_num=0);

        /// @brief Initialize the psram.
        MyQSPI_ERRORS initPSRAM();

        /// @brief Write 1 byte of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void MYQSPI_PSRAM_FUNC_WRAPPER(write8)(uint32_t addr, uint8_t data);

        /// @brief Write 2 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void MYQSPI_PSRAM_FUNC_WRAPPER(write16)(uint32_t addr, uint16_t data);

        /// @brief Write 4 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void MYQSPI_PSRAM_FUNC_WRAPPER(write32)(uint32_t addr, uint32_t data);

        /// @brief Write 8 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Data.
        void MYQSPI_PSRAM_FUNC_WRAPPER(write64)(uint32_t addr, uint64_t data);

        /// @brief Write 64 bytes of data to the psram.
        /// @param addr Write address. 
        /// @param data Pointer to the data. Make sure it's at least 64 bytes of length. 
        void MYQSPI_PSRAM_FUNC_WRAPPER(write512)(uint32_t addr, const uint8_t* data);

        /// @brief Write a block of data .
        /// @param addr Write address.
        /// @param data Pointer to the data buffer.
        /// @param data_len Length of the data buffer. MAX is 2048
        void MYQSPI_PSRAM_FUNC_WRAPPER(write)(uint32_t addr, const uint8_t* data, uint32_t data_len);

        /// @brief Read 1 byte of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint8_t MYQSPI_PSRAM_FUNC_WRAPPER(read8)(uint32_t addr);

        /// @brief Read 2 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint16_t MYQSPI_PSRAM_FUNC_WRAPPER(read16)(uint32_t addr);

        /// @brief Read 4 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint32_t MYQSPI_PSRAM_FUNC_WRAPPER(read32)(uint32_t addr);

        /// @brief Read 8 bytes of data from the psram.
        /// @param addr Read address. 
        /// @param data Data.
        uint64_t MYQSPI_PSRAM_FUNC_WRAPPER(read64)(uint32_t addr);

        /// @brief Read 64 bytes of data from the psram.
        /// @param addr Read address.
        /// @param data Pointer to the read buffer. Make sure it's at least 64 bytes of length
        void MYQSPI_PSRAM_FUNC_WRAPPER(read512)(uint32_t addr, uint8_t* data);

        /// @brief Read a block of data .
        /// @param addr Read address.
        /// @param data Pointer to the read buffer.
        /// @param data_len Length of the data to be read. MAX is 2048
        void MYQSPI_PSRAM_FUNC_WRAPPER(read)(uint32_t addr, uint8_t* data, const uint32_t data_len);

        /// @brief Write a value across a block of memory.
        /// @param addr Address of the first byte.
        /// @param val Value to write.
        /// @param size Size of the block to write.
        void MYQSPI_PSRAM_FUNC_WRAPPER(pmemset)(uint32_t addr, uint8_t val, const uint32_t size);

        /// @brief Copy a block of memory from one place to another.
        /// @param addr_dst Destination address.
        /// @param addr_src Source address.
        /// @param size Size of the block to copy.
        void MYQSPI_PSRAM_FUNC_WRAPPER(pmemcpy)(uint32_t addr_dst, uint32_t addr_src, const uint32_t size);

        /// @brief Get the size of the psram.
        /// @return Size of the psram bytes.
        uint32_t get_size(){ return psram_size;};

    private: // private Variables
        uint8_t cs_sck_pins, data_pins;

        PIO _pio;
        uint32_t dma_chan_read, dma_chan_write;
        dma_channel_config dma_write_config, dma_read_config;

        uint qspi_sm;
        pio_program qspi_program;
        uint8_t qspi_wrap_target;
        uint8_t qspi_wrap;
        uint8_t clock_divider;

        alignas(4) uint8_t buffer[132];

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
        spin_lock_t *psram_spinlock;
#endif // MYQSPI_PSRAM_USE_SPINLOCK
        uint32_t psram_size;
        
    private: // private Functions
        uint8_t find_clock_divisor();
};

#include "MyQSPI_PSRAM.hpp"

#endif // MY_QSPI_PSRAM_H
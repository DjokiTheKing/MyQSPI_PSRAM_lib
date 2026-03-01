#ifndef MY_QSPI_PSRAM_IMPL_H
#define MY_QSPI_PSRAM_IMPL_H

#include "MyQSPI_PSRAM.h"

/// @brief Initialize the class with the first pin for chip select and clock pins, data_pins, and choose a pio
/// @param cs_sck_pins first of the chip select and clock pins
/// @param data_pins first of the sio data pins
/// @param pio_num choose pio(optional, default is 0)
MyQSPI_PSRAM::MyQSPI_PSRAM(uint8_t cs_sck_pins, uint8_t data_pins, uint8_t pio_num)
:
cs_sck_pins(cs_sck_pins),
data_pins(data_pins)
{
    if (pio_num == 0)
    {
        _pio = pio0;
    }
    else if (pio_num == 1)
    {
        _pio = pio1;
    }
    #ifdef PICO_RP2350
    else if (pio_num == 2){
        _pio = pio2;
    }
    #endif 
    clock_divider = find_clock_divisor();
    if(clock_divider == 2) {
        qspi_program = qspi_rw_2_nf_program;
        qspi_wrap_target = qspi_rw_2_nf_wrap_target;
        qspi_wrap = qspi_rw_2_nf_wrap;
    }else if(clock_divider == 4){
        qspi_program = qspi_rw_4_nf_program;
        qspi_wrap_target = qspi_rw_4_nf_wrap_target;
        qspi_wrap = qspi_rw_4_nf_wrap;
    }
}

MyQSPI_ERRORS MyQSPI_PSRAM::initPSRAM()
{
    if(!clock_divider) {
        return MyQSPI_ERRORS::PSRAM_ERROR_COULD_NOT_FIND_SUITABLE_CLOCK_DIV;
    }


#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t psram_spinlock_num = spin_lock_claim_unused(true);
    psram_spinlock = spin_lock_init(psram_spinlock_num);
#endif

    pio_gpio_init(_pio, cs_sck_pins);
    pio_gpio_init(_pio, cs_sck_pins + 1);
    pio_gpio_init(_pio, data_pins);
    pio_gpio_init(_pio, data_pins + 1);
    pio_gpio_init(_pio, data_pins + 2);
    pio_gpio_init(_pio, data_pins + 3);

    gpio_set_slew_rate(cs_sck_pins, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(cs_sck_pins + 1, GPIO_SLEW_RATE_FAST);    
    gpio_set_slew_rate(data_pins, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(data_pins + 1, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(data_pins + 2, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(data_pins + 3, GPIO_SLEW_RATE_FAST);

    gpio_set_drive_strength(cs_sck_pins, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(cs_sck_pins + 1, GPIO_DRIVE_STRENGTH_12MA);    
    gpio_set_drive_strength(data_pins, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(data_pins + 1, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(data_pins + 2, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(data_pins + 3, GPIO_DRIVE_STRENGTH_12MA);

    gpio_set_input_hysteresis_enabled(cs_sck_pins, false);
    gpio_set_input_hysteresis_enabled(cs_sck_pins + 1, false);    
    gpio_set_input_hysteresis_enabled(data_pins, false);
    gpio_set_input_hysteresis_enabled(data_pins + 1, false);
    gpio_set_input_hysteresis_enabled(data_pins + 2, false);
    gpio_set_input_hysteresis_enabled(data_pins + 3, false);

    hw_set_bits(&_pio->input_sync_bypass, 0xfu << data_pins);

    uint qspi_offset = pio_add_program(_pio, &qspi_program);

    qspi_sm = pio_claim_unused_sm(_pio, true);

    pio_sm_config qspi_sm_config = pio_get_default_sm_config();

    sm_config_set_wrap(&qspi_sm_config, qspi_offset + qspi_wrap_target, qspi_offset + qspi_wrap); 
    sm_config_set_sideset(&qspi_sm_config, 2, false, false);

    sm_config_set_sideset_pins(&qspi_sm_config, cs_sck_pins);
    sm_config_set_set_pins(&qspi_sm_config, data_pins, 4);
    sm_config_set_out_pins(&qspi_sm_config, data_pins, 4);
    sm_config_set_in_pins(&qspi_sm_config, data_pins);
    
    sm_config_set_clkdiv(&qspi_sm_config, 1);

    sm_config_set_out_shift(&qspi_sm_config, false, true, 8);
    sm_config_set_in_shift(&qspi_sm_config, false, true, 8);

    pio_sm_set_consecutive_pindirs(_pio, qspi_sm, cs_sck_pins, 2, true);
    pio_sm_set_consecutive_pindirs(_pio, qspi_sm, data_pins, 4, false);

    if(pio_sm_init(_pio, qspi_sm, qspi_offset, &qspi_sm_config) != PICO_OK) {
        return MyQSPI_ERRORS::PIO_ERROR_COULD_NOT_INITIALIZE;
    }

    pio_sm_set_enabled(_pio, qspi_sm, true);

    busy_wait_us(150);

    pio_sm_put_blocking(_pio, qspi_sm, 0x02000000u);
    pio_sm_put_blocking(_pio, qspi_sm, 0x00000000u);
    pio_sm_put_blocking(_pio, qspi_sm, 0xF5000000u);

    pio_sm_set_enabled(_pio, qspi_sm, false);
    pio_remove_program_and_unclaim_sm(&qspi_program, _pio, qspi_sm, qspi_offset);

    uint spi_offset = pio_add_program(_pio, &spi_rw_program);

    uint spi_sm = pio_claim_unused_sm(_pio, true);
    pio_sm_config spi_sm_config = pio_get_default_sm_config();

    sm_config_set_wrap(&spi_sm_config, spi_offset + spi_rw_wrap_target, spi_offset + spi_rw_wrap);
    sm_config_set_sideset(&spi_sm_config, 2, false, false);

    sm_config_set_sideset_pins(&spi_sm_config, cs_sck_pins);
    sm_config_set_out_pins(&spi_sm_config, data_pins, 1);
    sm_config_set_in_pins(&spi_sm_config, data_pins+1);

    sm_config_set_clkdiv(&spi_sm_config, 2);

    sm_config_set_out_shift(&spi_sm_config, false, true, 8);
    sm_config_set_in_shift(&spi_sm_config, false, true, 8);

    pio_sm_set_consecutive_pindirs(_pio, spi_sm, cs_sck_pins, 2, true);
    pio_sm_set_consecutive_pindirs(_pio, spi_sm, data_pins, 1, true);
    pio_sm_set_consecutive_pindirs(_pio, spi_sm, data_pins+1, 1, false);

    if(pio_sm_init(_pio, spi_sm, spi_offset, &spi_sm_config) != PICO_OK){
        return MyQSPI_ERRORS::PIO_ERROR_COULD_NOT_INITIALIZE;
    }
    pio_sm_set_enabled(_pio, spi_sm, true);

    pio_sm_put_blocking(_pio, spi_sm, 0x08000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x00000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x66000000u);

    busy_wait_us(10);

    pio_sm_put_blocking(_pio, spi_sm, 0x08000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x00000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x99000000u);

    busy_wait_us(150);

    uint32_t kgd = 0, eid = 0;
    pio_sm_put_blocking(_pio, spi_sm, 0x20000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x30000000u);

    pio_sm_put_blocking(_pio, spi_sm, 0x9F000000u);
    for(int i = 0; i < 3; ++i) pio_sm_put_blocking(_pio, spi_sm, 0xFF000000u);

    pio_sm_get_blocking(_pio, spi_sm);
    kgd = pio_sm_get_blocking(_pio, spi_sm);
    eid = pio_sm_get_blocking(_pio, spi_sm);
    pio_sm_get_blocking(_pio, spi_sm);
    pio_sm_get_blocking(_pio, spi_sm);
    pio_sm_get_blocking(_pio, spi_sm);

    if(kgd != 0x5D) return MyQSPI_ERRORS::PSRAM_ERROR_COULD_NOT_DETECT_PSRAM;

    psram_size = 1024 * 1024;
    uint8_t size_id = eid >> 5;
    if (eid == 0x26 || size_id == 2) {
        psram_size *= 8;
    } else if (size_id == 0) {
        psram_size *= 2;
    } else if (size_id == 1) {
        psram_size *= 4;
    }

    busy_wait_us(2);

    pio_sm_put_blocking(_pio, spi_sm, 0x08000000u);
    pio_sm_put_blocking(_pio, spi_sm, 0x00000000u);
    
    pio_sm_put_blocking(_pio, spi_sm, 0x35000000u);

    busy_wait_us(10);
    
    pio_sm_set_enabled(_pio, spi_sm, false);
    pio_remove_program_and_unclaim_sm(&spi_rw_program, _pio, spi_sm, spi_offset);

    qspi_offset = pio_add_program(_pio, &qspi_program);

    qspi_sm = pio_claim_unused_sm(_pio, true);

    qspi_sm_config = pio_get_default_sm_config();

    sm_config_set_wrap(&qspi_sm_config, qspi_offset + qspi_wrap_target, qspi_offset + qspi_wrap); 
    sm_config_set_sideset(&qspi_sm_config, 2, false, false);

    sm_config_set_sideset_pins(&qspi_sm_config, cs_sck_pins);
    sm_config_set_set_pins(&qspi_sm_config, data_pins, 4);
    sm_config_set_out_pins(&qspi_sm_config, data_pins, 4);
    sm_config_set_in_pins(&qspi_sm_config, data_pins);
    
    sm_config_set_clkdiv(&qspi_sm_config, 1);

    sm_config_set_out_shift(&qspi_sm_config, false, true, 8);
    sm_config_set_in_shift(&qspi_sm_config, false, true, 8);

    pio_sm_set_consecutive_pindirs(_pio, qspi_sm, cs_sck_pins, 2, true);
    pio_sm_set_consecutive_pindirs(_pio, qspi_sm, data_pins, 4, false);

    if(pio_sm_init(_pio, qspi_sm, qspi_offset, &qspi_sm_config) != PICO_OK) {
        return MyQSPI_ERRORS::PIO_ERROR_COULD_NOT_INITIALIZE;
    }

    pio_sm_set_enabled(_pio, qspi_sm, true);

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS;
    
    dma_chan_read = dma_claim_unused_channel(true);
    dma_chan_write = dma_claim_unused_channel(true);

    dma_write_config = dma_channel_get_default_config(dma_chan_write);

    channel_config_set_transfer_data_size(&dma_write_config, DMA_SIZE_8);      

    channel_config_set_read_increment(&dma_write_config, true);                        
    channel_config_set_write_increment(&dma_write_config, false);    

    channel_config_set_high_priority(&dma_write_config, true);     

    channel_config_set_dreq(&dma_write_config, pio_get_dreq(_pio, qspi_sm, true));    
    
    dma_channel_set_config(dma_chan_write, &dma_write_config, false);
    dma_channel_set_write_addr(dma_chan_write, &_pio->txf[qspi_sm], false);

    dma_read_config = dma_channel_get_default_config(dma_chan_read);

    channel_config_set_transfer_data_size(&dma_read_config, DMA_SIZE_8);     

    channel_config_set_read_increment(&dma_read_config, false);  
    channel_config_set_write_increment(&dma_read_config, true);   

    channel_config_set_high_priority(&dma_read_config, true);

    channel_config_set_dreq(&dma_read_config, pio_get_dreq(_pio, qspi_sm, false));  
    
    dma_channel_set_config(dma_chan_read, &dma_read_config, false);
    dma_channel_set_read_addr(dma_chan_read, &_pio->rxf[qspi_sm], false);

    return MyQSPI_ERRORS::PSRAM_OK;
}

/// @brief Write 1 byte of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write8(uint32_t addr, uint8_t data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

    buffer[2+0] = 5*2-1;
    buffer[2+1] = 0;
    
    buffer[2+2] = 0x38u;

    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    buffer[2+6] = data;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 7);
    dma_channel_wait_for_finish_blocking(dma_chan_write);

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

/// @brief Write 2 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write16(uint32_t addr, uint16_t data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

    buffer[2+0] = 6*2-1;
    buffer[2+1] = 0;
    
    buffer[2+2] = 0x38u;

    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    *(reinterpret_cast<uint16_t*>(buffer+8)) = data;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 8);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}
/// @brief Write 4 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write32(uint32_t addr, uint32_t data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK    
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    buffer[2+0] = 8*2-1;
    buffer[2+1] = 0;
    
    buffer[2+2] = 0x38u;

    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    *(reinterpret_cast<uint32_t*>(buffer+8)) = data;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 10);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK    
    spin_unlock(psram_spinlock, intr_state);
#endif
}

/// @brief Write 8 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write64(uint32_t addr, uint64_t data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK    
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    buffer[2+0] = 12*2-1;
    buffer[2+1] = 0;
    
    buffer[2+2] = 0x38u;

    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    *(reinterpret_cast<uint64_t*>(buffer+8)) = data;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 14);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

/// @brief Write 64 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Pointer to the data. Make sure it's 64 bytes of length. 
void MyQSPI_PSRAM::write512(uint32_t addr, const uint8_t* data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    buffer[2+0] = 68*2-1;
    buffer[2+1] = 0;
    
    buffer[2+2] = 0x38u;

    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    memcpy(buffer+8, data, 64);
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 70);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

void MyQSPI_PSRAM::write(uint32_t addr, const uint8_t *data, const uint32_t data_len){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

/// @brief Read 1 byte of data from the psram.
/// @param addr Read address.
/// @param data Data.
uint8_t MyQSPI_PSRAM::read8(uint32_t addr){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    
    buffer[2+0] = 4*2-1;
    buffer[2+1] = 2-1;

    buffer[2+2] = 0xEBu;
    
    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer+8, 1);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint8_t tmp = buffer[2+6];
    spin_unlock(psram_spinlock, intr_state);
    return tmp;
#else
    return buffer[2+6];
#endif
}

/// @brief Read 2 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint16_t MyQSPI_PSRAM::read16(uint32_t addr){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    buffer[2+0] = 4*2-1;
    buffer[2+1] = 4-1;

    buffer[2+2] = 0xEBu;
    
    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer+8, 2);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint16_t tmp = *(reinterpret_cast<uint16_t*>(&buffer[2+6]));
    spin_unlock(psram_spinlock, intr_state);
    return tmp;
#else
    return *(reinterpret_cast<uint16_t*>(&buffer[2+6]));
#endif
}

/// @brief Read 4 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint32_t MyQSPI_PSRAM::read32(uint32_t addr){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif
    buffer[2+0] = 4*2-1;
    buffer[2+1] = 8-1;

    buffer[2+2] = 0xEBu;
    
    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer+8, 4);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t tmp = *(reinterpret_cast<uint32_t*>(&buffer[2+6]));
    spin_unlock(psram_spinlock, intr_state);
    return tmp;
#else
    return *(reinterpret_cast<uint32_t*>(&buffer[2+6]));
#endif
}

/// @brief Read 8 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint64_t MyQSPI_PSRAM::read64(uint32_t addr){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

    buffer[2+0] = 4*2-1;
    buffer[2+1] = 16-1;

    buffer[2+2] = 0xEBu;
    
    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer+8, 8);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint64_t tmp = *(reinterpret_cast<uint64_t*>(&buffer[2+6]));
    spin_unlock(psram_spinlock, intr_state);
    return tmp;
#else
    return *(reinterpret_cast<uint64_t*>(&buffer[2+6]));
#endif
}

/// @brief Read 64 bytes of data from the psram.
/// @param addr Read address.
/// @param data Pointer to the read buffer. Make sure it's at least 64 bytes of length
void MyQSPI_PSRAM::read512(uint32_t addr, uint8_t* data){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

    buffer[2+0] = 4*2-1;
    buffer[2+1] = 128-1;

    buffer[2+2] = 0xEBu;
    
    buffer[2+3] = (addr >> 16) & 0xFFu;
    buffer[2+4] = (addr >> 8) & 0xFFu;
    buffer[2+5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer+2, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, data, 64);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

/// @brief Read a block of data .
/// @param addr Read address.
/// @param data Pointer to the read buffer, must be at least data_len bytes long.
/// @param data_len Length of the data to be read. MAX is 2048
void MyQSPI_PSRAM::read(uint32_t addr, uint8_t* data, const uint32_t data_len){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

void MyQSPI_PSRAM::pmemset(uint32_t addr, uint8_t val, const uint32_t size){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

void MyQSPI_PSRAM::pmemcpy(uint32_t addr_dst, uint32_t addr_src, const uint32_t size){
#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    uint32_t  intr_state = spin_lock_blocking(psram_spinlock);
#endif

#ifdef MYQSPI_PSRAM_USE_SPINLOCK
    spin_unlock(psram_spinlock, intr_state);
#endif
}

uint8_t MyQSPI_PSRAM::find_clock_divisor()
{   
    for(uint32_t i = 2; i < 5 ; i += 2){
        if(SYS_CLK_HZ/i <= PSRAM_MAX_CLOCK) return i;
    }

    return 0;
}

#endif // MY_QSPI_PSRAM_IMPL_H
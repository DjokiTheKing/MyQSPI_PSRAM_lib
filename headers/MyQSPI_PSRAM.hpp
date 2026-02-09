#ifndef MY_QSPI_PSRAM_IMPL_H
#define MY_QSPI_PSRAM_IMPL_H

#include "MyQSPI_PSRAM.h"

/// @brief Initialize the class with the first pin for vga, pin order: hsync, vsync, green, blue, red; First go the low order bits then high
/// @param start_pin first of the pins (hsync pin)
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
    clock_divider = 4;
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
    std::cout << "Clock divider: " << int(clock_divider) << std::endl;

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

    gpio_set_drive_strength(cs_sck_pins, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(cs_sck_pins + 1, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(data_pins, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(data_pins + 1, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(data_pins + 2, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(data_pins + 3, GPIO_DRIVE_STRENGTH_4MA);

    uint spi_offset = pio_add_program(_pio, &spi_rw_program);

    uint spi_sm = pio_claim_unused_sm(_pio, true);
    pio_sm_config spi_sm_config = pio_get_default_sm_config();

    sm_config_set_wrap(&spi_sm_config, spi_offset + spi_rw_wrap_target, spi_offset + spi_rw_wrap);
    sm_config_set_sideset(&spi_sm_config, 2, false, false);

    sm_config_set_sideset_pins(&spi_sm_config, cs_sck_pins);
    sm_config_set_out_pins(&spi_sm_config, data_pins, 1);

    sm_config_set_clkdiv(&spi_sm_config, clock_divider*2);

    sm_config_set_out_shift(&spi_sm_config, false, true, 8);

    pio_sm_set_consecutive_pindirs(_pio, spi_sm, cs_sck_pins, 2, true);
    pio_sm_set_consecutive_pindirs(_pio, spi_sm, data_pins, 1, true);

    pio_sm_init(_pio, spi_sm, spi_offset, &spi_sm_config);
    pio_sm_set_enabled(_pio, spi_sm, true);

    busy_wait_us(150);

    pio_sm_put_blocking(pio0, spi_sm, 0x08000000u);
    pio_sm_put_blocking(pio0, spi_sm, 0x66000000u);

    busy_wait_us(10);

    pio_sm_put_blocking(pio0, spi_sm, 0x08000000u);
    pio_sm_put_blocking(pio0, spi_sm, 0x99000000u);

    busy_wait_us(100);

    pio_sm_put_blocking(pio0, spi_sm, 0x08000000u);
    pio_sm_put_blocking(pio0, spi_sm, 0x35000000u);

    busy_wait_us(150);

    pio_sm_set_enabled(_pio, spi_sm, false);
    pio_remove_program(_pio, &spi_rw_program, spi_offset);
    pio_sm_unclaim(_pio, spi_sm);

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS;

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
    pio_sm_set_consecutive_pindirs(_pio, qspi_sm, data_pins, 4, true);

    hw_set_bits(&_pio->input_sync_bypass, 0xfu << data_pins);

    pio_sm_init(_pio, qspi_sm, qspi_offset, &qspi_sm_config);
    pio_sm_set_enabled(_pio, qspi_sm, true);

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

/*
void write16_old(uint32_t addr, uint16_t data)
{
    buffer[0] = 6*2;
    buffer[1] = 0;

    buffer[2] = 0;
    buffer[3] = 0;

    buffer[4] = (addr >> 16) & 0xFFu;
    buffer[5] = 0x38u;

    buffer[6] = (addr) & 0xFFu;
    buffer[7] = (addr >> 8) & 0xFFu;

    buffer[8] = (data) & 0xFFu;
    buffer[9] = (data >> 8) & 0xFFu;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer, 5);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
}

void write_page_old(uint32_t page_num, const uint8_t* data, const uint16_t data_len)
{
    if(data_len > 1024) return;
    uint16_t size = 1024*2+4*2;
    uint32_t addr = page_num*1024;

    buffer[0] = (size)&(0xFFu);
    buffer[1] = (size>>8)&(0xFFu);

    buffer[2] = 0;
    buffer[3] = 0;

    buffer[4] = (addr >> 16) & 0xFFu;
    buffer[5] = 0x38u;

    buffer[6] = (addr) & 0xFFu;
    buffer[7] = (addr >> 8) & 0xFFu;

    memcpy(buffer+8, data, data_len);
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer, 4+512);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
}
        

uint16_t read16_old(uint32_t addr)
{
    cmd_read_buffer[0] = 4*2;
    cmd_read_buffer[1] = 0;

    cmd_read_buffer[2] = 2*2;
    cmd_read_buffer[3] = 0;

    cmd_read_buffer[4] = (addr >> 16) & 0xFFu;
    cmd_read_buffer[5] = 0xEBu;

    cmd_read_buffer[6] = (addr) & 0xFFu;
    cmd_read_buffer[7] = (addr >> 8) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, cmd_read_buffer, 4);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer, 1);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);

    uint16_t tmp_buf;
    memcpy(&tmp_buf, buffer, sizeof(tmp_buf));
    return tmp_buf;
}

const uint8_t* read_page_old(uint32_t page_num){
    uint16_t size = 1024*2;
    uint32_t addr = page_num*1024;
    
    cmd_read_buffer[0] = 4*2;
    cmd_read_buffer[1] = 0;

    cmd_read_buffer[2] = (size) & (0xFFu);
    cmd_read_buffer[3] = (size>>8) & (0xFFu);

    cmd_read_buffer[4] = (addr >> 16) & 0xFFu;
    cmd_read_buffer[5] = 0xEBu;

    cmd_read_buffer[6] = (addr) & 0xFFu;
    cmd_read_buffer[7] = (addr >> 8) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, cmd_read_buffer, 4);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer, 512);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);

    return buffer;
}
*/

/// @brief Write 1 byte of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write8(uint32_t addr, uint8_t data){
    buffer[0] = 5*2;
    buffer[1] = 0;
    buffer[2] = 0x38u;
    buffer[3] = (addr >> 16) & 0xFFu;
    buffer[4] = (addr >> 8) & 0xFFu;
    buffer[5] = (addr) & 0xFFu;
    buffer[6] = data;
    
    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer, 7);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    busy_wait_at_least_cycles(22);
}

/// @brief Write 2 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write16(uint32_t addr, uint16_t data){
    
}
/// @brief Write 4 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write32(uint32_t addr, uint32_t data){
    
}

/// @brief Write 8 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Data.
void MyQSPI_PSRAM::write64(uint32_t addr, uint64_t data){
    
}

/// @brief Write 64 bytes of data to the psram.
/// @param addr Write address. 
/// @param data Pointer to the data. Make sure it's at least 64 bytes of length. 
void MyQSPI_PSRAM::write512(uint32_t addr, const uint8_t* data){
    
}

/// @brief Write a block of data .
/// @param addr Write address.
/// @param data Pointer to the data buffer.
/// @param data_len Length of the data buffer. 
void MyQSPI_PSRAM::write(uint32_t addr, const uint8_t* data, const uint8_t data_len){
    
}

/// @brief Write a block of data with maximum length of 124 bytes. MAKE SURE data_len is at most 124, if the data_len is bigger stuff may break.
/// @param addr Write address.
/// @param data Pointer to the data buffer.
/// @param data_len Length of the data buffer. 
void MyQSPI_PSRAM::write_limited(uint32_t addr, const uint8_t* data, const uint8_t data_len){
    
}

/// @brief Read 1 byte of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint8_t MyQSPI_PSRAM::read8(uint32_t addr){
    buffer[0] = 4*2;
    buffer[1] = 1*2;

    buffer[2] = 0xEBu;
    
    buffer[3] = (addr >> 16) & 0xFFu;
    buffer[4] = (addr >> 8) & 0xFFu;
    buffer[5] = (addr) & 0xFFu;

    dma_channel_transfer_from_buffer_now(dma_chan_write, buffer, 6);
    dma_channel_transfer_to_buffer_now(dma_chan_read, buffer+6, 1);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
    busy_wait_at_least_cycles(22);
    return buffer[6];
}

/// @brief Read 2 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint16_t MyQSPI_PSRAM::read16(uint32_t addr){
    return 0;
}

/// @brief Read 4 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint32_t MyQSPI_PSRAM::read32(uint32_t addr){
    return 0;
}

/// @brief Read 8 bytes of data from the psram.
/// @param addr Read address. 
/// @param data Data.
uint64_t MyQSPI_PSRAM::read64(uint32_t addr){
    return 0;
}

/// @brief Read 64 bytes of data from the psram.
/// @param addr Read address.
/// @param data Pointer to the read buffer. Make sure it's at least 64 bytes of length
void MyQSPI_PSRAM::read512(uint32_t addr, uint8_t* data){
    
}

/// @brief Read a block of data .
/// @param addr Read address.
/// @param data Pointer to the read buffer.
/// @param data_len Length of the data to be read. 
void MyQSPI_PSRAM::read(uint32_t addr, uint8_t* data, const uint8_t data_len){
    
}

/// @brief Read a block of data with maximum length of 124 bytes. MAKE SURE data_len is at most 124, if the data_len is bigger stuff may break.
/// @param addr Write address.
/// @param data Pointer to the data buffer.
/// @param data_len Length of the data buffer. 
void MyQSPI_PSRAM::read_limited(uint32_t addr, uint8_t* data, const uint8_t data_len){
    
}

uint8_t MyQSPI_PSRAM::find_clock_divisor()
{   
    for(uint32_t i = 2; i < 5 ; i += 2){
        if(SYS_CLK_HZ/i <= PSRAM_MAX_CLOCK) return i;
    }

    return 0;
}

#endif // MY_QSPI_PSRAM_IMPL_H
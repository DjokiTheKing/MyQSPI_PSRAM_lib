# MyQSPI_PSRAM_lib

## Important
This library doesn't take care of respecting the page boundries of psram, so you should make sure not to cross them when using functions that read/write more than 1 byte.

## Tests
### rp2040, SYS_CLK_HZ: 297000000, PSRAM_FREQUENCY: 148500000

Write speed 8bit: 4.10482MB/s.\
Read speed 8bit: 2.67204MB/s.\
Errors: 0

Write speed 16bit: 8.58271MB/s.\
Read speed 16bit: 5.05776MB/s.\
Errors: 0

Write speed 32bit: 15.5190MB/s.\
Read speed 32bit: 10.2992MB/s.\
Errors: 0

Write speed 64bit: 23.1191MB/s.\
Read speed 64bit: 15.5188MB/s.\
Errors: 0

Write speed 512bit: 44.2050MB/s.\
Read speed 512bit: 51.0517MB/s.\
Errors: 2

Write speed 640 byte: 49.8823MB/s.\
Read speed 640 byte: 61.4213MB/s.\
Errors: 4

### rp2350, SYS_CLK_HZ: 297000000, PSRAM_FREQUENCY: 148500000
Write speed 8bit: 6.02627MB/s.\
Read speed 8bit: 3.58528MB/s.\
Errors: 0

Write speed 16bit: 12.0524MB/s.\
Read speed 16bit: 6.66435MB/s.\
Errors: 0

Write speed 32bit: 21.3755MB/s.\
Read speed 32bit: 12.5880MB/s.\
Errors: 0

Write speed 64bit: 33.3186MB/s.\
Read speed 64bit: 20.7868MB/s.\
Errors: 0

Write speed 512bit: 55.0888MB/s.\
Read speed 512bit: 55.2578MB/s.\
Errors: 4

Write speed 640 byte: 55.0212MB/s.\
Read speed 640 byte: 62.6425MB/s.\
Errors: 0

### Some errors popup due to wiring, and this being an overclock of the ram.
What's interesting is that the exact same code can read and write faster on rp2350 than on rp2040.

# BurstSPI
Mbed OS SPI class for driving TFT-LCDs(Using DMA).
Currently supports SPI1 & SPI2 of STM32L4 and STM32H7 MCUs.

Usage:
replace normal Mbed SPI class with BurstSPI class
replace normal Mbed SPI::write function with BurstSPI::fastWrite

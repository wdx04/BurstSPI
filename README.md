# BurstSPI
Mbed OS SPI class for driving TFT-LCDs(Using DMA).
Currently supports SPI1 & SPI2 on STM32F1/STM32F4/STM32F7/STM32L4/STM32L4+/STM32L5/STM32H7/STM32U5 MCUs.

Usage:

1. replace normal Mbed SPI class with BurstSPI class
2. replace normal Mbed SPI::write function with BurstSPI::fastWrite

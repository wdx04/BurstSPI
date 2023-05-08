#ifndef BURSTSPI_H
#define BURSTSPI_H

#include "mbed.h"


/** An SPI Master, used for communicating with SPI slave devices at very high speeds
 *
 * The default mbed SPI class allows for communication via the SPI bus at high clock frequencies,
 * however at these frequencies there is alot of overhead from the mbed code.
 * While this code makes sure your code is alot more robust, it is also relative slow.
 * This library adds to your default SPI commands some extra commands to transmit data rapidly
 * with DMA. Downsides are that currently it is TX only (all RX packets are discarded).
 * Currently only support SPI1 & SPI2 of STM32H7 and STM32L4 MCUs.
 */
class BurstSPI : public SPI
{
public:
    /** Create a SPI master connected to the specified pins
    *  mosi or miso can be specfied as NC if not used
    *
    *  @param mosi SPI Master Out, Slave In pin
    *  @param miso SPI Master In, Slave Out pin
    *  @param sclk SPI Clock pin
    */
    BurstSPI(PinName mosi, PinName miso, PinName sclk);

    /** Release the DMA resources
    */
    ~BurstSPI();

    /** Write to SPI slave using DMA
    *    If blocking is set to false, the user may need to call getFastWriteResult before next fastWrite
    *
    *  @param pData pointer to the data to write
    *  @param size length of data to write, in bytes
    *  @param blocking if true, block current thread until transfer is finished
    */
    bool fastWrite(const char *pData, uint16_t size, bool blocking = true);

    /** Block current thread and get the result of the last fastWrite call
    *    return true if the last non-blocking fastWrite completed successfully
    */
    bool getFastWriteResult();

protected:
    SPI_HandleTypeDef *hspi = nullptr;
    DMA_HandleTypeDef *hdma_tx = nullptr;
    EventFlags *transfer_state = nullptr;
};

#endif

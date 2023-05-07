#include "BurstSPI.h"

enum {
	TRANSFER_WAIT = 0,
	TRANSFER_COMPLETE = 1,
	TRANSFER_ERROR = 2
};

#if defined(TARGET_STM32H7)
#define SPI1_DMA_CLK_Enable() __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI1_DMA_IRQn DMA1_Stream6_IRQn
#define SPI1_DMA_Stream DMA1_Stream6
#define SPI1_DMA_TX_Request DMA_REQUEST_SPI1_TX
#define SPI1_DMA_IRQHandler DMA1_Stream6_IRQHandler
#define SPI2_DMA_CLK_Enable() __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI2_DMA_IRQn DMA1_Stream7_IRQn
#define SPI2_DMA_Stream DMA1_Stream7
#define SPI2_DMA_TX_Request DMA_REQUEST_SPI2_TX
#define SPI2_DMA_IRQHandler DMA1_Stream7_IRQHandler
#elif defined(TARGET_STM32L4)
#define SPI1_DMA_CLK_Enable() __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI1_DMA_IRQn DMA1_Channel3_IRQn
#define SPI1_DMA_Stream DMA1_Channel3
#define SPI1_DMA_TX_Request DMA_REQUEST_1
#define SPI1_DMA_IRQHandler DMA1_Channel3_IRQHandler
#define SPI2_DMA_CLK_Enable() __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI2_DMA_IRQn DMA1_Channel5_IRQn
#define SPI2_DMA_Stream DMA1_Channel5
#define SPI2_DMA_TX_Request DMA_REQUEST_1
#define SPI2_DMA_IRQHandler DMA1_Channel5_IRQHandler
#endif

static SPI_HandleTypeDef *spi1_hspi = nullptr;
static SPI_HandleTypeDef *spi2_hspi = nullptr;
static DMA_HandleTypeDef spi1_hdma_tx;
static DMA_HandleTypeDef spi2_hdma_tx;
static EventFlags spi1_transfer_state;
static EventFlags spi2_transfer_state;

BurstSPI::BurstSPI(PinName mosi, PinName miso, PinName sclk)
    : SPI(mosi, miso, sclk)
{
}

BurstSPI::~BurstSPI()
{
    if(hspi != nullptr)
    {
        if(hspi == spi1_hspi)
        {
            __HAL_RCC_SPI1_FORCE_RESET();
            __HAL_RCC_SPI1_RELEASE_RESET();
            HAL_DMA_DeInit(&spi1_hdma_tx);
            HAL_NVIC_DisableIRQ(SPI1_DMA_IRQn);
            HAL_NVIC_DisableIRQ(SPI1_IRQn);
            hspi = spi1_hspi = nullptr;
        }
        else if(hspi == spi2_hspi)
        {
            __HAL_RCC_SPI2_FORCE_RESET();
            __HAL_RCC_SPI2_RELEASE_RESET();
            HAL_DMA_DeInit(&spi2_hdma_tx);
            HAL_NVIC_DisableIRQ(SPI2_DMA_IRQn);
            HAL_NVIC_DisableIRQ(SPI2_IRQn);
            hspi = spi2_hspi = nullptr;
        }
    }
}

bool BurstSPI::fastWrite(const char *pData, uint16_t size) {
    if(hspi == nullptr)
    {
        struct spi_s *spiobj = (struct spi_s *)(&(_peripheral->spi.spi));
        hspi = &spiobj->handle;
        if(hspi->Instance == SPI1)
        {
            SPI1_DMA_CLK_Enable();
            spi1_hspi = hspi;
            hdma_tx = &spi1_hdma_tx;
            transfer_state = &spi1_transfer_state;
            hdma_tx->Instance = SPI1_DMA_Stream;
            hdma_tx->Init.Request = SPI1_DMA_TX_Request;
        }
        else if(hspi->Instance == SPI2)
        {
            SPI2_DMA_CLK_Enable();
            spi2_hspi = hspi;
            hdma_tx = &spi2_hdma_tx;
            transfer_state = &spi2_transfer_state;
            hdma_tx->Instance = SPI2_DMA_Stream;
            hdma_tx->Init.Request = SPI1_DMA_TX_Request;
        }
#if defined(TARGET_STM32H7)
        hdma_tx->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        hdma_tx->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_tx->Init.MemBurst = DMA_MBURST_SINGLE;
        hdma_tx->Init.PeriphBurst = DMA_MBURST_SINGLE;
#endif
        hdma_tx->Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_tx->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_tx->Init.MemInc = DMA_MINC_ENABLE;
        hdma_tx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_tx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_tx->Init.Mode = DMA_NORMAL;
        hdma_tx->Init.Priority = DMA_PRIORITY_LOW;
        HAL_DMA_Init(hdma_tx);
        __HAL_LINKDMA(hspi, hdmatx, *hdma_tx);
        if(hspi->Instance == SPI1)
        {
            HAL_NVIC_SetPriority(SPI1_DMA_IRQn, 1, 1);
            HAL_NVIC_EnableIRQ(SPI1_DMA_IRQn);
            HAL_NVIC_SetPriority(SPI1_IRQn, 1, 0);
            HAL_NVIC_EnableIRQ(SPI1_IRQn);
        }
        else if(hspi->Instance == SPI2)
        {
            HAL_NVIC_SetPriority(SPI2_DMA_IRQn, 1, 1);
            HAL_NVIC_EnableIRQ(SPI2_DMA_IRQn);
            HAL_NVIC_SetPriority(SPI2_IRQn, 1, 0);
            HAL_NVIC_EnableIRQ(SPI2_IRQn);
        }
    }
    transfer_state->clear();
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    uint32_t alignedAddr = (uint32_t)pData & ~0x1F;
    SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, size + ((uint32_t)pData - alignedAddr));
#endif
    if(HAL_SPI_Transmit_DMA(hspi, (uint8_t*)pData, size) != HAL_OK)
    {
        return false;
    }
    return transfer_state->wait_any(TRANSFER_COMPLETE|TRANSFER_ERROR) == TRANSFER_COMPLETE;
}

extern "C" void SPI1_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&spi1_hdma_tx);
}

extern "C" void SPI2_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&spi2_hdma_tx);
}

extern "C" void SPI1_IRQHandler(void)
{
    HAL_SPI_IRQHandler(spi1_hspi);
}

extern "C" void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(spi2_hspi);
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == spi1_hspi)
    {
        spi1_transfer_state.set(TRANSFER_COMPLETE);
    }
    else if(hspi == spi2_hspi)
    {
        spi2_transfer_state.set(TRANSFER_COMPLETE);
    }
}

extern "C" void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == spi1_hspi)
    {
        spi1_transfer_state.set(TRANSFER_ERROR);
    }
    else if(hspi == spi2_hspi)
    {
        spi2_transfer_state.set(TRANSFER_ERROR);
    }
}

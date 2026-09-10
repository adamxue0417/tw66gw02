#include "test_support.h"
UART_HandleTypeDef huart1, huart2;
ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_usart2_rx;
TypeDefCOM COM1, COM2;
DisplayMode_t DisplayMode;
uint16_t adc_value[2];
int mock_flash_fail;
unsigned mock_flash_erases, mock_flash_writes;
int mock_adc_mode;
const uint8_t *mock_tx_data;
uint16_t mock_tx_length;
unsigned mock_adc_starts, mock_adc_calibrations, mock_adc_stops;
static uint32_t mock_tick;
static DMA_HandleTypeDef mock_adc_dma;
static DMA_Channel_TypeDef mock_adc_channel;

HAL_StatusTypeDef HAL_FLASH_Unlock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Lock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *erase, uint32_t *error)
{
    (void)error;
    if (mock_flash_fail == 1) { return HAL_ERROR; }
    memset((void *)erase->PageAddress, 0xFF, erase->NbPages * 1024u);
    mock_flash_erases++;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type, uint32_t address, uint64_t data)
{
    (void)type;
    if (mock_flash_fail == 2) { return HAL_ERROR; }
    mock_flash_writes++;
    *(uint16_t *)address &= (uint16_t)data;
    if (mock_flash_fail == 3) { *(uint16_t *)address = 0u; }
    return HAL_OK;
}
void MX_ADC_Init(void)
{
    hadc.Instance = ADC1;
    mock_adc_dma.Instance = &mock_adc_channel;
    hadc.DMA_Handle = &mock_adc_dma;
}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc)
{ (void)adc; mock_adc_calibrations++; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *adc, uint32_t *data, uint32_t length)
{
    (void)data; (void)length;
    mock_adc_starts++;
    if (mock_adc_mode == 3) { return HAL_ERROR; }
    HAL_ADC_ConvCpltCallback(adc); /* Exercise completion before start returns. */
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *adc)
{ (void)adc; mock_adc_stops++; return HAL_OK; }
uint32_t HAL_GetTick(void)
{
    if (mock_adc_mode == 0) { HAL_ADC_ConvCpltCallback(&hadc); }
    else if (mock_adc_mode == 1) { HAL_ADC_ErrorCallback(&hadc); }
    return mock_tick++;
}
void HAL_Delay(uint32_t delay) { mock_tick += delay; (void)HAL_GetTick(); }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length, uint32_t timeout)
{ (void)uart; (void)data; (void)length; (void)timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length)
{
    if (uart->gState != HAL_UART_STATE_READY) { return HAL_BUSY; }
    mock_tx_data = data; mock_tx_length = length;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    return HAL_OK;
}
uint16_t COM_TakeRx(TypeDefCOM *com, uint8_t *dest, uint16_t capacity)
{
    uint16_t length = com->rxFlag ? com->rxLen : 0u;
    if (length > capacity) { length = capacity; }
    memcpy(dest, com->rxBuf, length); com->rxFlag = 0u;
    return length;
}
void uart_dma_poll_check(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma, TypeDefCOM *com)
{ (void)uart; (void)dma; (void)com; }
void HAL_GPIO_WritePin(GPIO_TypeDef *gpio, uint16_t pin, GPIO_PinState state)
{ (void)gpio; (void)pin; (void)state; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *gpio, uint16_t pin) { (void)gpio; (void)pin; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *gpio, uint16_t pin) { (void)gpio; (void)pin; return GPIO_PIN_SET; }
uint8_t OtaBoot_CanStartUpdate(void) { return 1u; }

exec(open('outputs/optimization/edit_firmware.py',encoding='utf-8').read().split("s=read('Core/Src/main.c')")[0])
s=read('Core/Src/usart.c')
s=s.replace('uint8_t U2_Data[2];','''uint8_t U2_Data[2];
static uint16_t s_last_pos2;

static HAL_StatusTypeDef StartReceive(UART_HandleTypeDef *uart, TypeDefCOM *com)
{
    HAL_StatusTypeDef status;
    __HAL_UART_ENABLE_IT(uart, UART_IT_IDLE);
    status = HAL_UART_Receive_DMA(uart, com->dmaBuf, COM_RXSIZE);
    if (status == HAL_OK) {
        __HAL_DMA_DISABLE_IT(uart->hdmarx, DMA_IT_HT);
        if (uart == &huart2) { s_last_pos2 = 0u; }
    } else { com->overrunCount++; }
    return status;
}

static void PublishReceive(TypeDefCOM *com, uint16_t length)
{
    if ((length == 0u) || (length > COM_RXSIZE)) { return; }
    if (com->rxFlag != 0u) { com->overrunCount++; return; }
    memcpy(com->rxBuf, com->dmaBuf, length);
    com->rxLen = length;
    __DMB();
    com->rxFlag = 1u;
}''')
s=function(s,'com_init','''void com_init(void)
{
    if ((StartReceive(&huart1, &COM1) != HAL_OK) ||
        (StartReceive(&huart2, &COM2) != HAL_OK)) { Error_Handler(); }
}''')
s=function(s,'_usart_callback','''void _usart_callback(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma, TypeDefCOM *com)
{
    uint32_t remain;
    if ((uart == 0) || (dma == 0) || (com == 0)) { return; }
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_IDLE) == RESET) { return; }
    __HAL_UART_CLEAR_IDLEFLAG(uart);
    /* Stop only RX: HAL_UART_DMAStop would also abort an in-flight TX. */
    if (HAL_UART_AbortReceive(uart) != HAL_OK) { com->overrunCount++; return; }
    remain = __HAL_DMA_GET_COUNTER(dma);
    if (remain <= COM_RXSIZE) { PublishReceive(com, (uint16_t)(COM_RXSIZE - remain)); }
    (void)StartReceive(uart, com);
}''')
s=function(s,'uart_dma_poll_check','''void uart_dma_poll_check(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma, TypeDefCOM *com)
{
    uint32_t mask, remain;
    uint16_t current;
    if ((uart != &huart2) || (dma == 0) || (com != &COM2)) { return; }
    mask = __get_PRIMASK();
    __disable_irq();
    remain = __HAL_DMA_GET_COUNTER(dma);
    if (remain <= COM_RXSIZE) {
        current = (uint16_t)(COM_RXSIZE - remain);
        if ((current != s_last_pos2) && (com->rxFlag == 0u)) {
            /* DMA writes only beyond current; the prefix is stable until the ISR restarts it. */
            PublishReceive(com, current);
            s_last_pos2 = current;
        }
    }
    __set_PRIMASK(mask);
}''')
s=s.replace('    if (length > capacity)', '    if (length > COM_RXSIZE) { length = COM_RXSIZE; }\n    if (length > capacity)')
s=function(s,'HAL_UART_RxCpltCallback','''void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    TypeDefCOM *com = (uart == &huart1) ? &COM1 : ((uart == &huart2) ? &COM2 : 0);
    if (com != 0) {
        PublishReceive(com, COM_RXSIZE);
        (void)StartReceive(uart, com);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    TypeDefCOM *com = (uart == &huart1) ? &COM1 : ((uart == &huart2) ? &COM2 : 0);
    if (com != 0) {
        com->overrunCount++;
        if (HAL_UART_AbortReceive(uart) == HAL_OK) { (void)StartReceive(uart, com); }
    }
}''')
s=s.replace('  HAL_UART_Transmit_DMA(&huart1, data, num);','  if ((data != 0) && (num != 0u)) { (void)HAL_UART_Transmit_DMA(&huart1, data, num); }')
s=s.replace('  HAL_UART_Transmit_DMA(&huart2, data, num);','  (void)USART2_SendData_DMA(data, num);')
write('Core/Src/usart.c',s)

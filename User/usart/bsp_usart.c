#include "./usart/bsp_usart.h"

void USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_AHB1PeriphClockCmd(DEBUG_USART_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(DEBUG_USART_CLK, ENABLE);

    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_PIN;
    GPIO_Init(DEBUG_USART_TX_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_PIN;
    GPIO_Init(DEBUG_USART_RX_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(DEBUG_USART_TX_PORT, DEBUG_USART_TX_SOURCE, DEBUG_USART_TX_AF);
    GPIO_PinAFConfig(DEBUG_USART_RX_PORT, DEBUG_USART_RX_SOURCE, DEBUG_USART_RX_AF);

    USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(DEBUG_USART, &USART_InitStructure);
    USART_Cmd(DEBUG_USART, ENABLE);
}

int fputc(int ch, FILE *f)
{
    USART_SendData(DEBUG_USART, (uint8_t)ch);
    while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET);
    return ch;
}

int fgetc(FILE *f)
{
    while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_RXNE) == RESET);
    return (int)USART_ReceiveData(DEBUG_USART);
}

#ifndef DRIVER_NRF24L01_H
#define DRIVER_NRF24L01_H

#include "main.h"
#include "stdint.h"

/* 数据包宽度，收发双方必须一致 */
#define NRF24L01_TX_PACKET_WIDTH 32
#define NRF24L01_RX_PACKET_WIDTH 32

/* 全局数据包，用户直接读写这两个数组即可 */
extern uint8_t driver_nrf24l01_tx_packet[NRF24L01_TX_PACKET_WIDTH];
extern uint8_t driver_nrf24l01_rx_packet[NRF24L01_RX_PACKET_WIDTH];

/* 收发地址，收发双方必须一致 */
extern uint8_t driver_nrf24l01_tx_address[5];
extern uint8_t driver_nrf24l01_rx_address[5];

/* 引脚宏定义 */
#define NRF_CE_HIGH() HAL_GPIO_WritePin(NRF24L01_CE_GPIO_Port, NRF24L01_CE_Pin, GPIO_PIN_SET)
#define NRF_CE_LOW() HAL_GPIO_WritePin(NRF24L01_CE_GPIO_Port, NRF24L01_CE_Pin, GPIO_PIN_RESET)
#define NRF_CSN_HIGH() HAL_GPIO_WritePin(NRF24L01_CSN_GPIO_Port, NRF24L01_CSN_Pin, GPIO_PIN_SET)
#define NRF_CSN_LOW() HAL_GPIO_WritePin(NRF24L01_CSN_GPIO_Port, NRF24L01_CSN_Pin, GPIO_PIN_RESET)

#define TX_RX_BUFFER_SIZE 128

/* 底层寄存器操作 */
void driver_nrf24l01_write_reg(uint8_t reg, uint8_t value);
uint8_t driver_nrf24l01_read_reg(uint8_t reg);
void driver_nrf24l01_write_regs(uint8_t reg, uint8_t *Data, uint8_t length);
void driver_nrf24l01_read_regs(uint8_t reg, uint8_t *Data, uint8_t length);

/* 功能函数 */
void driver_nrf24l01_init(void);
uint8_t driver_nrf24l01_send_packet(void);    /* 返回值：1=成功，2=MAX_RT，3=状态非法，4=超时 */
uint8_t driver_nrf24l01_receive_packet(void); /* 返回值：1=收到数据，0=无数据，2=状态非法，3=掉电 */

#endif

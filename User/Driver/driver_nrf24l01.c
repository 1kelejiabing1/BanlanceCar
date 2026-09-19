#include "driver_nrf24l01.h"
#include "driver_nrf24l01_define.h"
#include "spi.h"
#include "driver_oled.h"


#define NRF24L01_TX_TIMEOUT (1000)

/* ========== 全局变量 ========== */
static SPI_HandleTypeDef *s_hspi = &hspi1;

static uint8_t tx_buffer[TX_RX_BUFFER_SIZE];
static uint8_t rx_buffer[TX_RX_BUFFER_SIZE];

uint8_t driver_nrf24l01_tx_packet[NRF24L01_TX_PACKET_WIDTH];
uint8_t driver_nrf24l01_rx_packet[NRF24L01_RX_PACKET_WIDTH];

uint8_t driver_nrf24l01_tx_address[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
uint8_t driver_nrf24l01_rx_address[5] = {0x11, 0x22, 0x33, 0x44, 0x55};

/* ========== 底层寄存器操作 ========== */

/**
 * @brief  写单个寄存器
 */
void driver_nrf24l01_write_reg(uint8_t reg, uint8_t value)
{
    tx_buffer[0] = NRF24L01_W_REGISTER | reg;
    tx_buffer[1] = value;
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, 2, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
}

/**
 * @brief  读单个寄存器
 */
uint8_t driver_nrf24l01_read_reg(uint8_t reg)
{
    tx_buffer[0] = NRF24L01_R_REGISTER | reg;
    tx_buffer[1] = NRF24L01_NOP;
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, 2, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
    return rx_buffer[1];
}

/**
 * @brief  写多个寄存器
 */
void driver_nrf24l01_write_regs(uint8_t reg, uint8_t *Data, uint8_t length)
{
    tx_buffer[0] = NRF24L01_W_REGISTER | reg;
    for (uint8_t i = 0; i < length; i++)
    {
        tx_buffer[i + 1] = Data[i];
    }
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, length + 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
}

/**
 * @brief  读多个寄存器
 */
void driver_nrf24l01_read_regs(uint8_t reg, uint8_t *Data, uint8_t length)
{
    tx_buffer[0] = NRF24L01_R_REGISTER | reg;
    for (uint8_t i = 0; i < length; i++)
    {
        tx_buffer[i + 1] = NRF24L01_NOP;
    }
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, length + 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
    for (uint8_t i = 0; i < length; i++)
    {
        Data[i] = rx_buffer[i + 1];
    }
}

/**
 * @brief  写 TX FIFO
 */
static void driver_nrf24l01_write_tx_payload(uint8_t *Data, uint8_t length)
{
    tx_buffer[0] = NRF24L01_W_TX_PAYLOAD;
    for (uint8_t i = 0; i < length; i++)
    {
        tx_buffer[i + 1] = Data[i];
    }
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, length + 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
}

/**
 * @brief  读 RX FIFO
 */
static void driver_nrf24l01_read_rx_payload(uint8_t *Data, uint8_t length)
{
    tx_buffer[0] = NRF24L01_R_RX_PAYLOAD;
    for (uint8_t i = 0; i < length; i++)
    {
        tx_buffer[i + 1] = NRF24L01_NOP;
    }
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, length + 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
    for (uint8_t i = 0; i < length; i++)
    {
        Data[i] = rx_buffer[i + 1];
    }
}

/**
 * @brief  清空 TX FIFO
 */
static void driver_nrf24l01_flush_tx(void)
{
    tx_buffer[0] = NRF24L01_FLUSH_TX;
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
}

/**
 * @brief  清空 RX FIFO
 */
static void driver_nrf24l01_flush_rx(void)
{
    tx_buffer[0] = NRF24L01_FLUSH_RX;
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
}

/**
 * @brief  读状态寄存器（发送 NOP 指令，同时返回 STATUS）
 */
static uint8_t driver_nrf24l01_read_status(void)
{
    tx_buffer[0] = NRF24L01_NOP;
    NRF_CSN_LOW();
    HAL_SPI_TransmitReceive(s_hspi, tx_buffer, rx_buffer, 1, HAL_MAX_DELAY);
    NRF_CSN_HIGH();
    return rx_buffer[0];
}

/* ========== 模式切换 ========== */

/**
 * @brief  进入掉电模式（CE=0, PWR_UP=0）
 */
void driver_nrf24l01_power_down(void)
{
    NRF_CE_LOW();
    uint8_t config = driver_nrf24l01_read_reg(NRF24L01_CONFIG);
    config &= ~0x02;
    driver_nrf24l01_write_reg(NRF24L01_CONFIG, config);
}

/**
 * @brief  进入待机模式1（CE=0, PWR_UP=1）
 */
void driver_nrf24l01_standby_1(void)
{
    NRF_CE_LOW();
    uint8_t config = driver_nrf24l01_read_reg(NRF24L01_CONFIG);
    config |= 0x02;
    driver_nrf24l01_write_reg(NRF24L01_CONFIG, config);
}

/**
 * @brief  进入接收模式（CE=1, PWR_UP=1, PRIM_RX=1）
 */
void driver_nrf24l01_rx_mode(void)
{
    NRF_CE_LOW();
    uint8_t config = driver_nrf24l01_read_reg(NRF24L01_CONFIG);
    config |= 0x03;
    driver_nrf24l01_write_reg(NRF24L01_CONFIG, config);
    NRF_CE_HIGH();
}

/**
 * @brief  进入发送模式（CE=1, PWR_UP=1, PRIM_RX=0）
 */
void driver_nrf24l01_tx_mode(void)
{
    NRF_CE_LOW();
    uint8_t config = driver_nrf24l01_read_reg(NRF24L01_CONFIG);
    config |= 0x02;
    config &= ~0x01;
    driver_nrf24l01_write_reg(NRF24L01_CONFIG, config);
    NRF_CE_HIGH();
}

/* ========== 初始化 ========== */

/**
 * @brief  NRF24L01 初始化
 * @note   调用前确保 MX_GPIO_Init() 和 MX_SPI1_Init() 已完成
 *         MX_SPI1_Init() 必须配置为：Mode 0 (CPOL=0, CPHA=0)、NSS 软件、MSB 在前
 */
void driver_nrf24l01_init(void)
{
    /* 引脚默认状态：CE=0, CSN=1 */
    NRF_CE_LOW();
    NRF_CSN_HIGH();

    /* 以下配置收发双方必须完全一致 */
    driver_nrf24l01_write_reg(NRF24L01_CONFIG, 0x08);     // 不屏蔽中断，使能CRC，1字节CRC，PWR_UP=0
    driver_nrf24l01_write_reg(NRF24L01_EN_AA, 0x3F);      // 使能通道0~5自动应答
    driver_nrf24l01_write_reg(NRF24L01_EN_RXADDR, 0x01);  // 只使能接收通道0
    driver_nrf24l01_write_reg(NRF24L01_SETUP_AW, 0x03);   // 地址宽度5字节
    driver_nrf24l01_write_reg(NRF24L01_SETUP_RETR, 0x03); // 重传间隔250us，重传3次
    driver_nrf24l01_write_reg(NRF24L01_RF_CH, 0x02);      // 射频通道2（2402MHz）
    driver_nrf24l01_write_reg(NRF24L01_RF_SETUP, 0x0E);   // 2Mbps，0dBm

    /* 接收通道0数据包宽度 */
    driver_nrf24l01_write_reg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);

    /* 接收通道0地址 */
    driver_nrf24l01_write_regs(NRF24L01_RX_ADDR_P0, driver_nrf24l01_rx_address, 5);

    /* 清空 FIFO、清所有中断标志位 */
    driver_nrf24l01_flush_tx();
    driver_nrf24l01_flush_rx();
    driver_nrf24l01_write_reg(NRF24L01_STATUS, 0x70);

    /* 进入接收模式 */
    driver_nrf24l01_rx_mode();

    /* 等待芯片稳定 */
    HAL_Delay(5);
}

/* ========== 发送 ========== */

/**
 * @brief  发送一个数据包
 * @retval 1=发送成功，2=达到最大重传（MAX_RT），3=状态非法，4=超时
 * @note   发送前请先修改全局数组 driver_nrf24l01_tx_packet
 */
uint8_t driver_nrf24l01_send_packet(void)
{
    uint8_t status;
    uint8_t send_flag;
    uint32_t timeout = NRF24L01_TX_TIMEOUT;

    /* 1. 配置发送地址和接收通道0地址（自动应答需要 RX_ADDR_P0 = TX_ADDR） */
    driver_nrf24l01_write_regs(NRF24L01_TX_ADDR, driver_nrf24l01_tx_address, 5);
    driver_nrf24l01_write_regs(NRF24L01_RX_ADDR_P0, driver_nrf24l01_tx_address, 5);

    /* 2. 写 TX FIFO */
    driver_nrf24l01_write_tx_payload(driver_nrf24l01_tx_packet, NRF24L01_TX_PACKET_WIDTH);

    /* 3. 进入发送模式，开始发送 */
    driver_nrf24l01_tx_mode();

    /* 4. 轮询状态寄存器，等待发送完成 */
    while (1)
    {
        status = driver_nrf24l01_read_status();

        if (timeout-- == 0)
        {
            send_flag = 4;          // 超时
            driver_nrf24l01_init(); // 重新初始化，从错误中恢复
            break;
        }
        if ((status & 0x30) == 0x30) // MAX_RT 和 TX_DS 同时为1，状态非法
        {
            send_flag = 3;
            driver_nrf24l01_init();
            break;
        }
        else if (status & 0x10) // MAX_RT，达到最大重传
        {
            send_flag = 2;
            driver_nrf24l01_init();
            break;
        }
        else if (status & 0x20) // TX_DS，发送成功
        {
            send_flag = 1;
            break;
        }
    }

    /* 5. 清标志位，清 TX FIFO */
    driver_nrf24l01_write_reg(NRF24L01_STATUS, 0x30);
    driver_nrf24l01_flush_tx();

    /* 6. 恢复接收通道0地址，回到接收模式 */
    driver_nrf24l01_write_regs(NRF24L01_RX_ADDR_P0, driver_nrf24l01_rx_address, 5);
    driver_nrf24l01_rx_mode();

    return send_flag;
}

/* ========== 接收 ========== */

/**
 * @brief  查询是否收到数据包
 * @retval 1=收到数据（数据在 driver_nrf24l01_rx_packet 中），0=无数据，2=状态非法，3=掉电
 */
uint8_t driver_nrf24l01_receive_packet(void)
{
    uint8_t status = driver_nrf24l01_read_status();
    uint8_t config = driver_nrf24l01_read_reg(NRF24L01_CONFIG);

    /* 检查是否掉电 */
    if ((config & 0x02) == 0x00)
    {
        driver_nrf24l01_init();
        return 3;
    }
    /* 状态非法 */
    if ((status & 0x30) == 0x30)
    {
        driver_nrf24l01_init();
        return 2;
    }
    /* 收到数据 */
    if (status & 0x40)
    {
        driver_nrf24l01_read_rx_payload(driver_nrf24l01_rx_packet, NRF24L01_RX_PACKET_WIDTH);
        driver_nrf24l01_write_reg(NRF24L01_STATUS, 0x40); // 清 RX_DR
        driver_nrf24l01_flush_rx();
        return 1;
    }
    return 0;
}

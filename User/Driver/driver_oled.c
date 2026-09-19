/**
 ******************************************************************************
 * @file    driver_oled.c
 * @brief   SSD1306 OLED 驱动（软件模拟 I2C 版本）
 * @note    只需要两个 GPIO 推挽输出即可，无需 I2C 外设
 ******************************************************************************
 */

#include "driver_oled.h"
#include "driver_oled_font.h"
#include "main.h"
#include <string.h>

/* ============================ 显存 ============================ */

/* 显存：8 页 × 128 列，每字节对应一列 8 个像素 */
static uint8_t OLED_GRAM[8][128];

/* ============================ 底层 GPIO 宏 ============================ */
/* 直接操作 BSRR 寄存器，比 HAL_GPIO_WritePin 快得多 */

#define OLED_SCL_H()  (OLED_SCL_GPIO_Port->BSRR = OLED_SCL_Pin)
#define OLED_SCL_L()  (OLED_SCL_GPIO_Port->BSRR = (uint32_t)OLED_SCL_Pin << 16)
#define OLED_SDA_H()  (OLED_SDA_GPIO_Port->BSRR = OLED_SDA_Pin)
#define OLED_SDA_L()  (OLED_SDA_GPIO_Port->BSRR = (uint32_t)OLED_SDA_Pin << 16)

/* 短延时：保证 I2C 时序 */
static inline void OLED_I2C_Delay(void)
{
    for (volatile uint8_t i = 0; i < OLED_I2C_DELAY; i++);
}

/* ============================ 软件 I2C 基础时序 ============================ */

/**
 * @brief  软件 I2C 初始化：SCL / SDA 均拉高（空闲态）
 */
void OLED_I2C_Init(void)
{
    OLED_SCL_H();
    OLED_SDA_H();
}

/**
 * @brief  I2C 起始条件：SCL 高时 SDA 由高变低
 */
static inline void OLED_I2C_Start(void)
{
    OLED_SDA_H();
    OLED_SCL_H();
    OLED_I2C_Delay();
    OLED_SDA_L();
    OLED_I2C_Delay();
    OLED_SCL_L();
    OLED_I2C_Delay();
}

/**
 * @brief  I2C 停止条件：SCL 高时 SDA 由低变高
 */
static inline void OLED_I2C_Stop(void)
{
    OLED_SDA_L();
    OLED_SCL_H();
    OLED_I2C_Delay();
    OLED_SDA_H();
    OLED_I2C_Delay();
}

/**
 * @brief  发送一个字节（MSB 在前），第 9 个时钟不读取 ACK
 */
static inline void OLED_I2C_SendByte(uint8_t Byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (Byte & 0x80) OLED_SDA_H();
        else             OLED_SDA_L();
        Byte <<= 1;

        OLED_I2C_Delay();
        OLED_SCL_H();
        OLED_I2C_Delay();
        OLED_SCL_L();
    }

    /* 第 9 个时钟：忽略从机 ACK */
    OLED_SDA_H();
    OLED_I2C_Delay();
    OLED_SCL_H();
    OLED_I2C_Delay();
    OLED_SCL_L();
    OLED_I2C_Delay();
}

/* ============================ 命令 / 数据 ============================ */

/**
 * @brief  向 OLED 写一条命令
 */
static void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDR);   /* 从机地址 */
    OLED_I2C_SendByte(0x00);            /* 控制字：命令 */
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/**
 * @brief  向 OLED 连续写多个数据字节（一次 Start / Stop，关键提速点）
 * @param  buf 数据缓冲区
 * @param  len 字节数
 */
static void OLED_WriteDataBuf(const uint8_t *buf, uint16_t len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDR);
    OLED_I2C_SendByte(0x40);            /* 控制字：数据 */
    while (len--)
    {
        OLED_I2C_SendByte(*buf++);
    }
    OLED_I2C_Stop();
}

/* ============================ 刷新 ============================ */

/**
 * @brief  把显存整屏刷到 OLED
 * @note   每页一次 Start/Stop，共 8 次，速度比逐字节快 100 倍
 */
void OLED_Refresh(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        /* 设置页地址 */
        OLED_WriteCommand(0xB0 | page);
        /* 设置列地址 = 0（高 4 位 + 低 4 位） */
        OLED_WriteCommand(0x10);
        OLED_WriteCommand(0x00);
        /* 一次写 128 字节 */
        OLED_WriteDataBuf(OLED_GRAM[page], 128);
    }
}

/* ============================ 清屏 / 画点 ============================ */

void OLED_Clear(void)
{
    memset(OLED_GRAM, 0x00, OLED_GRAM_SIZE);
    OLED_Refresh();
}

void OLED_DrawPoint(uint8_t X, uint8_t Y, uint8_t Mode)
{
    if (X >= OLED_WIDTH || Y >= OLED_HEIGHT) return;

    uint8_t page = Y >> 3;      /* Y / 8 */
    uint8_t bit  = Y & 0x07;    /* Y % 8 */

    if (Mode)
        OLED_GRAM[page][X] |=  (uint8_t)(1 << bit);
    else
        OLED_GRAM[page][X] &= (uint8_t)~(1 << bit);
}

void OLED_SetBuffer(const uint8_t *buf)
{
    memcpy(OLED_GRAM, buf, OLED_GRAM_SIZE);
}

/* ============================ 字符 ============================ */

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    if (Line < 1 || Line > 4 || Column < 1 || Column > 16) return;
    if (Char < ' ' || Char > '~') Char = ' ';

    uint8_t page = (uint8_t)((Line - 1) * 2);
    uint8_t col  = (uint8_t)((Column - 1) * 8);
    uint8_t idx  = (uint8_t)(Char - ' ');

    for (uint8_t i = 0; i < 8; i++)
        OLED_GRAM[page][col + i] = OLED_F8x16[idx][i];
    for (uint8_t i = 0; i < 8; i++)
        OLED_GRAM[page + 1][col + i] = OLED_F8x16[idx][i + 8];
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
    {
        if (Column + i > 16) break;
        OLED_ShowChar(Line, (uint8_t)(Column + i), String[i]);
    }
}

/* ============================ 次方工具 ============================ */

static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t result = 1;
    while (Y--) result *= X;
    return result;
}

/* ============================ 数字 ============================ */

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    if (Length == 0 || Length > 10) return;
    for (uint8_t i = 0; i < Length; i++)
    {
        if (Column + i > 16) break;
        OLED_ShowChar(Line, (uint8_t)(Column + i),
                      (char)(Number / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    if (Length == 0 || Length > 10) return;

    uint32_t n;
    if (Number >= 0) { OLED_ShowChar(Line, Column, '+'); n = (uint32_t)Number; }
    else             { OLED_ShowChar(Line, Column, '-'); n = (uint32_t)(-Number); }

    for (uint8_t i = 0; i < Length; i++)
    {
        if (Column + i + 1 > 16) break;
        OLED_ShowChar(Line, (uint8_t)(Column + i + 1),
                      (char)(n / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    if (Length == 0 || Length > 8) return;
    for (uint8_t i = 0; i < Length; i++)
    {
        if (Column + i > 16) break;
        uint8_t s = (uint8_t)(Number / OLED_Pow(16, Length - i - 1) % 16);
        OLED_ShowChar(Line, (uint8_t)(Column + i),
                      (char)(s < 10 ? s + '0' : s - 10 + 'A'));
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    if (Length == 0 || Length > 16) return;
    for (uint8_t i = 0; i < Length; i++)
    {
        if (Column + i > 16) break;
        OLED_ShowChar(Line, (uint8_t)(Column + i),
                      (char)(Number / OLED_Pow(2, Length - i - 1) % 2 + '0'));
    }
}

/* ============================ 浮点数 ============================ */

void OLED_ShowFloat(uint8_t Line, uint8_t Column, double Number,
                    uint8_t IntLen, uint8_t FracLen)
{
    if (IntLen == 0 || IntLen > 10 || FracLen > 6) return;

    uint8_t col = Column;

    /* 符号位 */
    if (Number < 0)
    {
        OLED_ShowChar(Line, col++, '-');
        Number = -Number;
    }
    else
    {
        OLED_ShowChar(Line, col++, '+');
    }

    /* 四舍五入到指定小数位 */
    double scale = 1.0;
    for (uint8_t i = 0; i < FracLen; i++) scale *= 10.0;

    uint64_t total = (uint64_t)(Number * scale + 0.5);
    uint64_t ipart = total / (uint64_t)scale;
    uint64_t fpart = total % (uint64_t)scale;

    /* 整数部分 */
    for (uint8_t i = 0; i < IntLen; i++)
    {
        if (col > 16) return;
        OLED_ShowChar(Line, col++,
                      (char)(ipart / OLED_Pow(10, IntLen - i - 1) % 10 + '0'));
    }

    /* 小数点 + 小数部分 */
    if (FracLen > 0)
    {
        if (col > 16) return;
        OLED_ShowChar(Line, col++, '.');

        for (uint8_t i = 0; i < FracLen; i++)
        {
            if (col > 16) return;
            OLED_ShowChar(Line, col++,
                          (char)(fpart / OLED_Pow(10, FracLen - i - 1) % 10 + '0'));
        }
    }
}

/* ============================ 中文 ============================ */

/**
 * @brief  根据 UTF-8 编码在字库中查找 16x16 汉字点阵
 */
static const uint8_t *OLED_GetChineseFont(const char *utf8)
{
    extern const OLED_ChineseFont OLED_CF16x16[];
    extern const uint16_t OLED_CF16x16_Count;

    for (uint16_t i = 0; i < OLED_CF16x16_Count; i++)
    {
        if (strncmp(utf8, OLED_CF16x16[i].utf8, 3) == 0)
            return OLED_CF16x16[i].data;
    }
    return NULL;
}

void OLED_ShowChinese(uint8_t Line, uint8_t Column, const char *Chinese)
{
    if (Line < 1 || Line > 4 || Column < 1 || Column > 16) return;

    const uint8_t *font = OLED_GetChineseFont(Chinese);
    if (font == NULL) return;

    uint8_t page = (uint8_t)((Line - 1) * 2);
    uint8_t col  = (uint8_t)((Column - 1) * 8);

    for (uint8_t i = 0; i < 16; i++)
        OLED_GRAM[page][col + i] = font[i];
    for (uint8_t i = 0; i < 16; i++)
        OLED_GRAM[page + 1][col + i] = font[i + 16];
}

void OLED_ShowChineseString(uint8_t Line, uint8_t Column, const char *String)
{
    uint8_t col = Column;
    const char *p = String;

    while (*p)
    {
        uint8_t len;
        if ((*p & 0x80) == 0x00)       len = 1;   /* ASCII */
        else if ((*p & 0xE0) == 0xC0)  len = 2;   /* 2 字节 UTF-8 */
        else if ((*p & 0xF0) == 0xE0)  len = 3;   /* 3 字节 UTF-8（汉字） */
        else                           len = 4;   /* 4 字节 UTF-8 */

        if (len == 3)
        {
            if (col + 1 > 16) break;
            OLED_ShowChinese(Line, col, p);
            col += 2;
        }
        else if (len == 1)
        {
            if (col > 16) break;
            OLED_ShowChar(Line, col, *p);
            col += 1;
        }
        p += len;
    }
}

/* ============================ 初始化 ============================ */

void OLED_Init(void)
{
    /* 上电延时，约 100ms @ 72MHz */
    for (volatile uint32_t i = 0; i < 800000; i++);

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE);    /* 关闭显示 */

    OLED_WriteCommand(0xD5);    /* 时钟分频 / 振荡频率 */
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);    /* 多路复用率 = 64 */
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);    /* 显示偏移 = 0 */
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);    /* 显示起始行 = 0 */

    OLED_WriteCommand(0xA1);    /* 左右方向：0xA1 正常，0xA0 反置 */
    OLED_WriteCommand(0xC8);    /* 上下方向：0xC8 正常，0xC0 反置 */

    OLED_WriteCommand(0xDA);    /* COM 引脚硬件配置 */
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);    /* 对比度 */
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);    /* 预充电周期 */
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    /* VCOMH 电压 */
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);    /* 显示跟随 RAM 内容 */
    OLED_WriteCommand(0xA6);    /* 正常显示（0xA7 反色） */

    OLED_WriteCommand(0x8D);    /* 充电泵 */
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);    /* 打开显示 */

    OLED_Clear();
}
/**
 ******************************************************************************
 * @file    driver_oled_font.h
 * @brief   OLED 字库：8x16 ASCII + 16x16 中文示例
 ******************************************************************************
 */

#ifndef __DRIVER_OLED_FONT_H
#define __DRIVER_OLED_FONT_H

#include <stdint.h>

/* ============================ 8x16 ASCII 字库 ============================ */

/* 覆盖 0x20(空格) ~ 0x7E(~)，共 95 个字符，每个 16 字节 */
extern const uint8_t OLED_F8x16[95][16];

/* ============================ 16x16 中文字库 ============================ */

typedef struct
{
    char    utf8[4];    /* UTF-8 编码，最多 3 字节 + '\0' */
    uint8_t data[32];   /* 16x16 点阵 */
} OLED_ChineseFont;

extern const OLED_ChineseFont OLED_CF16x16[];
extern const uint16_t OLED_CF16x16_Count;

#endif /* __DRIVER_OLED_FONT_H */
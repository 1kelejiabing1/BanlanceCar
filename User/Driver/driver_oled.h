/**
 ******************************************************************************
 * @file    driver_oled.h
 * @brief   SSD1306 OLED 驱动（软件模拟 I2C 版本）
 *          支持：字符 / 字符串 / 整数 / 有符号数 / 十六进制 / 二进制 /
 *                浮点数 / 中文
 ******************************************************************************
 */

#ifndef __DRIVER_OLED_H
#define __DRIVER_OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ============================ 引脚配置 ============================ */
/* OLED 引脚宏（OLED_SCL_Pin / OLED_SDA_Pin 等）由 CubeMX 在 main.h 中定义 */
#include "main.h"

/* ============================ 常量 ============================ */

#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_GRAM_SIZE      1024
#define OLED_I2C_ADDR       0x78    /* SSD1306 8 位从机地址 */

/* 软件 I2C 延时（单位：空循环次数）
   如果屏幕显示不正常，把这个值调大；
   如果觉得慢，把这个值调小（一般 0~5 都可以）。 */
#define OLED_I2C_DELAY      2

/* ============================================================================
 * ============================ 使用总说明 ====================================
 * ============================================================================
 *
 * 【重要】本驱动采用"显存缓冲 + 一次性刷新"架构：
 *   1. 所有 OLED_ShowXxx() 函数只把内容写入 MCU 内存中的显存（GRAM），
 *      并不会立即显示到屏幕上；
 *   2. 必须调用一次 OLED_Refresh()，才会把整屏内容一次性刷到 OLED 上；
 *   3. 这样做的目的是把 N 次 I2C 传输合并成 8 次，速度提升约 100 倍。
 *
 * 【典型用法】
 *
 *   OLED_Init();                          // 上电初始化一次
 *
 *   while (1)
 *   {
 *       OLED_Clear();                     // 清显存（可选）
 *       OLED_ShowString(1, 1, "Hello");   // 第 1 行显示字符串
 *       OLED_ShowFloat(2, 1, 3.14, 1, 2); // 第 2 行显示 +3.14
 *       OLED_ShowNum(3, 1, 12345, 5);     // 第 3 行显示 12345
 *       OLED_ShowChineseString(4, 1, "温度:25.6C"); // 第 4 行中英混排
 *       OLED_Refresh();                   // 关键：一次性上屏
 *       HAL_Delay(100);
 *   }
 *
 * 【坐标系统】
 *   - Line  : 行号，1 ~ 4（共 4 行，每行 16 像素高）
 *   - Column: 列号，1 ~ 16（共 16 列，每列 8 像素宽）
 *   - 屏幕总分辨率：128 x 64
 *
 * 【中文字库说明】
 *   中文使用 16x16 点阵，一个字占 2 行 x 2 列；
 *   需在 driver_oled_font.c 中提供 OLED_CF16x16 字库，
 *   且源文件编码必须为 UTF-8。
 *
 * ============================================================================
 */

/* ============================================================================
 * ============================ 初始化与刷新 ==================================
 * ============================================================================ */

/**
 * @brief  OLED 初始化
 *
 * @note   在 main() 中 HAL_Init()、SystemClock_Config()、MX_GPIO_Init()
 *         之后调用一次即可。
 *         内部包含：约 100ms 上电延时、SSD1306 寄存器配置、清屏。
 *
 * @example
 *     int main(void)
 *     {
 *         HAL_Init();
 *         SystemClock_Config();
 *         MX_GPIO_Init();
 *         OLED_Init();          // <-- 在这里调用
 *         ...
 *     }
 */
void OLED_Init(void);

/**
 * @brief  清空显存（仅操作 MCU 内存，不自动上屏）
 *
 * @note   把整块显存填 0，相当于"清屏"。
 *         调用后需要再调用 OLED_Refresh() 才会真正清空屏幕。
 *
 * @example
 *     OLED_Clear();      // 清显存
 *     OLED_Refresh();    // 上屏（屏幕变全黑）
 *
 *     或者：
 *     OLED_Clear();
 *     OLED_ShowString(1, 1, "New Page");
 *     OLED_Refresh();    // 一次性显示"清除旧内容 + 新内容"
 */
void OLED_Clear(void);

/**
 * @brief  把显存内容整屏刷新到 OLED（软件 I2C 阻塞方式）
 *
 * @note   这是本驱动最关键的提速点：每页只做一次 Start/Stop，
 *         整屏 1024 字节分 8 页传输完成。
 *         函数返回时数据已经写完（阻塞），无需等待。
 *
 * @warning 每次修改完显示内容后必须调用，否则屏幕不会变化。
 *
 * @example
 *     OLED_ShowString(1, 1, "Hello");
 *     OLED_ShowNum(2, 1, 123, 3);
 *     OLED_Refresh();    // 统一上屏
 */
void OLED_Refresh(void);

/* ============================================================================
 * ============================ 字符 / 字符串 =================================
 * ============================================================================ */

/**
 * @brief  在指定位置显示一个 ASCII 字符（8x16 字体）
 *
 * @param  Line   行号，1 ~ 4
 * @param  Column 列号，1 ~ 16
 * @param  Char   要显示的字符，范围 0x20(空格) ~ 0x7E(~)
 *                超出范围的字符会自动用空格代替
 *
 * @note   只写显存，需配合 OLED_Refresh() 上屏。
 *
 * @example
 *     OLED_ShowChar(1, 1, 'A');      // 第 1 行第 1 列显示 'A'
 *     OLED_ShowChar(2, 5, '5');      // 第 2 行第 5 列显示 '5'
 *     OLED_Refresh();
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/**
 * @brief  在指定位置显示一个字符串（8x16 字体）
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16
 * @param  String 以 '\0' 结尾的字符串
 *
 * @note   超出屏幕右边界的内容会被自动截断，不会越界写显存。
 *         只写显存，需配合 OLED_Refresh() 上屏。
 *
 * @example
 *     OLED_ShowString(1, 1, "Hello World");   // 从第 1 行第 1 列开始
 *     OLED_ShowString(2, 3, "OK");            // 从第 2 行第 3 列开始
 *     OLED_ShowString(3, 1, "123.456");       // 也可以显示数字字符串
 *     OLED_Refresh();
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/* ============================================================================
 * ============================ 数字显示 ======================================
 * ============================================================================ */

/**
 * @brief  显示无符号十进制整数（自动补 0 对齐）
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16
 * @param  Number 要显示的数值，范围 0 ~ 4294967295
 * @param  Length 显示位数，1 ~ 10（不足前面自动补 0）
 *
 * @example
 *     OLED_ShowNum(1, 1, 123, 5);      // 显示 "00123"
 *     OLED_ShowNum(2, 1, 4567, 4);     // 显示 "4567"
 *     OLED_ShowNum(3, 1, 5, 1);        // 显示 "5"
 *     OLED_ShowNum(4, 1, 12345, 3);    // 显示 "345"（只取低 3 位）
 *     OLED_Refresh();
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  显示有符号十进制整数（带 +/- 符号）
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16（符号占 1 列，数字从下一列开始）
 * @param  Number 要显示的数值，范围 -2147483648 ~ 2147483647
 * @param  Length 数字部分显示位数（不含符号），1 ~ 10
 *
 * @note   正数会显示 '+' 号；负数显示 '-' 号。
 *
 * @example
 *     OLED_ShowSignedNum(1, 1,  123, 3);   // 显示 "+123"
 *     OLED_ShowSignedNum(2, 1, -456, 3);   // 显示 "-456"
 *     OLED_ShowSignedNum(3, 1,    0, 1);   // 显示 "+0"
 *     OLED_Refresh();
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/**
 * @brief  显示十六进制数（大写字母 A~F）
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16
 * @param  Number 要显示的数值
 * @param  Length 显示位数，1 ~ 8（不足前面补 0）
 *
 * @example
 *     OLED_ShowHexNum(1, 1, 0xAB, 2);       // 显示 "AB"
 *     OLED_ShowHexNum(2, 1, 0x1234, 4);     // 显示 "1234"
 *     OLED_ShowHexNum(3, 1, 0x5, 2);        // 显示 "05"
 *     OLED_ShowHexNum(4, 1, 0xDEADBEEF, 8); // 显示 "DEADBEEF"
 *     OLED_Refresh();
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  显示二进制数
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16
 * @param  Number 要显示的数值
 * @param  Length 显示位数，1 ~ 16（不足前面补 0）
 *
 * @note   16 位二进制会占满一整行（16 列）。
 *
 * @example
 *     OLED_ShowBinNum(1, 1, 0x05, 8);       // 显示 "00000101"
 *     OLED_ShowBinNum(2, 1, 0xFFFF, 16);    // 显示 "1111111111111111"
 *     OLED_ShowBinNum(3, 1, 0x01, 4);       // 显示 "0001"
 *     OLED_Refresh();
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/* ============================================================================
 * ============================ 浮点数显示 ====================================
 * ============================================================================ */

/**
 * @brief  显示浮点数（带符号 + 四舍五入）
 *
 * @param  Line    起始行号，1 ~ 4
 * @param  Column  起始列号，1 ~ 16
 * @param  Number  要显示的浮点数（double 类型）
 * @param  IntLen  整数部分显示位数（不含符号位），1 ~ 10
 * @param  FracLen 小数部分显示位数，0 ~ 6（为 0 时不显示小数点）
 *
 * @note   采用四舍五入到指定小数位。
 *         正数显示 '+'，负数显示 '-'。
 *
 * @example
 *     // 显示 +3.14（整数位 1 位，小数位 2 位）
 *     OLED_ShowFloat(1, 1, 3.14159, 1, 2);     // "+3.14"
 *
 *     // 显示 -0.5
 *     OLED_ShowFloat(2, 1, -0.5, 1, 1);        // "-0.5"
 *
 *     // 显示 +123.456
 *     OLED_ShowFloat(3, 1, 123.456, 3, 3);     // "+123.456"
 *
 *     // 不显示小数部分（相当于取整显示）
 *     OLED_ShowFloat(4, 1, 42.7, 2, 0);        // "+43"（四舍五入）
 *
 *     OLED_Refresh();
 */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, double Number,
                    uint8_t IntLen, uint8_t FracLen);

/* ============================================================================
 * ============================ 中文显示 ======================================
 * ============================================================================ */

/**
 * @brief  在指定位置显示一个 16x16 汉字
 *
 * @param  Line    行号，1 ~ 4（汉字占 2 行，Line 用完 1、3 或 2、4）
 * @param  Column  列号，1 ~ 16（汉字占 2 列）
 * @param  Chinese 指向该汉字 UTF-8 编码的指针（3 字节）
 *
 * @note   需要在 driver_oled_font.c 中的 OLED_CF16x16 字库里
 *         存在该汉字的点阵；找不到时静默跳过，不显示。
 *         源文件必须为 UTF-8 编码，否则匹配会失败。
 *
 * @warning 一般不建议直接调用此函数，因为它不会自动移动指针；
 *          显示多个汉字请使用 OLED_ShowChineseString()。
 *
 * @example
 *     // 显示一个字"温"（UTF-8 编码 3 字节）
 *     OLED_ShowChinese(1, 1, "温");
 *     OLED_Refresh();
 */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, const char *Chinese);

/**
 * @brief  显示中英文混合字符串（自动处理 UTF-8 和 ASCII）
 *
 * @param  Line   起始行号，1 ~ 4
 * @param  Column 起始列号，1 ~ 16
 * @param  String UTF-8 编码的字符串
 *
 * @note   函数会自动识别每个字符：
 *         - ASCII 字符  ：占 1 列
 *         - 3 字节 UTF-8（汉字）：占 2 列
 *         字库里没有的汉字会被跳过（占位但不显示内容）。
 *
 * @example
 *     OLED_ShowChineseString(1, 1, "温度:25.6C");
 *     // 显示：温(2列) 度(2列) :(1列) 2(1列) 5(1列) .(1列) 6(1列) C(1列)
 *
 *     OLED_ShowChineseString(2, 1, "电压:3.30V");
 *     OLED_ShowChineseString(3, 1, "Hello 世界");
 *     OLED_ShowChineseString(4, 1, "电流:0.25A");
 *     OLED_Refresh();
 */
void OLED_ShowChineseString(uint8_t Line, uint8_t Column, const char *String);

/* ============================================================================
 * ============================ 图形接口 ======================================
 * ============================================================================ */

/**
 * @brief  在显存中画一个点
 *
 * @param  X    横坐标，0 ~ 127（注意：不是列号，是像素坐标）
 * @param  Y    纵坐标，0 ~ 63 （注意：不是行号，是像素坐标）
 * @param  Mode 1 = 点亮，0 = 熄灭
 *
 * @note   坐标越界时函数会直接返回，不会写坏显存。
 *         需要配合 OLED_Refresh() 上屏。
 *         适合绘制自定义图形、波形、进度条等。
 *
 * @example
 *     // 在屏幕左上角画一个亮点
 *     OLED_DrawPoint(0, 0, 1);
 *
 *     // 画一条对角线
 *     for (uint8_t i = 0; i < 64; i++)
 *         OLED_DrawPoint(i, i, 1);
 *
 *     // 熄灭某个点
 *     OLED_DrawPoint(10, 10, 0);
 *
 *     OLED_Refresh();
 */
void OLED_DrawPoint(uint8_t X, uint8_t Y, uint8_t Mode);

/**
 * @brief  把外部 1024 字节缓冲区直接拷贝到显存
 *
 * @param  buf 指向 1024 字节图像数据的指针
 *
 * @note   数据布局：[page0 128字节][page1 128字节]...[page7 128字节]，
 *         与 SSD1306 显存布局一致。
 *         适合从 Flash/SD 卡加载整屏位图后一次性显示。
 *         需要配合 OLED_Refresh() 上屏。
 *
 * @example
 *     extern const uint8_t logo[1024];   // 你的位图数据
 *     OLED_SetBuffer(logo);
 *     OLED_Refresh();
 */
void OLED_SetBuffer(const uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_OLED_H */
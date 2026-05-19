#ifndef __SSD1306_H
#define __SSD1306_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------
 * 屏幕参数（0.96寸 128×64）
 * 使用 8×16 字体：4行，每行16字符
 * ------------------------------------------------------- */
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       8       /* 64 / 8 = 8 页 */

#define SSD1306_FONT_W      8       /* 字符宽度（像素） */
#define SSD1306_FONT_H      16      /* 字符高度（像素，占 2 页）*/
#define SSD1306_MAX_COL     16      /* 每行最多字符数 128/8=16 */
#define SSD1306_MAX_ROW     4       /* 共 4 行 64/16=4 */

/* I2C 设备地址 */
#define SSD1306_I2C_ADDR    (0x3C << 1)
#define SSD1306_I2C_TIMEOUT  100

/* 颜色 */
#define SSD1306_BLACK        0
#define SSD1306_WHITE        1

/* 函数声明 */
uint8_t SSD1306_Init(void);
void    SSD1306_Clear(void);
void    SSD1306_Fill(uint8_t color);
void    SSD1306_UpdateScreen(void);

void    SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void    SSD1306_DrawChar(uint8_t col, uint8_t row,
                         uint8_t ch, uint8_t color);
void    SSD1306_DrawString(uint8_t col, uint8_t row,
                           const char *str, uint8_t color);

/* 整行操作 */
void    SSD1306_ClearRow(uint8_t row);
void    SSD1306_PrintRow(uint8_t row, const char *str);
void    SSD1306_PrintRowInvert(uint8_t row, const char *str);

/* 矩形 */
void    SSD1306_FillRect(uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, uint8_t color);
void    SSD1306_DrawLine(uint8_t x1, uint8_t y1,
                         uint8_t x2, uint8_t y2, uint8_t color);

/* 工具 */
uint8_t SSD1306_I2C_Scan(void);

#endif /* __SSD1306_H */

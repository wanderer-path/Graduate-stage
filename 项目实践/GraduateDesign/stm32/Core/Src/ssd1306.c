#include "ssd1306.h"
#include "ssd1306_font8x16.h"
#include "i2c.h"
#include "debug.h"
#include <stdio.h>

/* 显存缓冲区：128 × 8页 = 1024 字节 */
static uint8_t ssd1306_buf[SSD1306_WIDTH * SSD1306_PAGES];


/* -------------------------------------------------------
 * 底层 I2C 发送
 * ------------------------------------------------------- */

static void SSD1306_SendCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR,
                             buf, 2, SSD1306_I2C_TIMEOUT);
}


/* -------------------------------------------------------
 * 初始化
 * ------------------------------------------------------- */

uint8_t SSD1306_Init(void)
{
    HAL_Delay(100);

    if (!SSD1306_I2C_Scan()) return 0;

    SSD1306_SendCmd(0xAE); /* Display OFF */
    SSD1306_SendCmd(0x20);
    SSD1306_SendCmd(0x00); /* 水平寻址模式 */
    SSD1306_SendCmd(0xB0);
    SSD1306_SendCmd(0xC8); /* COM 反向扫描 */
    SSD1306_SendCmd(0x00);
    SSD1306_SendCmd(0x10);
    SSD1306_SendCmd(0x40);
    SSD1306_SendCmd(0x81);
    SSD1306_SendCmd(0xFF); /* 最大对比度 */
    SSD1306_SendCmd(0xA1); /* 列映射反向 */
    SSD1306_SendCmd(0xA6); /* 正常显示 */
    SSD1306_SendCmd(0xA8);
    SSD1306_SendCmd(0x3F); /* 64 行 */
    SSD1306_SendCmd(0xA4);
    SSD1306_SendCmd(0xD3);
    SSD1306_SendCmd(0x00);
    SSD1306_SendCmd(0xD5);
    SSD1306_SendCmd(0xF0);
    SSD1306_SendCmd(0xD9);
    SSD1306_SendCmd(0x22);
    SSD1306_SendCmd(0xDA);
    SSD1306_SendCmd(0x12);
    SSD1306_SendCmd(0xDB);
    SSD1306_SendCmd(0x20);
    SSD1306_SendCmd(0x8D);
    SSD1306_SendCmd(0x14); /* 电荷泵使能 */
    SSD1306_SendCmd(0xAF); /* Display ON */

    SSD1306_Clear();
    SSD1306_UpdateScreen();

    DEBUG_INFO("SSD1306 init OK, 128x64, 8x16 font, 4 rows x 16 cols.");
    return 1;
}


/* -------------------------------------------------------
 * 显存操作
 * ------------------------------------------------------- */

void SSD1306_Clear(void)
{
    memset(ssd1306_buf, 0x00, sizeof(ssd1306_buf));
}

void SSD1306_Fill(uint8_t color)
{
    memset(ssd1306_buf, color ? 0xFF : 0x00, sizeof(ssd1306_buf));
}

/**
 * @brief  将显存刷新到屏幕
 *         修复：命令和数据合并进单次 I2C 传输，避免截断
 */
void SSD1306_UpdateScreen(void)
{
    /* 整页传输缓冲：1字节控制头 + 128字节数据 */
    static uint8_t tx_buf[SSD1306_WIDTH + 1];
    tx_buf[0] = 0x40;    /* 数据模式控制字节，固定 */

    for (uint8_t page = 0; page < SSD1306_PAGES; page++)
    {
        /* 发送页寻址命令（3条命令合并发送） */
        uint8_t cmd_buf[4] = {
            0x00,            /* 命令模式 */
            0xB0 | page,     /* 页地址 */
            0x00,            /* 列低位 = 0 */
            0x10             /* 列高位 = 0 */
        };
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR,
                                 cmd_buf, 4,
                                 SSD1306_I2C_TIMEOUT);

        /* 发送该页 128 字节显存数据 */
        memcpy(&tx_buf[1], &ssd1306_buf[page * SSD1306_WIDTH],
               SSD1306_WIDTH);
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR,
                                 tx_buf, SSD1306_WIDTH + 1,
                                 SSD1306_I2C_TIMEOUT);
    }
}


/* -------------------------------------------------------
 * 像素操作
 * ------------------------------------------------------- */

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    uint16_t idx = x + (y / 8) * SSD1306_WIDTH;
    if (color)
        ssd1306_buf[idx] |=  (1 << (y % 8));
    else
        ssd1306_buf[idx] &= ~(1 << (y % 8));
}


/* -------------------------------------------------------
 * 8×16 字符绘制
 * ------------------------------------------------------- */

/**
 * @brief  在指定行列绘制单个 8×16 字符
 * @param  col    列号 0~15
 * @param  row    行号 0~3
 * @param  ch     ASCII 字符
 * @param  color  SSD1306_WHITE 正常  SSD1306_BLACK 反色
 */
void SSD1306_DrawChar(uint8_t col, uint8_t row,
                      uint8_t ch, uint8_t color)
{
    if (col >= SSD1306_MAX_COL || row >= SSD1306_MAX_ROW) return;
    if (ch < 0x20 || ch > 0x7F) ch = 0x3F; /* 未知字符显示 ? */

    const uint8_t *glyph  = Font8x16[ch - 0x20];
    uint8_t        x0     = col * SSD1306_FONT_W;   /* 起始像素列 */
    uint8_t        page_t = row * 2;                /* 上半页号   */
    uint8_t        page_b = row * 2 + 1;            /* 下半页号   */

    for (uint8_t i = 0; i < SSD1306_FONT_W; i++)
    {
        uint8_t top = glyph[i];               /* 上半部分 8bit */
        uint8_t bot = glyph[i + SSD1306_FONT_W]; /* 下半部分 8bit */

        if (color == SSD1306_BLACK)
        {
            top = ~top;
            bot = ~bot;
        }

        ssd1306_buf[(uint16_t)page_t * SSD1306_WIDTH + x0 + i] = top;
        ssd1306_buf[(uint16_t)page_b * SSD1306_WIDTH + x0 + i] = bot;
    }
}

/**
 * @brief  从指定行列开始绘制字符串
 * @param  col  起始列 0~15
 * @param  row  行号 0~3
 * @param  str  字符串
 * @param  color SSD1306_WHITE / SSD1306_BLACK
 */
void SSD1306_DrawString(uint8_t col, uint8_t row,
                        const char *str, uint8_t color)
{
    while (*str && col < SSD1306_MAX_COL)
    {
        SSD1306_DrawChar(col, row, (uint8_t)*str, color);
        col++;
        str++;
    }
}


/* -------------------------------------------------------
 * 整行操作（最常用接口）
 * ------------------------------------------------------- */

/**
 * @brief  清空指定行（填充黑色）
 */
void SSD1306_ClearRow(uint8_t row)
{
    if (row >= SSD1306_MAX_ROW) return;
    uint8_t page_top = row * 2;
    uint8_t page_bot = row * 2 + 1;
    memset(&ssd1306_buf[page_top * SSD1306_WIDTH], 0x00, SSD1306_WIDTH);
    memset(&ssd1306_buf[page_bot * SSD1306_WIDTH], 0x00, SSD1306_WIDTH);
}

/**
 * @brief  在指定行打印字符串（先清行再写入）
 * @param  row  行号 0~3
 * @param  str  字符串，超过 16 字符自动截断
 */
void SSD1306_PrintRow(uint8_t row, const char *str)
{
    SSD1306_ClearRow(row);
    SSD1306_DrawString(0, row, str, SSD1306_WHITE);
}

/**
 * @brief  在指定行打印字符串（反色：白底黑字，用于高亮报警行）
 * @param  row  行号 0~3
 * @param  str  字符串
 */
void SSD1306_PrintRowInvert(uint8_t row, const char *str)
{
    /* 先将整行填充白色 */
    uint8_t page_top = row * 2;
    uint8_t page_bot = row * 2 + 1;
    memset(&ssd1306_buf[page_top * SSD1306_WIDTH], 0xFF, SSD1306_WIDTH);
    memset(&ssd1306_buf[page_bot * SSD1306_WIDTH], 0xFF, SSD1306_WIDTH);
    /* 再用黑色字写入 */
    SSD1306_DrawString(0, row, str, SSD1306_BLACK);
}


/* -------------------------------------------------------
 * 图形
 * ------------------------------------------------------- */

void SSD1306_FillRect(uint8_t x, uint8_t y,
                      uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t row = y; row < y + h && row < SSD1306_HEIGHT; row++)
        for (uint8_t col = x; col < x + w && col < SSD1306_WIDTH; col++)
            SSD1306_DrawPixel(col, row, color);
}

void SSD1306_DrawLine(uint8_t x1, uint8_t y1,
                      uint8_t x2, uint8_t y2, uint8_t color)
{
    int16_t dx  =  (int16_t)(x2 - x1);
    int16_t dy  = -(int16_t)(y2 - y1);
    int16_t sx  = x1 < x2 ? 1 : -1;
    int16_t sy  = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    while (1)
    {
        SSD1306_DrawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}


/* -------------------------------------------------------
 * I2C 扫描
 * ------------------------------------------------------- */

uint8_t SSD1306_I2C_Scan(void)
{
    HAL_StatusTypeDef ret =
        HAL_I2C_IsDeviceReady(&hi2c1, SSD1306_I2C_ADDR,
                               3, SSD1306_I2C_TIMEOUT);
    if (ret == HAL_OK)
    {
        DEBUG_INFO("SSD1306: found at I2C addr 0x%02X",
                   SSD1306_I2C_ADDR >> 1);
        return 1;
    }
    DEBUG_ERR("SSD1306: not found at 0x%02X, try 0x3D",
              SSD1306_I2C_ADDR >> 1);
    return 0;
}

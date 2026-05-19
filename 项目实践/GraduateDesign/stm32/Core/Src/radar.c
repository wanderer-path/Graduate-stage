#include "radar.h"
#include "usart.h"
#include "debug.h"

/* -------------------------------------------------------
 * 全局变量
 * ------------------------------------------------------- */
RadarResult_t radar_result  = {0};
uint8_t       radar_rx_byte = 0;

/* -------------------------------------------------------
 * 环形接收缓冲区
 * ------------------------------------------------------- */
static uint8_t  rx_buf[RADAR_RX_BUF_SIZE];
static uint16_t rx_head = 0;
static uint16_t rx_tail = 0;

/* -------------------------------------------------------
 * TinyFrame 解析器内部状态
 * ------------------------------------------------------- */
static TF_State_t tf_state    = TF_STATE_SOF;
static uint8_t    tf_field_cnt = 0;

/* 头部字段原始字节（用于校验和） */
static uint8_t  tf_id[2]       = {0};
static uint8_t  tf_len_raw[2]  = {0};
static uint8_t  tf_type_raw[2] = {0};

/* 解析后的字段值 */
static uint16_t tf_len         = 0;
static uint16_t tf_type        = 0;

/* 跳过模式：记录还需丢弃多少字节 */
static uint16_t tf_skip_remain = 0;

/* 校验和累计 */
static uint8_t  tf_head_xor    = 0;
static uint8_t  tf_data_xor    = 0;

/* DATA 区缓存 */
static uint8_t  tf_data[TF_MAX_DATA_LEN];
static uint16_t tf_data_idx    = 0;




/* -------------------------------------------------------
 * 环形缓冲区操作
 * ------------------------------------------------------- */

static void RxBuf_Write(uint8_t byte)
{
    uint16_t next = (rx_head + 1) % RADAR_RX_BUF_SIZE;
    if (next != rx_tail)
    {
        rx_buf[rx_head] = byte;
        rx_head = next;
    }
}

static uint8_t RxBuf_Read(uint8_t *out)
{
    if (rx_head == rx_tail) return 0;
    *out    = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RADAR_RX_BUF_SIZE;
    return 1;
}


/* -------------------------------------------------------
 * 内部：解析完整帧
 * ------------------------------------------------------- */
static void Radar_ParseFrame(void)
{

    switch (tf_type)
    {
        case TF_TYPE_FALL:
            if (tf_len >= 1)
            {
                radar_result.is_fall      = tf_data[0];
                radar_result.fall_updated = 1;
                //DEBUG_LOG("[Radar] FALL frame: is_fall=%d", tf_data[0]);
            }
            break;

        case TF_TYPE_PRESENCE:
            if (tf_len >= 1)
            {
                radar_result.is_human      = tf_data[0];
                radar_result.human_updated = 1;
                //DEBUG_LOG("[Radar] PRESENCE frame: is_human=%d", tf_data[0]);
            }
            break;

        default:
            /* 点云帧(0x0A08)等其他类型，忽略 */
            break;
    }
}


/* -------------------------------------------------------
 * 内部：TinyFrame 状态机逐字节处理
 * ------------------------------------------------------- */
static void TF_FeedByte(uint8_t byte)
{
    switch (tf_state)
    {
        /* ---- 等待 SOF = 0x01 ---- */
        case TF_STATE_SOF:
            if (byte == TF_SOF)
            {
                tf_head_xor  = byte;   /* SOF 纳入头校验和 */
                tf_field_cnt = 0;
                tf_state     = TF_STATE_ID;
            }
            break;

        /* ---- 接收 ID 2字节（大端序）---- */
        case TF_STATE_ID:
            tf_id[tf_field_cnt] = byte;
            tf_head_xor ^= byte;
            tf_field_cnt++;
            if (tf_field_cnt >= 2)
            {
                tf_field_cnt = 0;
                tf_state     = TF_STATE_LEN;
            }
            break;

        /* ---- 接收 LEN 2字节（大端序）---- */
		case TF_STATE_LEN:
			tf_len_raw[tf_field_cnt] = byte;
			tf_head_xor ^= byte;
			tf_field_cnt++;
			if (tf_field_cnt >= 2)
			{
				tf_len = ((uint16_t)tf_len_raw[0] << 8) | tf_len_raw[1];
				tf_field_cnt = 0;
				tf_state = TF_STATE_TYPE;   /* 继续接收TYPE，不在此处判断长度 */
			}
			break;

        /* ---- 接收 TYPE 2字节（大端序）---- */
        case TF_STATE_TYPE:
            tf_type_raw[tf_field_cnt] = byte;
            tf_head_xor ^= byte;
            tf_field_cnt++;
            if (tf_field_cnt >= 2)
            {
                tf_type      = ((uint16_t)tf_type_raw[0] << 8)
                               | tf_type_raw[1];
                tf_field_cnt = 0;
                tf_state     = TF_STATE_HEAD_CKSUM;
            }
            break;

        /* ---- 验证 HEAD_CKSUM ---- */
		case TF_STATE_HEAD_CKSUM:
		{
			uint8_t expected = ~tf_head_xor;
			if (byte != expected)
			{
				DEBUG_WARN("[Radar] HEAD_CKSUM fail: got=0x%02X exp=0x%02X",
						   byte, expected);
				tf_state = TF_STATE_SOF;
				break;
			}

			/* 校验通过，判断 LEN */
			if (tf_len == 0)
			{
				tf_data_idx = 0;
				tf_data_xor = 0;
				tf_state = TF_STATE_DATA_CKSUM;
			}
			else if (tf_len > TF_SKIP_THRESHOLD)
			{
				/*
				 * LEN 超过缓冲区上限（如点云帧0x0A08）
				 * 不resync，进入跳过模式丢弃数据区字节
				 * 跳完后在 DATA_CKSUM 状态丢弃校验和，回到SOF
				 */
				tf_skip_remain = tf_len;
				tf_data_xor    = 0;
				tf_state       = TF_STATE_SKIP;
				/* 不打印警告，点云帧是正常的，只是我们不处理 */
			}
			else
			{
				tf_data_idx = 0;
				tf_data_xor = 0;
				memset(tf_data, 0, sizeof(tf_data));
				tf_state = TF_STATE_DATA;
			}
			break;
		}

        /* ---- 接收 DATA 区 ---- */
        case TF_STATE_DATA:
            tf_data[tf_data_idx] = byte;
            tf_data_xor ^= byte;
            tf_data_idx++;
            if (tf_data_idx >= tf_len)
                tf_state = TF_STATE_DATA_CKSUM;
            break;
			
		/* ---- 跳过超大帧数据区 ---- */
		case TF_STATE_SKIP:
			tf_data_xor ^= byte;   /* 继续累计校验和（可选，用于验证） */
			tf_skip_remain--;
			if (tf_skip_remain == 0)
			{
				/* 数据区字节全部丢弃，等待DATA_CKSUM */
				tf_state = TF_STATE_DATA_CKSUM;
			}
			break;

        /* ---- 验证 DATA_CKSUM ---- */
        case TF_STATE_DATA_CKSUM:
        {
            uint8_t expected = ~tf_data_xor;
            if (byte != expected)
            {
                DEBUG_WARN("[Radar] DATA_CKSUM fail: got=0x%02X exp=0x%02X",
                           byte, expected);
            }
            else
            {
                Radar_ParseFrame();
            }
            tf_state = TF_STATE_SOF;
            break;
        }

        default:
            tf_state = TF_STATE_SOF;
            break;
    }
}


/* -------------------------------------------------------
 * 对外接口
 * ------------------------------------------------------- */

void Radar_Init(void)
{
    if (HAL_UART_Receive_IT(&huart1, &radar_rx_byte, 1) != HAL_OK)
    {
        DEBUG_ERR("[Radar] USART1 RX interrupt start failed!");
        return;
    }
    DEBUG_INFO("[Radar] TinyFrame parser ready.");
    DEBUG_INFO("[Radar] SOF=0x01 | FALL=0x0E02 | PRESENCE=0x0F09");
}

void Radar_RxCallback(uint8_t byte)
{
    RxBuf_Write(byte);
    HAL_UART_Receive_IT(&huart1, &radar_rx_byte, 1);
}

void Radar_Process(void)
{
    uint8_t byte;
    while (RxBuf_Read(&byte))
    {
        TF_FeedByte(byte);
    }
}

void Radar_Print(void)
{
    DEBUG_LOG("[Radar] Human: %s | Fall: %s",
              radar_result.is_human ? "EXIST " : "NO ONE",
              radar_result.is_fall  ? "FALL!!" : "NORMAL");
}

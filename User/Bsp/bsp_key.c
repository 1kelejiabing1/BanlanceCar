/**
 * @file    bsp_key.c
 * @brief   按键驱动：基于状态机的消抖 / 单击 / 双击 / 长按检测
 *
 * 实现思路：
 *   每个按键维护一个状态机（空闲 → 按下 → 长按 / 等待双击），
 *   通过周期性调用 bsp_key_scan() 推进状态，检测到有效事件后写入
 *   环形缓冲区事件队列，供应用层通过 bsp_key_get_event() 非阻塞获取。
 *   支持事件：单击 / 双击 / 长按 / 长按保持（连发）。
 */
#include "bsp_key.h"
#include "app_debug.h"
#include "FreeRTOS.h"
#include "task.h"

#define KEY_ID_MAX 4
// 按键硬件配置
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState press_level; // 按下时的电平
    KeyId_e id;
} KeyConfig_t;

// 按键状态机
typedef struct
{
    uint8_t state;       // 0:空闲 1:按下 2:长按触发 3:等待双击
    uint32_t press_time; // 按下时刻
    uint8_t click_cnt;   // 单击计数
} KeyState_t;

// 按键配置表（根据实际硬件修改）
static const KeyConfig_t g_key_config[KEY_ID_MAX] = {
    [K1] = {K1_GPIO_Port, K1_Pin, GPIO_PIN_RESET, K1},
    [K2] = {K2_GPIO_Port, K2_Pin, GPIO_PIN_RESET, K2},
    [K3] = {K3_GPIO_Port, K3_Pin, GPIO_PIN_RESET, K3},
    [K4] = {K4_GPIO_Port, K4_Pin, GPIO_PIN_RESET, K4},
};

// 按键状态数组
static KeyState_t g_key_state[KEY_ID_MAX];

// 事件队列（环形缓冲区）
#define EVENT_QUEUE_SIZE 16
static KeyEvent_t g_event_queue[EVENT_QUEUE_SIZE];
static volatile uint8_t g_event_head = 0;
static volatile uint8_t g_event_tail = 0;

// 时间参数
#define DEBOUNCE_MS 20          // 消抖时间(ms)
#define LONG_PRESS_MS 1000      // 长按时间(ms)
#define LONG_PRESS_HOLD_MS 2000 // 长按持续事件(ms)
#define DOUBLE_CLICK_MS 300     // 双击间隔(ms)

// 内部函数声明
static bool bsp_key_read_raw(KeyId_e id);
static void bsp_key_process(KeyId_e id, uint32_t current_tick);
static bool event_enqueue(KeyId_e id, KeyEventType_e event);
static uint32_t get_tick(void);

// 读取原始按键电平（已做硬件消抖）
static bool bsp_key_read_raw(KeyId_e id)
{
    const KeyConfig_t *cfg = &g_key_config[id];
    return HAL_GPIO_ReadPin(cfg->port, cfg->pin) == cfg->press_level;
}

// 获取系统tick（ms）
static uint32_t get_tick(void)
{
    return xTaskGetTickCount();
}

// 事件入队
static bool bsp_key_event_enqueue(KeyId_e id, KeyEventType_e event)
{
    uint8_t next = (g_event_head + 1) % EVENT_QUEUE_SIZE;
    if (next == g_event_tail)
    {
        return false; // 队列满
    }

    g_event_queue[g_event_head].id = id;
    g_event_queue[g_event_head].event = event;
    g_event_head = next;
    return true;
}

// 按键状态机处理
static void bsp_key_process(KeyId_e id, uint32_t current_tick)
{
    KeyState_t *ks = &g_key_state[id];
    bool is_pressed = bsp_key_read_raw(id);

    switch (ks->state)
    {
    case 0: // 空闲状态
        if (is_pressed)
        {
            ks->state = 1;
            ks->press_time = current_tick;
            ks->click_cnt = 0;
        }
        break;

    case 1: // 按下状态（去抖+等待松开）
        if (!is_pressed)
        {
            // 松开了，判断是否为有效按下（消抖）
            if ((current_tick - ks->press_time) >= DEBOUNCE_MS)
            {
                // 有效单击
                if (ks->click_cnt == 0)
                {
                    // 第一次单击，等待双击
                    ks->state = 3; // 进入等待双击状态
                    ks->press_time = current_tick;
                    ks->click_cnt = 1;
                }
                else
                {
                    // 已经是第二次单击，触发双击
                    ks->state = 0;
                    bsp_key_event_enqueue(id, KEY_EVENT_DOUBLE_CLICK);
                    ks->click_cnt = 0;
                }
            }
            else
            {
                // 抖动，忽略
                ks->state = 0;
            }
        }
        else
        {
            // 仍然按着，检查是否达到长按时间
            if ((current_tick - ks->press_time) >= LONG_PRESS_MS)
            {
                ks->state = 2; // 进入长按状态
            }
        }
        break;

    case 2: // 长按状态（已触发长按事件，等待松开）
        if (!is_pressed)
        {
            bsp_key_event_enqueue(id, KEY_EVENT_LONG_PRESS);
            ks->state = 0;
            ks->click_cnt = 0;
        }
        else
        {
            // 仍然按着，触发长按持续事件（可用于连发）
            // 还没完全开发好
            if ((current_tick - ks->press_time) >= LONG_PRESS_HOLD_MS)
            {
                ks->press_time = current_tick;
                bsp_key_event_enqueue(id, KEY_EVENT_LONG_PRESS_HOLD);
            }
        }
        break;

    case 3: // 等待双击状态
        if (is_pressed)
        {
            // 第二次按下
            ks->state = 1;
            ks->press_time = current_tick;
        }
        else if ((current_tick - ks->press_time) >= DOUBLE_CLICK_MS)
        {
            // 超时，没有第二次按下，触发单击
            ks->state = 0;
            bsp_key_event_enqueue(id, KEY_EVENT_CLICK);
            ks->click_cnt = 0;
        }
        break;
    }
}

// 初始化所有按键（配置GPIO在main.c中已完成，这里只初始化状态）
void bsp_key_init(void)
{
    for (int i = 0; i < KEY_ID_MAX; i++)
    {
        g_key_state[i].state = 0;
        g_key_state[i].press_time = 0;
        g_key_state[i].click_cnt = 0;
    }
    g_event_head = 0;
    g_event_tail = 0;
}

// 按键扫描函数（需要周期性调用，建议10ms）
void bsp_key_scan(void)
{
    uint32_t now = get_tick();

    // 扫描所有按键
    for (int i = 0; i < KEY_ID_MAX; i++)
    {
        bsp_key_process((KeyId_e)i, now);
    }
}

// 获取按键事件（非阻塞）
bool bsp_key_get_event(KeyEvent_t *event)
{
    if (g_event_head == g_event_tail)
    {
        return false; // 队列空
    }

    *event = g_event_queue[g_event_tail];
    g_event_tail = (g_event_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

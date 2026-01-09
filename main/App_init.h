/**
 * @file App_Init.h
 * @brief 应用初始化配置头文件
 * 
 * 使用说明：
 * 1. 通过修改下方的宏定义(0或1)来选择需要初始化的组件
 * 2. 设置为1表示启用该组件，设置为0表示禁用
 * 3. 移植时只需修改此文件即可
 * 
 * 详细使用文档请参阅: components/App_Init/README.md
 */

#ifndef _APP_INIT_H_
#define _APP_INIT_H_

#include "esp_err.h"

/* ==================== 组件启用开关 ==================== */
/* 修改以下宏定义来选择需要初始化的组件 (1=启用, 0=禁用) */

/** WiFi STA模式 - 连接到路由器 */
#define ENABLE_WIFI_STA         0

/** WiFi AP配网模式 - 创建热点供配网 */
#define ENABLE_AP_WIFI          0

/** 按键组件 */
#define ENABLE_BUTTON           0

/** LCD ST7789显示屏 */
#define ENABLE_LCD_ST7789       0

/** CST816T触摸屏 */
#define ENABLE_TOUCH_CST816T    0

/** LVGL 驱动 */
#define ENABLE_LVGL_DRIVER      0

/** INMP441麦克风 */
#define ENABLE_INMP441          1

/** MAX98357A扬声器 */
#define ENABLE_MAX98367A        1

/** SPI模式SD卡 */
#define ENABLE_SPI_SDCARD       0

/** SDIO模式SD卡 */
#define ENABLE_SDIO_SDCARD      0

/* ==================== 条件包含头文件 ==================== */
#if ENABLE_WIFI_STA
#include "wifi_sta.h"
#endif

#if ENABLE_AP_WIFI
#include "ap_wifi.h"
#include "wifi_manager.h"
#include "ws_server.h"
#endif

#if ENABLE_BUTTON
#include "button.h"
#endif

#if ENABLE_LCD_ST7789
#include "st7789_driver.h"
#include "cst816t_driver.h"
#endif

#if ENABLE_TOUCH_CST816T
#include "cst816t_driver.h"
#endif

#if ENABLE_LVGL_DRIVER
#include "lvgl.h"
#include "lv_port.h"
#include "widgets/lv_demo_widgets.h"

#endif


#if ENABLE_INMP441
#include "INMP441.h"
#endif

#if ENABLE_MAX98367A
#include "MAX98367A.h"
#endif

#if ENABLE_SPI_SDCARD
#include "spi-sdcard.h"
#endif

#if ENABLE_SDIO_SDCARD
#include "sdio-sdcard.h"
#endif

/* ==================== 各模块初始化函数声明 ==================== */

#if ENABLE_WIFI_STA
/**
 * @brief 初始化WiFi STA模式
 * @param ssid WiFi名称，NULL则使用默认值
 * @param password WiFi密码，NULL则使用默认值
 * @param max_retry 最大重试次数，0则使用默认值
 * @param connected_cb 连接成功回调，可为NULL
 * @param disconnected_cb 断开连接回调，可为NULL
 * @return ESP_OK 成功
 */
esp_err_t app_wifi_sta_init(const char *ssid, const char *password, uint8_t max_retry,
                            wifi_event_callback_t connected_cb, wifi_event_callback_t disconnected_cb);
#endif

#if ENABLE_AP_WIFI
/**
 * @brief 初始化WiFi AP配网模式
 * @param state_cb WiFi状态变化回调，可为NULL
 */
void app_ap_wifi_init(void);
#endif

#if ENABLE_BUTTON
/**
 * @brief 初始化按键组件
 * @return ESP_OK 成功
 */
esp_err_t app_button_init(void);

/**
 * @brief 注册按键事件
 * @param cfg 按键配置数组
 * @param count 按键数量
 * @return ESP_OK 成功
 */
esp_err_t app_button_register(button_config_t *cfg, uint8_t count);
#endif

#if ENABLE_LCD_ST7789
/**
 * @brief 初始化LCD ST7789（使用默认配置）
 * @return ESP_OK 成功
 */
esp_err_t app_lcd_init_default(void);

/**
 * @brief 初始化LCD ST7789（自定义配置）
 * @param cfg LCD配置
 * @return ESP_OK 成功
 */
esp_err_t app_lcd_init(st7789_cfg_t *cfg);
#endif

#if ENABLE_TOUCH_CST816T
/**
 * @brief 初始化触摸屏（使用默认配置）
 * @return ESP_OK 成功
 */
esp_err_t app_touch_init_default(void);

/**
 * @brief 初始化触摸屏（自定义配置）
 * @param cfg 触摸屏配置
 * @return ESP_OK 成功
 */
esp_err_t app_touch_init(cst816t_cfg_t *cfg);
#endif

#if ENABLE_LVGL_DRIVER
/**
 * @brief 初始化LVGL驱动
 */
void app_lvgl_driver_init(void);
#endif


#if ENABLE_INMP441
/**
 * @brief 初始化INMP441麦克风
 */
void app_mic_init(void);
#endif

#if ENABLE_MAX98367A
/**
 * @brief 初始化MAX98357A扬声器
 */
void app_speaker_init(void);
#endif

#if ENABLE_SPI_SDCARD
/**
 * @brief 初始化SPI模式SD卡
 * @return ESP_OK 成功
 */
esp_err_t app_spi_sdcard_init(void);
#endif

#if ENABLE_SDIO_SDCARD
/**
 * @brief 初始化SDIO模式SD卡
 * @return ESP_OK 成功
 */
esp_err_t app_sdio_sdcard_init(void);
#endif

/* ==================== 通用API函数声明 ==================== */

/**
 * @brief 初始化所有已启用的组件（使用默认配置）
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t app_init_all(void);

/**
 * @brief 打印当前启用的组件列表
 */
void app_print_enabled_components(void);

#endif /* _APP_INIT_H_ */


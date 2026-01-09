/**
 * @file App_Init.c
 * @brief 应用统一初始化实现
 * 
 * 根据App_Init.h中的宏定义选择性初始化各组件
 * 每个组件有独立的初始化函数，可单独调用
 */

#include <stdio.h>
#include "App_Init.h"
#include "esp_log.h"
#include "driver/gpio.h"
static const char *TAG = "APP_INIT";

/* ==================== 打印启用的组件 ==================== */

void app_print_enabled_components(void)
{
    ESP_LOGI(TAG, "========== 已启用的组件 ==========");
    
#if ENABLE_WIFI_STA
    ESP_LOGI(TAG, "  [√] WiFi STA模式");
#endif

#if ENABLE_AP_WIFI
    ESP_LOGI(TAG, "  [√] WiFi AP配网模式");
#endif

#if ENABLE_BUTTON
    ESP_LOGI(TAG, "  [√] 按键组件");
#endif

#if ENABLE_LCD_ST7789
    ESP_LOGI(TAG, "  [√] LCD ST7789显示屏");
#endif

#if ENABLE_TOUCH_CST816T
    ESP_LOGI(TAG, "  [√] CST816T触摸屏");
#endif

#if ENABLE_INMP441
    ESP_LOGI(TAG, "  [√] INMP441麦克风");
#endif

#if ENABLE_MAX98367A
    ESP_LOGI(TAG, "  [√] MAX98357A扬声器");
#endif

#if ENABLE_SPI_SDCARD
    ESP_LOGI(TAG, "  [√] SPI模式SD卡");
#endif

#if ENABLE_SDIO_SDCARD
    ESP_LOGI(TAG, "  [√] SDIO模式SD卡");
#endif

    ESP_LOGI(TAG, "===================================");
}

/* ==================== WiFi STA模块 ==================== */

#if ENABLE_WIFI_STA
esp_err_t app_wifi_sta_init(const char *ssid, const char *password, uint8_t max_retry,
                            wifi_event_callback_t connected_cb, wifi_event_callback_t disconnected_cb)
{
    ESP_LOGI(TAG, "初始化 WiFi STA模式...");
    
    wifi_sta_config cfg = {
        .ssid = (ssid != NULL) ? ssid : WIFI_SSID,
        .password = (password != NULL) ? password : WIFI_PASS,
        .max_retry = (max_retry > 0) ? max_retry : WIFI_MAX_RETRY
    };
    
    esp_err_t ret = wifi_sta_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi STA初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (connected_cb) {
        wifi_sta_set_connected_callback(connected_cb);
    }
    if (disconnected_cb) {
        wifi_sta_set_disconnected_callback(disconnected_cb);
    }
    
    ESP_LOGI(TAG, "WiFi STA初始化完成");
    return ESP_OK;
}
#endif

/* ==================== WiFi AP配网模块 ==================== */

#if ENABLE_AP_WIFI

void wifi_state_handle(WIFI_STATE state)
{
    if(state == WIFI_STATE_CONNECTED)    //wifi连接成功
    {
        ESP_LOGI(TAG,"Wifi connected");
    }
    else if(state == WIFI_STATE_CONNECTED)    //wifi连接失败
    {
        ESP_LOGI(TAG,"Wifi disconnected");
    }
}


void app_ap_wifi_init(void)
{
    ESP_LOGI(TAG, "初始化 WiFi AP配网模式...");
    ap_wifi_init(wifi_state_handle);
    // ap_wifi_apcfg(true);
    ESP_LOGI(TAG, "WiFi AP配网初始化完成");
}
#endif

/* ==================== 按键模块 ==================== */

#if ENABLE_BUTTON
// 获取按键电平


esp_err_t app_button_init(void)
{
    ESP_LOGI(TAG, "初始化按键...");
    return button_init_with_gpio();
}
#endif

/* ==================== LCD ST7789模块 ==================== */

#if ENABLE_LCD_ST7789
esp_err_t app_lcd_init_default(void)
{
    st7789_cfg_t cfg = {
        .mosi = LCD_MOSI,
        .clk = LCD_CLK,
        .cs = LCD_CS,
        .dc = LCD_DC,
        .rst = LCD_RST,
        .bl = LCD_BL,
        .spi_fre = 40000000,
        .width = 240,
        .height = 280,
        .spin = 0,
        .done_cb = NULL,
        .cb_param = NULL
    };
    return app_lcd_init(&cfg);
}

esp_err_t app_lcd_init(st7789_cfg_t *cfg)
{
    ESP_LOGI(TAG, "初始化 LCD ST7789显示屏...");
    
    esp_err_t ret = st7789_driver_hw_init(cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    st7789_lcd_backlight(true);
    ESP_LOGI(TAG, "LCD ST7789初始化完成 (%dx%d)", cfg->width, cfg->height);
    return ESP_OK;
}
#endif

/* ==================== 触摸屏CST816T模块 ==================== */

#if ENABLE_TOUCH_CST816T
esp_err_t app_touch_init_default(void)
{
    cst816t_cfg_t cfg = {
        .scl = TP_SCL,
        .sda = TP_SDA,
        .fre = 300*1000,
        .x_limit = 240,
        .y_limit = 280
    };
    return app_touch_init(&cfg);
}

esp_err_t app_touch_init(cst816t_cfg_t *cfg)
{
    ESP_LOGI(TAG, "初始化 CST816T触摸屏...");
    
    esp_err_t ret = cst816t_init(cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "触摸屏初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "CST816T触摸屏初始化完成");
    return ESP_OK;
}
#endif

/* ==================== INMP441麦克风模块 ==================== */

#if ENABLE_INMP441
void app_mic_init(void)
{
    ESP_LOGI(TAG, "初始化 INMP441麦克风...");
    i2s_rx_init();
    ESP_LOGI(TAG, "INMP441麦克风初始化完成");
}
#endif

/* ==================== MAX98357A扬声器模块 ==================== */

#if ENABLE_MAX98367A
void app_speaker_init(void)
{
    ESP_LOGI(TAG, "初始化 MAX98357A扬声器...");
    i2s_tx_init();
    ESP_LOGI(TAG, "MAX98357A扬声器初始化完成");
}
#endif

/* ==================== SPI SD卡模块 ==================== */

#if ENABLE_SPI_SDCARD
esp_err_t app_spi_sdcard_init(void)
{
    ESP_LOGI(TAG, "初始化 SPI模式SD卡...");
    
    spi2_init();
    esp_err_t ret = sd_spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI SD卡初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SPI模式SD卡初始化完成");
    return ESP_OK;
}
#endif

/* ==================== SDIO SD卡模块 ==================== */

#if ENABLE_SDIO_SDCARD
esp_err_t app_sdio_sdcard_init(void)
{
    ESP_LOGI(TAG, "初始化 SDIO模式SD卡...");
    
    esp_err_t ret = sd_sdio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SDIO SD卡初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SDIO模式SD卡初始化完成");
    return ESP_OK;
}
#endif

/* ==================== 统一初始化入口（使用默认配置）==================== */

esp_err_t app_init_all(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "开始初始化应用组件...");
    app_print_enabled_components();

#if ENABLE_WIFI_STA
    ret = app_wifi_sta_init(NULL, NULL, 0, NULL, NULL);
    if (ret != ESP_OK) return ret;
#endif

#if ENABLE_AP_WIFI
    app_ap_wifi_init();
#endif

#if ENABLE_BUTTON
    ret = app_button_init();
    if (ret != ESP_OK) return ret;
#endif



#if ENABLE_LVGL_DRIVER
    // LCD和触摸屏由lv_port_init统一初始化（包含回调配置）
    lv_port_init();
    st7789_lcd_backlight(true);
#endif

#if ENABLE_INMP441
    app_mic_init();
#endif

#if ENABLE_MAX98367A
    app_speaker_init();
#endif

#if ENABLE_SPI_SDCARD
    ret = app_spi_sdcard_init();
    if (ret != ESP_OK) return ret;
#endif

#if ENABLE_SDIO_SDCARD
    ret = app_sdio_sdcard_init();
    if (ret != ESP_OK) return ret;
#endif

    ESP_LOGI(TAG, "所有组件初始化完成!");
    return ESP_OK;
}

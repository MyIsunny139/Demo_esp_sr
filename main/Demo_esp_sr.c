#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "stdio.h"
#include "App_init.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

static const char *TAG = "MAIN";

void afe_sr_iface_task(void *arg)
{
    ESP_LOGI(TAG, "语音识别任务启动");
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待系统初始化完成
    while (1) 
    {
        // TODO: 在此添加音频采集和语音识别逻辑
        // 示例: 读取麦克风 -> AFE处理 -> 唤醒检测 -> 命令识别
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}


//? 音频直通任务：麦克风直接输出到扬声器（硬件测试）
//? 不经过队列，最低延迟，用于测试麦克风和扬声器是否正常工作
void audio_passthrough_task(void *pvParameters)
{
    uint8_t passthrough_buf[BUF_SIZE] = {0};
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    
    ESP_LOGI("AUDIO_PASSTHROUGH", "Direct passthrough task started - Mic -> Speaker");
    ESP_LOGI("AUDIO_PASSTHROUGH", "Sample rate: %d Hz, Bit width: %d, Buffer: %d bytes", 
             MAX98367A_SAMPLE_RATE, MAX98367A_BIT_WIDTH, BUF_SIZE);
    
    while (1) 
    {
        //? 直接从麦克风读取数据
        if (i2s_channel_read(rx_handle, passthrough_buf, BUF_SIZE, &bytes_read, 100) == ESP_OK)
        {
            //? 应用噪声过滤（去除低能量杂音）
            inmp441_filter_noise(passthrough_buf, bytes_read);
            
            // max98367a_set_gain(3.0f);  //? 设置适度增益，避免过大音量损伤听力
            //? 可选：应用适度增益（如需要）
            max98367a_apply_gain(passthrough_buf, bytes_read);
            
            //? 立即输出到扬声器
            if (i2s_channel_write(tx_handle, passthrough_buf, bytes_read, &bytes_written, portMAX_DELAY) != ESP_OK) {
                ESP_LOGW("AUDIO_PASSTHROUGH", "Failed to write audio");
            }
        }
        
    }
    vTaskDelete(NULL);
}

void app_main(void) 
{   
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(app_init_all());
    ESP_LOGI(TAG, "系统初始化完成，进入主循环");
    // BaseType_t ret1 = xTaskCreatePinnedToCore(afe_sr_iface_task,"afe_sr_iface_task",8192,NULL,5,NULL,0);
    // if(ret1 != pdPASS) {
    //     ESP_LOGE(TAG, "创建 afe_sr_iface_task 任务失败");
    // }

    //? 创建音频直通任务（用于测试麦克风和扬声器）
    BaseType_t ret2 = xTaskCreatePinnedToCore(audio_passthrough_task, "audio_passthrough_task", 4096, NULL, 5, NULL, 1);
    if (ret2 != pdPASS) {
        ESP_LOGE(TAG, "创建 audio_passthrough_task 任务失败");
    }
}

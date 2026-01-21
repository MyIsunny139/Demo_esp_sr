#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
#include "esp_mn_speech_commands.h"
#include "stdio.h"
#include "string.h"
#include "App_init.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

static const char *TAG = "MAIN";

//? ESP-SR 相关全局变量
static srmodel_list_t *sr_models = NULL;              // SR模型列表
static esp_afe_sr_data_t *afe_data = NULL;            // AFE实例数据
static const esp_afe_sr_iface_t *afe_handle = NULL;   // AFE接口句柄
static volatile bool is_wakenet_detected = false;     // 唤醒词检测标志（跨任务共享，防止编译器优化）
static esp_mn_iface_t *multinet = NULL;               // MultiNet命令词识别接口
static model_iface_data_t *mn_model_data = NULL;      // MultiNet模型数据

//? 命令词ID定义
enum {
    CMD_ID_TURN_ON_LIGHT = 0,
    CMD_ID_TURN_OFF_LIGHT,
    CMD_ID_TURN_ON_AC,
    CMD_ID_TURN_OFF_AC,
    CMD_ID_VOLUME_UP,
    CMD_ID_VOLUME_DOWN,
    CMD_ID_PLAY_MUSIC,
    CMD_ID_STOP_MUSIC,
};

/**
 * @brief 初始化命令词列表
 */
static void init_speech_commands(void)
{
    // 添加中文命令词
    esp_mn_commands_add(CMD_ID_TURN_ON_LIGHT, "da kai dian deng");
    esp_mn_commands_add(CMD_ID_TURN_OFF_LIGHT, "guan bi dian deng");
    esp_mn_commands_add(CMD_ID_TURN_ON_AC, "da kai kong tiao");
    esp_mn_commands_add(CMD_ID_TURN_OFF_AC, "guan bi kong tiao");
    esp_mn_commands_add(CMD_ID_VOLUME_UP, "zeng da yin liang");
    esp_mn_commands_add(CMD_ID_VOLUME_DOWN, "jian xiao yin liang");
    esp_mn_commands_add(CMD_ID_PLAY_MUSIC, "bo fang yin yue");
    esp_mn_commands_add(CMD_ID_STOP_MUSIC, "ting zhi bo fang");
    
    // 更新命令词到MultiNet
    esp_mn_error_t *err = esp_mn_commands_update();
    if (err) {
        ESP_LOGW(TAG, "有 %d 条命令词解析失败", err->num);
    }
    ESP_LOGI(TAG, "命令词初始化完成");
    esp_mn_commands_print();
}

/**
 * @brief 处理识别到的命令词
 */
static void handle_speech_command(int command_id)
{
    switch (command_id) {
        case CMD_ID_TURN_ON_LIGHT:
            ESP_LOGI(TAG, ">>> 执行: 打开电灯");
            break;
        case CMD_ID_TURN_OFF_LIGHT:
            ESP_LOGI(TAG, ">>> 执行: 关闭电灯");
            break;
        case CMD_ID_TURN_ON_AC:
            ESP_LOGI(TAG, ">>> 执行: 打开空调");
            break;
        case CMD_ID_TURN_OFF_AC:
            ESP_LOGI(TAG, ">>> 执行: 关闭空调");
            break;
        case CMD_ID_VOLUME_UP:
            ESP_LOGI(TAG, ">>> 执行: 增大音量");
            break;
        case CMD_ID_VOLUME_DOWN:
            ESP_LOGI(TAG, ">>> 执行: 减小音量");
            break;
        case CMD_ID_PLAY_MUSIC:
            ESP_LOGI(TAG, ">>> 执行: 播放音乐");
            break;
        case CMD_ID_STOP_MUSIC:
            ESP_LOGI(TAG, ">>> 执行: 停止播放");
            break;
        default:
            ESP_LOGW(TAG, "未知命令ID: %d", command_id);
            break;
    }
}

/**
 * @brief AFE Feed任务 - 将麦克风数据喂给AFE进行处理
 */
static void afe_feed_task(void *arg)
{
    ESP_LOGI(TAG, "AFE Feed任务启动");
    
    // 禁用噪声门限（SR需要完整音频数据）
    inmp441_set_noise_gate(0);
    
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_channel = afe_handle->get_feed_channel_num(afe_data);
    
    // I2S读取缓冲区：立体声32位数据
    size_t i2s_read_bytes = feed_chunksize * 2 * sizeof(int32_t);
    int32_t *i2s_raw_buf = (int32_t *)heap_caps_malloc(i2s_read_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    // AFE feed缓冲区：单声道16位
    int16_t *feed_buf = (int16_t *)heap_caps_malloc(feed_chunksize * feed_channel * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    if (!i2s_raw_buf || !feed_buf) {
        ESP_LOGE(TAG, "分配feed缓冲区失败");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Feed配置: chunksize=%d, channels=%d", feed_chunksize, feed_channel);
    
    size_t bytes_read = 0;
    while (1) {
        esp_err_t ret = i2s_channel_read(rx_handle, i2s_raw_buf, i2s_read_bytes, &bytes_read, portMAX_DELAY);
        if (ret == ESP_OK && bytes_read > 0) {
            int total_samples = bytes_read / sizeof(int32_t);
            
            // 提取左声道并转换为16位（INMP441: 24位数据MSB对齐在32位中）
            for (int i = 0, j = 0; i < total_samples && j < feed_chunksize; i += 2, j++) {
                feed_buf[j] = (int16_t)(i2s_raw_buf[i] >> 16);
            }
            
            // 将音频数据喂给AFE
            afe_handle->feed(afe_data, feed_buf);
        }
    }
    
    free(i2s_raw_buf);
    free(feed_buf);
    vTaskDelete(NULL);
}

/**
 * @brief AFE Fetch任务 - 从AFE获取处理后的音频，进行唤醒词和命令词识别
 */
void afe_sr_iface_task(void *arg)
{
    ESP_LOGI(TAG, "语音识别任务启动");
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待系统初始化完成
    
    //? 1. 初始化SR模型
    sr_models = esp_srmodel_init("model");
    if (!sr_models) {
        ESP_LOGE(TAG, "SR模型初始化失败，请检查分区表是否包含model分区");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "SR模型初始化成功，共 %d 个模型", sr_models->num);
    
    
    //? 2. 查找WakeNet和MultiNet模型
    char *wn_model_name = esp_srmodel_filter(sr_models, ESP_WN_PREFIX, NULL);
    char *mn_model_name = esp_srmodel_filter(sr_models, ESP_MN_PREFIX, NULL);
    
    if (wn_model_name) {
        ESP_LOGI(TAG, "找到唤醒词模型: %s", wn_model_name);
    } else {
        ESP_LOGW(TAG, "未找到唤醒词模型，将跳过唤醒检测");
    }
    
    if (mn_model_name) {
        ESP_LOGI(TAG, "找到命令词模型: %s", mn_model_name);
    } else {
        ESP_LOGW(TAG, "未找到命令词模型");
    }
    
    //? 3. 配置AFE
    // 使用单麦克风配置 ("M"表示Microphone)
    afe_config_t *afe_config = afe_config_init("M", sr_models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    if (!afe_config) {
        ESP_LOGE(TAG, "AFE配置初始化失败");
        vTaskDelete(NULL);
        return;
    }
    
    // 配置参数
    afe_config->wakenet_init = (wn_model_name != NULL);
    afe_config->wakenet_model_name = wn_model_name;
    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_3;
    
    //? 4. 创建AFE实例
    afe_handle = esp_afe_handle_from_config(afe_config);
    if (!afe_handle) {
        ESP_LOGE(TAG, "获取AFE句柄失败");
        vTaskDelete(NULL);
        return;
    }
    
    afe_data = afe_handle->create_from_config(afe_config);
    if (!afe_data) {
        ESP_LOGE(TAG, "创建AFE实例失败");
        vTaskDelete(NULL);
        return;
    }
    
    // 打印AFE处理流水线
    afe_handle->print_pipeline(afe_data);
    
    // 设置唤醒词检测阈值（范围0.4-0.9999，越低越灵敏）
    if (wn_model_name) {
        afe_handle->set_wakenet_threshold(afe_data, 1, 0.5);
    }
    
    //? 5. 初始化MultiNet命令词识别
    if (mn_model_name) {
        multinet = esp_mn_handle_from_name(mn_model_name);
        if (multinet) {
            mn_model_data = multinet->create(mn_model_name, 6000);  // 6秒超时
            if (mn_model_data) {
                ESP_LOGI(TAG, "MultiNet初始化成功");
                
                // 初始化命令词
                esp_mn_commands_alloc(multinet, mn_model_data);
                init_speech_commands();
            }
        }
    }
    
    // 6. 启动Feed任务
    BaseType_t ret = xTaskCreatePinnedToCore(afe_feed_task, "afe_feed_task", 4096, NULL, 5, NULL, 1);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建afe_feed_task失败");
    }
    
    //? 7. 主循环：Fetch处理后的音频并进行识别
    int fetch_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    ESP_LOGI(TAG, "Fetch配置: chunksize=%d", fetch_chunksize);
    
    int mn_chunk_num = 0;               // 命令词识别帧计数器（用于超时判断，每帧32ms）
    bool detecting_command = false;     // 命令词识别模式标志（true=正在识别命令词）
    
    while (1) {
        // 从AFE获取处理后的音频和状态
        afe_fetch_result_t *fetch_result = afe_handle->fetch(afe_data);
        if (!fetch_result) {
            continue;
        }
        
        // 检查唤醒词状态
        if (fetch_result->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGW(TAG, "========================================");
            ESP_LOGW(TAG, "唤醒词检测! 索引=%d", fetch_result->wake_word_index);
            ESP_LOGW(TAG, "请在6秒内说出命令词");
            ESP_LOGW(TAG, "========================================");
            is_wakenet_detected = true;
            detecting_command = true;
            mn_chunk_num = 0;
            
            // 清除MultiNet状态，准备接收命令
            if (multinet && mn_model_data) {
                multinet->clean(mn_model_data);
            }
        }
        
        // 命令词识别
        if (detecting_command && multinet && mn_model_data) {
            esp_mn_state_t mn_state = multinet->detect(mn_model_data, fetch_result->data);
            mn_chunk_num++;
            
            if (mn_state == ESP_MN_STATE_DETECTED) {
                // 识别到命令词
                esp_mn_results_t *results = multinet->get_results(mn_model_data);
                if (results) {
                    ESP_LOGI(TAG, "识别到命令: ID=%d, 置信度=%.2f", 
                             results->command_id[0], results->prob[0]);
                    
                    // 处理命令
                    handle_speech_command(results->command_id[0]);
                }
                
                // 识别成功后重置计时，继续等待下一个命令（而非立即退出）
                mn_chunk_num = 0;
                multinet->clean(mn_model_data);  // 清除状态，准备识别下一个命令
                ESP_LOGI(TAG, "可以继续说命令词(6秒内无操作自动退出)");
                
            } else if (mn_state == ESP_MN_STATE_TIMEOUT) {
                // 6秒内没有识别到新命令，退出识别模式
                ESP_LOGW(TAG, "6秒无新命令,退出识别模式");
                detecting_command = false;
                is_wakenet_detected = false;
            }
        }
    }
    
    //? 清理资源（正常情况下不会执行到这里）
    if (mn_model_data) {
        multinet->destroy(mn_model_data);
    }
    esp_mn_commands_free();
    afe_handle->destroy(afe_data);
    esp_srmodel_deinit(sr_models);
    
    vTaskDelete(NULL);
}


// 音频直通任务：麦克风直接输出到扬声器（硬件测试）
// 不经过队列，最低延迟，用于测试麦克风和扬声器是否正常工作
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
            
            // max98367a_set_gain(3.0f);  // 设置适度增益，避免过大音量损伤听力
            // 可选：应用适度增益（如需要）
            max98367a_apply_gain(passthrough_buf, bytes_read);
            
            // 立即输出到扬声器
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
    
    //? 创建语音识别任务（需要较大栈空间）
    BaseType_t ret1 = xTaskCreatePinnedToCore(afe_sr_iface_task, "afe_sr_iface_task", 16384, NULL, 5, NULL, 0);
    if(ret1 != pdPASS) {
        ESP_LOGE(TAG, "创建 afe_sr_iface_task 任务失败");
    }

    //? 创建音频直通任务（用于测试麦克风和扬声器）
    // BaseType_t ret2 = xTaskCreatePinnedToCore(audio_passthrough_task, "audio_passthrough_task", 4096, NULL, 5, NULL, 1);
    // if (ret2 != pdPASS) {
    //     ESP_LOGE(TAG, "创建 audio_passthrough_task 任务失败");
    // }
}

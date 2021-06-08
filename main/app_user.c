#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_spiffs.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "nvs_flash.h"

#include "app_user.h"

uint16_t lightChannelValue;


/******************************************************************************************************
 *                                          Bell->PWM 配置                                 
 ******************************************************************************************************/
#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH2_GPIO       GPIO_NUM_4
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2
#define LEDC_DUTY_RESOLUTION   LEDC_TIMER_10_BIT   
#define LEDC_PWM_MAX_DUTY      (uint16_t)pow(2, LEDC_DUTY_RESOLUTION)     // 把分辨率转换为十进制

#define LEDC_TEST_CH_NUM       (1)
#define LEDC_TEST_DUTY         (LEDC_PWM_MAX_DUTY/2)
#define LEDC_TEST_FADE_TIME    (3000)
 /*
    * Prepare individual configuration
    * for each channel of LED Controller
    * by selecting:
    * - controller's channel number
    * - output duty cycle, set initially to 0
    * - GPIO number where LED is connected to
    * - speed mode, either high or low
    * - timer servicing selected channel
    *   Note: if different channels use one timer,
    *         then frequency and bit_num of these channels
    *         will be the same
    */
ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
    {
        .channel    = LEDC_LS_CH2_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_LS_CH2_GPIO,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER
    },
    
};

void app_bell_contorl(uint16_t duty) 
{
    if (duty > LEDC_PWM_MAX_DUTY)  duty = LEDC_PWM_MAX_DUTY;
    ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, duty);
    ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
}

void app_bell_init(void)
{
    int ch = 0;
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RESOLUTION,    // resolution of PWM duty
        .freq_hz = 4000,                            // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,                 // timer mode
        .timer_num = LEDC_LS_TIMER,                 // timer index
        .clk_cfg = LEDC_AUTO_CLK,                   // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    app_bell_contorl(0);

    // ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_TEST_DUTY);
    // ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
}



/******************************************************************************************************
 *                                          LED 配置                                 
 ******************************************************************************************************/

#define LED1_IO    GPIO_NUM_32
#define LED2_IO    GPIO_NUM_33
#define LED3_IO    GPIO_NUM_25
#define LED4_IO    GPIO_NUM_26

#define LED_PIN_SEL  ((1ULL<<LED1_IO) | (1ULL<<LED2_IO) | (1ULL<<LED3_IO) | (1ULL<<LED4_IO))

#define LED_ON      0
#define LED_OFF     1

// channel: 通道；（0xff:全部）； state: 开关状态
void app_led_contorl(uint8_t channel, bool state)
{
    switch (channel)
    {
    case 0:
        gpio_set_level(LED1_IO, state);
        break;
    case 1:
        gpio_set_level(LED2_IO, state);
        break;
    case 2:
        gpio_set_level(LED3_IO, state);
        break;
    case 3:
        gpio_set_level(LED4_IO, state);
        break;
    default:
        gpio_set_level(LED1_IO, state);
        gpio_set_level(LED2_IO, state);
        gpio_set_level(LED3_IO, state);
        gpio_set_level(LED4_IO, state);
        break;
    }
}

void app_led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // app_led_contorl(0xFF, LED_OFF);
}


/******************************************************************************************************
 *                                          Relay 配置                                 
 ******************************************************************************************************/
#define RELAY1_IO    GPIO_NUM_16
#define RELAY2_IO    GPIO_NUM_17
#define RELAY3_IO    GPIO_NUM_5
#define RELAY4_IO    GPIO_NUM_18

#define RELAY_PIN_SEL  ((1ULL<<RELAY1_IO) | (1ULL<<RELAY2_IO) | (1ULL<<RELAY3_IO) | (1ULL<<RELAY4_IO))

#define RELAY_ON      1
#define RELAY_OFF     0

// channel: 通道；（0xff:全部）； state: 开关状态
void app_relay_contorl(uint8_t channel, bool state)
{
    switch (channel)
    {
    case 0:
        gpio_set_level(RELAY1_IO, state);
        break;
    case 1:
        gpio_set_level(RELAY2_IO, state);
        break;
    case 2:
        gpio_set_level(RELAY3_IO, state);
        break;
    case 3:
        gpio_set_level(RELAY4_IO, state);
        break;
    default:
        gpio_set_level(RELAY1_IO, state);
        gpio_set_level(RELAY2_IO, state);
        gpio_set_level(RELAY3_IO, state);
        gpio_set_level(RELAY4_IO, state);
        break;
    }

    app_led_contorl(channel, !state);   // 因为LED与Relay控制电平是反的
}

void app_light_contorl()
{

}

void app_relay_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = RELAY_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    app_relay_contorl(0xFF, RELAY_OFF);
}


esp_err_t nvs_readwrite_blob(const char* name, const char* key, void* out_value, size_t size, uint8_t rw_cmd)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(name, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read previously saved blob if available
    if (rw_cmd == 0) {  
         
        err = nvs_get_blob(my_handle, key, out_value, &size);
        if (err != ESP_OK) {
            return err;
        }
         
        // Close
        nvs_close(my_handle);
        return err;
    }

    // Write value including previously saved blob if available
    err = nvs_set_blob(my_handle, key, out_value, size);

    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

void app_spiffs_init(void)
{
    const char *TAG = "APP_SPIFFS"; 

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

#if 0   // test

    mqtt_data_t mqtt_buff = { 1, { 1 , 2 , 3}, 3 };
    mqtt_spiffs_erase();
    int32_t used_num = 0;
    mqtt_nvs_used(used_num, NVS_CMD_WRITE);     
    used_num = mqtt_nvs_used(used_num, NVS_CMD_READ);     
    for (int i = 1; i < 6; i++) {
        mqtt_buff.timestamp = i;
        mqtt_buff.len = i;
        memset(mqtt_buff.value, i, mqtt_buff.len);
        mqtt_spiffs_write(mqtt_buff);
    }
    
    // mqtt_spiffs_read(&mqtt_buff);
#else
    uint32_t data_size = spiffs_file_size("/spiffs/mqtt.bin");
    if (data_size == 0) { // 空文件，
        mqtt_nvs_used((int32_t)data_size, NVS_CMD_WRITE);      // 写0
    }
#endif

}

/*****************************************************************************************************
"r"	    打开一个用于读取的文件。该文件必须存在。
"w"	    创建一个用于写入的空文件。如果文件名称与已存在的文件相同，则会删除已有文件的内容，文件被视为一个新的空文件。
"a"	    追加到一个文件。写操作向文件末尾追加数据。如果文件不存在，则创建文件。
"r+"	打开一个用于更新的文件，可读取也可写入。该文件必须存在。
"w+"	创建一个用于读写的空文件。
"a+"	打开一个用于读取和追加的文件。
"wb"    只写打开或新建一个二进制文件；只允许写数据。
"wb+"   读写打开或建立一个二进制文件，允许读和写。
"ab+"   读写打开一个二进制文件，允许读或在文件末追加数据。
*****************************************************************************************************/

int32_t mqtt_nvs_used(int32_t used_num, uint8_t rw_cmd)
{
    int32_t nvs_used = 0;  
    nvs_handle_t my_handle;

    error_t err = nvs_open("nvs_used", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        if (rw_cmd == NVS_CMD_READ) {  // read
            err = nvs_get_i32(my_handle, "used_key", &nvs_used);
            switch (err) {
            case ESP_OK:
                printf("nvs_used = %d\n", nvs_used);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
            }
        } else {  // write
            nvs_used = used_num;
            err = nvs_set_i32(my_handle, "used_key", nvs_used);
            if (err != ESP_OK) {
                printf("Error (%s) NVS Set Failed!!\n", esp_err_to_name(err));
            } 

            err = nvs_commit(my_handle);
            if (err != ESP_OK) {
                printf("Error (%s) NVS Committing Failed!\n", esp_err_to_name(err));
            }   
        }
    }

    // Close
    nvs_close(my_handle);

    return nvs_used;
}


#if 1

uint32_t spiffs_file_size(const char *filename)
{
    const char *TAG = "APP_SPIFFS"; 
    // Check if destination file exists
    struct stat st;
    if (stat(filename, &st) != 0) {  // file does not exist
        //ESP_LOGE(TAG, "Failed to file does not exist");
        return 0;
    } 
    uint32_t file_size = st.st_size;
    ESP_LOGI(TAG, "%s size: %d", filename, file_size);
    return file_size;
}

void mqtt_spiffs_write(mqtt_data_t mqtt_data)
{
    const char *TAG = "APP_SPIFFS"; 

    // Use POSIX and C standard library functions to work with files.
    FILE* file = fopen("/spiffs/mqtt.bin", "ab+");   // 打开一个用于读取的文件。该文件必须存在。
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for new create");
        return;
    }

    ESP_LOGI(TAG, "File written spiffs");
    fwrite((void*)&mqtt_data, sizeof(uint8_t), sizeof(mqtt_data_t), file);
    fclose(file);  
}

void mqtt_spiffs_erase(void)
{
    const char *TAG = "APP_SPIFFS"; 

    FILE* file = fopen("/spiffs/mqtt.bin", "w");   // 创建一个用于写入的空文件,并清空已有数据
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for new create");
        return;
    }
    fclose(file);  

    int err = remove("/spiffs/mqtt.bin");
    if (err != 0) {
        ESP_LOGI(TAG, "File mqtt.bin remove err: %d", err);
    } else {
        ESP_LOGI(TAG, "File mqtt.bin remove ok");
    }
}

int mqtt_spiffs_read(mqtt_data_t *mqtt_data)  
{
    const char *TAG = "APP_SPIFFS"; 

    uint32_t data_size = spiffs_file_size("/spiffs/mqtt.bin");
    if (data_size == 0) {
        return 0;
    }

    uint32_t total_num = data_size / sizeof(mqtt_data_t);  // 计算一共有多少条数据
    ESP_LOGI(TAG, "total_num = %d", total_num);
    if (total_num == 0) {
        return -2;
    } 

    FILE* file = fopen("/spiffs/mqtt.bin", "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return -3;
    }

    int32_t used_num = 0; 
    used_num = mqtt_nvs_used(used_num, NVS_CMD_READ);    // 读取发布了多少了 
    ESP_LOGI(TAG, "used_num = %d", used_num);
 
    if (used_num == total_num) {     // 全部完成，全部清零
        used_num = 0;
        mqtt_nvs_used(used_num, NVS_CMD_WRITE);      // 写0
        mqtt_spiffs_erase(); 
        fclose(file);
        return 0;
    } 
        
    fseek(file, used_num*sizeof(mqtt_data_t), SEEK_SET);  // 从文件头开始设置偏移指针位置

    mqtt_nvs_used(++used_num, NVS_CMD_WRITE);      // 写++

    //for (int i = 0; i < data_num; i++) 
    {
		// fgets((char *)&mqtt_info, sizeof(mqtt_data_t), file);
        fread((void*)mqtt_data, sizeof(uint8_t), sizeof(mqtt_data_t),  file);   

        printf("time: %ld  len: %d\r\n", mqtt_data->timestamp, mqtt_data->len);
        for (int len = 0; len < mqtt_data->len; len++) {
            printf("%02x ", mqtt_data->value[len]);
        } printf("\r\n");
	}

    fclose(file);

    return 1;
}

#endif

void app_user_init(void)
{
    app_led_init();

    app_relay_init();

    app_bell_init();
}
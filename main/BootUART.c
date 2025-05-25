#include <stdio.h>
#include "esp_spiffs.h"
#include "driver/uart.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "BOOT_RESET.h"
#include "Uart_command.h"




static const char* TAG = "file system";
void app_main(void)
{
    esp_vfs_spiffs_conf_t config = 
    {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t result = esp_vfs_spiffs_register(&config);

    if(result != ESP_OK)
    {
        ESP_LOGE(TAG, "Fail to initialize SPIFFS %s", esp_err_to_name(result));
        return;
    }
    size_t total, used;
    result = esp_spiffs_info(config.partition_label, &total, &used);
    if(result != ESP_OK)
    {
        ESP_LOGE(TAG, "Fail to initalize partition info %s", esp_err_to_name(result));
        return;
    }
    else 
    {
        ESP_LOGI(TAG, "Partion info:  \n total: %d \n used: %d", total, used);
        
    }
    init_gpio_reset_boot();
    bootSet(MEMBOOT);
    reset();
    init_uart_boot();
    init_line();
    readout_unprotect();
    init_line();
    erase();
    write_flash("/storage/myprogram.bin");
    bootSet(FLASHBOOT);
    reset();
    


}
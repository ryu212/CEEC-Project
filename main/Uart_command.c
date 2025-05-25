    #include "driver/uart.h"
    #include "Uart_command.h"
    #include <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_log.h"
    #include "driver/gpio.h"
    
    #define BLOCK_SIZE 256 * 1024
    #define ACK 0x79
    #define NACK 0x1F
    #define INIT 0x7F
    #define ERASE 0x43
    #define READOUT_UNPROTECT 0x92
    #define WRITE_MEM 0x31
    QueueHandle_t uart_queue;
    uint8_t ack[1];
    uint8_t cmd[1];
    uint8_t cmd_pl[1];
    uint8_t cmd_combine[2];
    uint8_t data[1];
    uint8_t payload[258];
    uint8_t BLOCK[256];
    uint8_t checksum[1];
    void calculate_checksum(int n)
    {
        uint8_t _checksum = 0;
        for(int i = 0;i< n;i++)
        {
            _checksum ^= BLOCK[i];
        }
    }

    void update_payload_data(int n)
    {
        payload[0] = n;
        for(int i = 0; i<n;i++)
        {
            payload[i+1] = BLOCK[i];
        }
        calculate_checksum(n);
        payload[n+1] = checksum[0];
    }
    void update_payload_address(int32_t addr)
    {
        BLOCK[0] = (addr >> 24) & 0xFF;
        BLOCK[1] = (addr >> 16) & 0xFF;
        BLOCK[2] = (addr >> 8)  & 0xFF;
        BLOCK[3] = addr & 0xFF; 
        calculate_checksum(4);
        payload[4] = checksum[0];
    }
    void init_uart_boot()
    {
        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_EVEN,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
        uart_set_pin(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_2, 1024 * 2, 2048, 10, &uart_queue, 0);
        //gpio_set_pull_mode(GPIO_NUM_17, GPIO_PULLUP_ONLY);
        //gpio_set_pull_mode(GPIO_NUM_16, GPIO_PULLUP_ONLY);
    }
    
    int init_line()
    {
        uint8_t byte_data = 0x7F;
        uart_flush(UART_NUM_2);
        uart_write_bytes(UART_NUM_2, (const char*)&byte_data, 1);
        uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
        uart_flush(UART_NUM_2);
        uart_read_bytes(UART_NUM_2, ack, 1, pdMS_TO_TICKS(3000));
        if (*ack == 0x79)
        {
            printf("init line done\n");
            return 0;
        }
        printf("init line not work \n");
        printf("ack:%d\n", *ack);
        return 1;
    }

    int erase()
    {
        cmd[0] = ERASE;
        cmd_pl[0] = ERASE^0xFF;
        cmd_combine[0] = cmd[0];
        cmd_combine[1] = cmd_pl[0];
        uart_flush(UART_NUM_2);
        uart_write_bytes(UART_NUM_2, cmd_combine, 2);
        uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
        uart_flush(UART_NUM_2);
        uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
        if(ack[0] == ACK)
        {
            printf("ACK 1\n");
        }
        else  if (ack[0] == NACK)
        {
            printf("NACK 1\n");
            return 1;
        }
        else 
        {
            printf("Command not send\n");
            return 1;
        }

        cmd[0] = 0xFF;
        cmd_pl[0] = 0xFF^0xFF;
        cmd_combine[0] = cmd[0];
        cmd_combine[1] = cmd_pl[0];
        uart_flush(UART_NUM_2);
        uart_write_bytes(UART_NUM_2, cmd_combine, 2);
        uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
        uart_flush(UART_NUM_2);
        uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
        if(ack[0] == ACK)
        {
            printf("global erase done\n");
            return 0;
        }
        else  if (ack[0] == NACK)
        {
            printf("NACK 2\n");
            return 1;
        }
        else printf("erase global fail\n");
        return 1;
    }

    int readout_unprotect()
    {
        cmd[0] = READOUT_UNPROTECT;
        cmd_pl[0] = READOUT_UNPROTECT^0xFF;
        cmd_combine[0] = cmd[0];
        cmd_combine[1] = cmd_pl[0];
        uart_flush(UART_NUM_2);
        uart_write_bytes(UART_NUM_2, cmd_combine, 2);
        uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
        uart_flush(UART_NUM_2);
        uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
        if(ack[0] == ACK)
        {
            printf("ACK 1\n");
        }
        else  if (ack[0] == NACK)
        {
            printf("NACK 1\n");
            return 1;
        }
        else 
        {
            printf("Command not send\n");
            return 1;
        }
        uart_flush(UART_NUM_2);
        uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
        if(ack[0] == ACK)
        {
            printf("readout unprotect done\n");
            return 0;
        }
        else  if (ack[0] == NACK)
        {
            printf("NACK 2\n");
            return 1;
        }
        else 
        {
            printf("Command not send\n");
            return 1;
        }
    }   

    int write_flash(char* path)
    {
        FILE* f = fopen(path, "rb");
        if (!f) {
            perror("Không thể mở file\n");
            return 1;
        }
        int32_t start = 0x08000000;
        int32_t pos = 0; 
        int bytes_read = 0;
        while ((bytes_read = fread(BLOCK, 1, 256, f)) > 0) 
        {
            cmd[0] = WRITE_MEM;
            cmd_pl[0] = WRITE_MEM^0xFF;
            cmd_combine[0] = cmd[0];
            cmd_combine[1] = cmd_pl[0];
            uart_flush(UART_NUM_2);
            uart_write_bytes(UART_NUM_2, cmd_combine, 2);
            uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
            uart_flush(UART_NUM_2);
            uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
            if(ack[0] == ACK)
            {
                printf("ACK 1\n");
            }
            else  if (ack[0] == NACK)
            {
                printf("NACK 1\n");
                return 1;
            }
            else 
            {
                printf("Command not send\n");
                return 1;
            }
            update_payload_address(start);
            uart_flush(UART_NUM_2);
            uart_write_bytes(UART_NUM_2, payload, 5);
            uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
            uart_flush(UART_NUM_2);
            uart_read_bytes(UART_NUM_2, ack, 1, 3000 / portTICK_PERIOD_MS);
            if(ack[0] == ACK)
            {
                printf("ACK 2\n");
            }
            else  if (ack[0] == NACK)
            {
                printf("NACK 2\n");
                return 1;
            }
            else 
            {
                printf("Command not send\n");
                return 1;
            }
            update_payload_data(bytes_read);
            uart_flush(UART_NUM_2);
            uart_write_bytes(UART_NUM_2, payload, bytes_read + 2);
            uart_wait_tx_done(UART_NUM_2, 3000 / portTICK_PERIOD_MS);
            uart_flush(UART_NUM_2);
            uart_read_bytes(UART_NUM_2, ack, 1,3000 / portTICK_PERIOD_MS);
            if(ack[0] == ACK)
            {
                printf("payload start add %" PRId32 "send!!!\n", pos);
            }
            else  if (ack[0] == NACK)
            {
                printf("NACK 3\n");
                return 1;
            }
            else 
            {
                printf("Command not send\n");
                return 1;
            }
            pos += bytes_read;
            start += bytes_read;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        printf("done write!!");
        fclose(f);
        return 0;
    
    }



idf_component_register(SRCS "BOOT_RESET.c" "Uart_command.c" "BootUART.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)

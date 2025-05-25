#ifndef UART_CONFIG_H
#define UART_CONFIG_H

void init_uart_boot();
int init_line();
void calculate_checksum(int n);
void update_payload_data(int n);
void update_payload_address(int32_t addr);
int erase();
int readout_unprotect();
int write_flash(char* path);

#endif
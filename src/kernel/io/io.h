#ifndef MELVIX_IO_H
#define MELVIX_IO_H

#include <stdint.h>

/**
 * Receive from specified hardware port
 * @param port The hardware port
 * @return The hardware response
 */
uint8_t receive_b(uint16_t port);

/**
 * Receive from specified hardware port
 * @param port The hardware port
 * @return The hardware response
 */
uint16_t receive_w(uint16_t port);

/**
 * Receive from specified hardware port
 * @param port The hardware port
 * @return The hardware response
 */
uint32_t receive_l(uint16_t port);

/**
 * Send data to the specified hardware port
 * @param port The hardware port
 * @param data The data that should be sent
 */
void send_b(uint16_t port, uint8_t data);

/**
 * Send data to the specified hardware port
 * @param port The hardware port
 * @param data The data that should be sent
 */
void send_w(uint16_t port, uint16_t data);

/**
 * Send data to the specified hardware port
 * @param port The hardware port
 * @param data The data that should be sent
 */
void send_l(uint16_t port, uint32_t data);

/**
 * Initialize the serial conenction
 */
void init_serial();

/**
 * Write a single char to the serial port (QEMU logging)
 * @param ch The char
 */
void serial_put(char ch);

/**
 * Write a string to the serial port (QEMU logging)
 * @param data The string
 */
void serial_write(const char *data);

/**
 * Write a hexadecimal formatted int to the serial port (QEMU logging)
 * @param n The decimal number
 */
void serial_write_hex(int n);

/**
 * Write a decimal number to the serial port (QEMU logging)
 * @param n The decimal number
 */
void serial_write_dec(int n);

#endif

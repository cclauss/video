#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUM_I2C_ADDR (1 << 7)
#define NUM_I2C_REG (0xfe)

void pwr_brd_ctl_init();
void pwr_brd_ctl_task();
bool pwr_brd_raw_gpio_read(uint8_t* val);
void pwr_brd_i2c_bus_scan(bool found[NUM_I2C_ADDR]);
void pwr_brd_i2c_dump_all_regs(int16_t reg_values[NUM_I2C_REG], uint8_t addr);
bool temp_sensor_get_temp_raw(uint8_t* dest);
bool pwr_brd_i2c_read_reg(uint8_t dev_addr, uint8_t reg_id, uint8_t* dest, uint8_t num_bytes);
bool pwr_brd_i2c_write_reg(uint8_t dev_addr, uint8_t reg_id, uint8_t* src, uint8_t num_bytes);
bool pwr_brd_charger_power(bool on);

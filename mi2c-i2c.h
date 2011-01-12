/* 
 * I2C functions for the mi2c driver
 */

#ifndef MI2C_I2C_H
#define MI2C_I2C_H

int mi2c_i2c_write(unsigned char *buf, int count);
int mi2c_i2c_read(unsigned char *buf, int count);

int mi2c_read_reg(int reg);
int mi2c_write_reg(int reg, int val);

int mi2c_init_i2c(void);
void mi2c_cleanup_i2c(void);

#endif 


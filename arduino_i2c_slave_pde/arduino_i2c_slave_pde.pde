/*
  Simple i2c slave for the linux i2c driver example mi2c. 
*/

#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x10
#define BUFFER_SIZE 128

uint8_t i2c_buffer[BUFFER_SIZE];
int cmd;

/*
  This gets called when the master does a write.
*/
void onRecieveHandler(int numBytes)
{
  /* eat all the bytes, just for demo purposes, we don't use them*/
  for (int i = 0; i < numBytes && i < BUFFER_SIZE; i++) {
    i2c_buffer[i] = Wire.receive();
  }  
  
  /* our demo commands are only one byte long */
  cmd = i2c_buffer[0];
}

/*
  This gets called when the master does a read.
  We are simulating a one byte write command followed
  by a two byte response from slave behavior.
*/
void onRequestHandler()
{
  if (cmd == 0x01) {
    i2c_buffer[0] = 0x1e;
    i2c_buffer[1] = 0x55;
  }
  else if (cmd == 0x02) {
    i2c_buffer[0] = 0xbe;
    i2c_buffer[1] = 0xef;
  }
  else {
    i2c_buffer[0] = 0xa5;
    i2c_buffer[1] = 0x81;
  }
  
  Wire.send(i2c_buffer, 2);
}

void begin_i2c()
{
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onReceive(onRecieveHandler);
  Wire.onRequest(onRequestHandler);
}

void setup()
{
  begin_i2c();  
}

void loop()
{
}

#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"
#include "logging.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  RETURN_IF_ERROR_CODE(writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode));

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  if(temp == NULL) return ERR_CODE_INVALID_ARG;

  uint8_t writeBuffer[1] = {0};
  uint8_t readBuffer[2] = {0};

  error_code_t errCode;
  RETURN_IF_ERROR_CODE(i2cSendTo(devAddr, writeBuffer, sizeof(writeBuffer)));
  RETURN_IF_ERROR_CODE(i2cReceiveFrom(devAddr, readBuffer, sizeof(readBuffer)));

//  Unsure if conversion from twos complement is neccessary, if we're only using this sensor to detect hight temp then it probably isn't?
//  The code below works if we assume we wont recieve negative numbers, the active code includes twos complement conversions to cover edge cases
//  uint16_t convertedTemp = (readBuffer[0] << 3) | (readBuffer[1] >> 5);
//  *temp = convertedTemp * 0.125;

//  Conversion from twos complement, and conversion to Celsius
/*
  uint16_t convertedTemp = (readBuffer[0] << 3) | (readBuffer[1] >> 5);
  if(readBuffer[0] & 0x80){
      convertedTemp = (convertedTemp ^ 0x07FF) + 1;
      *temp = convertedTemp * -0.125;
  }
  else{
      *temp = convertedTemp * 0.125;
  }
*/
  int16_t convertedTemp = (readBuffer[0]<<8) | readBuffer[1];
  *temp = (float)(convertedTemp >> 5) * 0.125;
  
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}

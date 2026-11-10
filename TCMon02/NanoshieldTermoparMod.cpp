/**
 * @file NanoshieldTermoparMod.cpp
 * This is the library to access the Termopar Nanoshield version 2
 * 
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 */

#include "NanoshieldTermoparMod.h"

// MAX31856 registers
#define MAX31856_REG_CR0    0x00
#define MAX31856_REG_CR1    0x01
#define MAX31856_REG_MASK   0x02
#define MAX31856_REG_CJHF   0x03
#define MAX31856_REG_CJLF   0x04
#define MAX31856_REG_LTHFTH 0x05
#define MAX31856_REG_LTHFTL 0x06
#define MAX31856_REG_LTLFTH 0x07
#define MAX31856_REG_LTLFTL 0x08
#define MAX31856_REG_CJTO   0x09
#define MAX31856_REG_CJTH   0x0A
#define MAX31856_REG_CJTL   0x0B
#define MAX31856_REG_LTCBH  0x0C
#define MAX31856_REG_LTCBM  0x0D
#define MAX31856_REG_LTCBL  0x0E
#define MAX31856_REG_SR     0x0F

// MAX31856 register read/write masks
#define MAX31856_REG_READ   0x00
#define MAX31856_REG_WRITE  0x80

SPISettings NanoshieldTermoparMod::spiSettings = SPISettings(1000000, MSBFIRST, SPI_MODE1);

NanoshieldTermoparMod::NanoshieldTermoparMod(uint8_t cs, TcType type, TcAveraging avg) {
  this->cs = cs;
  this->type = type;
  this->avg = avg;
	this->internal = 0;
	this->external = 0;
	this->fault = 0;
  this->manual = 0;
}

void NanoshieldTermoparMod::beginManualMode() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPI.begin();

  // Initialize MAX31856
  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CR0 | MAX31856_REG_WRITE);
  SPI.transfer(0x20); // Setup CR0 register:
                      //   Automatic conversion mode
                      //   Enable fault detection mode 1 (OCFAULT)
                      //   Cold juntion sensor enabled
                      //   Fault detection in comparator mode
                      //   Noise rejection = 60Hz
  SPI.transfer(((uint8_t)this->avg << 4) | ((uint8_t)this->type & 0x0F)); // Setup CR1 register:
                                                              //   Setup selected averaging mode
                                                              //   Setup selected thermocouple type
  SPI.transfer(0x03); // Setup MASK register:
                      //   Enable overvoltage/undervoltage detection
                      //   Enable open circuit detection
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  this->manual = 1;
}

void NanoshieldTermoparMod::begin() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPI.begin();

  // Initialize MAX31856
  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CR0 | MAX31856_REG_WRITE);
  SPI.transfer(0x90); // Setup CR0 register:
                      //   Automatic conversion mode
                      //   Enable fault detection mode 1 (OCFAULT)
                      //   Cold juntion sensor enabled
                      //   Fault detection in comparator mode
                      //   Noise rejection = 60Hz
  SPI.transfer(((uint8_t)avg << 4) | ((uint8_t)type & 0x0F)); // Setup CR1 register:
                                                              //   Setup selected averaging mode
                                                              //   Setup selected thermocouple type
  SPI.transfer(0x03); // Setup MASK register:
                      //   Enable overvoltage/undervoltage detection
                      //   Enable open circuit detection
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  this->manual = 0;
}

void NanoshieldTermoparMod::read() {
  if (this->manual == 1) {
    this->manualRead();
    return;
  }
	uint16_t cj = 0;
	uint32_t ltc = 0;

  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CJTH | MAX31856_REG_READ);
  cj |= (uint16_t)SPI.transfer(0) << 8;
  cj |= SPI.transfer(0);
  ltc |= (uint32_t)SPI.transfer(0) << 24;
  ltc |= (uint32_t)SPI.transfer(0) << 16;
  ltc |= (uint32_t)SPI.transfer(0) << 8;
	fault = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();  
  
  internal = (cj / 4) * 0.015625;
  external = (ltc / 8192) * 0.0078125;
  this->cjx = cj;
  this->ltcx = ltc;
}

void NanoshieldTermoparMod::manualRead() {
  
  uint16_t cj = 0;
  uint32_t ltc = 0;

  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CR0 | MAX31856_REG_WRITE);
  SPI.transfer(0x50); // Setup CR0 register:
                      //   Automatic conversion mode
                      //   Enable fault detection mode 1 (OCFAULT)
                      //   Cold juntion sensor enabled
                      //   Fault detection in comparator mode
                      //   Noise rejection = 60Hz
  digitalWrite(cs, HIGH);

  //delay(50);
  switch(this->avg) {
    case TC_AVG_OFF: delay(170); break;
    case TC_AVG_2_SAMPLES: delay(205); break;
    case TC_AVG_4_SAMPLES: delay(275); break;
    case TC_AVG_8_SAMPLES: delay(400); break;
    default: delay(670);
  }
  
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CJTH | MAX31856_REG_READ);
  cj |= (uint16_t)SPI.transfer(0) << 8;
  cj |= SPI.transfer(0);
  ltc |= (uint32_t)SPI.transfer(0) << 24;
  ltc |= (uint32_t)SPI.transfer(0) << 16;
  ltc |= (uint32_t)SPI.transfer(0) << 8;
  fault = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();

  this->cjx = cj;
  this->ltcx = ltc;
  
}

void NanoshieldTermoparMod::askForManualRead() {

  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CR0 | MAX31856_REG_WRITE);
  SPI.transfer(0x50); // Setup CR0 register:
                      //   Automatic conversion mode
                      //   Enable fault detection mode 1 (OCFAULT)
                      //   Cold juntion sensor enabled
                      //   Fault detection in comparator mode
                      //   Noise rejection = 60Hz
  digitalWrite(cs, HIGH);

}

void NanoshieldTermoparMod::getManualRead() {

  uint16_t cj = 0;
  uint32_t ltc = 0;
  SPI.beginTransaction(spiSettings);

  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CJTH | MAX31856_REG_READ);
  cj |= (uint16_t)SPI.transfer(0) << 8;
  cj |= SPI.transfer(0);
  ltc |= (uint32_t)SPI.transfer(0) << 24;
  ltc |= (uint32_t)SPI.transfer(0) << 16;
  ltc |= (uint32_t)SPI.transfer(0) << 8;
  fault = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();

  this->cjx = cj;
  this->ltcx = ltc;

}

void NanoshieldTermoparMod::setType(TcType type) {
  this->type = type;
  // this->beginManualMode();
}

double NanoshieldTermoparMod::getInternal() {
	return internal;
}

double NanoshieldTermoparMod::getExternal() {
	return external;
}

uint16_t NanoshieldTermoparMod::getInternalBytes() {
  return cjx;
}

uint32_t NanoshieldTermoparMod::getExternalBytes() {
  return ltcx;
}

bool NanoshieldTermoparMod::isExternalOutOfRange() {
	return (fault & 0x40) != 0;
}

bool NanoshieldTermoparMod::isInternalOutOfRange() {
	return (fault & 0x80) != 0;
}

bool NanoshieldTermoparMod::isOverUnderVoltage() {
	return (fault & 0x02) != 0;
}

bool NanoshieldTermoparMod::isOpen() {
	return (fault & 0x01) != 0;
}

bool NanoshieldTermoparMod::hasError() {
	return (fault & 0xC3) != 0;
}

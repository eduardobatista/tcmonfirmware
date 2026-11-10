#include <SPI.h>
#include "NanoshieldTermoparMod.h"

#define CONTROLOUT 10  // Digital pin number 10

#define SAMPLEPERIOD 5 // in milliseconds
#define PWMCYCLES 50  // Numer of sample period cycles

#define CTRLTIMEOUT 5000 // in milliseconds

// Identificação devolvida pelo comando 'v' (terminada em '\n').
#define FW_VERSION "TCMon02 2.1"

// Termopar Nanoshield on CS pin D8, type T thermocouple, no averaging
NanoshieldTermoparMod tc(8, TC_TYPE_T, TC_AVG_OFF);

uint32_t lpc;
uint16_t cj;
uint16_t adc[2];

unsigned long lastsampletime; 

void setup()
{
    Serial.begin(19200,SERIAL_8N1);
    tc.beginManualMode();
    // tc.begin();

    // Set mux pins:
    pinMode(9, OUTPUT);
    pinMode(6, OUTPUT);    
    pinMode(5, OUTPUT);
    
    pinMode(4, OUTPUT);
    digitalWrite(4,HIGH);
    adc[0] = analogRead(0);
    adc[1] = analogRead(1);

    // Pino CONTROLOUT de Saída PWM 
    // (não será usado PWM interno, por ser muito rápido e sobrecarregar o MOSFET)
    pinMode(CONTROLOUT, OUTPUT);
    
    // Garantindo que a placa de aquecimento está desligada:
    digitalWrite(CONTROLOUT, HIGH);

    lastsampletime = millis();
    
}

void setaMux(char c) {    
     c = c - 0x30;
     digitalWrite(5, c & 0x01);
     digitalWrite(6, (c & 0x02) >> 1);    
     digitalWrite(9, (c & 0x04) >> 2);
}

void printErrors() {  
    Serial.write(0x80);
    // Serial.write(0x00);
    Serial.write(tc.fault);   
    if (tc.isOpen()) {
        Serial.write(0x00);
    } else if (tc.isOverUnderVoltage()) {
        Serial.write(0x01);
    } else if (tc.isInternalOutOfRange()) {
        Serial.write(0x02);
    } else if (tc.isExternalOutOfRange()) {
        Serial.write(0x04);
    } else {
        // Falha sem código identificado: mantém a resposta com 5 bytes.
        Serial.write(0xFF);
    }
    Serial.write(cj >> 8);
    Serial.write(cj & 0xFF);
}

void printData() {
    lpc = tc.getExternalBytes();
    cj = tc.getInternalBytes();
                   
    Serial.write(lpc >> 24);
    Serial.write((lpc >> 16) & 0xFF);
    Serial.write((lpc >> 8) & 0xFF);

    Serial.write(cj >> 8);
    Serial.write(cj & 0xFF); 
}


char data;
int16_t valaux;
uint8_t outaux;
bool pwmon = true;
uint8_t pwmduty = 0;
uint8_t newpwmduty = 0;
uint8_t pwmcounter = 0;
int16_t delayms = 0;
uint8_t hascmd = 0;
uint16_t ctcontroltimeout = 0;
void loop() {

    while ( (millis() - lastsampletime) < SAMPLEPERIOD ) {
       delayMicroseconds(10);
    }
    lastsampletime = lastsampletime + SAMPLEPERIOD;
    if ( (millis() - lastsampletime) > SAMPLEPERIOD ) { lastsampletime = millis(); }  

    // Slow PWM -------------------------------
    if (pwmon) {
      if (ctcontroltimeout > CTRLTIMEOUT) {
        digitalWrite(CONTROLOUT, HIGH); // Turn OFF!
      } else {
        ctcontroltimeout = ctcontroltimeout + SAMPLEPERIOD;
        if (pwmduty == 0) {
          digitalWrite(CONTROLOUT, HIGH); // Turn OFF!
        } else if (pwmcounter == 0) {
          digitalWrite(CONTROLOUT, LOW); // Turn ON!
        } else if (pwmcounter == pwmduty) {
          digitalWrite(CONTROLOUT, HIGH); // Turn OFF!
        }
        pwmcounter++;
        if (pwmcounter == PWMCYCLES) { 
          pwmcounter = 0;
          pwmduty = newpwmduty; 
        }
      }

      
    }
    // ----------------------------------------


    if (delayms >= SAMPLEPERIOD) {
      delayms = delayms - SAMPLEPERIOD; 
      return;   
    } 


    if ( hascmd != 0 ) {

      switch (hascmd) {

        case 'R':
          // tc.read();
          tc.getManualRead(); 
          if (tc.hasError()) { printErrors(); } 
          else { printData(); }
          hascmd = 0;
          break;

        case 'r':
          data = Serial.read();
          setaMux(data);
          tc.askForManualRead();
          delayms = 175;
          hascmd = 'R';
          break;

        case 's':
          data = Serial.read();
          switch (data) {
              case 'T': tc.setType(TC_TYPE_T); break;
              case 'B': tc.setType(TC_TYPE_B); break;
              case 'E': tc.setType(TC_TYPE_E); break;
              case 'J': tc.setType(TC_TYPE_J); break;
              case 'K': tc.setType(TC_TYPE_K); break;
              case 'N': tc.setType(TC_TYPE_N); break;
              case 'R': tc.setType(TC_TYPE_R); break;
              case 'S': tc.setType(TC_TYPE_S); break;
              default: tc.setType(TC_TYPE_T);
          }
          tc.beginManualMode();
          Serial.write('k');
          Serial.write(data);
          hascmd = 0;
          break;

        case 'm':
          newpwmduty = Serial.read() >> 1; //round(((float)Serial.read()) / 100.0 * ((float)PWMCYCLES)) ;
          hascmd = 0;
          ctcontroltimeout = 0;
          break;

        case 'a':
          data = Serial.read() & 0x01;
          adc[data] = analogRead(data);
          Serial.write((adc[data] >> 8) & 0xFF);
          Serial.write((adc[data]) & 0xFF);
          // Serial.println("Leitura:");
          // Serial.println(adc[data]);
          hascmd = 0;
          break;

      }
      
    } else if (Serial.available() > 0) {
  
       data = Serial.read();

       switch (data) {

        case 'h':
          // tc.begin();
          tc.beginManualMode();
          Serial.write('k');
          break;

        case 'r':
          delayms = 5;
          hascmd = 'r';
          break;

        case 'c':
          tc.read();
          cj = tc.getInternalBytes();
          Serial.write(cj >> 8);
          Serial.write(cj & 0xFF);
          break;

        case 's':
          delayms = 5;
          hascmd = 's';
          break;

        case 'm':
          delayms = 20;
          hascmd = 'm';
          break;

        case 'a':
          delayms = 5;
          hascmd = 'a';
          break;

        case 'v':
          Serial.print(FW_VERSION "\n");
          break;

        case 't':
          unsigned long ax = millis();
          // delay(1);
          // tc.read();
          printData();
          ax = millis() - ax;
          Serial.println("---");
          Serial.println(ax); 
          Serial.println(tc.manual);
          if (tc.hasError()) {
            printErrors();
          } else {  
            printData();
          } 
          break;

        
        default:
          break;

       }
    
    } 
      
}

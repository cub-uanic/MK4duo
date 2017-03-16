/**
 * MK4duo 3D Printer Firmware
 *
 * Based on Marlin, Sprinter and grbl
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 * Copyright (C) 2013 - 2016 Alberto Cotronei @MagoKimbra
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * This is the main Hardware Abstraction Layer (HAL).
 * To make the firmware work with different processors and toolchains,
 * all hardware related code should be packed into the hal files.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Description:          *** HAL for Arduino ***
 *
 * Contributors:
 * Copyright (c) 2014 Bob Cousins bobcousins42@googlemail.com
 *                    Nico Tonnhofer wurstnase.reprap@gmail.com
 *
 * Copyright (c) 2015 - 2016 Alberto Cotronei @MagoKimbra
 *
 * ARDUINO_ARCH_ARM
 */

#ifndef HAL_AVR_H
#define HAL_AVR_H

// --------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "fastio.h"
#include "watchdog_AVR.h"

//#define EXTERNALSERIAL  // Force using arduino serial
#ifndef EXTERNALSERIAL
  #include "HardwareSerial.h"
  #define MKSERIAL MKSerial
#else
  #define MKSERIAL Serial
#endif

// --------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------

#ifndef CRITICAL_SECTION_START
  #define CRITICAL_SECTION_START  unsigned char _sreg = SREG; cli();
  #define CRITICAL_SECTION_END    SREG = _sreg;
#endif

#ifdef analogInputToDigitalPin
  #undef analogInputToDigitalPin
#endif

#define analogInputToDigitalPin(p) ((p) + 0xA0)

#define PACK

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#undef LOW
#define LOW         0
#undef HIGH
#define HIGH        1

// Voltage for Pin
#define HAL_VOLTAGE_PIN 5.0

#define ADV_NEVER 65535

/**
 * Optimized math functions for AVR
 */

// intRes = intIn1 * intIn2 >> 16
// uses:
// r26 to store 0
// r27 to store the byte 1 of the 24 bit result
#define MultiU16X8toH16(intRes, charIn1, intIn2) \
  asm volatile ( \
                 "clr r26 \n\t" \
                 "mul %A1, %B2 \n\t" \
                 "movw %A0, r0 \n\t" \
                 "mul %A1, %A2 \n\t" \
                 "add %A0, r1 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "lsr r0 \n\t" \
                 "adc %A0, r26 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "clr r1 \n\t" \
                 : \
                 "=&r" (intRes) \
                 : \
                 "d" (charIn1), \
                 "d" (intIn2) \
                 : \
                 "r26" \
               )

// intRes = longIn1 * longIn2 >> 24
// uses:
// r26 to store 0
// r27 to store bits 16-23 of the 48bit result. The top bit is used to round the two byte result.
// note that the lower two bytes and the upper byte of the 48bit result are not calculated.
// this can cause the result to be out by one as the lower bytes may cause carries into the upper ones.
// B0 A0 are bits 24-39 and are the returned value
// C1 B1 A1 is longIn1
// D2 C2 B2 A2 is longIn2
//
#define MultiU24X32toH16(intRes, longIn1, longIn2) \
  asm volatile ( \
                 "clr r26 \n\t" \
                 "mul %A1, %B2 \n\t" \
                 "mov r27, r1 \n\t" \
                 "mul %B1, %C2 \n\t" \
                 "movw %A0, r0 \n\t" \
                 "mul %C1, %C2 \n\t" \
                 "add %B0, r0 \n\t" \
                 "mul %C1, %B2 \n\t" \
                 "add %A0, r0 \n\t" \
                 "adc %B0, r1 \n\t" \
                 "mul %A1, %C2 \n\t" \
                 "add r27, r0 \n\t" \
                 "adc %A0, r1 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "mul %B1, %B2 \n\t" \
                 "add r27, r0 \n\t" \
                 "adc %A0, r1 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "mul %C1, %A2 \n\t" \
                 "add r27, r0 \n\t" \
                 "adc %A0, r1 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "mul %B1, %A2 \n\t" \
                 "add r27, r1 \n\t" \
                 "adc %A0, r26 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "lsr r27 \n\t" \
                 "adc %A0, r26 \n\t" \
                 "adc %B0, r26 \n\t" \
                 "mul %D2, %A1 \n\t" \
                 "add %A0, r0 \n\t" \
                 "adc %B0, r1 \n\t" \
                 "mul %D2, %B1 \n\t" \
                 "add %B0, r0 \n\t" \
                 "clr r1 \n\t" \
                 : \
                 "=&r" (intRes) \
                 : \
                 "d" (longIn1), \
                 "d" (longIn2) \
                 : \
                 "r26" , "r27" \
               )

// TEMPERATURE
#if EXTRUDERS > 0 && HAS(TEMP_0)
  #define EXT0_ANALOG_INPUTS 1
  #define EXT0_SENSOR_INDEX 0
  #define EXT0_ANALOG_CHANNEL TEMP_0_PIN
  #define ACCOMMA0 ,
#else
  #define EXT0_ANALOG_INPUTS 0
  #define EXT0_SENSOR_INDEX TEMP_0_PIN
  #define EXT0_ANALOG_CHANNEL
  #define ACCOMMA0
#endif

#if EXTRUDERS > 1 && HAS(TEMP_1)
  #define EXT1_ANALOG_INPUTS 1
  #define EXT1_SENSOR_INDEX EXT0_ANALOG_INPUTS
  #define EXT1_ANALOG_CHANNEL ACCOMMA0 TEMP_1_PIN
  #define ACCOMMA1 ,
#else
  #define EXT1_ANALOG_INPUTS 0
  #define EXT1_SENSOR_INDEX TEMP_1_PIN
  #define EXT1_ANALOG_CHANNEL
  #define ACCOMMA1 ACCOMMA0
#endif

#if EXTRUDERS > 2 && HAS(TEMP_2)
  #define EXT2_ANALOG_INPUTS 1
  #define EXT2_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS
  #define EXT2_ANALOG_CHANNEL ACCOMMA1 TEMP_2_PIN
  #define ACCOMMA2 ,
#else
  #define ACCOMMA2 ACCOMMA1
  #define EXT2_ANALOG_INPUTS 0
  #define EXT2_SENSOR_INDEX TEMP_2_PIN
  #define EXT2_ANALOG_CHANNEL
#endif

#if EXTRUDERS > 3 && HAS(TEMP_3)
  #define EXT3_ANALOG_INPUTS 1
  #define EXT3_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS
  #define EXT3_ANALOG_CHANNEL ACCOMMA2 TEMP_3_PIN
  #define ACCOMMA3 ,
#else
  #define ACCOMMA3 ACCOMMA2
  #define EXT3_ANALOG_INPUTS 0
  #define EXT3_SENSOR_INDEX TEMP_3_PIN
  #define EXT3_ANALOG_CHANNEL
#endif

#if HAS(TEMP_BED)
  #define BED_ANALOG_INPUTS 1
  #define BED_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS
  #define BED_ANALOG_CHANNEL ACCOMMA3 TEMP_BED_PIN
  #define BED_KOMMA ,
#else
  #define BED_ANALOG_INPUTS 0
  #define BED_SENSOR_INDEX TEMP_BED_PIN
  #define BED_ANALOG_CHANNEL
  #define BED_KOMMA ACCOMMA3
#endif

#if HAS(TEMP_CHAMBER)
  #define CHAMBER_ANALOG_INPUTS 1
  #define CHAMBER_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS+BED_ANALOG_INPUTS
  #define CHAMBER_ANALOG_CHANNEL BED_KOMMA TEMP_CHAMBER_PIN
  #define CHAMBER_KOMMA ,
#else
  #define CHAMBER_ANALOG_INPUTS 0
  #define CHAMBER_SENSOR_INDEX TEMP_CHAMBER_PIN
  #define CHAMBER_ANALOG_CHANNEL
  #define CHAMBER_KOMMA BED_KOMMA
#endif

#if HAS(TEMP_COOLER)
  #define COOLER_ANALOG_INPUTS 1
  #define COOLER_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS+BED_ANALOG_INPUTS+CHAMBER_ANALOG_INPUTS
  #define COOLER_ANALOG_CHANNEL CHAMBER_KOMMA TEMP_COOLER_PIN
  #define COOLER_KOMMA ,
#else
  #define COOLER_ANALOG_INPUTS 0
  #define COOLER_SENSOR_INDEX TEMP_COOLER_PIN
  #define COOLER_ANALOG_CHANNEL
  #define COOLER_KOMMA CHAMBER_KOMMA
#endif

#if HAS(FILAMENT_SENSOR)
  #define FILAMENT_ANALOG_INPUTS 1
  #define FILAMENT_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS+BED_ANALOG_INPUTS+CHAMBER_ANALOG_INPUTS+COOLER_ANALOG_INPUTS
  #define FILAMENT_ANALOG_CHANNEL COOLER_KOMMA FILWIDTH_PIN
  #define FILAMENT_KOMMA ,
#else
  #define FILAMENT_ANALOG_INPUTS 0
  #define FILAMENT_SENSOR_INDEX FILWIDTH_PIN
  #define FILAMENT_ANALOG_CHANNEL
  #define FILAMENT_KOMMA COOLER_KOMMA
#endif

#if HAS(POWER_CONSUMPTION_SENSOR)
  #define POWER_ANALOG_INPUTS 1
  #define POWER_SENSOR_INDEX EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS+BED_ANALOG_INPUTS+CHAMBER_ANALOG_INPUTS+COOLER_ANALOG_INPUTS+FILAMENT_ANALOG_INPUTS
  #define POWER_ANALOG_CHANNEL FILAMENT_KOMMA POWER_CONSUMPTION_PIN
#else
  #define POWER_ANALOG_INPUTS 0
  #define POWER_ANALOG_CHANNEL
#endif

#define ANALOG_INPUTS (EXT0_ANALOG_INPUTS+EXT1_ANALOG_INPUTS+EXT2_ANALOG_INPUTS+EXT3_ANALOG_INPUTS+BED_ANALOG_INPUTS+CHAMBER_ANALOG_INPUTS+COOLER_ANALOG_INPUTS+FILAMENT_ANALOG_INPUTS+POWER_ANALOG_INPUTS)
#if ANALOG_INPUTS > 0
  /** Channels are the MUX-part of ADMUX register */
  #define ANALOG_INPUT_CHANNELS {EXT0_ANALOG_CHANNEL EXT1_ANALOG_CHANNEL EXT2_ANALOG_CHANNEL EXT3_ANALOG_CHANNEL BED_ANALOG_CHANNEL CHAMBER_ANALOG_CHANNEL COOLER_ANALOG_CHANNEL FILAMENT_ANALOG_CHANNEL POWER_ANALOG_CHANNEL}
#endif

#define ANALOG_REF_AREF 0
#define ANALOG_REF_AVCC _BV(REFS0)
#define ANALOG_REF ANALOG_REF_AVCC
#define ANALOG_PRESCALER _BV(ADPS0)|_BV(ADPS1)|_BV(ADPS2)
#define OVERSAMPLENR 16
// Temperature PID_dT
#define PID_dT ((OVERSAMPLENR * 10) / (float)(TEMP_TIMER_FREQUENCY * PID_dT_FACTOR))

// --------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------

typedef uint16_t HAL_TIMER_TYPE;
typedef uint32_t millis_t;

// --------------------------------------------------------------------------
// Timer
// --------------------------------------------------------------------------

#define HAL_STEPPER_TIMER_RATE  ((F_CPU) / 8.0)
#define TEMP_TIMER_FREQUENCY    ((F_CPU) / 64.0 / 256.0)

// Delays
#define CYCLES_EATEN_BY_CODE 240
#define CYCLES_EATEN_BY_E     60

#define STEPPER_TIMER OCR1A
#define TEMP_TIMER 0

#define ENABLE_STEPPER_INTERRUPT()    SBI(TIMSK1, OCIE1A)
#define DISABLE_STEPPER_INTERRUPT()   CBI(TIMSK1, OCIE1A)

#define ENABLE_TEMP_INTERRUPT()       SBI(TIMSK0, OCIE0B)
#define DISABLE_TEMP_INTERRUPT()      CBI(TIMSK0, OCIE0B)

#define HAL_timer_start (timer_num, frequency)
#define HAL_timer_set_count(timer, count) timer = (count)
#define HAL_timer_isr_prologue(timer_num)

#define HAL_TIMER_SET_STEPPER_COUNT(n)  HAL_timer_set_count(STEPPER_TIMER, n)
#define HAL_TIMER_SET_TEMP_COUNT(n)     HAL_timer_set_count(TEMP_TIMER, n)

#define HAL_STEP_TIMER_ISR  ISR(TIMER1_COMPA_vect)
#define HAL_TEMP_TIMER_ISR  ISR(TIMER0_COMPB_vect)

#define _ENABLE_ISRs() \
        do { \
          cli(); \
          ENABLE_TEMP_INTERRUPT(); \
          ENABLE_STEPPER_INTERRUPT(); \
        } while(0)

#define _DISABLE_ISRs() \
        do { \
          DISABLE_TEMP_INTERRUPT(); \
          DISABLE_STEPPER_INTERRUPT(); \
          sei(); \
        } while(0)

// Clock speed factor
#define CYCLES_PER_US ((F_CPU) / 1000000UL) // 16 or 20
// Stepper pulse duration, in cycles
#define STEP_PULSE_CYCLES ((MINIMUM_STEPPER_PULSE) * CYCLES_PER_US)

class InterruptProtectedBlock {
  uint8_t sreg;
  public:
    inline void protect() {
      cli();
    }

    inline void unprotect() {
      SREG = sreg;
    }

    inline InterruptProtectedBlock(bool later = false) {
      sreg = SREG;
      if (!later) cli();
    }

    inline ~InterruptProtectedBlock() {
      SREG = sreg;
    }
};

class HAL {
  public:

    HAL();

    virtual ~HAL();

    static unsigned long AnalogInputValues[ANALOG_INPUTS];
    static bool Analog_is_ready;

    // do any hardware-specific initialization here
    static inline void hwSetup(void) { /* noop */ }

    static inline void clear_reset_source() { MCUSR = 0; }
    static inline uint8_t get_reset_source() { return MCUSR; }

    static int getFreeRam();
    static void resetHardware();

    static void analogStart();
    static void analogRead();

    // SPI related functions
    static void spiBegin() {
      #if SS_PIN >= 0
        SET_INPUT(MISO_PIN);
        SET_OUTPUT(MOSI_PIN);
        SET_OUTPUT(SCK_PIN);
        // SS must be in output mode even it is not chip select
        SET_OUTPUT(SS_PIN);
        // set SS high - may be chip select for another SPI device
        WRITE(SS_PIN, HIGH);
      #endif
    }
    static inline void spiInit(uint8_t spiRate) {
      uint8_t r = 0;
      for (uint8_t b = 2; spiRate > b && r < 6; b <<= 1, r++);

      SET_OUTPUT(SS_PIN);
      WRITE(SS_PIN, HIGH);
      SET_OUTPUT(SCK_PIN);
      SET_OUTPUT(MOSI_PIN);
      SET_INPUT(MISO_PIN);
      #ifdef PRR
        PRR &= ~(1 << PRSPI);
      #elif defined PRR0
        PRR0 &= ~(1 << PRSPI);
      #endif
      // See avr processor documentation
      SPCR = (1 << SPE) | (1 << MSTR) | (r >> 1);
      SPSR = (r & 1 || r == 6 ? 0 : 1) << SPI2X;
    }
    static inline uint8_t spiReceive(uint8_t send = 0xFF) {
      SPDR = send;
      while (!(SPSR & (1 << SPIF))) {}
      return SPDR;
    }
    static inline void spiReadBlock(uint8_t* buf, size_t nbyte) {
      if (nbyte-- == 0) return;
      SPDR = 0XFF;
      for (size_t i = 0; i < nbyte; i++) {
        while (!(SPSR & (1 << SPIF))) {}
        buf[i] = SPDR;
        SPDR = 0XFF;
      }
      while (!(SPSR & (1 << SPIF))) {}
      buf[nbyte] = SPDR;
    }
    static inline void spiSend(uint8_t b) {
      SPDR = b;
      while (!(SPSR & (1 << SPIF))) {}
    }
    static inline void spiSend(const uint8_t* buf, size_t n) {
      if (n == 0) return;
      SPDR = buf[0];
      if (n > 1) {
        uint8_t b = buf[1];
        size_t i = 2;
        while (1) {
          while (!(SPSR & (1 << SPIF))) {}
          SPDR = b;
          if (i == n) break;
          b = buf[i++];
        }
      }
      while (!(SPSR & (1 << SPIF))) {}
    }
    static inline __attribute__((always_inline))
    void spiSendBlock(uint8_t token, const uint8_t* buf) {
      SPDR = token;
      for (uint16_t i = 0; i < 512; i += 2) {
        while (!(SPSR & (1 << SPIF))) {}
        SPDR = buf[i];
        while (!(SPSR & (1 << SPIF))) {}
        SPDR = buf[i + 1];
      }
      while (!(SPSR & (1 << SPIF))) {}
    }

    static inline void digitalWrite(uint8_t pin,uint8_t value) {
      ::digitalWrite(pin, value);
    }
    static inline uint8_t digitalRead(uint8_t pin) {
      return ::digitalRead(pin);
    }
    static inline void pinMode(uint8_t pin,uint8_t mode) {
      ::pinMode(pin, mode);
    }

    static inline void delayMicroseconds(unsigned int delayUs) {
      ::delayMicroseconds(delayUs);
    }
    static inline void delayMilliseconds(unsigned int delayMs) {
      ::delay(delayMs);
    }
    static inline unsigned long timeInMilliseconds() {
      return millis();
    }

    static inline void serialSetBaudrate(long baud) {
      MKSERIAL.begin(baud);
    }
    static inline bool serialByteAvailable() {
      return MKSERIAL.available() > 0;
    }
    static inline uint8_t serialReadByte() {
      return MKSERIAL.read();
    }
    static inline void serialWriteByte(char b) {
      MKSERIAL.write(b);
    }
    static inline void serialFlush() {
      MKSERIAL.flush();
    }

  protected:
  private:

    static int32_t  AnalogInputRead[ANALOG_INPUTS];
    static uint8_t  adcCounter[ANALOG_INPUTS],
                    adcSamplePos;
    static const uint8_t AnalogInputChannels[] PROGMEM;
};

/**
 * math function
 */

#undef ATAN2
#undef FABS
#undef POW
#undef SQRT
#undef CEIL
#undef FLOOR
#undef LROUND
#undef FMOD
#define ATAN2(y, x) atan2(y, x)
#define FABS(x) fabs(x)
#define POW(x, y) pow(x, y)
#define SQRT(x) sqrt(x)
#define CEIL(x) ceil(x)
#define FLOOR(x) floor(x)
#define LROUND(x) lround(x)
#define FMOD(x, y) fmod(x, y)

#endif // HAL_AVR_H

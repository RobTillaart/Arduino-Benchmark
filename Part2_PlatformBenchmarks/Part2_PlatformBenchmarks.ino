/*
 * Universal Arduino Benchmark Suite — Part 2: Platform Benchmarks
 * Compatible with: Arduino (AVR, ARM, SAMD), ESP32, ESP8266, RP2040 (Pico), STM32, and more!
 *
 * This benchmark tests:
 * - CPU: Stress testing with temperature tracking
 * - Communication: I2C (Wire)
 * - Connectivity: WiFi throughput, BLE/Bluetooth, LED matrix (Uno R4)
 * - Cryptography: SHA-1 (RP2040), AES/MD5/hardware RNG (ESP32)
 * - Multi-core: Dual-core performance (ESP32, Portenta H7)
 * - Board-specific: ESP32 DAC/touch/sleep, RP2040 DMA/PIO, Yun Bridge, and more
 *
 * METHODOLOGY:
 * - All computation benchmarks use volatile accumulators to prevent compiler optimization
 * - Results are checksummed and printed to ensure actual execution
 * - Checksums are consumed outside timed blocks for accuracy
 * - Serial timing includes flush() to measure actual transmission time
 * - ESP32: CPU cycle counter used to cross-check micros() jitter
 * - Temperature tracking (CPU stress test only) shows start temp and total gain
 * - All performance metrics reported as operations per millisecond (ops/ms) or appropriate units per test
 *
 * Upload to ANY Arduino-compatible board and view results in Serial Monitor (115200 baud)
 */

#include <limits.h>

#ifndef NOT_AN_INTERRUPT
#define NOT_AN_INTERRUPT -1
#endif

// ==================== BOARD DETECTION ====================

// ESP32 Family (all variants)
#if defined(ESP32)
#if defined(ARDUINO_NANO_ESP32)
#define BOARD_NAME "Arduino Nano ESP32"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define BOARD_NAME "ESP32-S3"
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#define BOARD_NAME "ESP32-S2"
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define BOARD_NAME "ESP32-C3"
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
#define BOARD_NAME "ESP32-C6"
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
#define BOARD_NAME "ESP32-H2"
#else
#define BOARD_NAME "ESP32"
#endif
#define HAS_WIFI
#define HAS_BLE
#ifdef BOARD_HAS_PSRAM
#define HAS_PSRAM
#endif
#include <WiFi.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <driver/gpio.h>
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/version.h"
#if defined(MBEDTLS_SHA3_C)
#include "mbedtls/sha3.h"
#endif
#define EEPROM_SIZE 512

// ESP8266 Family
#elif defined(ESP8266)
#define BOARD_NAME "ESP8266"
#define HAS_WIFI
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#define EEPROM_SIZE 512

// RP2040 Family (Pico, Pico W, Arduino Nano RP2040 Connect)
#elif defined(ARDUINO_ARCH_RP2040)
#if defined(ARDUINO_NANO_RP2040_CONNECT)
#define BOARD_NAME "Arduino Nano RP2040 Connect"
#define HAS_WIFI
#define HAS_BLE
#include <WiFiNINA.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#define BOARD_NAME "Raspberry Pi Pico W"
#define HAS_WIFI
#include <WiFi.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
#define BOARD_NAME "Raspberry Pi Pico"
#elif defined(ARDUINO_NANO_RP2040_CONNECT)
#define BOARD_NAME "Arduino Nano RP2040 Connect"
#define HAS_WIFI
#include <WiFiNINA.h>
#else
#ifndef BOARD_NAME
#define BOARD_NAME "RP2040"
#endif
#endif
#include <EEPROM.h>
// RP2040 temperature sensor - analogReadTemp already declared in Arduino.h
#define HAS_TEMPERATURE
// RP2040 hardware register access
#include "hardware/gpio.h"
#if defined(ARDUINO_ARCH_RP2040) && __has_include("pico/multicore.h")
#include "pico/multicore.h"
#define HAS_PICO_MULTICORE
#endif

// Renesas RA4M1 (Arduino Uno R4 WiFi & Minima)
#elif defined(ARDUINO_UNOR4_WIFI)
#define BOARD_NAME "Arduino Uno R4 WiFi"
#define HAS_WIFI
#define HAS_LED_MATRIX
#include <WiFiS3.h>
#include <EEPROM.h>
#include "Arduino_LED_Matrix.h"
#include <ArduinoBLE.h>
#elif defined(ARDUINO_UNOR4_MINIMA)
#define BOARD_NAME "Arduino Uno R4 Minima"
#define HAS_LED_MATRIX
#include <EEPROM.h>
#include "Arduino_LED_Matrix.h"
#elif defined(ARDUINO_ARCH_RENESAS)
#define BOARD_NAME "Renesas (Uno R4 family)"
#include <EEPROM.h>

// Arduino SAMD Family (Zero, MKR, Nano 33 IoT)
#elif defined(ARDUINO_SAMD_ZERO)
#define BOARD_NAME "Arduino Zero"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKR1000)
#define BOARD_NAME "Arduino MKR1000"
#define HAS_WIFI
#define BOARD_SAMD
#include <WiFi101.h>
#elif defined(ARDUINO_SAMD_MKRZERO)
#define BOARD_NAME "Arduino MKR Zero"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
#define BOARD_NAME "Arduino MKR WiFi 1010"
#define HAS_WIFI
#define BOARD_SAMD
#include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKRFOX1200)
#define BOARD_NAME "Arduino MKR FOX 1200"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKRWAN1300) || defined(ARDUINO_SAMD_MKRWAN1310)
#define BOARD_NAME "Arduino MKR WAN 1300/1310"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKRGSM1400)
#define BOARD_NAME "Arduino MKR GSM 1400"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKRNB1500)
#define BOARD_NAME "Arduino MKR NB 1500"
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_MKRVIDOR4000)
#define BOARD_NAME "Arduino MKR Vidor 4000"
#define HAS_FPGA
#define BOARD_SAMD
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
#define BOARD_NAME "Arduino Nano 33 IoT"
#define HAS_WIFI
#define HAS_BLE
#define HAS_IMU
#define BOARD_SAMD
#include <WiFiNINA.h>
#elif defined(ARDUINO_ARCH_SAMD)
#define BOARD_NAME "SAMD (ARM Cortex-M0+)"
#define BOARD_SAMD

// Arduino Nano 33 BLE Family (nRF52840)
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
#define BOARD_NAME "Arduino Nano 33 BLE"
#define HAS_BLE
#define HAS_IMU
#define BOARD_NRF52
#elif defined(ARDUINO_NANO33BLE)
#define BOARD_NAME "Arduino Nano 33 BLE (Rev2)"
#define HAS_BLE
#define HAS_IMU
#define BOARD_NRF52

// Arduino Portenta Family
#elif defined(ARDUINO_PORTENTA_H7_M7)
#define BOARD_NAME "Arduino Portenta H7"
#define HAS_WIFI
#define HAS_BLE
#define HAS_DUAL_CORE
#define BOARD_STM32H7
#define USBCON
#include <WiFi.h>
#elif defined(ARDUINO_PORTENTA_C33)
#define BOARD_NAME "Arduino Portenta C33"
#define HAS_WIFI
#define HAS_BLE

// Arduino Giga Family
#elif defined(ARDUINO_GIGA)
#define BOARD_NAME "Arduino Giga R1 WiFi"
#define HAS_WIFI
#define HAS_BLE
#define HAS_DUAL_CORE
#define BOARD_STM32H7
#define USBCON
#include <WiFi.h>

// Arduino Mega + WiFi
#elif defined(ARDUINO_AVR_MEGA2560)
#define BOARD_NAME "Arduino Mega 2560"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_ADK)
#define BOARD_NAME "Arduino Mega ADK"
#include <EEPROM.h>
#define BOARD_AVR

// INK4u Board (AVR128DA28 - Next Gen Arduino Uno)
#elif defined(ARDUINO_AVR_INK4U) || defined(__AVR_AVR128DA28__)
#define BOARD_NAME "INK4u Board (AVR128DA28)"
#include <EEPROM.h>
#define BOARD_AVR
#define BOARD_MEGAAVR  // Modern AVR with advanced features
#define HAS_UPDI       // UPDI programming interface
#define BOARD_SRAM_KB 16
#define BOARD_FLASH_KB 128

// Multiduino (your custom Uno-derivative)
#elif defined(ARDUINO_AVR_MULTIDUINO)
#define BOARD_NAME "Multiduino"
#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>  // Adafruit RTClib for DS1307
#define BOARD_AVR
#define HAS_RTC
#define RTC_I2C_ADDRESS 0x68  // Standard DS1307 I2C address

// Arduino Uno family
#elif defined(ARDUINO_AVR_UNO_MINI_LIMITED_EDITION) || defined(ARDUINO_AVR_UNO_MINI_LE) || defined(ARDUINO_AVR_UNO_MINI)
#define BOARD_NAME "Arduino Uno Mini LE"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_UNO)
#define BOARD_NAME "Arduino Uno"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
#define BOARD_NAME "Arduino Uno WiFi Rev2"
#define HAS_WIFI
#include <EEPROM.h>
#include <WiFiNINA.h>
#define BOARD_AVR


// Arduino Nano Family (Classic)
#elif defined(ARDUINO_AVR_NANO)
#define BOARD_NAME "Arduino Nano"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_NANO_EVERY)
#define BOARD_NAME "Arduino Nano Every"
#include <EEPROM.h>
#define BOARD_AVR

// MegaCoreX (ATmega x09 series - modern AVR)
#elif defined(__AVR_ATmegax09__)
#define BOARD_NAME "MegaCoreX (ATmega x09)"
#include <EEPROM.h>
#define BOARD_AVR
#define BOARD_MEGAAVR  // Modern AVR architecture like INK4u
#define BOARD_SRAM_KB 6  // Varies: 4809 has 6KB, 4809 has 6KB, etc.
#define BOARD_FLASH_KB 48  // Varies by variant

// Arduino Yun (ATmega32U4 + Atheros AR9331 Linux)
#elif defined(ARDUINO_AVR_YUN)
#define BOARD_NAME "Arduino Yun"
#include <EEPROM.h>
#include <Bridge.h>
#define BOARD_AVR
#define HAS_YUN_BRIDGE

// Arduino Leonardo/Micro
#elif defined(ARDUINO_AVR_LEONARDO)
#define BOARD_NAME "Arduino Leonardo"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_MICRO)
#define BOARD_NAME "Arduino Micro"
#include <EEPROM.h>
#define BOARD_AVR

// Arduino Pro/Mini
#elif defined(ARDUINO_AVR_PRO)
#define BOARD_NAME "Arduino Pro"
#include <EEPROM.h>
#define BOARD_AVR
#elif defined(ARDUINO_AVR_MINI)
#define BOARD_NAME "Arduino Mini"
#include <EEPROM.h>
#define BOARD_AVR

// Arduino Due (SAM)
#elif defined(ARDUINO_SAM_DUE)
#define BOARD_NAME "Arduino Due"
#define BOARD_SAM

// Generic AVR catch-all
#elif defined(__AVR__)
#define BOARD_NAME "AVR-based Arduino"
#include <EEPROM.h>
#define BOARD_AVR

// Arduino Uno Q (Dual-processor: STM32U585 MCU + Qualcomm QRB2210 Linux MPU)
#elif defined(ARDUINO_UNO_Q)
#define BOARD_NAME "Arduino Uno Q (MCU)"
#define BOARD_STM32U5
#define BOARD_SRAM_KB 786
#define BOARD_FLASH_KB 2048
#define HAS_DUAL_PROCESSOR  // MCU + Linux MPU
#define HAS_FPU
#define HAS_DSP
#define HAS_RNG
#define USING_ZEPHYR
#define HAS_ROUTER_BRIDGE
#define HAS_RPC_BRIDGE
#include "Arduino_RouterBridge.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/flash.h>

// STM32 Family (Blue Pill, Black Pill, Nucleo)
#elif defined(ARDUINO_ARCH_STM32)
#if defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_BLUEPILL_F103CB)
#define BOARD_NAME "STM32 Blue Pill"
#elif defined(ARDUINO_BLACKPILL_F401CC) || defined(ARDUINO_BLACKPILL_F411CE)
#define BOARD_NAME "STM32 Black Pill"
#elif defined(ARDUINO_NUCLEO_F401RE)
#define BOARD_NAME "STM32 Nucleo-F401RE"
#elif defined(ARDUINO_NUCLEO_F411RE)
#define BOARD_NAME "STM32 Nucleo-F411RE"
#else
#define BOARD_NAME "STM32"
#endif
#include <EEPROM.h>
#define BOARD_STM32

// Teensy Family
#elif defined(TEENSYDUINO)
#if defined(__IMXRT1062__)
#if defined(ARDUINO_TEENSY41)
#define BOARD_NAME "Teensy 4.1"
#else
#define BOARD_NAME "Teensy 4.0"
#endif
#define HAS_DSP  // Cortex-M7 DSP extensions (SIMD, MAC, saturating arithmetic)
#elif defined(__MK66FX1M0__)
#define BOARD_NAME "Teensy 3.6"
#elif defined(__MK64FX512__)
#define BOARD_NAME "Teensy 3.5"
#elif defined(__MK20DX256__)
#define BOARD_NAME "Teensy 3.2"
#elif defined(__MK20DX128__)
#define BOARD_NAME "Teensy 3.1/3.0"
#elif defined(__MKL26Z64__)
#define BOARD_NAME "Teensy LC"
#else
#define BOARD_NAME "Teensy"
#endif
#include <EEPROM.h>
#define BOARD_TEENSY

// Adafruit Feather Family
#elif defined(ADAFRUIT_FEATHER_M0)
#define BOARD_NAME "Adafruit Feather M0"
#define BOARD_SAMD
#elif defined(ADAFRUIT_FEATHER_M4)
#define BOARD_NAME "Adafruit Feather M4"
#define BOARD_SAMD

// Seeed Studio
#elif defined(SEEED_XIAO_M0)
#define BOARD_NAME "Seeeduino XIAO"
#define BOARD_SAMD
#elif defined(ARDUINO_SEEED_XIAO_NRF52840) || defined(ARDUINO_Seeed_XIAO_nRF52840)
#define BOARD_NAME "Seeed XIAO nRF52840"
#define HAS_BLE
#define BOARD_NRF52
#elif defined(ARDUINO_SEEED_XIAO_RP2040)
#define BOARD_NAME "Seeed XIAO RP2040"
#elif defined(SEEED_WIO_TERMINAL)
#define BOARD_NAME "Seeed Wio Terminal"
#define HAS_WIFI
#define HAS_BLE
#define HAS_LCD
#define BOARD_SAMD

// Arduino / Genuino 101 (Intel Curie – ARC32 / ARC EM processor)
#elif defined(ARDUINO_ARCH_ARC32)
#if defined(ARDUINO_GENUINO101) || defined(ARDUINO_ARDUINO101)
#define BOARD_NAME "Arduino/Genuino 101"
#else
#define BOARD_NAME "Intel Curie (ARC32)"
#endif
#define BOARD_ARC32
#define HAS_BLE          // Nordic nRF51822 BLE co-processor, CurieBLE library
#define HAS_IMU          // BMI160 6-axis IMU
#define BOARD_SRAM_KB 32
#define BOARD_FLASH_KB 196
#include <EEPROM.h>
#include <CurieBLE.h>

// Unknown/Generic
#else
#define BOARD_NAME "Unknown Arduino-Compatible"
#endif

// ==================== CONFIGURATION ====================
#define BENCHMARK_ITERATIONS 10000
#define MEMORY_TEST_SIZE 1024


#if defined(ARDUINO_ARCH_RP2040)
#include <Hash.h>
#endif

#include "BenchmarkHelpers.h"
#include <SPI.h>
#if !defined(ARDUINO_UNO_Q)
#include <Wire.h>
#endif

#if defined(ESP32)
#include <HEXBuilder.h>
#include "mbedtls/aes.h"
#include "esp_sleep.h"
#endif

#if defined(ARDUINO_ARCH_RP2040)
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/interp.h"
#include "hardware/pwm.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#endif

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
#include "RTC.h"
#endif

#if defined(__AVR__)
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#endif

#if defined(BOARD_STM32H7)
#include <mbed.h>
#include "stm32h7xx_ll_adc.h"
#endif

#if defined(BOARD_SAMD)
#include <samd.h>  // PM->SLEEP.reg, SCB->SCR
#endif

#if defined(BOARD_NRF52)
#include <nrf.h>   // NRF_POWER, __WFE
#endif

#if defined(ESP8266)
#include <user_interface.h>  // wifi_set_sleep_type, light_sleep
#endif

#if defined(TEENSYDUINO) && defined(__IMXRT1062__)
#include <imxrt.h>  // Teensy 4.x ARM registers
#endif


// ==================== SERIAL OUTPUT ABSTRACTION ====================
// Uno Q uses Monitor instead of Serial for output
#if defined(ARDUINO_UNO_Q)
#define SERIAL_OUT Monitor
#define F_STR(x) x  // Monitor doesn't support F() macro
#else
#define SERIAL_OUT Serial
#define F_STR(x) F(x)  // Use F() macro for flash storage on other boards
#endif

// Serial baud rate
#define SERIAL_BAUD 115200

// ==================== CSV OUTPUT MACROS ====================
// CSV_ROW(label, value) — emits "label,value\n" in CSV mode; compiles away otherwise
// CSV_BLANK(label)      — emits "label,\n" in CSV mode (metric N/A on this board)
#if CSV_OUTPUT
#define CSV_ROW(label, value) do { SERIAL_OUT.print(F_STR(label)); SERIAL_OUT.print(F_STR(",")); SERIAL_OUT.println(value); } while(0)
#define CSV_BLANK(label)      SERIAL_OUT.println(F_STR(label ","))
#else
#define CSV_ROW(label, value) do {} while(0)
#define CSV_BLANK(label)      do {} while(0)
#endif

// ==================== GLOBAL VARIABLES ====================
uint8_t testBuffer[256];
unsigned long gMinBenchUs = 20000;
const uint8_t kJitterTrials = 5;

#ifdef HAS_RTC
RTC_DS1307 rtc;
#endif


// ==================== HELPER FUNCTIONS ====================

void printDivider() {
#if !CSV_OUTPUT
  SERIAL_OUT.println(F_STR("========================================"));
#endif
}

void printHeader(const char *title) {
#if !CSV_OUTPUT
  SERIAL_OUT.println();
  printDivider();
  SERIAL_OUT.println(title);
  printDivider();
#else
  (void)title;
#endif
}

unsigned long benchmarkStart;

void startBenchmark() {
  benchmarkStart = micros();
}

unsigned long endBenchmark() {
  return micros() - benchmarkStart;
}

template<typename T, size_t N>
struct MedianCollector {
  T values[N];
  uint8_t count;

  void add(T value) {
    if (count < N) {
      values[count++] = value;
    }
  }

  T median() const {
    if (count == 0) {
      return T();
    }
    T sorted[N];
    for (uint8_t i = 0; i < count; i++) {
      sorted[i] = values[i];
    }
    for (uint8_t i = 1; i < count; i++) {
      T key = sorted[i];
      int8_t j = i - 1;
      while (j >= 0 && sorted[j] > key) {
        sorted[j + 1] = sorted[j];
        j--;
      }
      sorted[j + 1] = key;
    }
    uint8_t mid = count / 2;
    if (count % 2 == 1) {
      return sorted[mid];
    }
    return (sorted[mid - 1] + sorted[mid]) / 2;
  }
};

void calibrateBenchmarkTime() {
  unsigned long start = micros();
  unsigned long next = start;
  uint32_t spins = 0;
  while (next == start && spins < 100000) {
    next = micros();
    spins++;
  }
  unsigned long microsResolution = next - start;

  unsigned long delayStart = micros();
  delayMicroseconds(1000);
  unsigned long delayElapsed = micros() - delayStart;
  long delayError = abs((long)delayElapsed - 1000);

  if (microsResolution <= 2 && delayError <= 5) {
    gMinBenchUs = 5000;
  } else if (microsResolution <= 8 && delayError <= 25) {
    gMinBenchUs = 20000;
  } else {
    gMinBenchUs = 50000;
  }
}
// ==================== CPU BENCHMARKS ====================

// ATmega328P/2560/1284 families have an internal die temperature sensor on
// ADC channel 8, read against the internal 1.1V bandgap reference.
// Absolute accuracy is ±10°C without per-chip calibration, but relative
// measurements (temperature gain during stress) are much tighter.
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
    defined(__AVR_ATmega2560__) || \
    defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)
#define AVR_HAS_TEMP_SENSOR
static float readAVRTemperature() {
  uint8_t savedADMUX = ADMUX;
  uint8_t savedADCSRA = ADCSRA;

  // Internal 1.1V reference (REFS = 11), MUX = 1000 = temp sensor
  ADMUX = (1 << REFS1) | (1 << REFS0) | 0x08;
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Enable, prescaler 128

  delay(25);  // Reference + sensor settle time

  // Discard first conversion after reference switch
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  // Average 4 readings for noise reduction
  long sum = 0;
  for (uint8_t i = 0; i < 4; i++) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    sum += ADC;
  }

  // Restore previous ADC state
  ADMUX = savedADMUX;
  ADCSRA = savedADCSRA;

  // Datasheet typical: T(°C) = (ADC - 324.31) / 1.22
  return (sum / 4.0f - 324.31f) / 1.22f;
}
#endif

void benchmarkCPUStress() {
  printHeader("CPU: STRESS TEST with Temperature");

  // Check if temperature sensor is available
  bool hasTempSensor = false;
#if defined(ESP32)
  hasTempSensor = true;
#elif defined(ARDUINO_ARCH_RP2040)
  hasTempSensor = true;
#elif defined(ARDUINO_UNO_Q) && defined(CONFIG_SENSOR)
  // STM32U585 die temperature via Zephyr sensor API
  const struct device *tempDev = DEVICE_DT_GET(DT_INST(0, st_stm32_temp_cal));
  if (device_is_ready(tempDev)) {
    hasTempSensor = true;
  }
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  hasTempSensor = false;  // RA4M1 has sensor but not easily accessible
#elif defined(BOARD_TEENSY) && defined(__IMXRT1062__)
  hasTempSensor = true;  // Teensy 4.x has temp sensor
#elif defined(ARDUINO_GIGA)
  hasTempSensor = true;  // STM32H7 internal temp sensor via HAL
#elif defined(AVR_HAS_TEMP_SENSOR)
  hasTempSensor = true;  // ATmega die temp sensor (ADC channel 8)
#endif

  if (!hasTempSensor) {
    SERIAL_OUT.println(F_STR("No temp sensor, stress only"));
  }

  // Initial temperature reading
  float startTemp = 0;
#if defined(ARDUINO_GIGA)
  mbed::AnalogIn mcuADCVref(ADC_VREF), mcuADCTemp(ADC_TEMP);
  int mcuVref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(mcuADCVref.read_u16(), ADC_RESOLUTION_16B);
#endif
#if defined(ESP32)
  startTemp = temperatureRead();
  SERIAL_OUT.print(F_STR("Start Temperature: "));
  SERIAL_OUT.print(startTemp);
  SERIAL_OUT.println(F_STR(" °C"));
#elif defined(ARDUINO_ARCH_RP2040)
  startTemp = analogReadTemp(3.3f);
  SERIAL_OUT.print(F_STR("Start Temperature: "));
  SERIAL_OUT.print(startTemp);
  SERIAL_OUT.println(F_STR(" °C"));
#elif defined(ARDUINO_UNO_Q) && defined(CONFIG_SENSOR)
  if (hasTempSensor) {
    struct sensor_value val;
    sensor_sample_fetch(tempDev);
    sensor_channel_get(tempDev, SENSOR_CHAN_DIE_TEMP, &val);
    startTemp = val.val1 + val.val2 / 1000000.0f;
    SERIAL_OUT.print(F_STR("Start Temperature: "));
    SERIAL_OUT.print(startTemp);
    SERIAL_OUT.println(F_STR(" °C (die)"));
  }
#elif defined(BOARD_TEENSY) && defined(__IMXRT1062__)
  startTemp = tempmonGetTemp();
  SERIAL_OUT.print(F_STR("Start Temperature: "));
  SERIAL_OUT.print(startTemp);
  SERIAL_OUT.println(F_STR(" °C"));
#elif defined(ARDUINO_GIGA)
  startTemp = __HAL_ADC_CALC_TEMPERATURE(mcuVref, mcuADCTemp.read_u16(), ADC_RESOLUTION_16B);
  SERIAL_OUT.print(F_STR("Start Temperature: "));
  SERIAL_OUT.print(startTemp);
  SERIAL_OUT.println(F_STR(" °C"));
#elif defined(AVR_HAS_TEMP_SENSOR)
  startTemp = readAVRTemperature();
  SERIAL_OUT.print(F_STR("Start Temperature: ~"));
  SERIAL_OUT.print(startTemp);
  SERIAL_OUT.println(F_STR(" °C (die, uncalibrated)"));
#endif

  SERIAL_OUT.println(F_STR("Running 10s stress test..."));

  // CPU stress test - run all cores
  unsigned long stressStart = millis();
  unsigned long lastService = millis();
#if defined(ARDUINO_ARCH_RP2040)
  unsigned long lastIo = millis();
#endif
  unsigned long iterations = 0;
  volatile float result = 1.0f;
  const float twoPi = 6.2831853f;
#if defined(ARDUINO_ARCH_RP2040)
  uint32_t lcg = 0x12345678u;
#endif

  while (millis() - stressStart < 10000) {
    // Mix of integer and float operations
    for (int i = 0; i < 100; i++) {
#if defined(ARDUINO_ARCH_RP2040)
      lcg = lcg * 1664525u + 1013904223u;
      uint32_t mixed = lcg ^ (lcg >> 16);
      float f1 = (mixed & 0xFFFF) * 0.0001f;
      float f2 = (mixed & 0xFF) * 0.00001f;
      result = result * 1.0001f + f1;
      result = result * 0.9999f + f2;
#else
#if defined(ARDUINO_UNO_Q) || defined(ARDUINO_UNO_Q_MCU)
      // Keep it deterministic and heavy without libm (no sin/cos/fmod/sqrt)
      // Mix float + integer-ish noise
      uint32_t x = (uint32_t)iterations * 1664525u + 1013904223u;
      float f1 = (float)(x & 0xFFFFu) * 0.0001f;
      float f2 = (float)((x >> 16) & 0xFFFFu) * 0.00005f;
      result = result * 1.00013f + f1;
      result = result * 0.99991f + f2;
#else
      result = result * 1.0001f + sqrtf((float)i);
#if defined(__ZEPHYR__) || defined(BOARD_STM32U5) || defined(BOARD_ARC32)
      // Manual fmod implementation for Zephyr and ARC32 (avoids libm linking issues)
      float divisor = twoPi;
      result = result - ((int)(result / divisor)) * divisor;
#else
      result = fmodf(result, twoPi);
#endif
      result = sinf(result) + cosf(result);
#endif
#endif
      iterations++;
    }

    // Periodic yield to prevent watchdog timeout (but don't print)
    if (millis() - lastService >= 5) {
#if defined(ARDUINO_ARCH_RP2040) || defined(ESP32) || defined(ESP8266)
      yield();
#if defined(ESP32) || defined(ESP8266)
      delay(0);
#endif
#endif
      lastService = millis();
    }

#if defined(ARDUINO_ARCH_RP2040)
    if (millis() - lastIo >= 250) {
      SERIAL_OUT.write('.');
      lastIo = millis();
    }
#endif
  }

  unsigned long stressDuration = millis() - stressStart;

  SERIAL_OUT.println();
#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Done: "));
  SERIAL_OUT.print(iterations);
  SERIAL_OUT.print(F_STR(" iters in "));
  SERIAL_OUT.print(stressDuration);
  SERIAL_OUT.println(F_STR(" ms"));

  SERIAL_OUT.print(F_STR("Rate: "));
  SERIAL_OUT.print((float)iterations / stressDuration);
  SERIAL_OUT.println(F_STR(" iters/ms"));
#endif
  CSV_ROW("Stress Rate (iters/ms)", (float)iterations / stressDuration);

  // Final temperature reading
  if (hasTempSensor) {
    delay(100);  // Let sensor stabilize
    float endTemp = 0;
#if defined(ESP32)
    endTemp = temperatureRead();
#elif defined(ARDUINO_ARCH_RP2040)
    endTemp = analogReadTemp(3.3f);
#elif defined(ARDUINO_UNO_Q) && defined(CONFIG_SENSOR)
    {
      struct sensor_value val;
      sensor_sample_fetch(tempDev);
      sensor_channel_get(tempDev, SENSOR_CHAN_DIE_TEMP, &val);
      endTemp = val.val1 + val.val2 / 1000000.0f;
    }
#elif defined(BOARD_TEENSY) && defined(__IMXRT1062__)
    endTemp = tempmonGetTemp();
#elif defined(ARDUINO_GIGA)
    endTemp = __HAL_ADC_CALC_TEMPERATURE(mcuVref, mcuADCTemp.read_u16(), ADC_RESOLUTION_16B);
#elif defined(AVR_HAS_TEMP_SENSOR)
    endTemp = readAVRTemperature();
#endif

#if !CSV_OUTPUT
    SERIAL_OUT.println();
    SERIAL_OUT.print(F_STR("Final Temperature: "));
    SERIAL_OUT.print(endTemp);
    SERIAL_OUT.println(F_STR(" °C"));

    SERIAL_OUT.print(F_STR("Total Temperature Gain: +"));
    SERIAL_OUT.print(endTemp - startTemp);
    SERIAL_OUT.println(F_STR(" °C"));

    if (endTemp - startTemp > 10) {
      SERIAL_OUT.println(F_STR("WARNING: significant heating"));
    } else if (endTemp - startTemp > 5) {
      SERIAL_OUT.println(F_STR("Normal temp increase"));
    } else {
      SERIAL_OUT.println(F_STR("Good thermal performance"));
    }
#endif
    CSV_ROW("Temp Gain (C)", endTemp - startTemp);
  } else {
    CSV_BLANK("Temp Gain (C)");
  }
}

void benchmarkSerial() {
  printHeader("I/O: SERIAL COMMUNICATION");

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Baud rate: "));
  SERIAL_OUT.println(SERIAL_BAUD);

  SERIAL_OUT.print(F_STR("TX buffer: "));
#if defined(SERIAL_TX_BUFFER_SIZE)
  SERIAL_OUT.print(SERIAL_TX_BUFFER_SIZE);
  SERIAL_OUT.println(F_STR(" bytes"));
#elif defined(SERIAL_BUFFER_SIZE)
  SERIAL_OUT.print(SERIAL_BUFFER_SIZE);
  SERIAL_OUT.println(F_STR(" bytes"));
#else
  SERIAL_OUT.println(F_STR("unknown"));
#endif

  SERIAL_OUT.print(F_STR("RX buffer: "));
#if defined(SERIAL_RX_BUFFER_SIZE)
  SERIAL_OUT.print(SERIAL_RX_BUFFER_SIZE);
  SERIAL_OUT.println(F_STR(" bytes"));
#elif defined(SERIAL_BUFFER_SIZE)
  SERIAL_OUT.print(SERIAL_BUFFER_SIZE);
  SERIAL_OUT.println(F_STR(" bytes"));
#else
  SERIAL_OUT.println(F_STR("unknown"));
#endif
  SERIAL_OUT.println();
#endif
  CSV_ROW("Serial Baud Rate", SERIAL_BAUD);
#if defined(SERIAL_TX_BUFFER_SIZE)
  CSV_ROW("Serial TX Buffer (bytes)", SERIAL_TX_BUFFER_SIZE);
#elif defined(SERIAL_BUFFER_SIZE)
  CSV_ROW("Serial TX Buffer (bytes)", SERIAL_BUFFER_SIZE);
#else
  CSV_BLANK("Serial TX Buffer (bytes)");
#endif
#if defined(SERIAL_RX_BUFFER_SIZE)
  CSV_ROW("Serial RX Buffer (bytes)", SERIAL_RX_BUFFER_SIZE);
#elif defined(SERIAL_BUFFER_SIZE)
  CSV_ROW("Serial RX Buffer (bytes)", SERIAL_BUFFER_SIZE);
#else
  CSV_BLANK("Serial RX Buffer (bytes)");
#endif

  // Calculate expected bytes
  int expectedBytes = 0;
  for (int i = 0; i < 100; i++) {
    expectedBytes += (i < 10) ? 1 : (i < 100) ? 2
                                              : 3;  // digit count
  }

  // Print benchmark - measure enqueue time (CPU overhead)
  MedianCollector<unsigned long, kJitterTrials> enqueueTimeMedian = {};
  MedianCollector<float, kJitterTrials> enqueueRateMedian = {};
  for (uint8_t trial = 0; trial < kJitterTrials; trial++) {
    SERIAL_OUT.flush();
    startBenchmark();
    for (int i = 0; i < 100; i++) {
      SERIAL_OUT.print(i);
    }
    unsigned long enqueueTime = endBenchmark();
    enqueueTimeMedian.add(enqueueTime);
    enqueueRateMedian.add(expectedBytes * 1000.0f / enqueueTime);
  }
  unsigned long enqueueTime = enqueueTimeMedian.median();
  float enqueueRate = enqueueRateMedian.median();

  // Measure what flush() actually does
  unsigned long flushStart = micros();
  SERIAL_OUT.flush();  // Wait for TX buffer to drain to UART FIFO
  unsigned long flushTime = micros() - flushStart;

  // Calculate theoretical wire time (10 bits per byte: start + 8 data + stop)
  // At 115200 baud: each bit = 1/115200 sec = 8.68 μs
  // Each byte = 10 bits = 86.8 μs
  unsigned long theoreticalWireTime = (unsigned long)(expectedBytes * 10 * 1000000.0 / SERIAL_BAUD);

#if !CSV_OUTPUT
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("Enqueue ("));
  SERIAL_OUT.print(expectedBytes);
  SERIAL_OUT.print(F_STR(" B): "));
  SERIAL_OUT.print(enqueueTime);
  SERIAL_OUT.print(F_STR(" us (median "));
  SERIAL_OUT.print(enqueueRate);
  SERIAL_OUT.println(F_STR(" B/ms)"));

  SERIAL_OUT.print(F_STR("flush(): "));
  SERIAL_OUT.print(flushTime);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Wire time: "));
  SERIAL_OUT.print(theoreticalWireTime);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Throughput: "));
  SERIAL_OUT.print(expectedBytes * 1000.0 / theoreticalWireTime);
  SERIAL_OUT.print(F_STR(" B/ms ("));
  SERIAL_OUT.print(SERIAL_BAUD / 10);
  SERIAL_OUT.println(F_STR(" B/s max)"));

  SERIAL_OUT.print(F_STR("Ratio: "));
  SERIAL_OUT.print((float)enqueueTime / theoreticalWireTime * 100);
  SERIAL_OUT.print(F_STR("% ("));
  if (enqueueTime < theoreticalWireTime) {
    SERIAL_OUT.println(F_STR("buffered)"));
  } else {
    SERIAL_OUT.println(F_STR("CPU-bound)"));
  }
#endif
  CSV_ROW("Serial Enqueue (B/ms)", enqueueRate);
  CSV_ROW("Serial Wire Time (us)", theoreticalWireTime);
  CSV_ROW("Serial Throughput (B/ms)", expectedBytes * 1000.0 / theoreticalWireTime);
}
// ==================== BOARD-SPECIFIC BENCHMARKS ====================

#ifdef HAS_LED_MATRIX
void benchmarkLEDMatrix() {
  printHeader("DISPLAY: LED Matrix (Uno R4)");

  SERIAL_OUT.println(F_STR("12x8 LED Matrix Available: YES"));
  SERIAL_OUT.println(F_STR("Running LED animation test..."));

  ArduinoLEDMatrix matrix;
  matrix.begin();

  // Test 1: Frame loading speed - all pixels on (with delays for visual effect)
  uint32_t frameOn[3] = {
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
  };
  uint32_t frameOff[3] = {
    0x00000000,
    0x00000000,
    0x00000000
  };

  unsigned long blinkStart = millis();
  for (int i = 0; i < 20; i++) {
    matrix.loadFrame(frameOn);
    delay(50);
    matrix.loadFrame(frameOff);
    delay(50);
  }
  unsigned long blinkTime = millis() - blinkStart;

  // Test 2: Pattern switching speed (no delays)
  uint32_t pattern1[3] = {
    0x55555555,
    0xAAAAAAAA,
    0x55555555
  };
  uint32_t pattern2[3] = {
    0xAAAAAAAA,
    0x55555555,
    0xAAAAAAAA
  };

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    matrix.loadFrame(i % 2 == 0 ? pattern1 : pattern2);
  }
  unsigned long patternTime = endBenchmark();

  // Test 3: Bitmap rendering - must pass array name directly
  uint8_t bitmap[8][12] = {
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 }
  };

  startBenchmark();
  for (int i = 0; i < 50; i++) {
    matrix.renderBitmap(bitmap, 8, 12);
  }
  unsigned long bitmapTime = endBenchmark();

  matrix.loadFrame(frameOff);

  SERIAL_OUT.print(F_STR("Blink animation (20 cycles): "));
  SERIAL_OUT.print(blinkTime);
  SERIAL_OUT.println(F_STR(" ms"));

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Pattern switching (100 frames): "));
  SERIAL_OUT.print(patternTime);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(100.0 / patternTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("LED Matrix Pattern (ops/ms)", 100.0 / patternTime * 1000);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Bitmap rendering (50 frames): "));
  SERIAL_OUT.print(bitmapTime);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(50.0 / bitmapTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("LED Matrix Bitmap (ops/ms)", 50.0 / bitmapTime * 1000);
}
#endif

#ifdef HAS_WIFI
void benchmarkWiFi() {
  printHeader("WIRELESS: WiFi CAPABILITIES");

  SERIAL_OUT.print(F_STR("WiFi Available: YES"));

// Identify WiFi chip/library
#if defined(ARDUINO_UNOR4_WIFI)
  SERIAL_OUT.println(F_STR(" (ESP32-S3 via WiFiS3)"));
#elif defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
  SERIAL_OUT.println(F_STR(" (Nina W102 via WiFiNINA)"));
#elif defined(ARDUINO_SAMD_MKR1000)
  SERIAL_OUT.println(F_STR(" (WINC1500 via WiFi101)"));
#elif defined(ESP32) || defined(ESP8266)
  SERIAL_OUT.println(F_STR(" (Native)"));
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  SERIAL_OUT.println(F_STR(" (CYW43439)"));
#else
  SERIAL_OUT.println();
#endif

// MAC Address - different methods for different libraries
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
  // ESP and Pico W need mode set first
  WiFi.mode(WIFI_STA);
  delay(100);
  SERIAL_OUT.print(F_STR("MAC Address: "));
  SERIAL_OUT.println(WiFi.macAddress());
#elif defined(ARDUINO_UNOR4_WIFI)
  // Uno R4 WiFi - no mode setting needed
  SERIAL_OUT.print(F_STR("MAC Address: "));
  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) SERIAL_OUT.print("0");
    SERIAL_OUT.print(mac[i], HEX);
    if (i > 0) SERIAL_OUT.print(":");
  }
  SERIAL_OUT.println();
#else
  // WiFiNINA/WiFi101 - need to get MAC as byte array
  byte mac[6];
  WiFi.macAddress(mac);
  SERIAL_OUT.print(F_STR("MAC Address: "));
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) SERIAL_OUT.print("0");
    SERIAL_OUT.print(mac[i], HEX);
    if (i > 0) SERIAL_OUT.print(":");
  }
  SERIAL_OUT.println();
#endif

  // Scan networks
  SERIAL_OUT.print(F_STR("Scanning networks... "));
  unsigned long wifiScanStart = millis();
  int n = WiFi.scanNetworks();
  unsigned long wifiScanMs = millis() - wifiScanStart;
  SERIAL_OUT.print(n);
  SERIAL_OUT.println(F_STR(" networks found"));

  if (n > 0) {
    SERIAL_OUT.println(F_STR("Strongest 3 networks:"));
    for (int i = 0; i < min(3, n); i++) {
      SERIAL_OUT.print(F_STR("  "));
      SERIAL_OUT.print(i + 1);
      SERIAL_OUT.print(F_STR(": "));
      SERIAL_OUT.print(WiFi.SSID(i));
      SERIAL_OUT.print(F_STR(" ("));
      SERIAL_OUT.print(WiFi.RSSI(i));
      SERIAL_OUT.println(F_STR(" dBm)"));
    }
  }

// Clean up - different methods for different libraries
#if defined(ESP32) || defined(ESP8266)
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
#elif defined(ARDUINO_UNOR4_WIFI)
  // WiFiS3 doesn't have scanDelete or mode
  // Just let it finish naturally
#else
  // WiFiNINA/WiFi101 - end() to clean up
  WiFi.end();
#endif
  CSV_ROW("WiFi Scan (ms)", wifiScanMs);
  CSV_BLANK("WiFi Connect (ms)");
  CSV_BLANK("WiFi TX (KB/s)");
}
#endif

#ifdef HAS_BLE
void benchmarkBLE() {
  printHeader("WIRELESS: Bluetooth LE");
  SERIAL_OUT.println(F_STR("BLE Available: YES"));

#if defined(ESP32)
  SERIAL_OUT.println(F_STR("Initializing BLE..."));
  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  SERIAL_OUT.print(F_STR("Scanning for BLE devices (5 sec)... "));
  BLEScanResults *foundDevices = pBLEScan->start(5, false);
  int deviceCount = foundDevices->getCount();
  SERIAL_OUT.print(deviceCount);
  SERIAL_OUT.println(F_STR(" devices found"));

  if (deviceCount > 0) {
    SERIAL_OUT.println(F_STR("Discovered devices:"));
    for (int i = 0; i < min(5, deviceCount); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      SERIAL_OUT.print(F_STR("  "));
      SERIAL_OUT.print(i + 1);
      SERIAL_OUT.print(F_STR(": "));
      if (device.haveName()) {
        SERIAL_OUT.print(device.getName().c_str());
      } else {
        SERIAL_OUT.print(F_STR("Unknown"));
      }
      SERIAL_OUT.print(F_STR(" ["));
      SERIAL_OUT.print(device.getAddress().toString().c_str());
      SERIAL_OUT.print(F_STR("] RSSI: "));
      SERIAL_OUT.print(device.getRSSI());
      SERIAL_OUT.println(F_STR(" dBm"));
    }
  }

  pBLEScan->clearResults();
  BLEDevice::deinit(false);

#elif defined(BOARD_NRF52) || defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_SAMD_NANO_33_IOT)
  // Boards using ArduinoBLE library (nRF52840, RP2040 Connect, SAMD Nano 33 IoT)
  SERIAL_OUT.println(F_STR("BLE: ArduinoBLE scan"));
  if (!BLE.begin()) {
    SERIAL_OUT.println(F_STR("BLE.begin() failed"));
  } else {
    SERIAL_OUT.println(F_STR("Scanning for BLE devices (2 sec)..."));
    BLE.scan();
    int bleCount = 0;
    unsigned long scanStart = millis();
    while (millis() - scanStart < 2000) {
      BLEDevice peripheral = BLE.available();
      if (peripheral) {
        bleCount++;
        SERIAL_OUT.print(F_STR("  Found: "));
        if (peripheral.localName().length()) {
          SERIAL_OUT.print(peripheral.localName());
        } else {
          SERIAL_OUT.print(F_STR("(unnamed)"));
        }
        SERIAL_OUT.print(F_STR(" ["));
        SERIAL_OUT.print(peripheral.address());
        SERIAL_OUT.print(F_STR("] RSSI: "));
        SERIAL_OUT.print(peripheral.rssi());
        SERIAL_OUT.println(F_STR(" dBm"));
      }
    }
    BLE.stopScan();
    BLE.end();
    SERIAL_OUT.print(F_STR("Devices found in 2s: "));
    SERIAL_OUT.println(bleCount);
  }
#elif defined(BOARD_ARC32)
  // Intel Curie / Arduino 101 — CurieBLE library (Nordic nRF51822 co-processor)
  SERIAL_OUT.println(F_STR("BLE: Intel Curie (CurieBLE / nRF51822 co-processor)"));
  BLE.begin();
  SERIAL_OUT.println(F_STR("BLE initialized"));
  int bleCount = 0;
  BLE.scan();
  unsigned long scanStart = millis();
  while (millis() - scanStart < 2000) {
    BLEDevice peripheral = BLE.available();
    if (peripheral) bleCount++;
  }
  BLE.stopScan();
  SERIAL_OUT.print(F_STR("Devices found in 2s: "));
  SERIAL_OUT.println(bleCount);
  BLE.end();
#else
  // Generic ArduinoBLE fallback for boards with BLE hardware
  // Covers: Portenta H7, Portenta C33, Giga R1, Wio Terminal, and others
  SERIAL_OUT.println(F_STR("BLE: Generic ArduinoBLE scan"));
  if (!BLE.begin()) {
    SERIAL_OUT.println(F_STR("BLE.begin() failed"));
  } else {
    SERIAL_OUT.println(F_STR("Scanning for BLE devices (2 sec)..."));
    BLE.scan();
    int bleCount = 0;
    unsigned long scanStart = millis();
    while (millis() - scanStart < 2000) {
      BLEDevice peripheral = BLE.available();
      if (peripheral) {
        bleCount++;
        SERIAL_OUT.print(F_STR("  Found: "));
        if (peripheral.localName().length()) {
          SERIAL_OUT.print(peripheral.localName());
        } else {
          SERIAL_OUT.print(F_STR("(unnamed)"));
        }
        SERIAL_OUT.print(F_STR(" ["));
        SERIAL_OUT.print(peripheral.address());
        SERIAL_OUT.print(F_STR("] RSSI: "));
        SERIAL_OUT.print(peripheral.rssi());
        SERIAL_OUT.println(F_STR(" dBm"));
      }
    }
    BLE.stopScan();
    BLE.end();
    SERIAL_OUT.print(F_STR("Devices found in 2s: "));
    SERIAL_OUT.println(bleCount);
  }
#endif
  CSV_BLANK("BLE Advertise Start (ms)");
  CSV_BLANK("BLE Scan Found (ms)");
}
#endif
// ==================== ADVANCED MATH BENCHMARKS ====================

void benchmarkAdvancedMath() {
  printHeader("ADVANCED MATH: Transcendental Functions");

  volatile float checksum = 0;

#if defined(ARDUINO_UNO_Q) || defined(ARDUINO_UNO_Q_MCU)
  // Uno Q (Zephyr) lacks standard libm; skip transcendentals
  SERIAL_OUT.println(F_STR("Skipped (no libm on this platform)"));
  CSV_BLANK("exp() (ops/ms)");
  CSV_BLANK("log() (ops/ms)");
  CSV_BLANK("log10() (ops/ms)");
  CSV_BLANK("pow() (ops/ms)");
  CSV_BLANK("atan2() (ops/ms)");
  CSV_BLANK("asin() (ops/ms)");
  CSV_BLANK("acos() (ops/ms)");
  CSV_BLANK("tan() (ops/ms)");
  CSV_BLANK("fmod() (ops/ms)");
  return;
#endif

  // --- Exponential / Logarithmic ---
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    checksum += expf((float)i / 10.0f);
  }
  unsigned long expTime = endBenchmark();

  startBenchmark();
  for (int i = 1; i <= 100; i++) {
    checksum += logf((float)i);
  }
  unsigned long logTime = endBenchmark();

  startBenchmark();
  for (int i = 1; i <= 100; i++) {
    checksum += log10f((float)i);
  }
  unsigned long log10Time = endBenchmark();

  // --- Power ---
  startBenchmark();
  for (int i = 1; i <= 100; i++) {
    checksum += powf((float)i, 1.5f);
  }
  unsigned long powTime = endBenchmark();

  // --- Inverse trig ---
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    checksum += atan2f((float)i, (float)(100 - i));
  }
  unsigned long atan2Time = endBenchmark();

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    checksum += asinf((float)i / 100.0f);
  }
  unsigned long asinTime = endBenchmark();

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    checksum += acosf((float)i / 100.0f);
  }
  unsigned long acosTime = endBenchmark();

  // --- Additional trig ---
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    checksum += tanf((float)i / 100.0f);
  }
  unsigned long tanTime = endBenchmark();

  // --- Misc ---
  startBenchmark();
  for (int i = 0; i < 1000; i++) {
#if defined(__ZEPHYR__) || defined(BOARD_STM32U5) || defined(BOARD_ARC32)
    // Manual fmod for Zephyr and ARC32 (avoids libm linking issues)
    float val = (float)i;
    float divisor = 7.3f;
    checksum += val - ((int)(val / divisor)) * divisor;
#else
    checksum += fmodf((float)i, 7.3f);
#endif
  }
  unsigned long fmodTime = endBenchmark();

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(checksum);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("exp() (100 ops): "));
  SERIAL_OUT.print(expTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / expTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("log() (100 ops): "));
  SERIAL_OUT.print(logTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / logTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("log10() (100 ops): "));
  SERIAL_OUT.print(log10Time);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / log10Time * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("pow() (100 ops): "));
  SERIAL_OUT.print(powTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / powTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("atan2() (100 ops): "));
  SERIAL_OUT.print(atan2Time);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / atan2Time * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("asin() (100 ops): "));
  SERIAL_OUT.print(asinTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / asinTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("acos() (100 ops): "));
  SERIAL_OUT.print(acosTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / acosTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("tan() (100 ops): "));
  SERIAL_OUT.print(tanTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / tanTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("fmod() (1000 ops): "));
  SERIAL_OUT.print(fmodTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(1000.0 / fmodTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("exp() (ops/ms)", 100.0 / expTime * 1000);
  CSV_ROW("log() (ops/ms)", 100.0 / logTime * 1000);
  CSV_ROW("log10() (ops/ms)", 100.0 / log10Time * 1000);
  CSV_ROW("pow() (ops/ms)", 100.0 / powTime * 1000);
  CSV_ROW("atan2() (ops/ms)", 100.0 / atan2Time * 1000);
  CSV_ROW("asin() (ops/ms)", 100.0 / asinTime * 1000);
  CSV_ROW("acos() (ops/ms)", 100.0 / acosTime * 1000);
  CSV_ROW("tan() (ops/ms)", 100.0 / tanTime * 1000);
  CSV_ROW("fmod() (ops/ms)", 1000.0 / fmodTime * 1000);
}
#if defined(ARDUINO_ARCH_RP2040)
void benchmarkSHA1() {
  printHeader("CRYPTO: SHA1");

  SERIAL_OUT.println(F_STR("Example digests:"));
  SERIAL_OUT.print(F_STR("SHA1:"));
  SERIAL_OUT.println(sha1("abc"));

  uint8_t hash[20];
  sha1("test", &hash[0]);
  SERIAL_OUT.print(F_STR("SHA1:"));
  for (uint16_t i = 0; i < 20; i++) {
    SERIAL_OUT.printf("%02x", hash[i]);
  }
  SERIAL_OUT.println();

  const uint32_t minDurationMs = max(5UL, (gMinBenchUs + 999UL) / 1000UL);
  volatile uint32_t checksum = 0;

  TimedLoopResult stringResult = runTimedLoop(minDurationMs, 10, [&]() {
    for (uint8_t i = 0; i < 10; i++) {
      String digest = sha1("abc");
      checksum += digest.length();
    }
  });

  TimedLoopResult bufferResult = runTimedLoop(minDurationMs, 10, [&]() {
    for (uint8_t i = 0; i < 10; i++) {
      sha1("test", &hash[0]);
      checksum += hash[0];
    }
  });

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(checksum);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("SHA1 String ("));
  SERIAL_OUT.print(stringResult.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(stringResult.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(stringResult.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("SHA1 Buffer ("));
  SERIAL_OUT.print(bufferResult.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(bufferResult.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(bufferResult.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("SHA1 (ops/ms)", bufferResult.opsPerMs);
}
#endif

#if defined(ESP32)
static inline int benchmarkMd5(const unsigned char *input, size_t size, unsigned char *output) {
#if defined(MBEDTLS_VERSION_MAJOR) && MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_md5(input, size, output);
#else
  return mbedtls_md5_ret(input, size, output);
#endif
}

static inline int benchmarkSha1(const unsigned char *input, size_t size, unsigned char *output) {
#if defined(MBEDTLS_VERSION_MAJOR) && MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha1(input, size, output);
#else
  return mbedtls_sha1_ret(input, size, output);
#endif
}

static inline int benchmarkSha256(const unsigned char *input, size_t size, unsigned char *output) {
#if defined(MBEDTLS_VERSION_MAJOR) && MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256(input, size, output, 0);
#else
  return mbedtls_sha256_ret(input, size, output, 0);
#endif
}

static inline int benchmarkSha512(const unsigned char *input, size_t size, unsigned char *output) {
#if defined(MBEDTLS_VERSION_MAJOR) && MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha512(input, size, output, 0);
#else
  return mbedtls_sha512_ret(input, size, output, 0);
#endif
}

void benchmarkESP32Crypto() {
  printHeader("CRYPTO: Hashing (ESP32)");

  constexpr uint8_t inputSize = 128;
  uint8_t input[inputSize];
  for (uint8_t i = 0; i < inputSize; i++) {
    input[i] = (uint8_t)(i * 3 + 7);
  }

  uint8_t digest[64];
  char hexDigest[129];
  char hexInput[(inputSize * 2) + 1];
  volatile uint32_t checksum = 0;
  uint32_t lastProgressMs = millis();
  const uint32_t maxCryptoMs = 15000;
  const uint32_t cryptoStartMs = lastProgressMs;
  bool cryptoTimedOut = false;
  bool cryptoAbortPrinted = false;
  auto benchYield = []() {
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)
    yield();
#endif
  };
  auto checkCryptoTimeout = [&]() -> bool {
    if (!cryptoTimedOut && (millis() - cryptoStartMs > maxCryptoMs)) {
      SERIAL_OUT.println(F_STR("Warning: crypto benchmark timed out."));
      cryptoTimedOut = true;
    }
    return cryptoTimedOut;
  };
  auto abortCryptoIfTimedOut = [&]() -> bool {
    if (cryptoTimedOut && !cryptoAbortPrinted) {
      SERIAL_OUT.println(F_STR("Crypto benchmark aborted due to timeout."));
      cryptoAbortPrinted = true;
    }
    return cryptoTimedOut;
  };

  auto toHex = [&](const uint8_t *data, size_t len, char *out, size_t outCapacity) -> bool {
    const size_t requiredSize = (len * 2) + 1;
    if (outCapacity < requiredSize) {
      return false;
    }
#if defined(ESP32)
    size_t written = HEXBuilder::bytes2hex(out, outCapacity, data, len);
    return written >= requiredSize;
#else
    static const char kHexChars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
      const uint8_t byteValue = data[i];
      out[i * 2] = kHexChars[(byteValue >> 4) & 0x0F];
      out[(i * 2) + 1] = kHexChars[byteValue & 0x0F];
    }
    out[len * 2] = '\0';
    return true;
#endif
  };

  const uint32_t minDurationMs = max(5UL, (gMinBenchUs + 999UL) / 1000UL);

  SERIAL_OUT.println(F_STR("Example digests:"));
  benchmarkMd5(input, inputSize, digest);
  if (!toHex(digest, 16, hexDigest, sizeof(hexDigest))) {
    SERIAL_OUT.println(F_STR("MD5 hex buffer too small."));
    return;
  }
  SERIAL_OUT.print(F_STR("MD5: "));
  SERIAL_OUT.println(hexDigest);

  benchmarkSha1(input, inputSize, digest);
  if (!toHex(digest, 20, hexDigest, sizeof(hexDigest))) {
    SERIAL_OUT.println(F_STR("SHA1 hex buffer too small."));
    return;
  }
  SERIAL_OUT.print(F_STR("SHA1: "));
  SERIAL_OUT.println(hexDigest);

  benchmarkSha256(input, inputSize, digest);
  if (!toHex(digest, 32, hexDigest, sizeof(hexDigest))) {
    SERIAL_OUT.println(F_STR("SHA256 hex buffer too small."));
    return;
  }
  SERIAL_OUT.print(F_STR("SHA256: "));
  SERIAL_OUT.println(hexDigest);

  benchmarkSha512(input, inputSize, digest);
  if (!toHex(digest, 64, hexDigest, sizeof(hexDigest))) {
    SERIAL_OUT.println(F_STR("SHA512 hex buffer too small."));
    return;
  }
  SERIAL_OUT.print(F_STR("SHA512: "));
  SERIAL_OUT.println(hexDigest);

#if defined(MBEDTLS_SHA3_C)
  {
    mbedtls_sha3_context sha3;
    mbedtls_sha3_init(&sha3);
    mbedtls_sha3_starts(&sha3, 256);
    mbedtls_sha3_update(&sha3, input, inputSize);
    mbedtls_sha3_finish(&sha3, digest);
    mbedtls_sha3_free(&sha3);
    if (!toHex(digest, 32, hexDigest, sizeof(hexDigest))) {
      SERIAL_OUT.println(F_STR("SHA3-256 hex buffer too small."));
      return;
    }
    SERIAL_OUT.print(F_STR("SHA3-256: "));
    SERIAL_OUT.println(hexDigest);
  }
#else
  SERIAL_OUT.println(F_STR("SHA3-256: not available in this build"));
#endif

  SERIAL_OUT.println(F_STR("...running HEX encode"));
  TimedLoopResult hexResult = runTimedLoop(minDurationMs, 20, [&]() -> bool {
    for (uint8_t i = 0; i < 20; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      if (!toHex(input, inputSize, hexInput, sizeof(hexInput))) {
        SERIAL_OUT.println(F_STR("HEX encode buffer too small."));
        return false;
      }
      checksum += hexInput[0];
      if ((i % 5) == 0) {
        benchYield();
      }
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
  SERIAL_OUT.println(F_STR("...running MD5"));
  TimedLoopResult md5Result = runTimedLoop(minDurationMs, 10, [&]() -> bool {
    for (uint8_t i = 0; i < 10; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      benchmarkMd5(input, inputSize, digest);
      checksum += digest[0];
      if ((i % 3) == 0) {
        benchYield();
      }
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
  SERIAL_OUT.println(F_STR("...running SHA1"));
  TimedLoopResult sha1Result = runTimedLoop(minDurationMs, 10, [&]() -> bool {
    for (uint8_t i = 0; i < 10; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      benchmarkSha1(input, inputSize, digest);
      checksum += digest[0];
      if ((i % 3) == 0) {
        benchYield();
      }
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
  SERIAL_OUT.println(F_STR("...running SHA256"));
  TimedLoopResult sha256Result = runTimedLoop(minDurationMs, 10, [&]() -> bool {
    for (uint8_t i = 0; i < 10; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      benchmarkSha256(input, inputSize, digest);
      checksum += digest[0];
      if ((i % 3) == 0) {
        benchYield();
      }
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
  SERIAL_OUT.println(F_STR("...running SHA512"));
  TimedLoopResult sha512Result = runTimedLoop(minDurationMs, 10, [&]() -> bool {
    for (uint8_t i = 0; i < 10; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      benchmarkSha512(input, inputSize, digest);
      checksum += digest[0];
      if ((i % 2) == 0) {
        benchYield();
      }
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
#if defined(MBEDTLS_SHA3_C)
  SERIAL_OUT.println(F_STR("...running SHA3-256"));
  TimedLoopResult sha3Result = runTimedLoop(minDurationMs, 5, [&]() -> bool {
    for (uint8_t i = 0; i < 5; i++) {
      if (checkCryptoTimeout()) {
        return false;
      }
      mbedtls_sha3_context sha3;
      mbedtls_sha3_init(&sha3);
      mbedtls_sha3_starts(&sha3, 256);
      mbedtls_sha3_update(&sha3, input, inputSize);
      mbedtls_sha3_finish(&sha3, digest);
      mbedtls_sha3_free(&sha3);
      checksum += digest[0];
      benchYield();
    }
    return true;
  });
  if (abortCryptoIfTimedOut()) {
    return;
  }
#endif

  const uint8_t saltSize = 16;
  uint8_t salt[saltSize];
  for (uint8_t i = 0; i < saltSize; i++) {
    salt[i] = (uint8_t)(0xA5 ^ i);
  }
  const char *password = "esp32-benchmark";
  const uint32_t pbkdf2Iterations = 500;

  SERIAL_OUT.println(F_STR("...running PBKDF2-HMAC-SHA256"));
  TimedLoopResult pbkdf2Result = runTimedLoop(minDurationMs, 1, [&]() -> bool {
    if (checkCryptoTimeout()) {
      return false;
    }
#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000
    if (mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
                                      reinterpret_cast<const unsigned char *>(password),
                                      strlen(password),
                                      salt,
                                      saltSize,
                                      pbkdf2Iterations,
                                      32,
                                      digest)
        == 0) {
      checksum += digest[0];
    }
#else
      mbedtls_md_context_t ctx;
      mbedtls_md_init(&ctx);
      const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
      if (info != nullptr && mbedtls_md_setup(&ctx, info, 1) == 0) {
        if (mbedtls_pkcs5_pbkdf2_hmac(&ctx,
                                      reinterpret_cast<const unsigned char *>(password),
                                      strlen(password),
                                      salt,
                                      saltSize,
                                      pbkdf2Iterations,
                                      32,
                                      digest)
            == 0) {
          checksum += digest[0];
        }
      }
      mbedtls_md_free(&ctx);
#endif
    if (millis() - lastProgressMs > 1000) {
      SERIAL_OUT.print('.');
      lastProgressMs = millis();
    }
    benchYield();
    return !checkCryptoTimeout();
  });
  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(checksum);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("HEX encode ("));
  SERIAL_OUT.print(hexResult.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(hexResult.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(hexResult.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("MD5 ("));
  SERIAL_OUT.print(md5Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(md5Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(md5Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("SHA1 ("));
  SERIAL_OUT.print(sha1Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(sha1Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(sha1Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("SHA256 ("));
  SERIAL_OUT.print(sha256Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(sha256Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(sha256Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));

  SERIAL_OUT.print(F_STR("SHA512 ("));
  SERIAL_OUT.print(sha512Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(sha512Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(sha512Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("SHA1 (ops/ms)", sha1Result.opsPerMs);
  CSV_ROW("MD5 (ops/ms)", md5Result.opsPerMs);
  CSV_ROW("SHA256 (ops/ms)", sha256Result.opsPerMs);

#if !CSV_OUTPUT && defined(MBEDTLS_SHA3_C)
  SERIAL_OUT.print(F_STR("SHA3-256 ("));
  SERIAL_OUT.print(sha3Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(sha3Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(sha3Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("PBKDF2-HMAC-SHA256 ("));
  SERIAL_OUT.print(pbkdf2Result.totalOps);
  SERIAL_OUT.print(F_STR(" ops): "));
  SERIAL_OUT.print(pbkdf2Result.elapsedMicros);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(pbkdf2Result.opsPerMs);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
  SERIAL_OUT.print(F_STR("  Iterations per op: "));
  SERIAL_OUT.println(pbkdf2Iterations);
#endif
}
#endif
// ==================== TIMING PRECISION BENCHMARKS ====================

void benchmarkTimingPrecision() {
  printHeader("TIMING: millis() and micros() Precision");

  // Test millis() accuracy
  unsigned long startMillis = millis();
  delay(1000);
  unsigned long endMillis = millis();
  long millisError = abs((long)(endMillis - startMillis) - 1000);

  SERIAL_OUT.print(F_STR("millis() 1s: "));
  SERIAL_OUT.print(endMillis - startMillis);
  SERIAL_OUT.print(F_STR(" ms (+/-"));
  SERIAL_OUT.print(millisError);
  SERIAL_OUT.println(F_STR(" ms)"));

  // Test micros() resolution - wait for value to change to measure granularity
  unsigned long micro1 = micros();
  unsigned long micro2 = micro1;
  uint16_t iterations = 0;
  const uint16_t maxIterations = 10000;

  // Wait for micros() to increment
  while (micro2 == micro1 && iterations < maxIterations) {
    micro2 = micros();
    iterations++;
  }

  unsigned long microResolution = micro2 - micro1;

  SERIAL_OUT.print(F_STR("micros() res: "));
  if (iterations >= maxIterations) {
    SERIAL_OUT.println(F_STR("TIMEOUT"));
  } else {
    SERIAL_OUT.print(microResolution);
    SERIAL_OUT.print(F_STR(" us ("));
    SERIAL_OUT.print(iterations);
    SERIAL_OUT.println(F_STR(" reads)"));
  }

  // Additional resolution test - measure minimum measurable difference
  unsigned long minDiff = 0xFFFFFFFF;
  for (int i = 0; i < 100; i++) {
    unsigned long t1 = micros();
    unsigned long t2 = micros();
    while (t2 == t1) {
      t2 = micros();
    }
    unsigned long diff = t2 - t1;
    if (diff > 0 && diff < minDiff) {
      minDiff = diff;
    }
  }

  SERIAL_OUT.print(F_STR("Min step: "));
  SERIAL_OUT.print(minDiff);
  SERIAL_OUT.println(F_STR(" us"));

  // Test micros() consistency over short period
  unsigned long startMicros = micros();
  delayMicroseconds(1000);
  unsigned long endMicros = micros();
  long microsError = abs((long)(endMicros - startMicros) - 1000);

  SERIAL_OUT.print(F_STR("micros() 1ms: "));
  SERIAL_OUT.print(endMicros - startMicros);
  SERIAL_OUT.print(F_STR(" us (+/-"));
  SERIAL_OUT.print(microsError);
  SERIAL_OUT.println(F_STR(" us)"));

// ESP32: CPU cycle counter cross-check
#ifdef ESP32
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("ESP32 CPU Cycle Counter:"));

  // Get CPU frequency
  uint32_t cpuFreqMHz = ESP.getCpuFreqMHz();
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(cpuFreqMHz);
  SERIAL_OUT.println(F_STR(" MHz"));

  // Test cycle counter vs micros()
  uint32_t cycleStart = ESP.getCycleCount();
  unsigned long microsStart = micros();

  // Busy wait for ~1000 us
  delayMicroseconds(1000);

  uint32_t cycleEnd = ESP.getCycleCount();
  unsigned long microsEnd = micros();

  uint32_t cycleDelta = cycleEnd - cycleStart;
  unsigned long microsDelta = microsEnd - microsStart;

  SERIAL_OUT.print(F_STR("Cycles elapsed: "));
  SERIAL_OUT.println(cycleDelta);
  SERIAL_OUT.print(F_STR("micros() elapsed: "));
  SERIAL_OUT.print(microsDelta);
  SERIAL_OUT.println(F_STR(" μs"));

  // Calculate expected cycles
  float expectedCycles = microsDelta * cpuFreqMHz;
  SERIAL_OUT.print(F_STR("Expected cycles: "));
  SERIAL_OUT.println((uint32_t)expectedCycles);

  // Calculate jitter
  float jitterPercent = abs((float)cycleDelta - expectedCycles) / expectedCycles * 100.0f;
  SERIAL_OUT.print(F_STR("Timing jitter: "));
  SERIAL_OUT.print(jitterPercent, 2);
  SERIAL_OUT.println(F_STR("%"));
#endif

  // Clock frequency estimate
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("Clock accuracy: "));
  float accuracy = 100.0f - ((float)millisError / 10.0f);
  SERIAL_OUT.print(accuracy);
  SERIAL_OUT.println(F_STR("%"));
}

// ==================== DELAY TIMING BENCHMARKS ====================

// Helper: take median of N long values (sorts in-place)
static long medianOfSorted(long *arr, uint8_t n) {
  // Insertion sort
  for (uint8_t i = 1; i < n; i++) {
    long key = arr[i];
    int8_t j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = key;
  }
  return arr[n / 2];
}

// Measure the raw time of a single delay(ms) call (median of 5)
static long measureDelayMs(int ms) {
  long samples[5];
  for (uint8_t i = 0; i < 5; i++) {
    unsigned long t1 = micros();
    delay(ms);
    unsigned long t2 = micros();
    samples[i] = (long)(t2 - t1);
  }
  return medianOfSorted(samples, 5);
}

// Measure the raw time of a single delayMicroseconds(us) call (median of 5)
static long measureDelayUs(int us) {
  long samples[5];
  for (uint8_t i = 0; i < 5; i++) {
    unsigned long t1 = micros();
    delayMicroseconds(us);
    unsigned long t2 = micros();
    samples[i] = (long)(t2 - t1);
  }
  return medianOfSorted(samples, 5);
}

void benchmarkDelayTiming() {
  printHeader("TIMING: delay() and delayMicroseconds() Accuracy");

  // --- delay() ---
  // Measure call overhead via delay(0)
  long delayOverhead = measureDelayMs(0);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("delay(0) call overhead: "));
  SERIAL_OUT.print(delayOverhead / 1000.0f, 3);
  SERIAL_OUT.println(F_STR(" ms"));
#endif
  CSV_ROW("delay(0) overhead (us)", delayOverhead);

  // Measure delay(1..50), subtract the call overhead
  long delayErrorSum = 0;
  long delayMaxError = 0;
  const int delaySteps = 50;  // 1..50

  for (int ms = 1; ms <= delaySteps; ms++) {
    long raw = measureDelayMs(ms);
    long corrected = raw - delayOverhead;       // remove call overhead
    long expected = (long)ms * 1000L;
    long err = corrected - expected;
    long absErr = err < 0 ? -err : err;
    delayErrorSum += absErr;
    if (absErr > delayMaxError) delayMaxError = absErr;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)
    yield();
#endif
  }

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("delay(1..50) avg deviation: "));
  SERIAL_OUT.print(delayErrorSum / (float)delaySteps / 1000.0f, 3);
  SERIAL_OUT.println(F_STR(" ms"));
  SERIAL_OUT.print(F_STR("delay(1..50) max deviation: "));
  SERIAL_OUT.print(delayMaxError / 1000.0f, 3);
  SERIAL_OUT.println(F_STR(" ms"));
#endif
  CSV_ROW("delay() avg deviation (us)", delayErrorSum / (float)delaySteps);

  // --- delayMicroseconds() ---
  // Measure call overhead via delayMicroseconds(0)
  long dusOverhead = measureDelayUs(0);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("delayMicroseconds(0) call overhead: "));
  SERIAL_OUT.print(dusOverhead);
  SERIAL_OUT.println(F_STR(" us"));
#endif

  // Measure delayMicroseconds(1..50), subtract the call overhead
  long dusErrorSum = 0;
  long dusMaxError = 0;
  const int dusSteps = 50;  // 1..50

  for (int us = 1; us <= dusSteps; us++) {
    long raw = measureDelayUs(us);
    long corrected = raw - dusOverhead;         // remove call overhead
    long err = corrected - (long)us;
    long absErr = err < 0 ? -err : err;
    dusErrorSum += absErr;
    if (absErr > dusMaxError) dusMaxError = absErr;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)
    yield();
#endif
  }

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("delayMicroseconds(1..50) avg deviation: "));
  SERIAL_OUT.print(dusErrorSum / (float)dusSteps, 1);
  SERIAL_OUT.println(F_STR(" us"));
  SERIAL_OUT.print(F_STR("delayMicroseconds(1..50) max deviation: "));
  SERIAL_OUT.print(dusMaxError);
  SERIAL_OUT.println(F_STR(" us"));
#endif
  CSV_ROW("delayMicroseconds() avg deviation (us)", dusErrorSum / (float)dusSteps);
}

// ==================== STACK DEPTH BENCHMARK ====================

volatile int recursionCounter = 0;

// Timed recursion function - frame layout intentionally matches the probe
int testRecursion(int depth) {
  recursionCounter++;
  volatile char buffer[32];  // Consume stack space
  buffer[0] = (char)depth;
  buffer[1] = (char)(depth >> 1);

  if (depth > 0) {
    return testRecursion(depth - 1) + (buffer[0] & 1);
  }
  return 1;
}

// --- Phase 1 helpers: discover the actual stack limit ---

volatile int probeReachedDepth = 0;
volatile size_t probeFrameBytes = 0;
volatile size_t probeTotalUsed = 0;

// Recurse until we'd exceed the stack budget.  Same frame layout as
// testRecursion() so the per-frame cost is representative.
int probeStackLimit(int depth, uintptr_t stackBase, size_t stackBudget) {
  volatile char marker;
  volatile char buffer[32];
  buffer[0] = (char)depth;
  buffer[1] = (char)(depth >> 1);

  uintptr_t here = (uintptr_t)&marker;
  size_t consumed = (stackBase > here) ? (stackBase - here)
                                       : (here - stackBase);

  probeReachedDepth = depth;
  probeTotalUsed    = consumed;

  // Compute running average of per-frame cost once we have enough data
  size_t frameEst;
  if (depth >= 2) {
    frameEst = consumed / (size_t)depth;
    probeFrameBytes = frameEst;
  } else {
    frameEst = 80;  // conservative first-pass guess
  }
  if (frameEst < 16) frameEst = 16;

  // Stop if the next frame + 512-byte safety margin would blow the budget.
  // The margin covers interrupt handlers, unwinding, and Serial I/O after
  // we return.
  if (consumed + frameEst + 512 > stackBudget) {
    return 1;
  }

  return probeStackLimit(depth + 1, stackBase, stackBudget) + (buffer[0] & 1);
}

// Forward-declare sbrk at file scope (extern "C" is not allowed inside
// a function body in C++).  Guarded so it only appears on platforms that
// actually provide it via newlib / picolibc.
#if !defined(ESP32) && !defined(ESP8266) && !defined(__AVR__) && !defined(__ZEPHYR__)
extern "C" char *sbrk(int i);
#endif

// Platform-specific: how many bytes of stack can we still use?
// Called from benchmarkStackDepth() with &stackBase on the caller's frame.
static size_t getStackBudget(uintptr_t callerFrame) {
#if defined(ESP32)
  // Arduino loop() runs inside a FreeRTOS task whose stack is a fixed
  // heap allocation (default 8 KB on most ESP32 variants).
  // uxTaskGetStackHighWaterMark() returns the *minimum* free stack (bytes)
  // that has ever existed since the task was created -- a safe lower bound
  // for what we can still use right now.
  size_t budget = (size_t)uxTaskGetStackHighWaterMark(NULL);
  if (budget < 1024) budget = 1024;      // floor: always try at least 1 KB
  return budget;
#elif defined(ESP8266)
  // ESP8266 continuation stack is about 4 KB; cont_get_free_stack() is not
  // always exposed, so use a conservative fixed estimate.
  (void)callerFrame;
  return 2048;
#elif defined(__AVR__)
  extern int __heap_start, *__brkval;
  uintptr_t heapTop = (__brkval == 0) ? (uintptr_t)&__heap_start
                                      : (uintptr_t)__brkval;
  return (callerFrame > heapTop) ? (size_t)(callerFrame - heapTop) : 256;
#elif defined(__ZEPHYR__)
  // Zephyr RTOS: the thread stack is a fixed-size region separate from
  // the heap.  The kernel thread APIs (k_current_get, etc.) are not
  // available inside Arduino LLEXT sketches, so we use a conservative
  // fixed estimate.  The default Arduino-on-Zephyr main thread stack is
  // typically 8-16 KB; use 4 KB to stay well within bounds.
  (void)callerFrame;
  return 4096;
#elif defined(BOARD_STM32H7)
  // Arduino Mbed OS boards (Portenta H7, Giga R1): thread stacks are
  // managed by the RTOS in a separate region from the heap.  sbrk(0) is
  // not meaningful here.  Use CMSIS-RTOS2 to query remaining stack space.
  (void)callerFrame;
  {
    osThreadId_t tid = osThreadGetId();
    if (tid) {
      size_t space = (size_t)osThreadGetStackSpace(tid);
      if (space > 1024) return space;
    }
  }
  return 8192;  // conservative fallback
#elif defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_RP2040)     \
   || defined(BOARD_STM32U5) || defined(BOARD_SAMD)                 \
   || defined(BOARD_NRF52)                                          \
   || defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)  \
   || defined(BOARD_ARC32)
  // ARM Cortex boards with newlib -- sbrk(0) gives the heap break
  uintptr_t heapTop = (uintptr_t)sbrk(0);
  return (callerFrame > heapTop) ? (size_t)(callerFrame - heapTop) : 256;
#else
  // Unknown platform: probe with malloc to estimate free memory,
  // then assume roughly half is usable as stack.
  (void)callerFrame;
  size_t low = 0, high = 1024;
  while (true) {
    void *p = malloc(high);
    if (p) { free(p); low = high; if (high > (SIZE_MAX / 2)) break; high *= 2; }
    else break;
  }
  size_t left = low, right = high;
  while (left + 1 < right) {
    size_t mid = left + (right - left) / 2;
    void *p = malloc(mid);
    if (p) { free(p); left = mid; } else { right = mid; }
  }
  return left / 2;
#endif
}

void benchmarkStackDepth() {
  printHeader("MEMORY: Stack Depth Test");

  // ---- Phase 1: discover the maximum safe recursion depth ----
  SERIAL_OUT.println(F_STR("Probing stack limit..."));

  volatile char stackAnchor;
  uintptr_t baseAddr    = (uintptr_t)&stackAnchor;
  size_t    stackBudget = getStackBudget(baseAddr);

  SERIAL_OUT.print(F_STR("Stack budget: "));
  SERIAL_OUT.print((unsigned long)stackBudget);
  SERIAL_OUT.println(F_STR(" bytes"));

  probeReachedDepth = 0;
  probeFrameBytes   = 0;
  probeTotalUsed    = 0;
  int probeResult = probeStackLimit(0, baseAddr, stackBudget);
  (void)probeResult;

  int maxDepth       = probeReachedDepth;
  size_t frameBytes  = (size_t)probeFrameBytes;
  size_t totalUsed   = (size_t)probeTotalUsed;

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Max safe depth: "));
  SERIAL_OUT.println(maxDepth);
  SERIAL_OUT.print(F_STR("Per-frame cost: "));
  SERIAL_OUT.print((unsigned long)frameBytes);
  SERIAL_OUT.println(F_STR(" bytes"));
  SERIAL_OUT.print(F_STR("Stack used:     "));
  SERIAL_OUT.print((unsigned long)totalUsed);
  SERIAL_OUT.print(F_STR(" / "));
  SERIAL_OUT.print((unsigned long)stackBudget);
  SERIAL_OUT.println(F_STR(" bytes"));
#endif

  if (maxDepth < 2) {
    SERIAL_OUT.println(F_STR("Stack too small to time - skipping"));
    return;
  }

  // ---- Phase 2: time recursion at 90 % of the discovered limit ----
  int timedDepth = (maxDepth * 9) / 10;
  if (timedDepth < 1) timedDepth = 1;

  SERIAL_OUT.print(F_STR("Timing at depth "));
  SERIAL_OUT.print(timedDepth);
  SERIAL_OUT.println(F_STR("..."));

  // Warm-up pass (also validates the depth is safe)
  recursionCounter = 0;
  int warmup = testRecursion(timedDepth);
  (void)warmup;

  if (recursionCounter != timedDepth + 1) {
    SERIAL_OUT.print(F_STR("FAIL during warm-up @ depth "));
    SERIAL_OUT.println(recursionCounter);
    return;
  }

  // Median of kJitterTrials timed runs
  MedianCollector<float, kJitterTrials> usMedian = {};
  for (uint8_t trial = 0; trial < kJitterTrials; trial++) {
    recursionCounter = 0;
    unsigned long t0 = micros();
    int r = testRecursion(timedDepth);
    unsigned long t1 = micros();
    (void)r;
    if (recursionCounter == timedDepth + 1) {
      usMedian.add((float)(t1 - t0));
    }
  }

  float medianUs = usMedian.median();
#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Recursion ("));
  SERIAL_OUT.print(timedDepth);
  SERIAL_OUT.print(F_STR(" deep): "));
  SERIAL_OUT.print(medianUs, 1);
  SERIAL_OUT.println(F_STR(" us"));

  if (medianUs > 0.0f) {
    float usPerLevel = medianUs / (float)timedDepth;
    SERIAL_OUT.print(F_STR("Per level: "));
    SERIAL_OUT.print(usPerLevel, 2);
    SERIAL_OUT.println(F_STR(" us"));
  }
#endif
  CSV_ROW("Stack Depth (bytes)", (unsigned long)totalUsed);
}
// ==================== MULTI-CORE BENCHMARKS ====================

#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040)

#ifdef ESP32
TaskHandle_t Task1;
TaskHandle_t Task2;
volatile unsigned long core0Count = 0;
volatile unsigned long core1Count = 0;
volatile bool testRunning = false;

void core0Task(void *parameter) {
  volatile uint32_t accumulator = 0;
  while (true) {
    if (testRunning) {
      core0Count++;
      accumulator += 3;
      accumulator ^= (accumulator << 1);
    }
  }
}

void core1Task(void *parameter) {
  volatile uint32_t accumulator = 0;
  while (true) {
    if (testRunning) {
      core1Count++;
      accumulator += 3;
      accumulator ^= (accumulator << 1);
    }
  }
}
#endif

#if defined(ARDUINO_ARCH_RP2040) && defined(HAS_PICO_MULTICORE)
volatile unsigned long rp2040Core0Count = 0;
volatile unsigned long rp2040Core1Count = 0;
volatile bool rp2040TestRunning = false;
volatile bool rp2040Core1Ready = false;

void rp2040Core1Task() {
  volatile uint32_t accumulator = 0;
  rp2040Core1Ready = true;
  while (true) {
    if (rp2040TestRunning) {
      rp2040Core1Count++;
      accumulator += 3;
      accumulator ^= (accumulator << 1);
    }
  }
}
#endif

void benchmarkMultiCore() {
  printHeader("MULTI-CORE: Parallel Performance");

#ifdef ESP32
  // Check number of cores available
  uint8_t numCores = 1;
#if CONFIG_FREERTOS_UNICORE
  numCores = 1;
#else
// For multi-core ESP32 variants, we can check portNUM_PROCESSORS or assume 2
#if defined(portNUM_PROCESSORS)
  numCores = portNUM_PROCESSORS;
#elif !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32C5) && !defined(CONFIG_IDF_TARGET_ESP32H2) && !defined(CONFIG_IDF_TARGET_ESP32C2) && !defined(CONFIG_IDF_TARGET_ESP32C3)
  numCores = 2;  // Classic ESP32, S2, S3
#endif
#endif

  if (numCores < 2) {
    SERIAL_OUT.println(F_STR("Single-Core ESP32 Variant"));
    SERIAL_OUT.println(F_STR("Running single-core workload for 1 second..."));

    volatile unsigned long count = 0;
    volatile uint32_t accumulator = 0;
    unsigned long start = millis();
    while (millis() - start < 1000) {
      count++;
      accumulator += 3;
      accumulator ^= (accumulator << 1);
    }

    SERIAL_OUT.print(F_STR("Iterations in 1 second: "));
    SERIAL_OUT.println(count);
    SERIAL_OUT.println(F_STR("Note: This variant has only one core"));
    CSV_ROW("Core 0 (iters/ms)", count / 1000.0f);
    CSV_BLANK("Core 1 (iters/ms)");
  } else {
    SERIAL_OUT.println(F_STR("ESP32 Dual-Core Test"));

    // Create tasks on both cores
    xTaskCreatePinnedToCore(
      core0Task,
      "Task0",
      10000,
      NULL,
      1,
      &Task1,
      0);

    xTaskCreatePinnedToCore(
      core1Task,
      "Task1",
      10000,
      NULL,
      1,
      &Task2,
      1);

    delay(100);  // Let tasks start

    // Run test
    core0Count = 0;
    core1Count = 0;
    testRunning = true;
    delay(1000);
    testRunning = false;

#if !CSV_OUTPUT
    SERIAL_OUT.print(F_STR("Core 0 iterations: "));
    SERIAL_OUT.println(core0Count);
    SERIAL_OUT.print(F_STR("Core 1 iterations: "));
    SERIAL_OUT.println(core1Count);
    SERIAL_OUT.print(F_STR("Total iterations: "));
    SERIAL_OUT.println(core0Count + core1Count);
    SERIAL_OUT.print(F_STR("Core balance: "));
    float balance = (float)min(core0Count, core1Count) / (float)max(core0Count, core1Count) * 100.0f;
    SERIAL_OUT.print(balance);
    SERIAL_OUT.println(F_STR("%"));
#endif
    CSV_ROW("Core 0 (iters/ms)", core0Count / 1000.0f);
    CSV_ROW("Core 1 (iters/ms)", core1Count / 1000.0f);

    // Clean up
    vTaskDelete(Task1);
    vTaskDelete(Task2);
  }  // End of multi-core else block

#elif defined(ARDUINO_ARCH_RP2040)
  SERIAL_OUT.println(F_STR("RP2040 Dual-Core Test"));
#if defined(HAS_PICO_MULTICORE)
  SERIAL_OUT.println(F_STR("Running dual-core workload for 1 second..."));

  rp2040Core0Count = 0;
  rp2040Core1Count = 0;
  rp2040TestRunning = false;
  rp2040Core1Ready = false;

  multicore_reset_core1();
  multicore_launch_core1(rp2040Core1Task);

  unsigned long readyStart = millis();
  while (!rp2040Core1Ready && millis() - readyStart < 200) {
    delay(1);
  }

  rp2040TestRunning = true;
  unsigned long start = millis();
  volatile uint32_t accumulator = 0;
  while (millis() - start < 1000) {
    rp2040Core0Count++;
    accumulator += 3;
    accumulator ^= (accumulator << 1);
  }
  rp2040TestRunning = false;

  unsigned long total = rp2040Core0Count + rp2040Core1Count;
  unsigned long dominant = max(rp2040Core0Count, rp2040Core1Count);
  float scaling = dominant > 0 ? (float)total / (float)dominant : 0.0f;

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Core 0 iterations: "));
  SERIAL_OUT.println(rp2040Core0Count);
  SERIAL_OUT.print(F_STR("Core 1 iterations: "));
  SERIAL_OUT.println(rp2040Core1Count);
  SERIAL_OUT.print(F_STR("Total iterations: "));
  SERIAL_OUT.println(total);
  SERIAL_OUT.print(F_STR("Scaling efficiency: "));
  SERIAL_OUT.print(scaling, 2);
  SERIAL_OUT.println(F_STR("x"));
#endif
  CSV_ROW("Core 0 (iters/ms)", rp2040Core0Count / 1000.0f);
  CSV_ROW("Core 1 (iters/ms)", rp2040Core1Count / 1000.0f);
#else
  SERIAL_OUT.println(F_STR("Single core performance:"));

  volatile unsigned long count = 0;
  unsigned long start = millis();
  while (millis() - start < 1000) {
    count++;
  }

  SERIAL_OUT.print(F_STR("Iterations in 1 second: "));
  SERIAL_OUT.println(count);
  SERIAL_OUT.println(F_STR("Note: Full dual-core requires multicore library"));
  CSV_ROW("Core 0 (iters/ms)", count / 1000.0f);
  CSV_BLANK("Core 1 (iters/ms)");
#endif
#endif
}
#endif
// ==================== SERIAL BAUD RATE BENCHMARK ====================
// NOTE: Baud rate sweeping causes boards to disconnect and require replug
// Disabled for stability

/*
void benchmarkSerialBaudRates() {
  printHeader("SERIAL: Baud Rate Sweep");
  
  const long baudRates[] = {9600, 57600, 115200, 230400, 460800, 921600};
  const int numRates = 6;
  
  SERIAL_OUT.println(F_STR("Testing throughput at different baud rates..."));
  delay(100);
  
  for (int i = 0; i < numRates; i++) {
    SERIAL_OUT.end();
    delay(100);
    SERIAL_OUT.begin(baudRates[i]);
    delay(100);
    
    // Test throughput at this baud rate
    const char testData[] = "0123456789";
    startBenchmark();
    for (int j = 0; j < 19; j++) {
      SERIAL_OUT.print(testData);
    }
    SERIAL_OUT.flush();
    unsigned long baudTime = endBenchmark();
    
    SERIAL_OUT.print(F_STR("Baud "));
    SERIAL_OUT.print(baudRates[i]);
    SERIAL_OUT.print(F_STR(": "));
    SERIAL_OUT.print(baudTime);
    SERIAL_OUT.print(F_STR(" μs ("));
    float bytesPerSec = 190000000.0f / baudTime;
    SERIAL_OUT.print(bytesPerSec, 0);
    SERIAL_OUT.println(F_STR(" bytes/sec)"));
    
    delay(50);
  }
  
  // Return to standard baud
  SERIAL_OUT.end();
  delay(100);
  SERIAL_OUT.begin(115200);
  delay(500);
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Returned to 115200 baud"));
}
*/

// ==================== HARDWARE RNG BENCHMARK ====================

// Include Zephyr RNG for STM32U5 boards
#if defined(BOARD_STM32U5) && defined(__ZEPHYR__)
#include <zephyr/random/random.h>
#endif

#if defined(ESP32) || defined(BOARD_STM32U5)
void benchmarkHardwareRNG() {
  printHeader("CRYPTO: Hardware RNG");

#if defined(ESP32)
  SERIAL_OUT.println(F_STR("Using ESP32 hardware RNG"));
#elif defined(BOARD_STM32U5)
#if defined(__ZEPHYR__)
  SERIAL_OUT.println(F_STR("Using STM32U585 hardware RNG (Zephyr)"));
#else
  SERIAL_OUT.println(F_STR("Using STM32U585 RNG"));
#endif
#endif

  // Test RNG speed
  startBenchmark();
  volatile uint32_t rngSum = 0;
  for (int i = 0; i < 1000; i++) {
#if defined(ESP32)
    rngSum += esp_random();
#elif defined(BOARD_STM32U5) && defined(__ZEPHYR__)
    rngSum += sys_rand32_get();  // True hardware RNG via Zephyr
#elif defined(BOARD_STM32U5)
    rngSum += random(0, 0xFFFFFFFF);  // Fallback to Arduino random
#endif
  }
  unsigned long rngTime = endBenchmark();

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(rngSum);

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Hardware RNG (1000 values): "));
  SERIAL_OUT.print(rngTime);
  SERIAL_OUT.print(F_STR(" μs ("));
  SERIAL_OUT.print(1000.0f / rngTime * 1000, 2);
  SERIAL_OUT.println(F_STR(" values/ms)"));
#endif
  // 1000 values * 4 bytes each = 4000 bytes; convert to bytes/sec
  CSV_ROW("RNG (bytes/sec)", (rngTime > 0) ? (4000000.0f / rngTime) : 0.0f);

  // Test randomness distribution
  uint32_t bins[4] = { 0, 0, 0, 0 };
  for (int i = 0; i < 10000; i++) {
#if defined(ESP32)
    uint32_t val = esp_random();
#elif defined(BOARD_STM32U5) && defined(__ZEPHYR__)
    uint32_t val = sys_rand32_get();
#elif defined(BOARD_STM32U5)
    uint32_t val = random(0, 0xFFFFFFFF);
#endif
    bins[val % 4]++;
  }

  SERIAL_OUT.println(F_STR("Distribution (10000 samples):"));
  for (int i = 0; i < 4; i++) {
    SERIAL_OUT.print(F_STR("  Bin "));
    SERIAL_OUT.print(i);
    SERIAL_OUT.print(F_STR(": "));
    SERIAL_OUT.print(bins[i]);
    SERIAL_OUT.print(F_STR(" ("));
    SERIAL_OUT.print(bins[i] / 100.0f, 1);
    SERIAL_OUT.println(F_STR("%)"));
  }
  SERIAL_OUT.println(F_STR("(Ideal: 25% each bin)"));
}
#endif
// ==================== SOFTWARE ATSE BENCHMARK ====================

// Arduino Uno R4 WiFi has an ESP32-S3 WiFi module with secure element capabilities
// SoftwareATSE library is included with the WiFiS3 library on Uno R4 WiFi
#if defined(ARDUINO_UNOR4_WIFI)

#include <SoftwareATSE.h>

constexpr int kAtseKeyId = 999;  // Reserved for benchmark use to avoid clobbering other keys.

void benchmarkSoftwareATSE() {
  printHeader("CRYPTO: SoftwareATSE (Uno R4 WiFi)");

  SERIAL_OUT.println(F_STR("Initializing SoftwareATSE..."));

  // Initialize SoftwareATSE
  if (!SATSE.begin()) {
    SERIAL_OUT.println(F_STR("SoftwareATSE initialization failed"));
    SERIAL_OUT.println(F_STR("The WiFi module may not be responding or"));
    SERIAL_OUT.println(F_STR("secure element features may not be available."));
    return;
  }

  SERIAL_OUT.println(F_STR("SoftwareATSE initialized successfully!"));
  SERIAL_OUT.println();

  // ===== Public Key Generation Benchmark =====
  SERIAL_OUT.println(F_STR("--- Public Key Generation ---"));

  bool keyGenSupported = false;
  const uint16_t keySlot = kAtseKeyId;
  unsigned long keyGenStart = millis();
  byte publicKey[64];   // Buffer for public key
  byte privateKey[32];  // Buffer for private key

  if (SATSE.generatePrivateKey(keySlot, privateKey) == 1) {
    // Try to generate public key in the same slot
    if (SATSE.generatePublicKey(kAtseKeyId, publicKey) == 1) {
      unsigned long keyGenTime = millis() - keyGenStart;
      keyGenSupported = true;

      SERIAL_OUT.print(F_STR("SoftwareATSE: Public key generation (ms): "));
      SERIAL_OUT.println(keyGenTime);

      SERIAL_OUT.print(F_STR("  Rate: "));
      if (keyGenTime > 0) {
        SERIAL_OUT.print(1000.0f / keyGenTime, 3);
        SERIAL_OUT.println(F_STR(" keys/sec"));
      } else {
        SERIAL_OUT.println(F_STR("N/A (too fast)"));
      }
    } else {
      SERIAL_OUT.println(F_STR("SoftwareATSE: Public key generation - not supported"));
    }
  } else {
    SERIAL_OUT.println(F_STR("SoftwareATSE: Private key generation failed; skipping public key test"));
  }

  SERIAL_OUT.println();

  // ===== Configuration Write Benchmark =====
  SERIAL_OUT.println(F_STR("--- Secure Storage Operations ---"));

  uint8_t testConfig[256];
  for (int i = 0; i < 256; i++) {
    testConfig[i] = i & 0xFF;
  }

  unsigned long writeStart = micros();
  bool writeSuccess = false;

  if (SATSE.writeConfiguration(testConfig) == 1) {
    unsigned long writeTime = micros() - writeStart;
    writeSuccess = true;

    SERIAL_OUT.print(F_STR("Config write (\xC2\xB5s): "));
    SERIAL_OUT.println(writeTime);
    SERIAL_OUT.print(F_STR("  (ms): "));
    SERIAL_OUT.println(writeTime / 1000.0f, 3);

#if !CSV_OUTPUT
    SERIAL_OUT.print(F_STR("  Write speed: "));
    if (writeTime > 0) {
      SERIAL_OUT.print(256.0f * 1000000.0f / writeTime, 2);
      SERIAL_OUT.println(F_STR(" bytes/sec"));
    } else {
      SERIAL_OUT.println(F_STR("N/A (too fast)"));
    }
#endif
    CSV_ROW("Secure Storage Write (bytes/sec)", writeTime > 0 ? 256.0f * 1000000.0f / writeTime : 0.0f);
  } else {
    SERIAL_OUT.println(F_STR("Config write - not supported"));
  }

  SERIAL_OUT.println();

  // ===== Signing Benchmark =====
  SERIAL_OUT.println(F_STR("--- EC Signing ---"));

  if (keyGenSupported) {
    uint8_t testData[32];
    for (int i = 0; i < 32; i++) {
      testData[i] = (uint8_t)(i * 7 + 13);
    }

    uint8_t signature[64];

    // Signing benchmark
    const int NUM_SIGN_OPS = 10;
    unsigned long signStart = micros();
    int successfulSigns = 0;

    for (int i = 0; i < NUM_SIGN_OPS; i++) {
      if (SATSE.ecSign(kAtseKeyId, testData, signature)) {
        successfulSigns++;
      }
      yield();
    }

    unsigned long signTime = micros() - signStart;

    if (successfulSigns > 0) {
      float avgSignTimeMs = (signTime / 1000.0f) / successfulSigns;
      SERIAL_OUT.print(F_STR("EC signing (avg ms/op): "));
      SERIAL_OUT.println(avgSignTimeMs, 3);

      SERIAL_OUT.print(F_STR("  Rate: "));
      SERIAL_OUT.print(1000.0f / avgSignTimeMs, 2);
      SERIAL_OUT.println(F_STR(" ops/sec"));

      SERIAL_OUT.print(F_STR("  Successful operations: "));
      SERIAL_OUT.print(successfulSigns);
      SERIAL_OUT.print(F_STR("/"));
      SERIAL_OUT.println(NUM_SIGN_OPS);
    } else {
      SERIAL_OUT.println(F_STR("EC signing - not supported"));
    }
  } else {
    SERIAL_OUT.println(F_STR("Signing tests skipped (key generation not supported)"));
  }

  SERIAL_OUT.println();

  // ===== Secure RNG Benchmark =====
  SERIAL_OUT.println(F_STR("--- Secure Random Number Generator ---"));

  uint8_t rngTest[16];
  if (SATSE.random(rngTest, 16)) {

    // Benchmark RNG throughput
    const int RNG_BUFFER_SIZE = 256;
    const int RNG_ITERATIONS = 20;
    uint8_t rngBuffer[RNG_BUFFER_SIZE];

    unsigned long rngStart = micros();
    int successfulReads = 0;
    uint32_t rngChecksum = 0;

    for (int i = 0; i < RNG_ITERATIONS; i++) {
      if (SATSE.random(rngBuffer, RNG_BUFFER_SIZE)) {
        successfulReads++;
        for (int j = 0; j < RNG_BUFFER_SIZE; j++) {
          rngChecksum += rngBuffer[j];
        }
      }
      yield();
    }

    unsigned long rngTime = micros() - rngStart;

    SERIAL_OUT.print(F_STR("RNG checksum: "));
    SERIAL_OUT.println(rngChecksum);

    if (successfulReads > 0) {
      unsigned long totalBytes = successfulReads * RNG_BUFFER_SIZE;
      float bytesPerSec = (totalBytes * 1000000.0f) / rngTime;

#if !CSV_OUTPUT
      SERIAL_OUT.print(F_STR("Secure RNG throughput: "));
      SERIAL_OUT.print(bytesPerSec, 2);
      SERIAL_OUT.println(F_STR(" bytes/sec"));
#endif
      CSV_ROW("RNG (bytes/sec)", bytesPerSec);

      SERIAL_OUT.print(F_STR("  Total bytes generated: "));
      SERIAL_OUT.println(totalBytes);

      SERIAL_OUT.print(F_STR("  Time: "));
      SERIAL_OUT.print(rngTime / 1000.0f, 2);
      SERIAL_OUT.println(F_STR(" ms"));

      SERIAL_OUT.print(F_STR("  Successful reads: "));
      SERIAL_OUT.print(successfulReads);
      SERIAL_OUT.print(F_STR("/"));
      SERIAL_OUT.println(RNG_ITERATIONS);

      // Distribution test
      SERIAL_OUT.println(F_STR("Distribution test (first 1000 bytes):"));
      uint32_t byteBins[4] = { 0, 0, 0, 0 };

      for (int i = 0; i < 1000; i++) {
        uint8_t randomByte;
        if (SATSE.random(&randomByte, 1)) {
          byteBins[randomByte % 4]++;
        }
      }

      for (int i = 0; i < 4; i++) {
        SERIAL_OUT.print(F_STR("  Bin "));
        SERIAL_OUT.print(i);
        SERIAL_OUT.print(F_STR(": "));
        SERIAL_OUT.print(byteBins[i]);
        SERIAL_OUT.print(F_STR(" ("));
        SERIAL_OUT.print(byteBins[i] / 10.0f, 1);
        SERIAL_OUT.println(F_STR("%)"));
      }
      SERIAL_OUT.println(F_STR("  (Ideal: 25% each bin)"));
    } else {
      SERIAL_OUT.println(F_STR("Secure RNG - read operations failed"));
    }
  } else {
    SERIAL_OUT.println(F_STR("Secure RNG - not supported"));
  }

  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("SoftwareATSE benchmarks complete"));
}

#else  // !ARDUINO_UNOR4_WIFI

// Stub function for non-Uno R4 WiFi boards
void benchmarkSoftwareATSE() {
  // This function intentionally left empty for non-R4 boards
  // It won't be called due to compile-time guards in setup()
}

#endif  // ARDUINO_UNOR4_WIFI

// ==================== ARDUINO BRIDGE (UNO Q MCU<->LINUX) ====================

#if defined(BOARD_STM32U5) && defined(HAS_DUAL_PROCESSOR)

void benchmarkArduinoBridge() {
  printHeader("ARDUINO BRIDGE (MCU<->LINUX RPC)");

#ifdef HAS_RPC_BRIDGE
  SERIAL_OUT.println(F_STR("Testing MCU<->Linux RPC communication..."));
  SERIAL_OUT.println(F_STR("Using MessagePack RPC over Serial1"));
  SERIAL_OUT.println();

  // Note: For RouterBridge, Bridge object is pre-initialized
  // For RPClite, we need to create transport and client

#ifdef HAS_ROUTER_BRIDGE
  // Using Arduino_RouterBridge high-level API
  SERIAL_OUT.println(F_STR("Bridge Type: RouterBridge (high-level API)"));
  SERIAL_OUT.println();

  // Test 1: Simple RPC call latency
  SERIAL_OUT.println(F_STR("1. RPC Call Latency Test"));
  const int PING_COUNT = 50;
  unsigned long totalLatency = 0;
  int successfulCalls = 0;

  for (int i = 0; i < PING_COUNT; i++) {
    unsigned long start = micros();

    // Call a simple echo/ping function on Linux side
    // Assumes Linux has registered an "echo" or "ping" method
    String result;
    bool ok = Bridge.call("echo", result, "ping").result(result);

    unsigned long elapsed = micros() - start;

    if (ok) {
      totalLatency += elapsed;
      successfulCalls++;
    }

    if (i % 10 == 0) {
      yield();
    }
  }

  if (successfulCalls > 0) {
    SERIAL_OUT.print(F_STR("  Successful calls: "));
    SERIAL_OUT.print(successfulCalls);
    SERIAL_OUT.print(F_STR("/"));
    SERIAL_OUT.println(PING_COUNT);
    SERIAL_OUT.print(F_STR("  Average latency: "));
    SERIAL_OUT.print(totalLatency / successfulCalls);
    SERIAL_OUT.println(F_STR(" µs"));
    SERIAL_OUT.print(F_STR("  Calls per second: "));
    SERIAL_OUT.println((successfulCalls * 1000000.0) / totalLatency, 0);
  } else {
    SERIAL_OUT.println(F_STR("  ERROR: No successful RPC calls"));
    SERIAL_OUT.println(F_STR("  Make sure Linux side has 'echo' method registered"));
  }
  SERIAL_OUT.println();

  // Test 2: Integer arithmetic RPC
  SERIAL_OUT.println(F_STR("2. Integer Arithmetic RPC Test"));
  int mathTests = 0;
  int mathSuccess = 0;
  unsigned long mathTime = micros();

  for (int i = 0; i < 20; i++) {
    int sum;
    if (Bridge.call("add", sum, i, i + 1).result(sum)) {
      if (sum == (i + i + 1)) {
        mathSuccess++;
      }
      mathTests++;
    }
  }

  mathTime = micros() - mathTime;
  SERIAL_OUT.print(F_STR("  Tests: "));
  SERIAL_OUT.print(mathSuccess);
  SERIAL_OUT.print(F_STR("/"));
  SERIAL_OUT.println(mathTests);
  SERIAL_OUT.print(F_STR("  Average time: "));
  SERIAL_OUT.print(mathTime / mathTests);
  SERIAL_OUT.println(F_STR(" µs"));
  SERIAL_OUT.println();

  // Test 3: String operations
  SERIAL_OUT.println(F_STR("3. String RPC Test"));
  int strTests = 0;
  int strSuccess = 0;
  unsigned long strTime = micros();

  for (int i = 0; i < 10; i++) {
    String message = "Test_" + String(i);
    String response;
    if (Bridge.call("loopback", response, message).result(response)) {
      if (response == message) {
        strSuccess++;
      }
      strTests++;
    }
  }

  strTime = micros() - strTime;
  SERIAL_OUT.print(F_STR("  Tests: "));
  SERIAL_OUT.print(strSuccess);
  SERIAL_OUT.print(F_STR("/"));
  SERIAL_OUT.println(strTests);
  SERIAL_OUT.print(F_STR("  Average time: "));
  SERIAL_OUT.print(strTime / strTests);
  SERIAL_OUT.println(F_STR(" µs"));
  SERIAL_OUT.println();

  // Test 4: Async call test
  SERIAL_OUT.println(F_STR("4. Async RPC Call Test"));
  unsigned long asyncStart = micros();

  // Make multiple async calls
  RpcCall call1 = Bridge.call("add", 10, 20);
  RpcCall call2 = Bridge.call("add", 30, 40);
  RpcCall call3 = Bridge.call("add", 50, 60);

  // Now wait for results
  int result1, result2, result3;
  bool ok1 = call1.result(result1);
  bool ok2 = call2.result(result2);
  bool ok3 = call3.result(result3);

  unsigned long asyncTime = micros() - asyncStart;

  SERIAL_OUT.print(F_STR("  3 async calls completed in: "));
  SERIAL_OUT.print(asyncTime);
  SERIAL_OUT.println(F_STR(" µs"));
  if (ok1 && ok2 && ok3) {
    SERIAL_OUT.print(F_STR("  Results: "));
    SERIAL_OUT.print(result1);
    SERIAL_OUT.print(F_STR(", "));
    SERIAL_OUT.print(result2);
    SERIAL_OUT.print(F_STR(", "));
    SERIAL_OUT.println(result3);
  }
  SERIAL_OUT.println();

  SERIAL_OUT.println(F_STR("NOTE: These tests require corresponding RPC methods"));
  SERIAL_OUT.println(F_STR("      registered on the Linux side (echo, add, loopback)."));
  SERIAL_OUT.println(F_STR("      See Arduino_RouterBridge examples for Linux setup."));

#elif defined(HAS_RPCLITE)
  // Using Arduino_RPClite low-level API
  SERIAL_OUT.println(F_STR("Bridge Type: RPClite (low-level API)"));
  SERIAL_OUT.println();

  SerialTransport transport(Serial1);
  RPCClient client(transport);

  SERIAL_OUT.println(F_STR("1. RPC Call Latency Test"));
  const int PING_COUNT = 50;
  unsigned long totalLatency = 0;
  int successfulCalls = 0;

  for (int i = 0; i < PING_COUNT; i++) {
    unsigned long start = micros();

    String result;
    bool ok = client.call("echo", result, "ping");

    unsigned long elapsed = micros() - start;

    if (ok) {
      totalLatency += elapsed;
      successfulCalls++;
    }

    if (i % 10 == 0) {
      yield();
    }
  }

  if (successfulCalls > 0) {
    SERIAL_OUT.print(F_STR("  Successful calls: "));
    SERIAL_OUT.print(successfulCalls);
    SERIAL_OUT.print(F_STR("/"));
    SERIAL_OUT.println(PING_COUNT);
    SERIAL_OUT.print(F_STR("  Average latency: "));
    SERIAL_OUT.print(totalLatency / successfulCalls);
    SERIAL_OUT.println(F_STR(" µs"));
  }
  SERIAL_OUT.println();

  SERIAL_OUT.println(F_STR("NOTE: RPClite requires manual transport setup."));
  SERIAL_OUT.println(F_STR("      Ensure Serial1 is properly initialized."));
#endif

#else
  SERIAL_OUT.println(F_STR("Arduino RPC Bridge library not detected."));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("To enable Bridge/RPC benchmarks:"));
  SERIAL_OUT.println(F_STR("  1. Install Arduino_RouterBridge or Arduino_RPClite"));
  SERIAL_OUT.println(F_STR("  2. Ensure Linux side has RPC server running"));
  SERIAL_OUT.println(F_STR("  3. Register test methods (echo, add, loopback)"));
  SERIAL_OUT.println(F_STR("  4. Re-compile and upload sketch"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Library: github.com/arduino-libraries/Arduino_RouterBridge"));
  SERIAL_OUT.println(F_STR("Library: github.com/arduino-libraries/Arduino_RPClite"));
#endif

  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Arduino Bridge benchmarks complete"));
}

#else  // !BOARD_STM32U5 || !HAS_DUAL_PROCESSOR

// Stub function for non-Uno Q boards
void benchmarkArduinoBridge() {
  // This function intentionally left empty for non-Uno Q boards
  // It won't be called due to compile-time guards in setup()
}

#endif  // BOARD_STM32U5 && HAS_DUAL_PROCESSOR

// ==================== ARDUINO YUN BRIDGE (MCU<->LINUX VIA SERIAL1) ====================

#if defined(HAS_YUN_BRIDGE)
void benchmarkYunBridge() {
  printHeader("YUN SERIAL1/BRIDGE KILLER TEST");

  SERIAL_OUT.println(F_STR("Initializing Bridge daemon link..."));
  Bridge.begin();
  delay(200);
  SERIAL_OUT.println(F_STR("Bridge ready."));
  SERIAL_OUT.println();

  // Test 1: Round-trip latency using Bridge key/value operations.
  SERIAL_OUT.println(F_STR("1. MCU<->Linux round-trip latency (Bridge.put + Bridge.get)"));

  const int LATENCY_ITERS = 50;
  const char *latencyKey = "bench.lat";
  unsigned long latencyTotal = 0;
  unsigned long latencyMin = ULONG_MAX;
  unsigned long latencyMax = 0;
  int latencySuccess = 0;

  char writeValue[20];
  char readBuffer[24];

  for (int i = 0; i < LATENCY_ITERS; i++) {
    snprintf(writeValue, sizeof(writeValue), "pkt_%d", i);

    unsigned long t0 = micros();
    Bridge.put(latencyKey, writeValue);
    Bridge.get(latencyKey, readBuffer, sizeof(readBuffer));
    unsigned long elapsed = micros() - t0;

    if (strcmp(readBuffer, writeValue) == 0) {
      latencySuccess++;
      latencyTotal += elapsed;
      if (elapsed < latencyMin) latencyMin = elapsed;
      if (elapsed > latencyMax) latencyMax = elapsed;
    }

    if ((i % 10) == 0) {
      yield();
    }
  }

  SERIAL_OUT.print(F_STR("  Successful round-trips: "));
  SERIAL_OUT.print(latencySuccess);
  SERIAL_OUT.print(F_STR("/"));
  SERIAL_OUT.println(LATENCY_ITERS);

  if (latencySuccess > 0) {
    SERIAL_OUT.print(F_STR("  Avg latency: "));
    SERIAL_OUT.print(latencyTotal / latencySuccess);
    SERIAL_OUT.println(F_STR(" us"));
    SERIAL_OUT.print(F_STR("  Min latency: "));
    SERIAL_OUT.print(latencyMin);
    SERIAL_OUT.println(F_STR(" us"));
    SERIAL_OUT.print(F_STR("  Max latency: "));
    SERIAL_OUT.print(latencyMax);
    SERIAL_OUT.println(F_STR(" us"));
  } else {
    SERIAL_OUT.println(F_STR("  ERROR: Bridge round-trip validation failed."));
  }
  SERIAL_OUT.println();

  // Test 2: Throughput test for MCU->Linux transfer (Bridge.put payload stream).
  SERIAL_OUT.println(F_STR("2. MCU->Linux throughput (Bridge.put payload stream)"));

  const int THROUGHPUT_ITERS = 100;
  const int PAYLOAD_BYTES = 64;
  char payload[PAYLOAD_BYTES + 1];
  for (int i = 0; i < PAYLOAD_BYTES; i++) {
    payload[i] = 'A' + (i % 26);
  }
  payload[PAYLOAD_BYTES] = '\0';

  unsigned long throughputStart = micros();
  for (int i = 0; i < THROUGHPUT_ITERS; i++) {
    Bridge.put("bench.tp", payload);
    if ((i % 20) == 0) {
      yield();
    }
  }
  unsigned long throughputTime = micros() - throughputStart;

  unsigned long totalBytes = (unsigned long)THROUGHPUT_ITERS * PAYLOAD_BYTES;
  float bytesPerSecond = (throughputTime > 0)
                           ? (totalBytes * 1000000.0f) / throughputTime
                           : 0.0f;

  SERIAL_OUT.print(F_STR("  Payload size: "));
  SERIAL_OUT.print(PAYLOAD_BYTES);
  SERIAL_OUT.println(F_STR(" bytes"));
  SERIAL_OUT.print(F_STR("  Total bytes sent: "));
  SERIAL_OUT.println(totalBytes);
  SERIAL_OUT.print(F_STR("  Total time: "));
  SERIAL_OUT.print(throughputTime / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("  Throughput: "));
  SERIAL_OUT.print(bytesPerSecond, 1);
  SERIAL_OUT.println(F_STR(" bytes/sec"));
#endif
  CSV_ROW("Yun Bridge Throughput (bytes/sec)", bytesPerSecond);

  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Yun Bridge killer test complete."));
}
#else
void benchmarkYunBridge() {
  // Stub for non-Yun boards.
}
#endif

// ==================== MULTIDUINO-SPECIFIC BENCHMARKS ====================

#ifdef HAS_RTC
void benchmarkRTC() {
  printHeader("RTC BENCHMARK (DS1307)");

  // Initialize RTC
  if (!rtc.begin()) {
    SERIAL_OUT.println(F_STR("ERROR: RTC not found!"));
    SERIAL_OUT.println(F_STR("Check wiring: SDA->A4, SCL->A5"));
    SERIAL_OUT.println();
    return;
  }

  SERIAL_OUT.println(F_STR("RTC initialized successfully"));

  // Check if RTC is running
  if (!rtc.isrunning()) {
    SERIAL_OUT.println(F_STR("WARNING: RTC is not running!"));
    SERIAL_OUT.println(F_STR("Starting RTC and setting time..."));
    // Set to compile time as default
    rtc.adjust(DateTime(F_STR(__DATE__), F_STR(__TIME__)));
  }

  // Display current time
  DateTime now = rtc.now();
  SERIAL_OUT.print(F_STR("Current Time: "));
  SERIAL_OUT.print(now.year(), DEC);
  SERIAL_OUT.print('/');
  if (now.month() < 10) SERIAL_OUT.print('0');
  SERIAL_OUT.print(now.month(), DEC);
  SERIAL_OUT.print('/');
  if (now.day() < 10) SERIAL_OUT.print('0');
  SERIAL_OUT.print(now.day(), DEC);
  SERIAL_OUT.print(" ");
  if (now.hour() < 10) SERIAL_OUT.print('0');
  SERIAL_OUT.print(now.hour(), DEC);
  SERIAL_OUT.print(':');
  if (now.minute() < 10) SERIAL_OUT.print('0');
  SERIAL_OUT.print(now.minute(), DEC);
  SERIAL_OUT.print(':');
  if (now.second() < 10) SERIAL_OUT.print('0');
  SERIAL_OUT.println(now.second(), DEC);
  SERIAL_OUT.println();

  // Benchmark 1: RTC Read Speed
  SERIAL_OUT.println(F_STR("Test: RTC Read Speed"));
  volatile uint32_t readChecksum = 0;
  unsigned long startTime = micros();
  uint32_t reads = 0;

  unsigned long testDuration = 1000000UL;  // 1 second
  while (micros() - startTime < testDuration) {
    DateTime reading = rtc.now();
    readChecksum += reading.unixtime();
    reads++;
  }
  unsigned long elapsed = micros() - startTime;

  float readsPerMs = (reads * 1000.0f) / elapsed;
  SERIAL_OUT.print(F_STR("  Reads: "));
  SERIAL_OUT.print(reads);
  SERIAL_OUT.print(F_STR(" in "));
  SERIAL_OUT.print(elapsed / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
  SERIAL_OUT.print(F_STR("  Speed: "));
  SERIAL_OUT.print(readsPerMs, 2);
  SERIAL_OUT.println(F_STR(" reads/ms"));
  SERIAL_OUT.print(F_STR("  Time per read: "));
  SERIAL_OUT.print((float)elapsed / reads, 2);
  SERIAL_OUT.println(F_STR(" μs"));
  SERIAL_OUT.print(F_STR("  Checksum: 0x"));
  SERIAL_OUT.println((unsigned long)readChecksum, HEX);
  SERIAL_OUT.println();

  // Benchmark 2: RTC Write Speed (Using adjust - time setting)
  SERIAL_OUT.println(F_STR("Test: RTC Write Speed (Time Adjust)"));
  startTime = micros();
  uint32_t writes = 0;

  testDuration = 1000000UL;  // 1 second
  DateTime testTime = DateTime(2025, 1, 1, 12, 0, 0);
  while (micros() - startTime < testDuration) {
    rtc.adjust(testTime);
    writes++;
  }
  elapsed = micros() - startTime;

  // Restore current time
  rtc.adjust(now);

  float writesPerMs = (writes * 1000.0f) / elapsed;
  SERIAL_OUT.print(F_STR("  Writes: "));
  SERIAL_OUT.print(writes);
  SERIAL_OUT.print(F_STR(" in "));
  SERIAL_OUT.print(elapsed / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
  SERIAL_OUT.print(F_STR("  Speed: "));
  SERIAL_OUT.print(writesPerMs, 2);
  SERIAL_OUT.println(F_STR(" writes/ms"));
  SERIAL_OUT.print(F_STR("  Time per write: "));
  SERIAL_OUT.print((float)elapsed / writes, 2);
  SERIAL_OUT.println(F_STR(" μs"));
  SERIAL_OUT.println();

  // Benchmark 3: NVRAM Read/Write Speed (DS1307 has 56 bytes of NVRAM)
  SERIAL_OUT.println(F_STR("Test: NVRAM Read/Write (56 bytes)"));
  const uint8_t nvramSize = 56;
  uint8_t nvramData[nvramSize];

  // Write test
  for (uint8_t i = 0; i < nvramSize; i++) {
    nvramData[i] = i;
  }

  startTime = micros();
  uint32_t nvramWrites = 0;
  testDuration = 1000000UL;  // 1 second

  while (micros() - startTime < testDuration) {
    for (uint8_t addr = 0; addr < nvramSize; addr++) {
      rtc.writenvram(addr, nvramData[addr]);
    }
    nvramWrites++;
  }
  elapsed = micros() - startTime;

  uint32_t totalBytesWritten = nvramWrites * nvramSize;
  float nvramWriteSpeed = (totalBytesWritten * 1000.0f) / elapsed;

  SERIAL_OUT.print(F_STR("  NVRAM Writes: "));
  SERIAL_OUT.print(totalBytesWritten);
  SERIAL_OUT.print(F_STR(" bytes in "));
  SERIAL_OUT.print(elapsed / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
  SERIAL_OUT.print(F_STR("  Speed: "));
  SERIAL_OUT.print(nvramWriteSpeed, 2);
  SERIAL_OUT.println(F_STR(" bytes/ms"));

  // Read test
  volatile uint32_t nvramChecksum = 0;
  startTime = micros();
  uint32_t nvramReads = 0;

  while (micros() - startTime < testDuration) {
    for (uint8_t addr = 0; addr < nvramSize; addr++) {
      nvramChecksum += rtc.readnvram(addr);
    }
    nvramReads++;
  }
  elapsed = micros() - startTime;

  uint32_t totalBytesRead = nvramReads * nvramSize;
  float nvramReadSpeed = (totalBytesRead * 1000.0f) / elapsed;

  SERIAL_OUT.print(F_STR("  NVRAM Reads: "));
  SERIAL_OUT.print(totalBytesRead);
  SERIAL_OUT.print(F_STR(" bytes in "));
  SERIAL_OUT.print(elapsed / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
  SERIAL_OUT.print(F_STR("  Speed: "));
  SERIAL_OUT.print(nvramReadSpeed, 2);
  SERIAL_OUT.println(F_STR(" bytes/ms"));
  SERIAL_OUT.print(F_STR("  Checksum: 0x"));
  SERIAL_OUT.println((unsigned long)nvramChecksum, HEX);
  SERIAL_OUT.println();

  SERIAL_OUT.println(F_STR("RTC benchmarks complete"));
}
#endif  // HAS_RTC
// ==================== ESP32 ADDITIONAL BENCHMARKS ====================

#if defined(ESP32)

// ESP32 DAC Benchmark (GPIO25/26 on classic ESP32)
#if defined(CONFIG_IDF_TARGET_ESP32)
void benchmarkESP32DAC() {
  printHeader("I/O: DAC OUTPUT (ESP32)");

  SERIAL_OUT.println(F_STR("ESP32 has 2x 8-bit DACs on GPIO25 and GPIO26"));

  const int dacPin = 25;  // DAC1

  // DAC write speed benchmark
  volatile uint32_t dacValue = 0;
  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    dacWrite(dacPin, dacValue & 0xFF);
    dacValue++;
  }
  unsigned long dacTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("dacWrite() (10000 ops): "));
  SERIAL_OUT.print(dacTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(10000.0 / dacTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("DAC (ops/ms)", 10000.0 / dacTime * 1000);

  // DAC ramp test - measure update rate
  startBenchmark();
  for (int cycle = 0; cycle < 100; cycle++) {
    for (int val = 0; val < 256; val++) {
      dacWrite(dacPin, val);
    }
  }
  unsigned long rampTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("DAC ramp (100 cycles, 25600 writes): "));
  SERIAL_OUT.print(rampTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(25600.0 / rampTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("DAC Ramp (ops/ms)", 25600.0 / rampTime * 1000);

  // Calculate effective sample rate
  float sampleRate = 25600.0 / (rampTime / 1000000.0);
  SERIAL_OUT.print(F_STR("Effective sample rate: "));
  SERIAL_OUT.print(sampleRate / 1000, 1);
  SERIAL_OUT.println(F_STR(" kHz"));

  dacWrite(dacPin, 0);  // Reset to 0
}
#endif  // CONFIG_IDF_TARGET_ESP32

// ESP32 Touch Sensor Benchmark
void benchmarkESP32Touch() {
  printHeader("I/O: CAPACITIVE TOUCH (ESP32)");

#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C3)
  SERIAL_OUT.println(F_STR("Touch sensors not available on this ESP32 variant"));
  CSV_BLANK("Touch (values/ms)");
  return;
#else
  // Touch pins vary by ESP32 variant
  int touchPin;
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
  touchPin = 1;  // T1 = GPIO1
#else
  touchPin = 4;  // T0 = GPIO4 on classic ESP32
#endif

  SERIAL_OUT.print(F_STR("Testing touch pin: GPIO"));
  SERIAL_OUT.println(touchPin);

  // Warm up reads
  for (int i = 0; i < 10; i++) {
    touchRead(touchPin);
  }

  // Touch read speed benchmark
  volatile uint32_t touchSum = 0;
  startBenchmark();
  for (int i = 0; i < 1000; i++) {
    touchSum += touchRead(touchPin);
  }
  unsigned long touchTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("touchRead() (1000 ops): "));
  SERIAL_OUT.print(touchTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(1000.0 / touchTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("Touch (values/ms)", 1000.0 / touchTime * 1000);

  SERIAL_OUT.print(F_STR("Average touch value: "));
  SERIAL_OUT.println(touchSum / 1000);

  // Touch threshold detection timing
  uint16_t baseline = touchRead(touchPin);
  SERIAL_OUT.print(F_STR("Baseline (no touch): "));
  SERIAL_OUT.println(baseline);

  SERIAL_OUT.print(F_STR("Time per read: "));
  SERIAL_OUT.print(touchTime / 1000.0, 2);
  SERIAL_OUT.println(F_STR(" us"));
#endif
}

// ESP32 Light Sleep Benchmark
void benchmarkESP32Sleep() {
  printHeader("POWER: LIGHT SLEEP TIMING (ESP32)");

  SERIAL_OUT.println(F_STR("Testing light sleep wake latency..."));
  SERIAL_OUT.println(F_STR("(Deep sleep would reset the MCU)"));
  SERIAL_OUT.flush();

  // Configure timer wake up for light sleep
  const uint64_t sleepTimeUs = 1000;  // 1ms sleep

  // Measure wake latency over multiple iterations
  unsigned long totalWakeTime = 0;
  const int iterations = 20;

  for (int i = 0; i < iterations; i++) {
    esp_sleep_enable_timer_wakeup(sleepTimeUs);

    unsigned long beforeSleep = micros();
    esp_light_sleep_start();
    unsigned long afterWake = micros();

    unsigned long actualSleep = afterWake - beforeSleep;
    totalWakeTime += actualSleep;

    yield();
  }

  float avgSleepTime = (float)totalWakeTime / iterations;
  float wakeOverhead = avgSleepTime - sleepTimeUs;

  SERIAL_OUT.print(F_STR("Target sleep time: "));
  SERIAL_OUT.print(sleepTimeUs);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Average actual time: "));
  SERIAL_OUT.print(avgSleepTime, 1);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Wake overhead: "));
  SERIAL_OUT.print(wakeOverhead, 1);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Iterations: "));
  SERIAL_OUT.println(iterations);

  // Test different sleep durations (with timeout protection)
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Sleep duration accuracy:"));

  uint64_t testDurations[] = { 100, 500, 1000, 5000, 10000 };
  for (int d = 0; d < 5; d++) {
    // Add timeout check to prevent freezing on some variants
    unsigned long testStart = millis();
    
    esp_sleep_enable_timer_wakeup(testDurations[d]);
    unsigned long before = micros();
    
    // Use timeout - if sleep doesn't wake in 50ms, something is wrong
    unsigned long timeout = 50000;  // 50ms timeout
    while ((micros() - before) < timeout) {
      esp_light_sleep_start();
      unsigned long actual = micros() - before;
      
      if (actual > 0) {
        float error = ((float)actual - testDurations[d]) / testDurations[d] * 100;

        SERIAL_OUT.print(F_STR("  Target "));
        SERIAL_OUT.print((uint32_t)testDurations[d]);
        SERIAL_OUT.print(F_STR(" us -> Actual "));
        SERIAL_OUT.print(actual);
        SERIAL_OUT.print(F_STR(" us ("));
        SERIAL_OUT.print(error, 1);
        SERIAL_OUT.println(F_STR("% error)"));
        break;
      }
    }
    
    // Check if timeout occurred
    if ((millis() - testStart) > 100) {
      SERIAL_OUT.println(F_STR("  [Sleep test timeout - skipping remaining tests]"));
      break;
    }
    
    yield();
  }
}

// ESP32 Hardware AES Benchmark
void benchmarkESP32AES() {
  printHeader("CRYPTO: HARDWARE AES (ESP32)");

  SERIAL_OUT.println(F_STR("Using ESP32 hardware AES accelerator"));

  // Test data
  uint8_t key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
  uint8_t plaintext[16] = { 0 };
  uint8_t ciphertext[16] = { 0 };
  uint8_t decrypted[16] = { 0 };

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  // AES-128 Encryption benchmark
  mbedtls_aes_setkey_enc(&aes, key, 128);

  volatile uint32_t checksum = 0;
  startBenchmark();
  for (int i = 0; i < 1000; i++) {
    plaintext[0] = i & 0xFF;
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, plaintext, ciphertext);
    checksum += ciphertext[0];
  }
  unsigned long encTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("AES-128 Encrypt (1000 blocks): "));
  SERIAL_OUT.print(encTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(1000.0 / encTime * 1000);
  SERIAL_OUT.println(F_STR(" blocks/ms)"));
#endif
  CSV_ROW("AES (ops/ms)", 1000.0 / encTime * 1000);

  // AES-128 Decryption benchmark
  mbedtls_aes_setkey_dec(&aes, key, 128);

  startBenchmark();
  for (int i = 0; i < 1000; i++) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, ciphertext, decrypted);
    checksum += decrypted[0];
  }
  unsigned long decTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("AES-128 Decrypt (1000 blocks): "));
  SERIAL_OUT.print(decTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(1000.0 / decTime * 1000);
  SERIAL_OUT.println(F_STR(" blocks/ms)"));
#endif
  CSV_ROW("AES Decrypt (ops/ms)", 1000.0 / decTime * 1000);

  // Throughput calculation (16 bytes per block)
  float encThroughput = (1000 * 16) / (encTime / 1000000.0) / 1024;
  float decThroughput = (1000 * 16) / (decTime / 1000000.0) / 1024;

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Encrypt throughput: "));
  SERIAL_OUT.print(encThroughput, 1);
  SERIAL_OUT.println(F_STR(" KB/s"));

  SERIAL_OUT.print(F_STR("Decrypt throughput: "));
  SERIAL_OUT.print(decThroughput, 1);
  SERIAL_OUT.println(F_STR(" KB/s"));
#endif
  CSV_ROW("AES Encrypt (KB/s)", encThroughput);
  CSV_ROW("AES Decrypt (KB/s)", decThroughput);

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(checksum);

  mbedtls_aes_free(&aes);
}

#endif  // ESP32
// ==================== RP2040 ADDITIONAL BENCHMARKS ====================

#if defined(ARDUINO_ARCH_RP2040)

// RP2040 DMA Benchmark
void benchmarkRP2040DMA() {
  printHeader("MEMORY: DMA vs CPU COPY (RP2040)");

  const size_t bufSize = 4096;
  uint8_t *src = (uint8_t *)malloc(bufSize);
  uint8_t *dst = (uint8_t *)malloc(bufSize);

  if (!src || !dst) {
    SERIAL_OUT.println(F_STR("Failed to allocate buffers"));
    if (src) free(src);
    if (dst) free(dst);
    return;
  }

  // Initialize source
  for (size_t i = 0; i < bufSize; i++) {
    src[i] = i & 0xFF;
  }

  // CPU memcpy benchmark
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    memcpy(dst, src, bufSize);
  }
  unsigned long cpuTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("CPU memcpy ("));
  SERIAL_OUT.print(bufSize * 100);
  SERIAL_OUT.print(F_STR(" bytes): "));
  SERIAL_OUT.print(cpuTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print((bufSize * 100.0) / cpuTime);
  SERIAL_OUT.println(F_STR(" MB/s)"));
#endif
  CSV_ROW("CPU memcpy (MB/s)", (bufSize * 100.0) / cpuTime);

  // DMA benchmark
  int dma_chan = dma_claim_unused_channel(true);

  dma_channel_config c = dma_channel_get_default_config(dma_chan);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, true);

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    dma_channel_configure(dma_chan, &c, dst, src, bufSize, true);
    dma_channel_wait_for_finish_blocking(dma_chan);
  }
  unsigned long dmaTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("DMA transfer ("));
  SERIAL_OUT.print(bufSize * 100);
  SERIAL_OUT.print(F_STR(" bytes): "));
  SERIAL_OUT.print(dmaTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print((bufSize * 100.0) / dmaTime);
  SERIAL_OUT.println(F_STR(" MB/s)"));

  // Speedup
  SERIAL_OUT.print(F_STR("DMA speedup: "));
  SERIAL_OUT.print((float)cpuTime / dmaTime, 2);
  SERIAL_OUT.println(F_STR("x"));
#endif
  CSV_ROW("DMA (MB/s)", (bufSize * 100.0) / dmaTime);

  // Verify correctness
  bool match = true;
  for (size_t i = 0; i < bufSize; i++) {
    if (dst[i] != src[i]) {
      match = false;
      break;
    }
  }
  SERIAL_OUT.print(F_STR("Data verification: "));
  SERIAL_OUT.println(match ? F_STR("PASS") : F_STR("FAIL"));

  dma_channel_unclaim(dma_chan);
  free(src);
  free(dst);
}

// RP2040 PIO Benchmark
void benchmarkRP2040PIO() {
  printHeader("I/O: PIO STATE MACHINE (RP2040)");

  SERIAL_OUT.println(F_STR("RP2040 Programmable I/O Information:"));
  SERIAL_OUT.println(F_STR("  2x PIO blocks, 4 state machines each"));
  SERIAL_OUT.print(F_STR("  System clock: "));
  SERIAL_OUT.print(clock_get_hz(clk_sys) / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));

  // PIO can run at system clock speed
  float maxPioFreq = clock_get_hz(clk_sys) / 1000000.0;
  SERIAL_OUT.print(F_STR("  Max PIO frequency: "));
  SERIAL_OUT.print(maxPioFreq, 0);
  SERIAL_OUT.println(F_STR(" MHz"));

  // Theoretical GPIO toggle rate (2 cycles per toggle)
  SERIAL_OUT.print(F_STR("  Theoretical GPIO toggle: "));
  SERIAL_OUT.print(maxPioFreq / 2, 0);
  SERIAL_OUT.println(F_STR(" MHz"));

  // Compare with digitalWrite
  const int testPin = 25;  // LED pin
  pinMode(testPin, OUTPUT);

  volatile uint32_t toggles = 0;
  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    digitalWrite(testPin, HIGH);
    digitalWrite(testPin, LOW);
    toggles += 2;
  }
  unsigned long gpioTime = endBenchmark();

  float gpioFreq = (toggles / 2.0) / (gpioTime / 1000000.0) / 1000;

  SERIAL_OUT.print(F_STR("  digitalWrite toggle rate: "));
  SERIAL_OUT.print(gpioFreq, 1);
  SERIAL_OUT.println(F_STR(" kHz"));

  // Direct register toggle
  startBenchmark();
  for (int i = 0; i < 100000; i++) {
    sio_hw->gpio_set = 1ul << testPin;
    sio_hw->gpio_clr = 1ul << testPin;
  }
  unsigned long regTime = endBenchmark();

  float regFreq = 100000.0 / (regTime / 1000000.0) / 1000000;

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("  Direct register toggle: "));
  SERIAL_OUT.print(regFreq, 2);
  SERIAL_OUT.println(F_STR(" MHz"));

  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("PIO speedup potential: "));
  SERIAL_OUT.print(maxPioFreq / 2 / (regFreq), 0);
  SERIAL_OUT.println(F_STR("x over direct register"));
#endif
  // regTime is for 100000 toggles; convert to cycles/ms
  CSV_ROW("PIO (cycles/ms)", 100000.0f / (regTime / 1000.0f));
}

// RP2040 Interpolator Benchmark
void benchmarkRP2040Interpolator() {
  printHeader("COMPUTE: HARDWARE INTERPOLATOR (RP2040)");

  SERIAL_OUT.println(F_STR("RP2040 has 2 hardware interpolators per core"));
  SERIAL_OUT.println(F_STR("Used for: affine texture mapping, lane operations"));
  SERIAL_OUT.println();

  // Configure interpolator 0 for simple linear interpolation
  interp_config cfg = interp_default_config();
  interp_config_set_blend(&cfg, true);
  interp_set_config(interp0, 0, &cfg);

  // Benchmark interpolator operations
  volatile uint32_t result = 0;

  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    // Set base and accumulator values
    interp0->base[0] = 0;
    interp0->base[1] = 1000;
    interp0->accum[0] = i % 256;  // Blend factor 0-255

    // Read interpolated result
    result += interp0->peek[0];
  }
  unsigned long interpTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Interpolator ops (10000): "));
  SERIAL_OUT.print(interpTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(10000.0 / interpTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("Interpolator (ops/ms)", 10000.0 / interpTime * 1000);

  // Compare with software interpolation
  volatile int32_t softResult = 0;
  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    int32_t a = 0;
    int32_t b = 1000;
    int32_t t = i % 256;
    softResult += a + ((b - a) * t) / 256;
  }
  unsigned long softTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Software lerp (10000): "));
  SERIAL_OUT.print(softTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(10000.0 / softTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("Interpolator Lerp (ops/ms)", 10000.0 / softTime * 1000);

  SERIAL_OUT.print(F_STR("Interpolator speedup: "));
  SERIAL_OUT.print((float)softTime / interpTime, 2);
  SERIAL_OUT.println(F_STR("x"));

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(result + softResult);
}

// RP2040 PWM Benchmark
void benchmarkRP2040PWM() {
  printHeader("I/O: PWM HARDWARE (RP2040)");

  const int pwmPin = 25;  // LED pin

  SERIAL_OUT.println(F_STR("RP2040 PWM Features:"));
  SERIAL_OUT.println(F_STR("  8 PWM slices, 2 channels each (16 outputs)"));
  SERIAL_OUT.println(F_STR("  16-bit counter, 16-bit wrap value"));

  uint32_t sysClk = clock_get_hz(clk_sys);
  SERIAL_OUT.print(F_STR("  System clock: "));
  SERIAL_OUT.print(sysClk / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));

  // Get slice for our pin
  uint slice_num = pwm_gpio_to_slice_num(pwmPin);
  uint channel = pwm_gpio_to_channel(pwmPin);

  SERIAL_OUT.print(F_STR("  Pin "));
  SERIAL_OUT.print(pwmPin);
  SERIAL_OUT.print(F_STR(" -> Slice "));
  SERIAL_OUT.print(slice_num);
  SERIAL_OUT.print(F_STR(", Channel "));
  SERIAL_OUT.println(channel == PWM_CHAN_A ? 'A' : 'B');

  // Configure for maximum frequency
  gpio_set_function(pwmPin, GPIO_FUNC_PWM);

  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 1.0f);  // No division
  pwm_config_set_wrap(&config, 255);     // 8-bit resolution
  pwm_init(slice_num, &config, true);

  float pwmFreq = (float)sysClk / 256;
  SERIAL_OUT.print(F_STR("  8-bit PWM frequency: "));
  SERIAL_OUT.print(pwmFreq / 1000, 1);
  SERIAL_OUT.println(F_STR(" kHz"));

  // Benchmark duty cycle updates
  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    pwm_set_gpio_level(pwmPin, i & 0xFF);
  }
  unsigned long updateTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Duty cycle updates (10000): "));
  SERIAL_OUT.print(updateTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(10000.0 / updateTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("RP2040 PWM Updates (ops/ms)", 10000.0 / updateTime * 1000);

  // Test different resolutions
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("PWM frequency vs resolution:"));

  uint16_t resolutions[] = { 256, 1024, 4096, 16384, 65535 };
  const char *resNames[] = { "8-bit", "10-bit", "12-bit", "14-bit", "16-bit" };

  for (int r = 0; r < 5; r++) {
    pwm_config_set_wrap(&config, resolutions[r] - 1);
    pwm_init(slice_num, &config, true);

    float freq = (float)sysClk / resolutions[r];
    SERIAL_OUT.print(F_STR("  "));
    SERIAL_OUT.print(resNames[r]);
    SERIAL_OUT.print(F_STR(": "));
    if (freq >= 1000000) {
      SERIAL_OUT.print(freq / 1000000, 2);
      SERIAL_OUT.println(F_STR(" MHz"));
    } else {
      SERIAL_OUT.print(freq / 1000, 1);
      SERIAL_OUT.println(F_STR(" kHz"));
    }
  }

  // Cleanup
  pwm_set_enabled(slice_num, false);
  gpio_set_function(pwmPin, GPIO_FUNC_SIO);
  pinMode(pwmPin, OUTPUT);
  digitalWrite(pwmPin, LOW);
}

// Pico W BLE Benchmark
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
void benchmarkPicoWBLE() {
  printHeader("WIRELESS: BLE (Pico W)");

  SERIAL_OUT.println(F_STR("Pico W has BLE via CYW43439 module"));
  SERIAL_OUT.println(F_STR("BLE requires BTstack library"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("To use BLE on Pico W:"));
  SERIAL_OUT.println(F_STR("  1. Include <BTstackLib.h>"));
  SERIAL_OUT.println(F_STR("  2. Call BTstack.setup()"));
  SERIAL_OUT.println(F_STR("  3. Use BTstack API for scanning/advertising"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Note: Full BLE benchmark requires BTstack integration"));
}
#endif

#endif  // ARDUINO_ARCH_RP2040
// ==================== UNO R4 WIFI ADDITIONAL BENCHMARKS ====================

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)

// Uno R4 DAC Benchmark
void benchmarkUnoR4DAC() {
  printHeader("I/O: DAC OUTPUT (Uno R4)");

  SERIAL_OUT.println(F_STR("Renesas RA4M1 has 1x 12-bit DAC on pin A0/DAC"));

  const int dacPin = DAC;

  // Set 12-bit resolution
  analogWriteResolution(12);

  // DAC write speed benchmark
  volatile uint32_t dacValue = 0;
  startBenchmark();
  for (int i = 0; i < 10000; i++) {
    analogWrite(dacPin, dacValue & 0xFFF);
    dacValue++;
  }
  unsigned long dacTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("analogWrite() 12-bit (10000 ops): "));
  SERIAL_OUT.print(dacTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(10000.0 / dacTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("Uno R4 DAC (ops/ms)", 10000.0 / dacTime * 1000);

  // DAC ramp test
  startBenchmark();
  for (int cycle = 0; cycle < 10; cycle++) {
    for (int val = 0; val < 4096; val += 16) {
      analogWrite(dacPin, val);
    }
  }
  unsigned long rampTime = endBenchmark();

  uint32_t rampOps = 10 * (4096 / 16);
#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("DAC ramp (10 cycles, "));
  SERIAL_OUT.print(rampOps);
  SERIAL_OUT.print(F_STR(" writes): "));
  SERIAL_OUT.print(rampTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print((float)rampOps / rampTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("Uno R4 DAC Ramp (ops/ms)", (float)rampOps / rampTime * 1000);

  // Calculate effective sample rate
  float sampleRate = rampOps / (rampTime / 1000000.0);
  SERIAL_OUT.print(F_STR("Effective sample rate: "));
  SERIAL_OUT.print(sampleRate / 1000, 1);
  SERIAL_OUT.println(F_STR(" kHz"));

  // Reset to 8-bit and clear
  analogWriteResolution(8);
  analogWrite(dacPin, 0);
}

// Uno R4 RTC Benchmark
void benchmarkUnoR4RTC() {
  printHeader("TIMING: INTERNAL RTC (Uno R4)");

  SERIAL_OUT.println(F_STR("Renesas RA4M1 has built-in RTC"));

  // Initialize RTC
  RTC.begin();

  // Set initial time
  RTCTime startTime(1, Month::JANUARY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
  RTC.setTime(startTime);

  // RTC read speed benchmark
  RTCTime currentTime;
  volatile uint32_t timeSum = 0;

  startBenchmark();
  for (int i = 0; i < 1000; i++) {
    RTC.getTime(currentTime);
    timeSum += currentTime.getSeconds();
  }
  unsigned long readTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("RTC.getTime() (1000 ops): "));
  SERIAL_OUT.print(readTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(1000.0 / readTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("RTC getTime (ops/ms)", 1000.0 / readTime * 1000);

  // RTC write speed benchmark
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    RTCTime newTime(1, Month::JANUARY, 2024, 12, 0, i % 60, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
    RTC.setTime(newTime);
  }
  unsigned long writeTime = endBenchmark();

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("RTC.setTime() (100 ops): "));
  SERIAL_OUT.print(writeTime);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(100.0 / writeTime * 1000);
  SERIAL_OUT.println(F_STR(" ops/ms)"));
#endif
  CSV_ROW("RTC setTime (ops/ms)", 100.0 / writeTime * 1000);

  // RTC tick accuracy test
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("RTC tick accuracy (1 second test):"));

  RTC.getTime(currentTime);
  int startSec = currentTime.getSeconds();
  unsigned long startMicros = micros();

  // Wait for second to change
  int currentSec = startSec;
  while (currentSec == startSec) {
    RTC.getTime(currentTime);
    currentSec = currentTime.getSeconds();
  }

  // Now wait for next second change
  startSec = currentSec;
  startMicros = micros();
  while (currentSec == startSec) {
    RTC.getTime(currentTime);
    currentSec = currentTime.getSeconds();
  }
  unsigned long elapsed = micros() - startMicros;

  float error = ((float)elapsed - 1000000) / 1000000 * 100;
  SERIAL_OUT.print(F_STR("  1 RTC second = "));
  SERIAL_OUT.print(elapsed);
  SERIAL_OUT.print(F_STR(" us ("));
  SERIAL_OUT.print(error, 3);
  SERIAL_OUT.println(F_STR("% error)"));

  SERIAL_OUT.print(F_STR("Checksum: "));
  SERIAL_OUT.println(timeSum);
}

// Uno R4 WiFi BLE Benchmark
#if defined(ARDUINO_UNOR4_WIFI)
void benchmarkUnoR4BLE() {
  printHeader("WIRELESS: BLE (Uno R4 WiFi)");

  SERIAL_OUT.println(F_STR("Uno R4 WiFi has BLE via ESP32-S3 module"));
  SERIAL_OUT.println(F_STR("BLE requires ArduinoBLE library"));
  SERIAL_OUT.println();

  SERIAL_OUT.println(F_STR("Running BLE discovery benchmark with ArduinoBLE..."));

  if (!BLE.begin()) {
    SERIAL_OUT.println(F_STR("BLE.begin() failed (check firmware/module state)"));
    return;
  }

  SERIAL_OUT.println(F_STR("Calling BLE.scan() for device discovery (5 sec)..."));
  unsigned long startMicros = micros();
  BLE.scan();

  int discovered = 0;
  unsigned long scanStartMs = millis();
  while (millis() - scanStartMs < 5000) {
    BLEDevice peripheral = BLE.available();
    if (!peripheral) {
      continue;
    }

    discovered++;
    SERIAL_OUT.print(F_STR("  Found #"));
    SERIAL_OUT.print(discovered);
    SERIAL_OUT.print(F_STR(": "));
    if (peripheral.localName().length()) {
      SERIAL_OUT.print(peripheral.localName());
    } else {
      SERIAL_OUT.print(F_STR("(unnamed)"));
    }
    SERIAL_OUT.print(F_STR(" ["));
    SERIAL_OUT.print(peripheral.address());
    SERIAL_OUT.println(F_STR("]"));
  }

  BLE.stopScan();
  BLE.end();

  unsigned long elapsedMicros = micros() - startMicros;
  SERIAL_OUT.print(F_STR("Discovered devices: "));
  SERIAL_OUT.println(discovered);
  SERIAL_OUT.print(F_STR("Scan elapsed: "));
  SERIAL_OUT.print(elapsedMicros / 1000.0f, 2);
  SERIAL_OUT.println(F_STR(" ms"));
}
#endif

#endif  // ARDUINO_UNOR4_WIFI || ARDUINO_UNOR4_MINIMA
// ==================== I2C WIRE COMMUNICATION ====================

#if !defined(ARDUINO_UNO_Q)
static unsigned long runI2CWriteBenchmark(const __FlashStringHelper* label, uint32_t clockHz, int iterations, int progressStep) {
  uint16_t statusCounts[6] = {0, 0, 0, 0, 0, 0};

  Wire.setClock(clockHz);
  startBenchmark();

  int completed = 0;
  for (int i = 0; i < iterations; i++) {
    if (progressStep > 0 && i > 0 && (i % progressStep) == 0) {
      SERIAL_OUT.print(F_STR("."));
    }
    Wire.beginTransmission(0x08);
    Wire.write(i & 0xFF);
    uint8_t status = Wire.endTransmission();

    if (status < 6) {
      statusCounts[status]++;
    } else {
      statusCounts[4]++;
    }

    completed++;

    // Abort early if the bus repeatedly reports hard failures/timeouts.
    if ((statusCounts[4] + statusCounts[5]) >= 10) {
      break;
    }
  }

  unsigned long elapsed = endBenchmark();

  if (progressStep > 0) {
    SERIAL_OUT.println();
  }

  SERIAL_OUT.print(label);
  SERIAL_OUT.print(F_STR(": "));
  SERIAL_OUT.print(elapsed);
  SERIAL_OUT.print(F_STR(" us for "));
  SERIAL_OUT.print(completed);
  SERIAL_OUT.print(F_STR(" writes ("));
  SERIAL_OUT.print((completed > 0 && elapsed > 0) ? (completed * 1000.0f / elapsed) : 0.0f);
  SERIAL_OUT.println(F_STR(" txn/ms)"));

  SERIAL_OUT.print(F_STR("  Status codes: ok="));
  SERIAL_OUT.print(statusCounts[0]);
  SERIAL_OUT.print(F_STR(", addrNACK="));
  SERIAL_OUT.print(statusCounts[2]);
  SERIAL_OUT.print(F_STR(", dataNACK="));
  SERIAL_OUT.print(statusCounts[3]);
  SERIAL_OUT.print(F_STR(", other="));
  SERIAL_OUT.print(statusCounts[4]);
  SERIAL_OUT.print(F_STR(", timeout="));
  SERIAL_OUT.println(statusCounts[5]);

  if (completed < iterations) {
    SERIAL_OUT.println(F_STR("  Early stop: repeated bus errors/timeouts"));
  }
  return elapsed;
}

void benchmarkWireI2C() {
  printHeader("I/O: WIRE I2C COMMUNICATION");

  // Number of I2C buses
  SERIAL_OUT.print(F_STR("I2C buses: "));
#if defined(WIRE_INTERFACES_COUNT)
  SERIAL_OUT.println(WIRE_INTERFACES_COUNT);
#elif defined(ESP32)
  SERIAL_OUT.println(2);
#elif defined(ARDUINO_ARCH_RP2040)
  SERIAL_OUT.println(2);
#elif defined(ARDUINO_SAM_DUE)
  SERIAL_OUT.println(2);
#elif defined(BOARD_STM32H7)
  SERIAL_OUT.println(3);
#elif defined(BOARD_NRF52)
  SERIAL_OUT.println(2);
#else
  SERIAL_OUT.println(1);
#endif

  // Internal buffer size
  SERIAL_OUT.print(F_STR("Buffer size: "));
#if defined(BUFFER_LENGTH)
  SERIAL_OUT.print(BUFFER_LENGTH);
  SERIAL_OUT.println(F_STR(" bytes"));
#elif defined(I2C_BUFFER_LENGTH)
  SERIAL_OUT.print(I2C_BUFFER_LENGTH);
  SERIAL_OUT.println(F_STR(" bytes"));
#elif defined(WIRE_BUFFER_LENGTH)
  SERIAL_OUT.print(WIRE_BUFFER_LENGTH);
  SERIAL_OUT.println(F_STR(" bytes"));
#else
  SERIAL_OUT.println(F_STR("unknown"));
#endif

  Wire.begin();

#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000, true);  // 3ms per transaction to avoid long stalls on stuck bus
  SERIAL_OUT.println(F_STR("I2C timeout guard: enabled (3 ms)"));
#endif

  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Testing I2C transaction speed"));
  SERIAL_OUT.println(F_STR("(No device - measuring bus overhead)"));
  SERIAL_OUT.println();

  const int i2cIterations = 1000;
  const int i2cProgressStep = 0;

  unsigned long i2c100Time = runI2CWriteBenchmark(F_STR("100 kHz"), 100000, i2cIterations, i2cProgressStep);
  unsigned long i2c400Time = runI2CWriteBenchmark(F_STR("400 kHz"), 400000, i2cIterations, i2cProgressStep);
  CSV_ROW("I2C 100kHz (us)", i2c100Time);
  CSV_ROW("I2C 400kHz (us)", i2c400Time);

#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(BOARD_STM32H7)
  runI2CWriteBenchmark(F_STR("1 MHz"), 1000000, i2cIterations, i2cProgressStep);
#endif

#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)
  Wire.end();
#endif
}
#endif
// Watchdog Timer Benchmark
void benchmarkWatchdog() {
  printHeader("SYSTEM: WATCHDOG TIMER");

#if defined(ESP32)
  SERIAL_OUT.println(F_STR("ESP32 Watchdog Information:"));
  SERIAL_OUT.println(F_STR("  Task WDT: Monitors FreeRTOS tasks"));
  SERIAL_OUT.println(F_STR("  Interrupt WDT: Monitors interrupt handling"));
  SERIAL_OUT.println(F_STR("  RTC WDT: Low-power watchdog"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Watchdog is managed by ESP-IDF/FreeRTOS"));
  SERIAL_OUT.println(F_STR("Default timeout: ~5 seconds"));

#elif defined(ARDUINO_ARCH_RP2040)
  SERIAL_OUT.println(F_STR("RP2040 Watchdog Features:"));
  SERIAL_OUT.println(F_STR("  Max timeout: ~8.3 seconds"));
  SERIAL_OUT.println(F_STR("  Resolution: 1 microsecond"));
  SERIAL_OUT.println();

  // Test watchdog setup time (but don't enable it)
  startBenchmark();
  for (int i = 0; i < 1000; i++) {
    // Just measure time to check if WDT caused reset
    bool wasWdt = watchdog_caused_reboot();
    (void)wasWdt;
  }
  unsigned long wdtCheckTime = endBenchmark();

  SERIAL_OUT.print(F_STR("watchdog_caused_reboot() (1000 calls): "));
  SERIAL_OUT.print(wdtCheckTime);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.print(F_STR("Last reset was WDT: "));
  SERIAL_OUT.println(watchdog_caused_reboot() ? F_STR("YES") : F_STR("NO"));

#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  SERIAL_OUT.println(F_STR("Renesas RA4M1 Watchdog:"));
  SERIAL_OUT.println(F_STR("  Independent WDT (IWDT)"));
  SERIAL_OUT.println(F_STR("  Configurable timeout periods"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Use WDT library for configuration"));

#elif defined(__AVR__)
  SERIAL_OUT.println(F_STR("AVR WDT: 16ms-8s, reset or IRQ"));

  // Check reset cause (register names differ across AVR families)
  #if defined(MCUSR) && defined(WDRF) && defined(BORF) && defined(EXTRF) && defined(PORF)
    // Classic AVR (ATmega328P, ATmega32U4, etc.)
    uint8_t mcusr = MCUSR;
    SERIAL_OUT.print(F_STR("Reset cause: "));
    if (mcusr & (1 << WDRF)) SERIAL_OUT.print(F_STR("WDT "));
    if (mcusr & (1 << BORF)) SERIAL_OUT.print(F_STR("Brown-out "));
    if (mcusr & (1 << EXTRF)) SERIAL_OUT.print(F_STR("External "));
    if (mcusr & (1 << PORF)) SERIAL_OUT.print(F_STR("Power-on "));
    SERIAL_OUT.println();

  #elif defined(RSTCTRL) && defined(RSTCTRL_WDRF_bm)
    // megaAVR-0/1 and newer AVR families (ATmega4809, AVR128DA28, etc.)
    uint8_t rstfr = RSTCTRL.RSTFR;
    SERIAL_OUT.print(F_STR("Reset cause: "));
    if (rstfr & RSTCTRL_WDRF_bm) SERIAL_OUT.print(F_STR("WDT "));
    #if defined(RSTCTRL_BORF_bm)
      if (rstfr & RSTCTRL_BORF_bm) SERIAL_OUT.print(F_STR("Brown-out "));
    #elif defined(RSTCTRL_BODRF_bm)
      if (rstfr & RSTCTRL_BODRF_bm) SERIAL_OUT.print(F_STR("Brown-out "));
    #endif
    #if defined(RSTCTRL_EXTRF_bm)
      if (rstfr & RSTCTRL_EXTRF_bm) SERIAL_OUT.print(F_STR("External "));
    #endif
    #if defined(RSTCTRL_PORF_bm)
      if (rstfr & RSTCTRL_PORF_bm) SERIAL_OUT.print(F_STR("Power-on "));
    #endif
    SERIAL_OUT.println();

  #elif defined(RSTFR)
    // Fallback for AVR parts exposing a standalone RSTFR register
    uint8_t rstfr = RSTFR;
    SERIAL_OUT.print(F_STR("Reset cause: "));
    if (rstfr & 0x01) SERIAL_OUT.print(F_STR("Power-on "));
    if (rstfr & 0x02) SERIAL_OUT.print(F_STR("Brown-out "));
    if (rstfr & 0x04) SERIAL_OUT.print(F_STR("External "));
    if (rstfr & 0x08) SERIAL_OUT.print(F_STR("WDT "));
    SERIAL_OUT.println();

  #else
    SERIAL_OUT.println(F_STR("Reset cause: not available for this AVR variant"));
  #endif

#elif defined(ARDUINO_UNO_Q)
  // STM32U585 has IWDG (independent) and WWDG (window) watchdogs
  SERIAL_OUT.println(F_STR("STM32U585 Watchdog (Zephyr):"));
  SERIAL_OUT.println(F_STR("  IWDG: Independent watchdog (LSI-clocked)"));
  SERIAL_OUT.println(F_STR("  WWDG: Window watchdog (APB-clocked)"));
  SERIAL_OUT.println();

#if defined(CONFIG_WATCHDOG)
  const struct device *wdtDev = DEVICE_DT_GET(DT_INST(0, st_stm32_watchdog));
  if (device_is_ready(wdtDev)) {
    SERIAL_OUT.println(F_STR("IWDG device ready"));

    // Configure a 2-second window, install + feed + disable
    struct wdt_timeout_cfg cfg = {};
    cfg.window.min = 0;
    cfg.window.max = 2000;          // 2 s timeout
    cfg.callback = NULL;            // reset on timeout (no ISR for IWDG)

    int chanId = wdt_install_timeout(wdtDev, &cfg);
    if (chanId >= 0) {
      SERIAL_OUT.print(F_STR("  Timeout installed (channel "));
      SERIAL_OUT.print(chanId);
      SERIAL_OUT.println(')');

      // Benchmark wdt_feed() jitter: 100 feeds, report avg + max
      int err = wdt_setup(wdtDev, 0);
      if (err == 0) {
        long feedMin = LONG_MAX;
        long feedMax = 0;
        long feedSum = 0;
        const int feeds = 100;
        for (int i = 0; i < feeds; i++) {
          unsigned long t1 = micros();
          wdt_feed(wdtDev, chanId);
          unsigned long t2 = micros();
          long dt = (long)(t2 - t1);
          feedSum += dt;
          if (dt < feedMin) feedMin = dt;
          if (dt > feedMax) feedMax = dt;
        }
        SERIAL_OUT.print(F_STR("  wdt_feed() avg: "));
        SERIAL_OUT.print(feedSum / feeds);
        SERIAL_OUT.println(F_STR(" us"));
        SERIAL_OUT.print(F_STR("  wdt_feed() max: "));
        SERIAL_OUT.print(feedMax);
        SERIAL_OUT.println(F_STR(" us"));

        // Disable WDT so we don't reset during remaining benchmarks
        wdt_disable(wdtDev);
      } else {
        SERIAL_OUT.print(F_STR("  wdt_setup failed: "));
        SERIAL_OUT.println(err);
      }
    } else {
      SERIAL_OUT.print(F_STR("  wdt_install_timeout failed: "));
      SERIAL_OUT.println(chanId);
    }
  } else {
    SERIAL_OUT.println(F_STR("IWDG device not ready"));
  }
#else
  SERIAL_OUT.println(F_STR("IWDG driver not enabled in Kconfig"));
#endif

#else
  SERIAL_OUT.println(F_STR("Watchdog information not available for this board"));
#endif
  CSV_BLANK("WDT Reset Latency (ms)");
}

// Sleep Mode Benchmark
void benchmarkSleepModes() {
  printHeader("POWER: SLEEP MODE TIMING");

  // ----------------------------------------------------------------
  // Helper lambdas would be nice but we keep plain C for AVR compat.
  // Strategy: enter the lightest sleep that preserves micros() tick
  // (idle / WFI), measure wake-up time, then repeat for a batch.
  // Deep-sleep / power-down modes reset the MCU so we skip those.
  // ----------------------------------------------------------------

#if defined(ESP32)
  // Already covered in benchmarkESP32Sleep()
  SERIAL_OUT.println(F_STR("See ESP32 Light Sleep benchmark above"));

#elif defined(ESP8266)
  // ESP8266 light-sleep (modem sleep keeps CPU, light sleep halts it)
  // Timer1 generates the wake interrupt that keeps millis()/micros() alive.
  SERIAL_OUT.println(F_STR("ESP8266 Light Sleep (timer wake):"));
  SERIAL_OUT.println();

  // Single wake latency
  unsigned long esp8266Start = micros();
  delay(1);  // enters idle/modem-sleep automatically via yield()
  unsigned long esp8266Wake = micros() - esp8266Start;

  SERIAL_OUT.print(F_STR("  Idle-yield wake: "));
  SERIAL_OUT.print(esp8266Wake);
  SERIAL_OUT.println(F_STR(" us (target 1000 us)"));

  // Batch WFI via inline asm – available on Xtensa LX106
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    asm volatile("waiti 0");   // wait-for-interrupt, level 0
  }
  unsigned long esp8266WfiTime = endBenchmark();

  SERIAL_OUT.print(F_STR("  100x waiti 0: "));
  SERIAL_OUT.print(esp8266WfiTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(ARDUINO_ARCH_RP2040)
  SERIAL_OUT.println(F_STR("RP2040 Sleep (WFI / WFE):"));
  SERIAL_OUT.println();

  // --- WFI (Wait For Interrupt) - wakes on SysTick every 1 ms ---
  unsigned long wfiSingle = 0;
  {
    unsigned long t = micros();
    __wfi();
    wfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(wfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __wfi();
  }
  unsigned long wfiTime100 = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(wfiTime100);
  SERIAL_OUT.println(F_STR(" us"));

  // --- WFE (Wait For Event) - lighter, wakes on any pending event ---
  unsigned long wfeSingle = 0;
  {
    __sev();  // set event flag so first WFE clears it, second actually sleeps
    __wfe();  // clear event flag
    unsigned long t = micros();
    __wfe();  // real sleep until next event/interrupt
    wfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFE single wake: "));
  SERIAL_OUT.print(wfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __sev();
    __wfe();
    __wfe();
  }
  unsigned long wfeTime100 = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFE: "));
  SERIAL_OUT.print(wfeTime100);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  // Renesas RA4M1 – WFI is available via CMSIS on this Arm core.
  // The SysTick keeps running so micros() survives.
  SERIAL_OUT.println(F_STR("Renesas RA4M1 Sleep (WFI / WFE):"));
  SERIAL_OUT.println();

  // Single WFI (SysTick wakes us ~every 1 ms)
  unsigned long ra4m1WfiSingle;
  {
    unsigned long t = micros();
    __WFI();
    ra4m1WfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(ra4m1WfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __WFI();
  }
  unsigned long ra4m1WfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(ra4m1WfiTime);
  SERIAL_OUT.println(F_STR(" us"));

  // WFE
  unsigned long ra4m1WfeSingle;
  {
    __SEV();
    __WFE();
    unsigned long t = micros();
    __WFE();
    ra4m1WfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFE single wake: "));
  SERIAL_OUT.print(ra4m1WfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __SEV();
    __WFE();
    __WFE();
  }
  unsigned long ra4m1WfeTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFE: "));
  SERIAL_OUT.print(ra4m1WfeTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(BOARD_SAMD)
  // SAMD21/51 – Cortex-M0+/M4: IDLE modes keep SysTick running.
  // PM->SLEEP.reg selects idle depth (IDLE0/1/2 on SAMD21, IDLE on SAMD51).
  SERIAL_OUT.println(F_STR("SAMD Sleep (IDLE + WFI/WFE):"));
  SERIAL_OUT.println();

  // --- Idle 0 (CPU halted, AHB/APB clocks run) ---
#if defined(__SAMD51__)
  PM->SLEEPCFG.reg = PM_SLEEPCFG_SLEEPMODE_IDLE;
  while (PM->SLEEPCFG.reg != PM_SLEEPCFG_SLEEPMODE_IDLE) {}  // sync
#else
  // SAMD21: Use SCB directly, PM->SLEEP.reg for idle depth
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;  // select idle (not standby)
  PM->SLEEP.reg = PM_SLEEP_IDLE_CPU;    // IDLE 0: halt CPU only
#endif

  unsigned long samdIdleSingle;
  {
    unsigned long t = micros();
    __WFI();
    samdIdleSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  IDLE WFI single: "));
  SERIAL_OUT.print(samdIdleSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __WFI();
  }
  unsigned long samdWfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x IDLE WFI: "));
  SERIAL_OUT.print(samdWfiTime);
  SERIAL_OUT.println(F_STR(" us"));

  // WFE
  unsigned long samdWfeSingle;
  {
    __SEV();
    __WFE();
    unsigned long t = micros();
    __WFE();
    samdWfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  IDLE WFE single: "));
  SERIAL_OUT.print(samdWfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __SEV();
    __WFE();
    __WFE();
  }
  unsigned long samdWfeTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x IDLE WFE: "));
  SERIAL_OUT.print(samdWfeTime);
  SERIAL_OUT.println(F_STR(" us"));

#if !defined(__SAMD51__)
  // SAMD21 deeper idle: IDLE 2 (CPU + AHB halted, APB still runs)
  PM->SLEEP.reg = PM_SLEEP_IDLE_APB;  // IDLE 2
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __WFI();
  }
  unsigned long samdIdle2Time = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x IDLE2 WFI: "));
  SERIAL_OUT.print(samdIdle2Time);
  SERIAL_OUT.println(F_STR(" us"));

  // Restore IDLE 0
  PM->SLEEP.reg = PM_SLEEP_IDLE_CPU;
#endif

#elif defined(BOARD_NRF52)
  // nRF52840 – Cortex-M4F: System ON idle mode (WFI).
  // SysTick keeps running; "System ON" is the only mode that preserves it.
  // "System OFF" is a deep-sleep reset – we skip that.
  SERIAL_OUT.println(F_STR("nRF52 Sleep (System ON, WFI/WFE):"));
  SERIAL_OUT.println();

  unsigned long nrfWfiSingle;
  {
    unsigned long t = micros();
    __WFI();
    nrfWfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(nrfWfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __WFI();
  }
  unsigned long nrfWfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(nrfWfiTime);
  SERIAL_OUT.println(F_STR(" us"));

  // WFE
  unsigned long nrfWfeSingle;
  {
    __SEV();
    __WFE();
    unsigned long t = micros();
    __WFE();
    nrfWfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFE single wake: "));
  SERIAL_OUT.print(nrfWfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __SEV();
    __WFE();
    __WFE();
  }
  unsigned long nrfWfeTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFE: "));
  SERIAL_OUT.print(nrfWfeTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(BOARD_STM32H7)
  // STM32H7 (Portenta H7, Giga R1) – Cortex-M7
  // D1 domain CSTOP = idle-sleep; deeper modes reset or need complex setup.
  // HAL_PWR_EnterSLEEPMode keeps SysTick alive.
  SERIAL_OUT.println(F_STR("STM32H7 Sleep (WFI/WFE):"));
  SERIAL_OUT.println();

  unsigned long stm32WfiSingle;
  {
    unsigned long t = micros();
    __WFI();
    stm32WfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(stm32WfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __WFI();
  }
  unsigned long stm32WfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(stm32WfiTime);
  SERIAL_OUT.println(F_STR(" us"));

  unsigned long stm32WfeSingle;
  {
    __SEV();
    __WFE();
    unsigned long t = micros();
    __WFE();
    stm32WfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFE single wake: "));
  SERIAL_OUT.print(stm32WfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    __SEV();
    __WFE();
    __WFE();
  }
  unsigned long stm32WfeTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFE: "));
  SERIAL_OUT.print(stm32WfeTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(TEENSYDUINO) && defined(__IMXRT1062__)
  // Teensy 4.0/4.1 – Cortex-M7 (iMXRT1062)
  // WFI enters "wait mode"; SysTick keeps ticking.
  SERIAL_OUT.println(F_STR("Teensy 4.x Sleep (WFI/WFE):"));
  SERIAL_OUT.println();

  SERIAL_OUT.flush();  // drain USB CDC output before entering WFI
  unsigned long teensyWfiSingle;
  {
    unsigned long t = micros();
    asm volatile("dsb" ::: "memory");  // ARM WFI preamble: complete pending stores
    asm volatile("wfi");
    teensyWfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(teensyWfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  SERIAL_OUT.flush();  // drain USB CDC before tight WFI loop
  startBenchmark();
  for (int i = 0; i < 100; i++) {
    asm volatile("dsb" ::: "memory");  // ARM WFI preamble; allows pending IRQs to run
    asm volatile("wfi");
  }
  unsigned long teensyWfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(teensyWfiTime);
  SERIAL_OUT.println(F_STR(" us"));

  // WFE on Cortex-M7 only wakes on SEV or a pending interrupt when SEVONPEND
  // (SCR bit 4) is set.  Without it the second wfe blocks forever because
  // there is no second core to issue SEV and the iMXRT1062 defaults SEVONPEND=0.
  // Teensyduino defines the System Control Register as SCB_SCR and the bit
  // as SCB_SCR_SEVONPEND.  Save SCR, enable SEVONPEND, run WFE tests, restore.
  uint32_t savedScr = SCB_SCR;
  SCB_SCR |= SCB_SCR_SEVONPEND;

  unsigned long teensyWfeSingle;
  {
    asm volatile("sev");   // prime the event latch
    asm volatile("wfe");   // consume the latch (returns immediately)
    unsigned long t = micros();
    asm volatile("wfe");   // sleep until the next pending interrupt sets the event
    teensyWfeSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFE single wake: "));
  SERIAL_OUT.print(teensyWfeSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    asm volatile("sev");
    asm volatile("wfe");
    asm volatile("wfe");
  }
  unsigned long teensyWfeTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFE: "));
  SERIAL_OUT.print(teensyWfeTime);
  SERIAL_OUT.println(F_STR(" us"));

  SCB_SCR = savedScr;  // restore original SCR (clears SEVONPEND if it was 0)

#elif defined(TEENSYDUINO)
  // Teensy 3.x (Cortex-M4) – same WFI approach
  SERIAL_OUT.println(F_STR("Teensy 3.x Sleep (WFI):"));
  SERIAL_OUT.println();

  unsigned long t3WfiSingle;
  {
    unsigned long t = micros();
    asm volatile("wfi");
    t3WfiSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  WFI single wake: "));
  SERIAL_OUT.print(t3WfiSingle);
  SERIAL_OUT.println(F_STR(" us"));

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    asm volatile("wfi");
  }
  unsigned long t3WfiTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x WFI: "));
  SERIAL_OUT.print(t3WfiTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(__AVR__)
  // AVR – IDLE keeps Timer0 (millis/micros tick) alive.
  // Power-down/save/standby stop Timer0 → we can't measure those with micros().
  // ADC Noise Reduction (SLEEP_MODE_ADC) does NOT wake from Timer0 overflow
  // or USART TX on ATmega328P, so it would hang; we skip it.
  SERIAL_OUT.println(F_STR("AVR Sleep Modes (timer-based wake):"));
  SERIAL_OUT.flush();  // drain TX buffer before entering any sleep mode

  // --- SLEEP_MODE_IDLE ---
  set_sleep_mode(SLEEP_MODE_IDLE);
  unsigned long avrIdleSingle;
  {
    unsigned long t = micros();
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    avrIdleSingle = micros() - t;
  }
  SERIAL_OUT.print(F_STR("  IDLE single wake: "));
  SERIAL_OUT.print(avrIdleSingle);
  SERIAL_OUT.println(F_STR(" us"));
  SERIAL_OUT.flush();

  startBenchmark();
  for (int i = 0; i < 100; i++) {
    sleep_enable();
    sleep_cpu();
    sleep_disable();
  }
  unsigned long avrIdleTime = endBenchmark();
  SERIAL_OUT.print(F_STR("  100x IDLE: "));
  SERIAL_OUT.print(avrIdleTime);
  SERIAL_OUT.println(F_STR(" us"));

#elif defined(BOARD_ARC32)
  // Intel Curie (ARC EM) – sleep modes managed by the Curie firmware; no sketch-level API.
  SERIAL_OUT.println(F_STR("Intel Curie: sleep managed by firmware (no sketch sleep API)"));
#else
  SERIAL_OUT.println(F_STR("Sleep mode test not available for this board"));
#endif
}
// ==================== SYSTEM INFO ====================

void printSystemInfo() {
  printHeader("SYSTEM INFORMATION");

#if !CSV_OUTPUT
  SERIAL_OUT.print(F_STR("Board: "));
  SERIAL_OUT.println(BOARD_NAME);
#endif
  CSV_ROW("Board", F_STR(BOARD_NAME));

#if defined(ESP32)
  SERIAL_OUT.print(F_STR("Chip Model: "));
  SERIAL_OUT.println(ESP.getChipModel());
  SERIAL_OUT.print(F_STR("Chip Revision: "));
  SERIAL_OUT.println(ESP.getChipRevision());
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(ESP.getCpuFreqMHz());
  SERIAL_OUT.println(F_STR(" MHz"));
  SERIAL_OUT.print(F_STR("Cores: "));
  SERIAL_OUT.println(ESP.getChipCores());
  SERIAL_OUT.print(F_STR("SDK Version: "));
  SERIAL_OUT.println(ESP.getSdkVersion());
#elif defined(ESP8266)
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(ESP.getCpuFreqMHz());
  SERIAL_OUT.println(F_STR(" MHz"));
  SERIAL_OUT.print(F_STR("Chip ID: "));
  SERIAL_OUT.println(ESP.getChipId());
  SERIAL_OUT.print(F_STR("SDK Version: "));
  SERIAL_OUT.println(ESP.getSdkVersion());
#elif defined(ARDUINO_ARCH_RP2040)
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#if defined(ARDUINO_NANO_RP2040_CONNECT)
  SERIAL_OUT.println(F_STR("Features: WiFi (Nina W102), BLE, IMU (LSM6DSOX), Mic"));
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  SERIAL_OUT.println(F_STR("Features: WiFi (CYW43439)"));
#endif
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  SERIAL_OUT.print(F_STR("MCU: Renesas RA4M1 (ARM Cortex-M4)"));
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#ifdef ARDUINO_UNOR4_WIFI
  SERIAL_OUT.println(F_STR("Features: WiFi (ESP32-S3), 12x8 LED Matrix"));
#else
  SERIAL_OUT.println(F_STR("Features: 12x8 LED Matrix"));
#endif
#elif defined(BOARD_SAMD)
  SERIAL_OUT.print(F_STR("MCU: SAMD (ARM Cortex-M0+)"));
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#if defined(ARDUINO_SAMD_NANO_33_IOT)
  SERIAL_OUT.println(F_STR("Features: WiFi, BLE, IMU (LSM6DS3)"));
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
  SERIAL_OUT.println(F_STR("Features: WiFi (Nina W102), Crypto (ECC508)"));
#endif
#elif defined(BOARD_NRF52)
  SERIAL_OUT.print(F_STR("MCU: nRF52840 (ARM Cortex-M4F)"));
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("CPU Frequency: 64 MHz"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Features: BLE 5.0, IMU (LSM9DS1)"));
#elif defined(BOARD_STM32H7)
  SERIAL_OUT.print(F_STR("MCU: STM32H7 (ARM Cortex-M7)"));
  SERIAL_OUT.println();
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(SystemCoreClock / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#ifdef HAS_DUAL_CORE
  SERIAL_OUT.println(F_STR("Cores: Dual Core (M7 + M4)"));
#endif
#elif defined(BOARD_TEENSY)
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#if defined(__IMXRT1062__)
  SERIAL_OUT.println(F_STR("MCU: i.MX RT1062 (ARM Cortex-M7)"));
#endif
#elif defined(BOARD_STM32U5)
  SERIAL_OUT.println(F_STR("MCU: STM32U585 (ARM Cortex-M33)"));
  SERIAL_OUT.print(F_STR("MCU Frequency: "));

  // Try to get actual clock frequency
#if defined(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
  SERIAL_OUT.print(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000);
#elif defined(F_CPU)
  SERIAL_OUT.print(F_CPU / 1000000);
#else
  SERIAL_OUT.print(F_STR("160"));  // STM32U585 default max frequency
#endif

  SERIAL_OUT.println(F_STR(" MHz"));
  SERIAL_OUT.println(F_STR("MCU Features: FPU, DSP, TrustZone"));
  SERIAL_OUT.println(F_STR("MCU RAM: 786 KB SRAM"));
  SERIAL_OUT.println(F_STR("MCU Flash: 2 MB"));
#ifdef USING_ZEPHYR
  SERIAL_OUT.println(F_STR("MCU RTOS: Zephyr"));
#endif
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Linux MPU: Qualcomm QRB2210"));
  SERIAL_OUT.println(F_STR("MPU: Quad Cortex-A53 @ up to 2.0 GHz"));
  SERIAL_OUT.println(F_STR("MPU RAM: 2-4 GB"));
  SERIAL_OUT.println(F_STR("MPU Storage: 16 GB eMMC"));
  SERIAL_OUT.println(F_STR("MPU OS: Debian-based Linux"));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("Communication: Arduino Bridge RPC (MCU<->Linux)"));
#ifdef HAS_ROUTER_BRIDGE
  SERIAL_OUT.println(F_STR("RPC Library: Arduino_RouterBridge (MessagePack)"));
#elif defined(HAS_RPCLITE)
  SERIAL_OUT.println(F_STR("RPC Library: Arduino_RPClite (MessagePack)"));
#else
  SERIAL_OUT.println(F_STR("RPC Library: Not detected"));
#endif
#elif defined(BOARD_ARC32)
  SERIAL_OUT.println(F_STR("MCU: Intel Curie (ARC EM4 32-bit)"));
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
  SERIAL_OUT.println(F_STR("FPU: None (software float emulation)"));
  SERIAL_OUT.println(F_STR("BLE: Nordic nRF51822 co-processor (CurieBLE)"));
  SERIAL_OUT.println(F_STR("Features: BLE 4.0, IMU (BMI160), EEPROM emulation"));
#else
  SERIAL_OUT.print(F_STR("CPU Frequency: "));
#if defined(F_CPU)
  SERIAL_OUT.print(F_CPU / 1000000);
  SERIAL_OUT.println(F_STR(" MHz"));
#else
  SERIAL_OUT.println(F_STR("Unknown"));
#endif
#endif

#if defined(ARDUINO_AVR_MULTIDUINO)
  SERIAL_OUT.println(F_STR("Features: RTC (DS1307)"));
#endif

#if defined(ARDUINO_AVR_YUN)
  SERIAL_OUT.println(F_STR("Linux: AR9331 400MHz, 64MB RAM, 16MB Flash"));
  SERIAL_OUT.println(F_STR("WiFi + USB Host (OpenWrt)"));
  SERIAL_OUT.println(F_STR("Bridge: Serial1 MCU<->Linux link (killer test enabled)"));
#endif

  // RAM Info
  SERIAL_OUT.print(F_STR("Free RAM: "));
#if defined(ESP32)
  SERIAL_OUT.print(ESP.getFreeHeap() / 1024);
  SERIAL_OUT.println(F_STR(" KB"));
  SERIAL_OUT.print(F_STR("Total Heap: "));
  SERIAL_OUT.print(ESP.getHeapSize() / 1024);
  SERIAL_OUT.println(F_STR(" KB"));
  SERIAL_OUT.print(F_STR("Min Free Heap: "));
  SERIAL_OUT.print(ESP.getMinFreeHeap() / 1024);
  SERIAL_OUT.println(F_STR(" KB"));
#elif defined(ESP8266)
  SERIAL_OUT.print(ESP.getFreeHeap() / 1024);
  SERIAL_OUT.println(F_STR(" KB"));
#elif defined(__AVR__)
  extern int __heap_start, *__brkval;
  int v;
  int freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
  SERIAL_OUT.print(freeRam);
  SERIAL_OUT.println(F_STR(" bytes"));
// Show total RAM for common AVR boards
#if defined(__AVR_ATmega328P__)
  SERIAL_OUT.println(F_STR("Total RAM: 2 KB"));
#elif defined(__AVR_ATmega2560__)
  SERIAL_OUT.println(F_STR("Total RAM: 8 KB"));
#elif defined(__AVR_ATmega32U4__)
  SERIAL_OUT.println(F_STR("Total RAM: 2.5 KB"));
#endif
#elif defined(ARDUINO_SAM_DUE)
  extern char _end;
  extern "C" char *sbrk(int i);
  char *ramstart = (char *)0x20070000;
  char *ramend = (char *)0x20088000;
  int freeRam = ramend - sbrk(0);
  SERIAL_OUT.print(freeRam / 1024);
  SERIAL_OUT.println(F_STR(" KB"));
  SERIAL_OUT.println(F_STR("Total RAM: 96 KB"));
#elif defined(ARDUINO_ARCH_RP2040)
  // RP2040 has 264KB RAM
  SERIAL_OUT.println(F_STR("~264 KB (RP2040)"));
#elif defined(BOARD_NRF52)
  SERIAL_OUT.println(F_STR("~256 KB (nRF52840)"));
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  SERIAL_OUT.println(F_STR("32 KB (RA4M1)"));
#elif defined(BOARD_ARC32)
  SERIAL_OUT.println(F_STR("24 KB (ARC sketch SRAM)"));
  SERIAL_OUT.println(F_STR("Total RAM: 80 KB (24 KB ARC + 8 KB ARC DRAM + 48 KB shared)"));
#elif defined(TEENSYDUINO) && defined(__IMXRT1062__)
  // Teensy 4.0/4.1: measure free RAM as stack-to-heap gap
  {
    extern char *__brkval;
    char stackTop;
    uint32_t freeBytes = (uint32_t)&stackTop - (uint32_t)__brkval;
    SERIAL_OUT.print(freeBytes / 1024);
    SERIAL_OUT.println(F_STR(" KB"));
  }
  SERIAL_OUT.println(F_STR("Total RAM: 1024 KB (512 KB DTCM + 512 KB ITCM)"));
#elif defined(BOARD_SRAM_KB)
  SERIAL_OUT.print(F_STR("Total RAM: "));
  SERIAL_OUT.print(BOARD_SRAM_KB);
  SERIAL_OUT.println(F_STR(" KB"));
#else
  SERIAL_OUT.println(F_STR("Unknown"));
#endif

  SERIAL_OUT.print(F_STR("Compile Date: "));
  SERIAL_OUT.print(__DATE__);
  SERIAL_OUT.print(F_STR(" "));
  SERIAL_OUT.println(__TIME__);

  SERIAL_OUT.print(F_STR("Benchmark Version: "));
  SERIAL_OUT.println(F_STR(BENCHMARK_VERSION));
  CSV_ROW("Benchmark Version", F_STR(BENCHMARK_VERSION));

  // CSV rows for system info (CPU freq + free RAM)
#if defined(ESP32)
  CSV_ROW("CPU (MHz)", ESP.getCpuFreqMHz());
  CSV_ROW("Free RAM (bytes)", ESP.getFreeHeap());
#elif defined(ESP8266)
  CSV_ROW("CPU (MHz)", ESP.getCpuFreqMHz());
  CSV_ROW("Free RAM (bytes)", ESP.getFreeHeap());
#elif defined(BOARD_STM32H7)
  CSV_ROW("CPU (MHz)", SystemCoreClock / 1000000);
  CSV_BLANK("Free RAM (bytes)");
#elif defined(F_CPU)
  CSV_ROW("CPU (MHz)", F_CPU / 1000000);
  CSV_BLANK("Free RAM (bytes)");
#else
  CSV_BLANK("CPU (MHz)");
  CSV_BLANK("Free RAM (bytes)");
#endif
}


// ==================== MAIN FUNCTIONS ====================

void setup() {
#if defined(ARDUINO_UNO_Q)
  SERIAL_OUT.begin();  // Uno Q Monitor doesn't use baud rate
  delay(3000);         // Wait longer for Monitor connection
#elif defined(ESP32)
  SERIAL_OUT.begin(SERIAL_BAUD);
  delay(3000);  // ESP32 needs extra time for serial initialization
#else
  SERIAL_OUT.begin(SERIAL_BAUD);
#if defined(USBCON)
  {
    unsigned long waitStart = millis();
    while (!SERIAL_OUT && (millis() - waitStart < 10000)) {
      delay(10);
    }
  }
#endif
  delay(2000);  // Wait for serial connection
#endif

  calibrateBenchmarkTime();

#if CSV_OUTPUT
  SERIAL_OUT.println(F_STR("Part,2"));
#else
  SERIAL_OUT.println();
  SERIAL_OUT.println();
  printDivider();
  SERIAL_OUT.println(F_STR("  UNIVERSAL ARDUINO BENCHMARK SUITE"));
  SERIAL_OUT.println(F_STR("  Part 2: Connectivity, Platform & Power"));
  printDivider();
  SERIAL_OUT.println();
#endif

  // System info
  printSystemInfo();

  // Advanced CPU benchmarks
  benchmarkCPUStress();
  benchmarkAdvancedMath();

  // Communication benchmarks
  benchmarkSerial();

  // Board-specific connectivity
#ifdef HAS_LED_MATRIX
  benchmarkLEDMatrix();
#else
  CSV_BLANK("LED Matrix Pattern (ops/ms)");
  CSV_BLANK("LED Matrix Bitmap (ops/ms)");
#endif
#ifdef HAS_WIFI
  benchmarkWiFi();
#else
  CSV_BLANK("WiFi Scan (ms)");
  CSV_BLANK("WiFi Connect (ms)");
  CSV_BLANK("WiFi TX (KB/s)");
#endif
#ifdef HAS_BLE
  benchmarkBLE();
#else
  CSV_BLANK("BLE Advertise Start (ms)");
  CSV_BLANK("BLE Scan Found (ms)");
#endif

  // Crypto & security benchmarks
#if defined(ARDUINO_ARCH_RP2040)
  benchmarkSHA1();
  CSV_BLANK("MD5 (ops/ms)");
  CSV_BLANK("SHA256 (ops/ms)");
  CSV_BLANK("AES (ops/ms)");
  CSV_BLANK("RNG (bytes/sec)");
#elif defined(ESP32)
  benchmarkESP32Crypto();
  benchmarkESP32AES();   // also emits CSV_ROW("AES (ops/ms)", ...)
  benchmarkHardwareRNG();
#elif defined(BOARD_STM32U5)
  CSV_BLANK("SHA1 (ops/ms)");
  CSV_BLANK("MD5 (ops/ms)");
  CSV_BLANK("SHA256 (ops/ms)");
  CSV_BLANK("AES (ops/ms)");
  benchmarkHardwareRNG();
#else
  CSV_BLANK("SHA1 (ops/ms)");
  CSV_BLANK("MD5 (ops/ms)");
  CSV_BLANK("SHA256 (ops/ms)");
  CSV_BLANK("AES (ops/ms)");
  CSV_BLANK("RNG (bytes/sec)");
#endif
#if defined(ARDUINO_UNOR4_WIFI)
  benchmarkSoftwareATSE();
#endif

  // System diagnostics
  benchmarkTimingPrecision();
  benchmarkDelayTiming();
  benchmarkStackDepth();

  // Multi-core benchmarks
#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
  benchmarkMultiCore();
#else
  CSV_BLANK("Core 0 (iters/ms)");
  CSV_BLANK("Core 1 (iters/ms)");
#endif

#if defined(BOARD_STM32U5) && defined(HAS_DUAL_PROCESSOR)
  //benchmarkArduinoBridge();
#endif

#if defined(HAS_YUN_BRIDGE)
  benchmarkYunBridge();
#else
  CSV_BLANK("Yun Bridge Throughput (bytes/sec)");
#endif

  // Multiduino-specific benchmarks
#ifdef HAS_RTC
  benchmarkRTC();
#endif

  // ESP32 Additional Benchmarks (AES already benchmarked in crypto section above)
#if defined(ESP32)
#if defined(CONFIG_IDF_TARGET_ESP32)
  benchmarkESP32DAC();
#else
  CSV_BLANK("DAC (ops/ms)");
#endif
  benchmarkESP32Touch();
  benchmarkESP32Sleep();
  CSV_BLANK("DMA (MB/s)");
  CSV_BLANK("PIO (cycles/ms)");
  CSV_BLANK("Interpolator (ops/ms)");
#elif defined(ARDUINO_ARCH_RP2040)
  // RP2040 Additional Benchmarks
  benchmarkRP2040DMA();
  benchmarkRP2040PIO();
  benchmarkRP2040Interpolator();
  benchmarkRP2040PWM();
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  benchmarkPicoWBLE();
#endif
  CSV_BLANK("DAC (ops/ms)");
  CSV_BLANK("Touch (values/ms)");
#else
  CSV_BLANK("DAC (ops/ms)");
  CSV_BLANK("Touch (values/ms)");
  CSV_BLANK("DMA (MB/s)");
  CSV_BLANK("PIO (cycles/ms)");
  CSV_BLANK("Interpolator (ops/ms)");
#endif

  // Uno R4 Additional Benchmarks
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  benchmarkUnoR4DAC();
  benchmarkUnoR4RTC();
#if defined(ARDUINO_UNOR4_WIFI)
  benchmarkUnoR4BLE();
#else
  CSV_BLANK("Uno R4 BLE Advertise (ms)");
#endif
#else
  CSV_BLANK("Uno R4 DAC (ops/ms)");
  CSV_BLANK("Uno R4 DAC Ramp (ops/ms)");
  CSV_BLANK("RTC getTime (ops/ms)");
  CSV_BLANK("RTC setTime (ops/ms)");
  CSV_BLANK("Uno R4 BLE Advertise (ms)");
#endif

  // I2C Wire communication
#if !defined(ARDUINO_UNO_Q)
  benchmarkWireI2C();
#else
  CSV_BLANK("I2C 100kHz (us)");
  CSV_BLANK("I2C 400kHz (us)");
#endif

  // Power management
  benchmarkWatchdog();
  benchmarkSleepModes();

  // Final summary
  printHeader("PART 2 BENCHMARK COMPLETE!");
  SERIAL_OUT.println(F_STR("Platform benchmarks finished."));
  SERIAL_OUT.println(F_STR("Both parts complete! All benchmarks have been run."));
  SERIAL_OUT.println();
  SERIAL_OUT.println(F_STR("To run again, press the RESET button or"));
  SERIAL_OUT.println(F_STR("re-upload the sketch."));
  printDivider();
}

void loop() {
  // Nothing to do after benchmarks complete
}

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include "CircularBuffer.h"
#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h> //used to define Arduino_h when we are inside Arduino rather than just fake Arduino
#endif

#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#if defined(PICO_BOARD) || (!defined(Arduino_h) && defined(ARCH_ESP32)) || defined(MOCK_PICO_PI)
void printAvailableLogging();
extern CircularBuffer<char> serialLogBuffer;
#define ASSERT_DEBUG(...) assert(__VA_ARGS__);
#define LOG_DEBUG(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_INFO(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_WARN(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_ERROR(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_CRIT(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_TRACE(...) serialLogBuffer.writeF(__VA_ARGS__);
#elifdef Arduino_h
#define ASSERT_DEBUG(...) assert(__VA_ARGS__)
#define LOG_DEBUG(...) Serial.printf(__VA_ARGS__)
#define LOG_INFO(...) Serial.printf(__VA_ARGS__)
#define LOG_WARN(...) Serial.printf(__VA_ARGS__)
#define LOG_ERROR(...) Serial.printf(__VA_ARGS__)
#define LOG_CRIT(...) Serial.printf(__VA_ARGS__)
#define LOG_TRACE(...) Serial.printf(__VA_ARGS__)
#else
#define ASSERT_DEBUG(...) assert(__VA_ARGS__)
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#define LOG_INFO(...) printf(__VA_ARGS__)
#define LOG_WARN(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) printf(__VA_ARGS__)
#define LOG_CRIT(...) printf(__VA_ARGS__)
#define LOG_TRACE(...) printf(__VA_ARGS__)
#endif
#else
#define ASSERT_DEBUG(...)
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_CRIT(...)
#define LOG_TRACE(...)
#endif

void print_named_data(const char *name, const uint8_t *data, const uint16_t data_size);

#endif
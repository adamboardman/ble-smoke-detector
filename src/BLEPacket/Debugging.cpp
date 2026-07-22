#include "Debugging.h"

void print_named_data(const char *name, const uint8_t *data, const uint16_t data_size) {
    LOG_DEBUG("%s:", name);
    for (int i = 0; i < data_size; i++) {
        LOG_DEBUG("%02x", data[i]);
    }
    LOG_DEBUG("\n");
    LOG_DEBUG("%s[c]:", name);
    for (int i = 0; i < data_size; i++) {
        if (data[i] >= 0x20 && data[i] < 0x7e) {
            LOG_DEBUG("%c", data[i]);
        } else {
            LOG_DEBUG(" ");
        }
    }
    LOG_DEBUG("\n");
}

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)

#ifdef MOCK_PICO_PI
#include "../test/pico_pi_mocks.h"
#else
#include "pico/time.h"
#endif

CircularBuffer<char> serialLogBuffer(2000);

#define SERIAL_LOG_BUFFER_LEN 160

void printAvailableLogging() {
    static char line[SERIAL_LOG_BUFFER_LEN];
    // check for any logging that may have happened during servicing an interrupt
    if (serialLogBuffer.full()) {
        const auto len = serialLogBuffer.consume_line(line, SERIAL_LOG_BUFFER_LEN-1);
        printf("Buffer Maxed - Ignoring %d bytes tail of partial line\n", len);
    }

    while (!serialLogBuffer.empty()) {
        auto len = serialLogBuffer.consume_line(line, SERIAL_LOG_BUFFER_LEN-1);
        if (line[len-1]=='\n') {
            len--;
        }
        for (int i=0; i<len; i++) {
            putchar(line[i]);
        }
        putchar('\n');
        // sleep_ms(1);
    }
    serialLogBuffer.clear_if_empty(); //try to keep logging within only one block
}

#endif

#include "Debugging.h"

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)

#ifdef MOCK_PICO_PI
#include "../test/pico_pi_mocks.h"
#else
#include "pico/time.h"
#endif

CircularBuffer<char> serialLogBuffer(3000);

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
        sleep_ms(1);
    }
    serialLogBuffer.clear_if_empty(); //try to keep logging within only one block
}

#endif

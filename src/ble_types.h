#ifndef BLE_TYPES_H
#define BLE_TYPES_H

#if !defined(PICO_BOARD) && !defined(MOCK_PICO_PI)
// Define some things that are missing without the pico sdk

#include <Arduino.h>
#include <cstdint>
#include <vector>
#include <nimble/ble.h>
#include <NimBLEDevice.h>
#include <time.h>

#define ASSERT_DEBUG(...)
#define LOG_DEBUG(...) Serial.printf(__VA_ARGS__)
#define LOG_INFO(...) Serial.printf(__VA_ARGS__)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_CRIT(...)
#define LOG_TRACE(...)

enum {
    PICO_OK = 0,
    PICO_ERROR_NONE = 0,
    PICO_ERROR_TIMEOUT = -1,
    PICO_ERROR_GENERIC = -2,
    PICO_ERROR_NO_DATA = -3,
};

typedef struct {
    uint16_t start_group_handle;
    uint16_t end_group_handle;
    uint16_t uuid16;
    uint8_t uuid128[16];
} gatt_client_service_t;

typedef uint16_t hci_con_handle_t;

#define BD_ADDR_LEN BLE_DEV_ADDR_LEN
typedef uint8_t bd_addr_t[BLE_DEV_ADDR_LEN];

typedef enum {
    BD_ADDR_TYPE_LE_PUBLIC = 0,
    BD_ADDR_TYPE_LE_RANDOM = 1,
    BD_ADDR_TYPE_LE_PUBLIC_IDENTITY = 2,
    BD_ADDR_TYPE_LE_RANDOM_IDENTITY = 3,
    BD_ADDR_TYPE_SCO = 0xfc,
    BD_ADDR_TYPE_ACL = 0xfd,
    BD_ADDR_TYPE_UNKNOWN = 0xfe,
} bd_addr_type_t;

typedef struct {
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    uint16_t uuid16;
    uint8_t uuid128[16];
} gatt_client_characteristic_t;

typedef enum {
    HCI_ROLE_MASTER = 0,
    HCI_ROLE_SLAVE = 1,
    HCI_ROLE_INVALID = 0xff,
} hci_role_t;

typedef struct btstack_linked_item {
    struct btstack_linked_item *next; // <-- next element in list, or NULL
} btstack_linked_item_t;

typedef struct {
    btstack_linked_item_t *item;

    void (*callback)(void *context);

    void *context;
} btstack_context_callback_registration_t;

typedef struct {
} gatt_client_notification_t;

#define ERROR_CODE_SUCCESS                                            0x00u
#define ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER                      0x02u

void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t *notification);

uint8_t att_server_notify(hci_con_handle_t con_handle, uint16_t attribute_handle, const uint8_t *value,
                          uint16_t value_len);

uint8_t att_server_request_to_send_notification(btstack_context_callback_registration_t *callback_registration,
                                                hci_con_handle_t con_handle);

uint8_t gatt_client_write_value_of_characteristic_without_response(hci_con_handle_t con_handle, uint16_t value_handle,
                                                                   uint16_t value_length, uint8_t *value);

uint8_t gatt_client_request_to_write_without_response(btstack_context_callback_registration_t *callback_registration,
                                                      hci_con_handle_t con_handle);

typedef struct {
} hci_connection_t;

hci_connection_t *hci_connection_for_handle(hci_con_handle_t con_handle);

uint64_t time_us_64();
uint32_t time_us_32();
void sleep_ms(uint32_t ms);

inline char char_for_nibble(uint8_t nibble){

    static const char * char_to_nibble = "0123456789ABCDEF";

    if (nibble < 16){
        return char_to_nibble[nibble];
    } else {
        return '?';
    }
}

static inline char char_for_high_nibble(int value){
    return char_for_nibble((value >> 4) & 0x0f);
}

static inline char char_for_low_nibble(int value){
    return char_for_nibble(value & 0x0f);
}

static char bd_addr_to_str_buffer[6*3];  // 12:45:78:01:34:67\0
inline char * bd_addr_to_str_with_delimiter(const bd_addr_t addr, char delimiter){
    char * p = bd_addr_to_str_buffer;
    int i;
    for (i = 0; i < 6 ; i++) {
        uint8_t byte = addr[i];
        *p++ = char_for_high_nibble(byte);
        *p++ = char_for_low_nibble(byte);
        *p++ = delimiter;
    }
    *--p = 0;
    return (char *) bd_addr_to_str_buffer;
}

inline char * bd_addr_to_str(const bd_addr_t addr){
    return bd_addr_to_str_with_delimiter(addr, ':');
}

static NimBLEUUID serviceUUID("298cfeca-a10d-49ee-8a74-e513547f7ef7");
static NimBLEUUID charUUID("a8d99167-e58c-4a0c-9565-e2f1a7fbc05d");

#endif

#endif
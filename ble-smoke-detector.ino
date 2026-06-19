#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>
#include <NimBLEAdvertisedDevice.h>
#include "include/ble_types.h"
#include "src/BleConnection.h"
#include "src/BleConnectionTracker.h"
#include "src/Message.h"
#include "src/ProtocolProcessor.h"

uint32_t time_us_32() {
  return micros();
}

uint64_t time_us_64() {
  return micros();
}

void sleep_ms(uint32_t ms) {
  delay(ms);
}

int scanTime = 5 * 1000;  // In milliseconds
NimBLEScan *pBLEScan = nullptr;

bool keep_running = true;
bool fresh_boot = true;
bool smoke_seen = false;
bool life_check = false;
uint16_t global_activity = 0;
uint8_t conn_count = 0;
bool connection_in_progress = false;

BleConnectionTracker connection_tracker;
BleConnectionTracker *connection_tracker_ptr = &connection_tracker;
ProtocolProcessor processor(connection_tracker);

struct InputPin {
  const uint8_t PIN;
  uint32_t numberChanges;
  bool state;
};

InputPin inputD0smoke = { D0, 0, 1 };

void generateMessageIfNeeded() {
  std::vector<uint8_t> outBuffer;
  const uint64_t timestamp_ms = connection_tracker.getTimeMs();
  if (connection_tracker.weHaveTheTime() && (fresh_boot || inputD0smoke.state == 0 || life_check)) {
    auto local_addr = NimBLEDevice::getAddress().reverseByteOrder();  //NimBLE stores addresses in reverse to most others
    uint64_t sender = local_addr;

    const BinaryWriter buffer(outBuffer);
    buffer.write_uint8('[');
    buffer.write_uint16_hex16(timestamp_ms / 1000);  //hexified seconds
    buffer.write_uint8(']');
    if (fresh_boot) {
      const std::string booted_string(" Booted");
      buffer.write_data(booted_string, booted_string.size());
      life_check = false;
    }
    if (life_check) {
      const std::string still_alive_string = " Still alive";
      buffer.write_data(still_alive_string, still_alive_string.size());
      life_check = false;
    }
    if (inputD0smoke.state == 0 || digitalRead(D0) == 0) {
      const std::string smoke_seen_string = " Smoke seen";
      buffer.write_data(smoke_seen_string, smoke_seen_string.size());
      inputD0smoke.state = 1;
    }
    const std::string messageContentString(reinterpret_cast<const char *>(outBuffer.data()), outBuffer.size());

    Message message(7, timestamp_ms, 0, sender);
    message.setContent(messageContentString);
    outBuffer.clear();
    message.setMessageFlags(0);
    message.setChannel("Smoke");
    buffer.write_uint32(timestamp_ms);
    buffer.write_uint8('-');
    buffer.write_uint64(sender);
    const std::string messageIdString(reinterpret_cast<const char *>(outBuffer.data()), outBuffer.size());
    message.setMessageId(messageIdString);
    message.setMessageTimestamp(timestamp_ms);

    std::vector<uint8_t> name_buffer;
    const BinaryWriter name_writer(name_buffer);
    name_writer.write_data(reinterpret_cast<const uint8_t *>(ble_smoke_detector_service_name),
                           strlen(ble_smoke_detector_service_name));
    name_writer.write_data(":", 1);
    name_writer.write_uint8_hex16(local_addr.getVal()[BLE_DEV_ADDR_LEN - 2]);
    name_writer.write_uint8_hex16(local_addr.getVal()[BLE_DEV_ADDR_LEN - 1]);
    message.setSenderNickname(std::string(reinterpret_cast<const char *>(name_buffer.data()), name_buffer.size()));

    if (Base *messageIfNew = connection_tracker.storeMessageAndReturnIfNew(message)) {
      //our messages will always be new
      connection_tracker.enqueueBroadcastPacket(messageIfNew);
      global_activity++;
      fresh_boot = false;
    }
  }
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
    bool foundName = false;
    bool foundCoded = false;

    if (advertisedDevice->haveName()) {
      LOG_DEBUG("Scan found: %s, with name: %s, rssi: %d\n", advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getName().c_str(), advertisedDevice->getRSSI());
      if (0 == memcmp(packet_service_name, advertisedDevice->getName().c_str(), strlen(packet_service_name))) {
        foundName = true;
      }
    }

    if (advertisedDevice->getSecondaryPhy() > 0) {
      LOG_DEBUG("pPhy: %d, sPhy: %d\n", advertisedDevice->getPrimaryPhy(), advertisedDevice->getSecondaryPhy());
      if (advertisedDevice->getPrimaryPhy() == BLE_HCI_LE_PHY_CODED  || advertisedDevice->getSecondaryPhy() == BLE_HCI_LE_PHY_CODED) {
        foundCoded = true;
      }
    }

    if (advertisedDevice->haveServiceUUID()) {
      if (advertisedDevice->getServiceUUID().equals(serviceUUID)) {
        auto address = advertisedDevice->getAddress();
        auto usableAddress = address.reverseByteOrder();  //NimBLE stores addresses in reverse to most others
        auto rssi = advertisedDevice->getRSSI();
        auto services = ServiceUUIDFound;
        if (foundName) {
          services = ServiceUUIDAndNameFound;
        }
        connection_tracker.addAvailablePeer(usableAddress.getVal(), usableAddress.getType(), services, rssi, foundCoded);
      }
    }
  }
} scanCallbacks;

void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  connection_tracker.printTime();
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";
  str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
  str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
  str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
  str += ", Length = ";
  LOG_DEBUG("%s %d\n", str.c_str(), length);
  auto &context = connection_tracker.connectionForConnHandle(pRemoteCharacteristic->getClient()->getConnHandle());
  processor.processWrite(context, 0, pData, length);
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Device connected\n");

    LOG_DEBUG("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
    const auto conAddress = pClient->getPeerAddress().reverseByteOrder();  //NimBLE stores addresses in reverse to most others
    bd_addr_t address;
    memcpy(address, conAddress.getVal(), BD_ADDR_LEN);
    bd_addr_type_t addressType = (bd_addr_type_t)conAddress.getType();
    const auto info = pClient->getConnInfo();
    uint8_t role = HCI_ROLE_INVALID;
    if (info.isMaster()) {
      role = HCI_ROLE_MASTER;
    }
    if (info.isSlave()) {
      role = HCI_ROLE_SLAVE;
    }
    auto connection = connection_tracker.reportConnection(pClient->getConnHandle(), address, addressType, role);
    if (connection.getUseCodedPhy()) {
      pClient->updatePhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_S8);
    }

    connection_in_progress = false;
    global_activity++;
    conn_count++;
  }

  void onConnectFail(NimBLEClient *pClient, int reason) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Device Connect fail: %s, reason: %d\n", pClient->getPeerAddress().toString().c_str(), reason);
    LOG_DEBUG(NimBLEUtils::returnCodeToString(reason));
    connection_in_progress = false;
    global_activity++;
  }

  void onDisconnect(NimBLEClient *pClient, int reason) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Device disconnected\n");
    connection_tracker.reportDisconnection(pClient->getConnHandle());
    connection_in_progress = false;
    global_activity++;
    conn_count--;
  }

  void onPhyUpdate(NimBLEClient *pClient, uint8_t txPhy, uint8_t rxPhy) {
    connection_tracker.printTime();
    LOG_DEBUG(" Device PhyUpdate - h:%d, txPhy: %d, rxPhy: %d\n", pClient->getConnHandle(), txPhy, rxPhy);
  }

} clientCallbacks;

void ARDUINO_ISR_ATTR isr(void *arg) {
  InputPin *s = static_cast<InputPin *>(arg);
  s->numberChanges += 1;
  s->state = digitalRead(s->PIN);
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Client connected on connection ID: %u\n%s\n", connInfo.getConnHandle(), connInfo.toString().c_str());

    const auto conAddress = connInfo.getAddress().reverseByteOrder();  //NimBLE stores addresses in reverse to most others
    bd_addr_t address;
    memcpy(address, conAddress.getVal(), BD_ADDR_LEN);
    bd_addr_type_t addressType = (bd_addr_type_t)conAddress.getType();
    uint8_t role = HCI_ROLE_INVALID;
    if (connInfo.isMaster()) {
      role = HCI_ROLE_MASTER;
    }
    if (connInfo.isSlave()) {
      role = HCI_ROLE_SLAVE;
    }
    connection_tracker.reportConnection(connInfo.getConnHandle(), address, addressType, role);
    global_activity++;
    conn_count++;
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Client disconnected on connection ID: %u\n", connInfo.getConnHandle());
    connection_tracker.reportDisconnection(connInfo.getConnHandle());
    global_activity++;
    conn_count--;
  }

  void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override {
    connection_tracker.printTime();
    LOG_DEBUG(" Client MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
  }

  void onPhyUpdate(NimBLEConnInfo &connInfo, uint8_t txPhy, uint8_t rxPhy) {
    connection_tracker.printTime();
    LOG_DEBUG(" Client PhyUpdate - h:%d, txPhy: %d, rxPhy: %d\n", connInfo.getConnHandle(), txPhy, rxPhy);
  }

} serverCallbacks;

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    LOG_DEBUG("%s : onRead(), value: %s\n",
              pCharacteristic->getUUID().toString().c_str(),
              pCharacteristic->getValue().c_str());
  }

  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    auto &connection = connection_tracker.connectionForConnHandle(connInfo.getConnHandle());
    processor.processWrite(connection, 0, (uint8_t *)pCharacteristic->getValue().c_str(), pCharacteristic->getValue().size());
    global_activity++;
  }

  // The value returned in code is the NimBLE host return code.
  void onStatus(NimBLECharacteristic *pCharacteristic, int code) override {
    LOG_DEBUG("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
  }

  // Peer subscribed to notifications/indications
  void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override {
    std::string str = "Address: ";
    str += connInfo.getAddress().toString();
    if (subValue == 0) {
      str += " Unsubscribed to ";
    } else if (subValue == 1) {
      str += " Subscribed to notifications for ";
    } else if (subValue == 2) {
      str += " Subscribed to indications for ";
    } else if (subValue == 3) {
      str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID());

    connection_tracker.printTime();
    LOG_DEBUG("Client ID: %d, %s\n", connInfo.getConnHandle(), str.c_str());
  }
} chrCallbacks;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO: Serial.println("Wakeup caused by GPIO"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

RTC_DATA_ATTR int bootCount = 0;
static uint32_t boot_time = 0;
static NimBLEServer *pServer;


void start_scanning_for_local_nodes() {
  Serial.println("Scanning...");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setScanCallbacks(&scanCallbacks);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);
  pBLEScan->setPhy(NimBLEScan::SCAN_ALL);
  global_activity++;
}

bool connect_to_first_neighbour() {
  auto neighbours = connection_tracker.getConnectableNeighbours();
  if (const auto item = neighbours.begin(); item != neighbours.end()) {
    const auto neighbour = *item;
    const auto peerAddress = NimBLEAddress(neighbour->getAddress(), neighbour->getAddressType());

    if (connection_tracker.getConnectionForAddress(neighbour->getAddress())) {
      return false;
    }

    const auto pClient = NimBLEDevice::createClient(peerAddress);
    if (pClient) {
      connection_tracker.printTime();
      LOG_DEBUG("Connecting...\n");
      pClient->setClientCallbacks(&clientCallbacks);
      if (neighbour->getUseCodedPhy()) {
        pClient->setConnectPhy(BLE_GAP_LE_PHY_CODED_MASK);
      }
      auto connected = pClient->connect(true, false, true);
      LOG_DEBUG(NimBLEUtils::returnCodeToString(pClient->getLastError()));
      if (connected) {
        if (neighbour->getUseCodedPhy()) {
          pClient->updatePhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_S8);
        }
        auto pService = pClient->getService(serviceUUID);
        LOG_DEBUG("Got service %d\n", pService);
        if (pService != nullptr) {
          auto pCharacteristic = pService->getCharacteristic(charUUID);
          LOG_DEBUG("Got characteristic %d\n", pCharacteristic);

          if (pCharacteristic != nullptr && pCharacteristic->canNotify()) {
            LOG_DEBUG("Can Notify\n");
            if (!pCharacteristic->subscribe(true, notifyCB)) {
              LOG_DEBUG("Failed to subscribe to characteristic\n");
            }
          }
          if (pCharacteristic != nullptr && pCharacteristic->canIndicate()) {
            LOG_DEBUG("Can Indicate\n");
          }
        }
      }
      if (!connected) {
        connection_tracker.reportConnectionFailure(neighbour->getAddress(), pClient->getLastError());
        NimBLEDevice::deleteClient(pClient);
        LOG_DEBUG("Failed to connect");
        return false;
      }
      connection_tracker.printTime();
      LOG_DEBUG("isConnected: %d\n", pClient->isConnected());
      connection_tracker.setConnectionStarted(neighbour);
      if (!pClient->isConnected()) {
        connection_in_progress = true;  //only for async connections
      }
      global_activity++;
      return true;
    }
    sleep_ms(20);
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  boot_time = time_us_64();

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  esp_deep_sleep_enable_gpio_wakeup(BIT(D0), ESP_GPIO_WAKEUP_GPIO_LOW);

  pinMode(D6, OUTPUT);
  digitalWrite(D6, LOW);
  pinMode(D10, INPUT_PULLUP);  //Pulldown to make stop sleeping
  pinMode(inputD0smoke.PIN, INPUT_PULLUP);
  attachInterruptArg(inputD0smoke.PIN, isr, &inputD0smoke, FALLING);

  NimBLEDevice::init(ble_smoke_detector_service_name);
  NimBLEDevice::setMTU(256 + 5);
  auto local_addr = NimBLEDevice::getAddress();
  Serial.printf("Smoke Detector: %s (power: %d)\n", local_addr.toString().c_str(), NimBLEDevice::getPower(NimBLETxPowerType::All));
}

uint64_t lastSendPackets = time_us_64();
uint64_t lastConnection = time_us_64();
uint64_t lastFlash = -two_seconds_in_us;
uint64_t lastScan = -five_minutes_in_us;  //start with a scan
auto last_activity = global_activity;

void loop() {
  const auto loopStart = time_us_64();
  generateMessageIfNeeded();

  if (pBLEScan) {
    NimBLEScanResults foundDevices = pBLEScan->getResults(scanTime, false);
    Serial.println("");
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    Serial.println("");
    pBLEScan->clearResults();  // delete results scan buffer to release memory
    pBLEScan->stop();
    pBLEScan = nullptr;
    global_activity++;
  }

  if (!pBLEScan && !connection_in_progress && (loopStart - lastConnection) > three_seconds_in_us) {
    const auto connecting = connect_to_first_neighbour();

    lastConnection = time_us_64();
    // stop sending from happening immediately
    lastSendPackets = time_us_64();
  }

  if (!pBLEScan && !connection_in_progress && (loopStart - lastSendPackets) > one_second_in_us) {
    if (connection_tracker.sendPackets()) {
      global_activity++;
    }
    lastSendPackets = time_us_64();
  }

  if (loopStart - lastFlash > (conn_count > 0 ? one_second_in_us : three_seconds_in_us)) {
    digitalWrite(D6, HIGH);
    sleep_ms(10);
    digitalWrite(D6, LOW);
    lastFlash = time_us_64();
  }

  if (last_activity != global_activity) {
    connection_tracker.printStats();
  }

  if (!pBLEScan && !connection_in_progress && loopStart - lastScan > (conn_count == 0 ? five_seconds_in_us : thirty_seconds_in_us)) {
    start_scanning_for_local_nodes();
    lastScan = time_us_64();
    // stop sending from happening immediately
    lastSendPackets = time_us_64();
  }

  if (digitalRead(D10) && (connection_tracker.weHaveTheTime() && !pBLEScan && !connection_in_progress && !connection_tracker.havePacketsToSend()) || (loopStart - boot_time) > five_minutes_in_us) {
    Serial.println("Will deep sleep in 2 seconds");
    delay(2000);
    esp_deep_sleep_start();
  }

  if (last_activity != global_activity) {
    last_activity = global_activity;
  }

  // Serial.printf("Input D0 state %d, changes: %" PRIu32 " times\n", inputD0smoke.state, inputD0smoke.numberChanges);
  // Serial.printf("Input D0 state %d\n", digitalRead(D0));

  delay(1000);
}

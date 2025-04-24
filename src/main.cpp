#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <map>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

const std::string TARGET_NAME = "Nordic_UART_Service";
const std::string NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const std::string NUS_RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const std::string NUS_TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

int main()
{
    auto connection = sdbus::createSystemBusConnection();

    sdbus::BusName bluezService("org.bluez");
    sdbus::ObjectPath adapterPath("/org/bluez/hci0");

    auto adapter = sdbus::createProxy(*connection, bluezService, adapterPath);
    adapter->callMethod("StartDiscovery").onInterface("org.bluez.Adapter1");
    std::cout << "Skanowanie urządzeń BLE przez 5s..." << std::endl;

    std::this_thread::sleep_for(5s);

    auto objectManager = sdbus::createProxy(*connection, bluezService, sdbus::ObjectPath("/"));
    std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> objects;

    objectManager->callMethod("GetManagedObjects")
                 .onInterface("org.freedesktop.DBus.ObjectManager")
                 .storeResultsTo(objects);

    sdbus::ObjectPath devicePath;
    for (const auto& [path, interfaces] : objects) {
        auto it = interfaces.find("org.bluez.Device1");
        if (it != interfaces.end()) {
            const auto& props = it->second;
            if (props.count("Name")) {
                std::string deviceName = props.at("Name").get<std::string>();
                if (deviceName == TARGET_NAME) {
                    devicePath = path;
                    std::cout << "Znaleziono urządzenie: " << deviceName << " na ścieżce: " << path << std::endl;
                    break;
                }
            }
        }
    }

    if (devicePath.empty()) {
        std::cerr << "Nie znaleziono urządzenia: " << TARGET_NAME << std::endl;
        return 1;
    }

    auto device = sdbus::createProxy(*connection, bluezService, devicePath);
    try {
        device->callMethod("Connect").onInterface("org.bluez.Device1");
        std::cout << "Połączono!" << std::endl;
    } catch (const sdbus::Error& e) {
        std::cerr << "Błąd połączenia: " << e.what() << std::endl;
        return 1;
    }

    sdbus::ObjectPath txCharPath, rxCharPath;

    for (const auto& [path, interfaces] : objects) {
        if (std::string(path).find(std::string(devicePath)) != 0) continue;
    
        auto it = interfaces.find("org.bluez.GattCharacteristic1");
        if (it != interfaces.end()) {
            const auto& props = it->second;
            if (props.count("UUID")) {
                std::string uuid = props.at("UUID").get<std::string>();
                if (uuid == NUS_TX_UUID) txCharPath = path;
                if (uuid == NUS_RX_UUID) rxCharPath = path;
            }
        }
    }

    if (txCharPath.empty() || rxCharPath.empty()) {
        std::cerr << "Nie znaleziono charakterystyk RX/TX!" << std::endl;
        return 1;
    }

    // Subskrypcja TX (notify)
    auto txChar = sdbus::createProxy(*connection, bluezService, txCharPath);
    txChar->callMethod("StartNotify").onInterface("org.bluez.GattCharacteristic1");
    std::cout << "Subskrybowano TX notify" << std::endl;

    // Wysłanie danych do RX
    auto rxChar = sdbus::createProxy(*connection, bluezService, rxCharPath);
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o', '\n'};
    rxChar->callMethod("WriteValue")
        .onInterface("org.bluez.GattCharacteristic1")
        .withArguments(data, std::map<std::string, sdbus::Variant>{});
    std::cout << "Wysłano dane do RX" << std::endl;

    std::this_thread::sleep_for(5s); // Trzymamy połączenie chwilę

    return 0;
}





#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <map>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

const std::string TARGET_NAME = "Nordic_UART_Service";

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

    for (const auto& [path, interfaces] : objects) {
        auto it = interfaces.find("org.bluez.Device1");
        if (it != interfaces.end()) {
            const auto& props = it->second;
            if (props.count("Name")) {
                std::string deviceName = props.at("Name").get<std::string>();
                if (deviceName == TARGET_NAME) {
                    std::cout << "Znaleziono urządzenie: " << deviceName << " na ścieżce: " << path << std::endl;
            
                    auto device = sdbus::createProxy(*connection, bluezService, path);
                    device->callMethod("Connect").onInterface("org.bluez.Device1");
                    std::cout << "Połączono!" << std::endl;
                    return 0;
                }
            }
        }
    }

    std::cerr << "Nie znaleziono urządzenia: " << TARGET_NAME << std::endl;
    return 1;
}





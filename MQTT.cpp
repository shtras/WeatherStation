#include "MQTT.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"

#include "lwip/dns.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

constexpr std::string_view MQTT_TOPIC_CO2 = "home/weather_station/co2\0";

namespace weather_station
{
MQTT::MQTT()
{
    if (cyw43_arch_init() != 0) {
        std::cout << "cyw43 init failed!\n";
        return;
    }
    cyw43_arch_enable_sta_mode();
    memset(&mqttClientInfo_, 0, sizeof(mqttClientInfo_));

    char unique_buf[4] = {0};
    pico_get_unique_board_id_string(unique_buf, sizeof(unique_buf));
    clientId_ = std::string("pico") + unique_buf;
    std::transform(clientId_.begin(), clientId_.end(), clientId_.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    std::cout << "MQTT Client ID: " << clientId_ << "\n";
    mqttClientInfo_.client_id = clientId_.c_str();
    mqttClientInfo_.client_user = MQTT_USERNAME;
    mqttClientInfo_.client_pass = MQTT_PASSWORD;
    mqttClientInfo_.keep_alive = 60;
}

MQTT::~MQTT()
{
    std::cout << "Closing MQTT client\n";
    cyw43_arch_deinit();
}

bool MQTT::Connect()
{
    int connectCount = 0;
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        std::cerr << "failed to connect.\n";
        connectCount++;
        if (connectCount >= 3) {
            panic("Failed to connect to WiFi");
        }
        std::cout << "Retrying in 5 seconds...\n";
        sleep_ms(5000);
    }
    std::cout << "Connected.\n";

    cyw43_arch_lwip_begin();
    auto err = dns_gethostbyname(MQTT_SERVER, &mqttServer_, MQTT::dnsFoundCallback, this);
    cyw43_arch_lwip_end();
    if (err == ERR_INPROGRESS) {
        std::cout << "DNS request in progress...\n";
    } else if (err != ERR_OK) {
        panic("DNS request failed");
    }

    return true;
}

void MQTT::dnsFound(const ip_addr_t* ipaddr)
{
    if (ipaddr) {
        std::cout << "DNS resolved MQTT server to " << ipaddr_ntoa(ipaddr) << "\n";
        mqttServer_ = *ipaddr;
        startClient();
    } else {
        panic("dns request failed");
    }
}

void MQTT::startClient()
{
    constexpr int port = 1883;
    std::cout << "Starting MQTT client to " << ipaddr_ntoa(&mqttServer_) << ":" << port << "\n";
    mqttClient_ = mqtt_client_new();
    if (!mqttClient_) {
        panic("Failed to create MQTT client instance");
    }
    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(mqttClient_, &mqttServer_, port, MQTT::mqttConnectionCallback, this, &mqttClientInfo_) !=
        ERR_OK) {
        panic("MQTT broker connection error");
    }
    mqtt_set_inpub_callback(mqttClient_, MQTT::mqttIncomingPublishCallback, MQTT::mqttIncomingDataCallback, this);
    cyw43_arch_lwip_end();
}

void MQTT::onConnection(mqtt_client_t* client, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED) {
        connected_ = true;
        std::cout << "MQTT connected\n";
        if (reportingState_ == ReportingState::Pending) {
            reportCO2();
        }
        // mqtt_sub_unsub(mqttClient_, "home/weather_station/co2", 1, MQTT::mqttSubscribeCallback, this, true);
        // mqtt_sub_unsub(mqttClient_, "home/weather_station/temperature", 1, MQTT::mqttSubscribeCallback, this, true);
        // mqtt_sub_unsub(mqttClient_, "home/weather_station/humidity", 1, MQTT::mqttSubscribeCallback, this, true);
    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        std::cout << "MQTT disconnected\n";
        if (!connected_) {
            panic("Failed to connect to mqtt server");
        }
    } else {
        panic("Unexpected status");
    }
}

void MQTT::onSubscribe(err_t err)
{
    if (err != ERR_OK) {
        panic("Subscribe failed");
    }
}

void MQTT::onIncomingPublish(const char* topic, u32_t tot_len)
{
    std::cout << "Incoming publish on topic: " << topic << " len=" << tot_len << "\n";
}

void MQTT::onIncomingData(const u8_t* data, u16_t len, u8_t flags)
{
    std::cout << "Incoming data len=" << len << " flags=" << (int)flags << ": "
              << std::string_view{(const char*)data, len} << "\n";
}

void MQTT::ReportWeather(int co2, float temp, float hum)
{
    co2_ = co2;
    temp_ = temp;
    hum_ = hum;
    if (!connected_) {
        reportingState_ = ReportingState::Pending;
        std::cout << "MQTT client not started yet, Enqueueing report\n";
        return;
    }
    if (reportingState_ != ReportingState::Idle) {
        std::cout << "Already publishing, skipping\n";
        return;
    }
    std::cout << "Starting weather report\n";
    reportCO2();
}

void MQTT::onPublish(err_t err)
{
    if (err != ERR_OK) {
        std::cerr << "Publish failed: " << err << "\n";
    } else {
        std::cout << "Publish successful\n";
    }
    switch (reportingState_) {
        case ReportingState::ReportingCO2:
            reportTemperature();
            break;
        case ReportingState::ReportingTemperature:
            reportHumidity();
            break;
        case ReportingState::ReportingHumidity:
            reportingState_ = ReportingState::Idle;
            break;
        default:
            break;
    }
}

void MQTT::reportCO2()
{
    std::stringstream ss;
    ss << co2_;
    std::string co2_str = ss.str();

    std::cout << "Reporting CO2: " << co2_str << "\n";
    reportingState_ = ReportingState::ReportingCO2;

    auto err = mqtt_publish(
        mqttClient_, "home/weather_station/co2", co2_str.c_str(), co2_str.size(), 1, 0,
        MQTT::mqttPublishRequestCallback, this
    );
    if (err != ERR_OK) {
        std::cerr << "Failed to publish CO2: " << err << "\n";
        reportingState_ = ReportingState::Idle;
    }
}

void MQTT::reportTemperature()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << temp_;
    std::string temp_str = ss.str();

    std::cout << "Reporting Temperature: " << temp_str << "\n";
    reportingState_ = ReportingState::ReportingTemperature;

    auto err = mqtt_publish(
        mqttClient_, "home/weather_station/temperature", temp_str.c_str(), temp_str.size(), 1, 0,
        MQTT::mqttPublishRequestCallback, this
    );
    if (err != ERR_OK) {
        std::cerr << "Failed to publish Temperature: " << err << "\n";
        reportingState_ = ReportingState::Idle;
    }
}

void MQTT::reportHumidity()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << hum_;
    std::string hum_str = ss.str();

    std::cout << "Reporting Humidity: " << hum_str << "\n";
    reportingState_ = ReportingState::ReportingHumidity;

    auto err = mqtt_publish(
        mqttClient_, "home/weather_station/humidity", hum_str.c_str(), hum_str.size(), 1, 0,
        MQTT::mqttPublishRequestCallback, this
    );
    if (err != ERR_OK) {
        std::cerr << "Failed to publish Humidity: " << err << "\n";
        reportingState_ = ReportingState::Idle;
    }
}
} // namespace weather_station
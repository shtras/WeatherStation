#pragma once

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include <string>

namespace weather_station
{
class MQTT
{
public:
    MQTT();
    ~MQTT();

    bool Connect();

    void ReportWeather(int co2, float temp, float hum);

private:
    void dnsFound(const ip_addr_t *ipaddr);
    static void dnsFoundCallback(const char *hostname, const ip_addr_t *ipaddr, void *arg)
    {
        static_cast<MQTT*>(arg)->dnsFound(ipaddr);
    }

    void onConnection(mqtt_client_t *client, mqtt_connection_status_t status);
    static void mqttConnectionCallback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
    {
        static_cast<MQTT*>(arg)->onConnection(client, status);
    }

    void onIncomingPublish(const char *topic, u32_t tot_len);
    static void mqttIncomingPublishCallback(void *arg, const char *topic, u32_t tot_len)
    {
        static_cast<MQTT*>(arg)->onIncomingPublish(topic, tot_len);
    }

    void onIncomingData(const u8_t *data, u16_t len, u8_t flags);
    static void mqttIncomingDataCallback(void *arg, const u8_t *data, u16_t len, u8_t flags)
    {
        static_cast<MQTT*>(arg)->onIncomingData(data, len, flags);
    }

    void onSubscribe(err_t err);
    static void mqttSubscribeCallback(void *arg, err_t err)
    {
        static_cast<MQTT*>(arg)->onSubscribe(err);
    }

    void onPublish(err_t err);
    static void mqttPublishRequestCallback(void *arg, err_t err)
    {
        static_cast<MQTT*>(arg)->onPublish(err);
    }

    void startClient();

    void reportCO2();
    void reportTemperature();
    void reportHumidity();

    enum class ReportingState
    {
        Idle,
        Pending,
        ReportingCO2,
        ReportingTemperature,
        ReportingHumidity
    };
    ReportingState reportingState_ = ReportingState::Idle;

    mqtt_client_t* mqttClient_ = nullptr;
    struct mqtt_connect_client_info_t mqttClientInfo_;
    std::string clientId_;
    ip_addr_t mqttServer_;
    bool connected_ = false;

    int co2_ = 0;
    float temp_ = 0;
    float hum_ = 0;
};
} // namespace weather_station
#pragma once

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

namespace weather_station
{
class TCPTest
{
public:
    TCPTest();
    ~TCPTest();

    bool connect();

    void reportWeather(int co2, float temp, float hum);

private:
    static err_t tcp_client_poll(void* p, tcp_pcb* arg)
    {
        return ((TCPTest*)p)->poll(arg);
    }
    static err_t tcp_client_sent(void* p, tcp_pcb* arg, u16_t len)
    {
        return ((TCPTest*)p)->sent(arg, len);
    }
    static err_t tcp_client_recv(void* p, tcp_pcb* arg, pbuf* buf, err_t err)
    {
        return ((TCPTest*)p)->recv(arg, buf, err);
    }
    static void tcp_client_err(void* p, err_t err)
    {
        ((TCPTest*)p)->error(err);
    }
    static err_t tcp_client_connected(void* p, tcp_pcb* arg, err_t err)
    {
        return ((TCPTest*)p)->conn(arg, err);
    }
    err_t poll(tcp_pcb* arg);
    err_t sent(tcp_pcb* tpcb, u16_t len);
    err_t recv(tcp_pcb* arg, pbuf* buf, err_t err);
    void error(err_t err);
    err_t conn(tcp_pcb* arg, err_t err);

    err_t close(int status);

    ip_addr_t remote_addr;
    tcp_pcb* pcb = nullptr;

    bool reporting_ = false;
    int co2_ = 0;
    float hum_ = 0;
    float temp_ = 0;
};
} // namespace weather_station

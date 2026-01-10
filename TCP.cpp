#include "TCP.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h" // needed to set hostname

#include <iostream>
#include <sstream>
#include <iomanip>

namespace weather_station
{
TCPTest::TCPTest()
{
    if (cyw43_arch_init() != 0) {
        std::cout << "cyw43 init failed!\n";
        return;
    }
    cyw43_arch_enable_sta_mode();
}

TCPTest::~TCPTest()
{
    std::cout << "Closing TCP client\n";
    cyw43_arch_deinit();
}

bool TCPTest::connect()
{
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        std::cout << "failed to connect.\n";
        return false;
    }
    std::cout << "Connected.\n";
    return true;
}

void TCPTest::reportWeather(int co2, float temp, float hum)
{
    if (reporting_) {
        return;
    }
    co2_ = co2;
    temp_ = temp;
    hum_ = hum;
    reporting_ = true;
    ip4addr_aton("192.168.1.157", &remote_addr);
    pcb = tcp_new_ip_type(IP_GET_TYPE(&remote_addr));
    if (!pcb) {
        std::cout << "Failed to create pcb\n";
        return;
    }

    tcp_arg(pcb, this);

    tcp_poll(pcb, &TCPTest::tcp_client_poll, 10);
    tcp_sent(pcb, &TCPTest::tcp_client_sent);
    tcp_recv(pcb, tcp_client_recv);
    tcp_err(pcb, tcp_client_err);

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(pcb, &remote_addr, 4242, tcp_client_connected);
    cyw43_arch_lwip_end();
}
err_t TCPTest::poll(tcp_pcb* arg)
{
    std::cout << "Poll\n";
    return close(-1);
}
err_t TCPTest::sent(tcp_pcb* tpcb, u16_t len)
{
    std::cout << "Sent " << (int)len << "\n";
    return ERR_OK;
}
err_t TCPTest::recv(tcp_pcb* arg, pbuf* buf, err_t err)
{
    if (!buf) {
        std::cout << "TCP: Received empty buffer\n";
        return close(-1);
    }
    std::cout << "TCP received " << (int)buf->len << " " << (int)buf->tot_len << "\n";
    //std::cout << std::string_view{(char*)buf->payload, buf->len} << "\n";
    tcp_recved(pcb, buf->tot_len);
    pbuf_free(buf);
    return close(0);
}
void TCPTest::error(err_t err)
{
    std::cout << "TCP error " << (int)err << "\n";
    close(-1);
}
err_t TCPTest::conn(tcp_pcb* arg, err_t err)
{
    if (err != ERR_OK) {
        std::cout << "connect failed " << (int)err << "\n";
        return close(err);
    }
    std::cout << "TCP Connected\n";

    std::stringstream ss;
    ss << "PUT /measurement HTTP/1.1\n"
          "Content-length: 14"
          "\n\n";
    ss << std::setw(4) << co2_ << std::setprecision(4) << std::setw(5) << hum_ << std::setprecision(4) << std::setw(5)
       << temp_;
    //std::cout << "Reporting: " << ss.str() << "\n";

    auto res = tcp_write(pcb, ss.str().data(), ss.str().size(), TCP_WRITE_FLAG_COPY);
    std::cout << "Writing result: " << (int)res << "\n";
    return ERR_OK;
}

err_t TCPTest::close(int status)
{
    tcp_arg(pcb, nullptr);
    tcp_poll(pcb, nullptr, 0);
    tcp_sent(pcb, nullptr);
    tcp_recv(pcb, nullptr);
    tcp_err(pcb, nullptr);
    auto res = tcp_close(pcb);
    if (res != ERR_OK) {
        std::cout << "close failed " << (int)res << ", calling abort\n";
        tcp_abort(pcb);
        res = ERR_ABRT;
    }
    reporting_ = false;
    return res;
}
} // namespace weather_station

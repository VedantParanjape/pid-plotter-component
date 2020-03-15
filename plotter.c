#include "plotter.h"

void plotter()
{
    ESP_ERROR_CHECK(init_queue());
    init_transport();
    
    xTaskCreatePinnedToCore(pid_const_transport, "tcp_pid_receive", 4096, NULL, 1, &pid_const_transport_handle, 0);
    xTaskCreatePinnedToCore(pid_transport, "udp_pid_send", 4096, NULL, 2, &pid_transport_handle, 1);
}
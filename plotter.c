#include "plotter.h"

void plotter()
{
    ESP_ERROR_CHECK(init_queue());
    init_transport();
    
    xTaskCreate(pid_const_transport, "recv", 4096, NULL, 2, &pid_const_transport_handle);
    xTaskCreate(pid_transport, "recvudp", 4096, NULL, 3, &pid_transport_handle);
}
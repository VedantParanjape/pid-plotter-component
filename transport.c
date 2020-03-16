#include "transport.h"

static const char* TAG_transport = "transport";

esp_err_t init_queue(void)
{
    pid_struct_queue = xQueueCreate(1000, sizeof(struct pid_terms));
    pid_const_read_write_mutex = xSemaphoreCreateMutex();

    if (pid_struct_queue == NULL)
    {
        logE(TAG_transport, "%s", "Queue creation failed");
        return ESP_FAIL;
    }
    else
    {
        logI(TAG_transport, "%s", "Queue created");
        return ESP_OK;
    }

    if (pid_const_read_write_mutex == NULL)
    {
        logE(TAG_transport, "%s", "Read/Write Mutex creation failed");
        return ESP_FAIL;
    }
    else
    {
        logI(TAG_transport, "%s", "Read/Write Mutex created");
        return ESP_OK;
    }
    
}

void init_transport(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_ERROR_CHECK(example_connect());
}

esp_err_t send_to_queue(struct pid_terms pid_data)
{
    BaseType_t qerror = xQueueSendToBack(pid_struct_queue, (void*)&pid_data, (TickType_t) 0/portTICK_PERIOD_MS);
    
    if(qerror == pdPASS)
    {
        logD(TAG_transport, "%s", "Data sent to Queue");
        return ESP_OK;
    }
    else if(qerror == errQUEUE_FULL)
    {
        logE(TAG_transport, "%s", "Data not sent to Queue, Queue full");
        return ESP_FAIL;
    }
    else
    {
        logE(TAG_transport, "%s", "Unknown error");
        return ESP_FAIL;
    }
}

struct data_recv receive_from_queue(void)
{
    struct pid_terms data;
    struct data_recv ret_data;
    BaseType_t qerror = xQueueReceive(pid_struct_queue, &(data), (TickType_t) 10000/portTICK_PERIOD_MS);
    configASSERT(qerror);

    if(qerror == pdPASS)
    {
        logD(TAG_transport, "%s", "Data received from Queue");
        ret_data.data = data;
        ret_data.err = ESP_OK;
    }
    else if(qerror == pdFALSE)
    {
        logW(TAG_transport, "%s", "Data not received from Queue");
        ret_data.err = ESP_FAIL;
    }
    else
    {
        logE(TAG_transport, "%s", "Unknown error");
        ret_data.err = ESP_FAIL;
    }

    return ret_data;
}

void pid_transport()
{
    struct network_data* handle = malloc(sizeof(struct network_data));
    network_manager(handle);

    while (true)
    {
        struct data_recv pid = receive_from_queue();
        char* message = NULL;

        if (pid.err == ESP_OK)
        {
            message = create_pid_data_to_json(pid.data.current, pid.data.error, pid.data.P, pid.data.I, pid.data.D);

            int len = send_data(handle, message);
            logD(TAG_transport, "%d %s", len, "bytes of data sent");
        }
        else if(pid.err == ESP_FAIL)
        {
            message = "no data received from Queue";
            int len = send_data(handle, message);
            logW(TAG_transport, "%d %s", len, "no data received from Queue");
        }
        else
        {
            message = "Unknown error";
            int len = send_data(handle, message);
            logE(TAG_transport, "%d %s", len, "Unknown error");
        }
    }

    close_network_manager(handle);
}

void pid_const_transport()
{
    struct tcp_network_data* handle = malloc(sizeof(struct tcp_network_data));
    tcp_network_manager(handle);
    
    char* message = NULL;
    int message_len = 0;
    char message_len_buffer[15];
        
    while (true)
    {
        message = tcp_recieve_data(handle);
        if (message != NULL)
        {
            message_len = strlen(message);

            if (xSemaphoreTake(pid_const_read_write_mutex, (TickType_t) 100) == pdTRUE)
            {
                logI(TAG_transport, "%s", "pid_const_data ownership taken for writing");

                pid_const_data = read_pid_data_from_json(message);
                xSemaphoreGive(pid_const_read_write_mutex);

                logI(TAG_transport, "%s", "pid_const_data ownership released");
            }
            else
            {
                logW(TAG_transport, "%s", "pid_const_data already locked, couldn't take ownership");
            }
            
            sprintf(message_len_buffer, "%d", message_len);
        }
        else
        {
            logE(TAG_transport, "%s", "tcp receive failed");
            vTaskDelete(pid_const_transport_handle);
        }
        
        int err = tcp_send_data(handle, message_len_buffer);
        if (err > 0)
        {
            strcpy(message_len_buffer, "");
        }
        else
        {
            logE(TAG_transport, "%s", "tcp send failed");
            vTaskDelete(pid_const_transport_handle);
        }
    }
}

struct pid_const pid_const_read()
{
    while(true)
    {
        if(xSemaphoreGetMutexHolder(pid_const_read_write_mutex) == NULL)
        {
            return pid_const_data;
            break;
        }
        else
        {
            logW(TAG_transport, "%s", "pid_const_data is locked for writing");
        }
        
    }
}
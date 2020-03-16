#include "tcp_handler.h"

static const char *TAG = "tcp_handler";

void tcp_network_manager(struct tcp_network_data* nm)
{
    nm->dest_addr.sin_addr.s_addr = inet_addr(TCP_HOST_IP_ADDR);
    nm->dest_addr.sin_family = AF_INET;
    nm->dest_addr.sin_port = htons(TCP_PORT);
    nm->addr_family = AF_INET;
    nm->ip_protocol = IPPROTO_IP;
    inet_ntoa_r(nm->dest_addr.sin_addr, nm->addr_str, sizeof(nm->addr_str) - 1);

    nm->sock = socket(nm->addr_family, SOCK_STREAM, nm->ip_protocol);
    if (nm->sock < 0)
    {
        logE(TAG, "Unable to create socket: errno %d", errno);
    }
    else
    {
        logI(TAG, "Socket created, connected to %s:%d", TCP_HOST_IP_ADDR, TCP_PORT);
    }

    int err = connect(nm->sock, (struct sockaddr *)&nm->dest_addr, sizeof(nm->dest_addr));
    if (err != 0) {
        tcp_close_network_manager(nm);
        logE(TAG, "Socket unable to connect: errno %d", errno);
        nm->sock = -1;
    }
    else
    {
        logI(TAG, "%s", "Successfully connected");
    }
}

int tcp_send_data(struct tcp_network_data* nm, char* payload)
{
    if(nm->sock < 0)
    {
        logE(TAG, "%s", "Socket doesnot exist");
        return -1;
    }
    else
    {
        int err = send(nm->sock, payload, strlen(payload), 0);
        if (err < 0) 
        {
            logE(TAG, "Error occurred during sending: errno %d", errno);
        }
        else
        {
            logD(TAG, "%s", "Message sent");
        }
        return err;
    }
}

char* tcp_recieve_data(struct tcp_network_data* nm)
{
    if (nm->sock < 0)
    {
        logE(TAG, "%s", "Socket doesnot exist");
        return NULL;
    }
    else
    {
        int len = recv(nm->sock, nm->rx_buffer, sizeof(nm->rx_buffer) - 1, 0);

        if (len < 0) 
        {
            logE(TAG, "recvfrom failed: errno %d", errno);
            return NULL;
        }
        else
        {
            nm->rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            logD(TAG, "Received %d bytes from %s:", len, nm->addr_str);
            logD(TAG, "%s", nm->rx_buffer);
            return nm->rx_buffer;
        }
    }
}

void tcp_close_network_manager(struct tcp_network_data* nm)
{
    logI(TAG, "%s", "Shutting down socket");
    shutdown(nm->sock, 0);
    close(nm->sock);
}
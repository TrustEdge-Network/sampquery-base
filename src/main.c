#include <stdio.h>
#include <stdlib.h>
#include "sampquery.h"
#include <string.h>

void OnQuery(struct sockaddr_in addr, enum E_SAMPQUERY_PACKET type, char *buffer)
{
    // making sampquery query packet
    struct sampquery_packet_query querypkt;
    sampquery_new_query(&querypkt, type);

    // creating the response packet.
    char newPacket[SAMPQUERY_OUTGOING_LEN];

    // filling the response packet with query information.
    sampquery_make_query(newPacket, querypkt);

    // fillign the response packet with the requested info.
    struct sampquery_packet_info info;
    sampquery_new_packet_info(&info, "Roleplay dos guri", "blank", 0, "PT/BR", 500, 3);
    sampquery_make_packet(newPacket, (void *)&info, type);

    // sending the response.
    socklen_t len = sizeof(addr);
    sampquery_send(newPacket, (struct sockaddr *)&addr, len);
}

void OnError(int err)
{
    printf("Error: %d", err);
}

int main(void)
{
    sampquery_server_init("10.0.0.112", 8080);
    sampquery_callback_onpacketreceived(OnQuery);
    sampquery_callback_onerror(OnError);

    sampquery_server_listen();
    while (1)
    {
    }
    return 1;
}
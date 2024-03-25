#ifndef __SAMPQUERY_H
#define __SAMPQUERY_H
#include "net.h"

#define SAMPQUERY_INCOMING_LEN 11
#define SAMPQUERY_OUTGOING_LEN 512

enum E_SAMPQUERY_PACKET
{
    SAMPQUERY_PACKET_INFO = 'i'
};

struct sampquery_packet_info
{
    int hostname_len;
    char *hostname;
    int gamemode_len;
    char *gamemode;
    unsigned char isPassworded;
    int language_len;
    char *language;
    unsigned short maxPlayers;
    unsigned short playerCount;
};

struct sampquery_packet_query
{
    char SAMP[4];
    unsigned char ipbytes[4];
    unsigned char portbytes[2];
    char opcode;
};

typedef void (*OnPacketReceived_callback)(struct sockaddr_in addr, enum E_SAMPQUERY_PACKET type, char *buffer);
typedef void (*OnError_callback)(int err);

// Initialize sampquery server
int sampquery_server_init(const char *hostname, unsigned short port);
int sampquery_server_listen(void);
int sampquery_server_stop(void);

int sampquery_callback_onpacketreceived(OnPacketReceived_callback callback);
int sampquery_callback_onerror(OnError_callback callback);

int sampquery_new_query(struct sampquery_packet_query *packet, enum E_SAMPQUERY_PACKET);
int sampquery_make_query(char *buffer, struct sampquery_packet_query packet);

int sampquery_new_packet_info(struct sampquery_packet_info *packet, const char *hostname, const char *gamemode, unsigned char isPassworded,
                              const char *language, unsigned short maxPlayers, unsigned short playerCount);

int sampquery_make_packet(char *buffer, void *packet, enum E_SAMPQUERY_PACKET type);

int sampquery_send(const char *buffer, struct sockaddr *address, socklen_t len);
#endif
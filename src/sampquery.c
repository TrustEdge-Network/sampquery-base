#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "net.h"
#include "sampquery.h"

static pthread_mutex_t mutex;
static OnPacketReceived_callback queryCallback = NULL;
static OnError_callback errorCallback = NULL;

static struct sockaddr_in __serverAddress;
static int __listenerFD = -1;
static int __clientFD = -1;

static int __initialized = 0;

static void sampquery_dbg(const char *__cp)
{
#ifdef SAMPQUERY_DBG_MODE
    printf(__cp);
#endif
}

int sampquery_server_init(const char *hostname, unsigned short port)
{
    pthread_mutex_init(&mutex, NULL);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(hostname);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    socklen_t len = sizeof(addr);

    __serverAddress = addr;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("Error creating socket.");
        if (errorCallback != NULL)
        {
            errorCallback(errno);
        }
        return 0;
    }

    __listenerFD = fd;

    int r = bind(fd, (struct sockaddr *)&addr, len);
    if (r < 0)
    {
        perror("Error binding the socket.");
        if (errorCallback != NULL)
        {
            errorCallback(errno);
        }
        return 0;
    }
    __initialized = 1;
    return 1;
};

void *sampquery_server_listen_thread(void *args)
{
    while (1)
    {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        char buffer[1024];
        ssize_t bytes_received = recvfrom(__listenerFD, buffer, 1024, 0, (struct sockaddr *)&clientaddr, &len);
        if (bytes_received < 0)
        {
            pthread_mutex_lock(&mutex);
            perror("Error receiving bytes.");
            if (errorCallback != NULL)
            {
                errorCallback(errno);
            }
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (bytes_received < SAMPQUERY_INCOMING_LEN)
        {
            sampquery_dbg("bytes_received < SAMPQUERY_INCOMMING_LEN\n");
            continue;
        }

        if (buffer[10] != SAMPQUERY_PACKET_INFO)
        {
            sampquery_dbg("incoming packet with invalid opcode.\n");
            continue;
        }

        if (queryCallback != NULL)
        {
            enum E_SAMPQUERY_PACKET type;
            type = buffer[10];
            queryCallback(clientaddr, type, buffer);
        }
    }
    return NULL;
}

int sampquery_server_listen(void)
{
    if (__initialized == 0)
    {
        perror("Sampquery not initialized.");
        return 0;
    }

    if (queryCallback == NULL)
    {
        perror("Query callback not setted.");
        return 0;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, sampquery_server_listen_thread, NULL);
    pthread_detach(thread);
    return 1;
}

int sampquery_callback_onpacketreceived(OnPacketReceived_callback callback)
{
    queryCallback = callback;
    return 1;
}

int sampquery_callback_onerror(OnError_callback callback)
{
    errorCallback = callback;
    return 1;
}

int sampquery_new_query(struct sampquery_packet_query *packet, struct sockaddr_in syn, enum E_SAMPQUERY_PACKET type)
{
    strncpy(packet->SAMP, "SAMP", 4);
    in_addr_t iaddr = syn.sin_addr.s_addr;
    memcpy(packet->ipbytes, (unsigned char *)&iaddr, 4);
    packet->portbytes[0] = (unsigned char)(syn.sin_port & 0xFF);
    packet->portbytes[1] = (unsigned char)(syn.sin_port >> 8 & 0xFF);
    packet->opcode = type;
    return 1;
}

int sampquery_make_query(char *buffer, struct sampquery_packet_query packet)
{
    memcpy(buffer, &packet.SAMP, 4);
    memcpy(buffer + 4, &packet.ipbytes, 4);
    memcpy(buffer + 8, &packet.portbytes, 2);
    memcpy(buffer + 10, &packet.opcode, 1);
    return 1;
}

int sampquery_new_packet_info(struct sampquery_packet_info *packet, const char *hostname, const char *gamemode, unsigned char isPassworded,
                              const char *language, unsigned short maxPlayers, unsigned short playerCount)
{
    packet->hostname_len = strlen(hostname);
    packet->hostname = malloc(packet->hostname_len);
    strncpy(packet->hostname, hostname, strlen(hostname));

    packet->gamemode_len = strlen(gamemode);
    packet->gamemode = malloc(packet->gamemode_len);
    strncpy(packet->gamemode, gamemode, strlen(gamemode));

    packet->isPassworded = isPassworded;

    packet->language_len = strlen(language);
    packet->language = malloc(packet->language_len);
    strncpy(packet->language, language, strlen(language));

    packet->maxPlayers = maxPlayers;

    packet->playerCount = playerCount;

    return 1;
}

int sampquery_make_packet(char *buffer, void *packet, enum E_SAMPQUERY_PACKET type)
{
    if (type == SAMPQUERY_PACKET_INFO)
    {
        struct sampquery_packet_info *pkt = (struct sampquery_packet_info *)packet;
        char *reader = (buffer + SAMPQUERY_INCOMING_LEN);

        *(unsigned char *)reader = pkt->isPassworded;
        reader += sizeof(unsigned char);
        // playerCount
        *(unsigned short *)reader = pkt->playerCount;
        reader += sizeof(unsigned short);
        // maxPlayers
        *(unsigned short *)reader = pkt->maxPlayers;
        reader += sizeof(unsigned short);
        // hostname
        int hostnameLen = pkt->hostname_len;
        *(unsigned int *)reader = hostnameLen;
        reader += sizeof(unsigned int);

        strncpy(reader, pkt->hostname, hostnameLen);
        reader += hostnameLen;
        // gamemode
        int gamemodeLen = pkt->gamemode_len;
        *(unsigned int *)reader = gamemodeLen;
        reader += sizeof(unsigned int);

        strncpy(reader, pkt->gamemode, gamemodeLen);
        reader += gamemodeLen;
        // language
        int languageLen = pkt->language_len;
        *(unsigned int *)reader = languageLen;
        reader += sizeof(unsigned int);

        strncpy(reader, pkt->language, languageLen);
        reader += languageLen;
    }

    return 1;
}

int sampquery_response(const char *buffer, struct sockaddr *address, socklen_t len)
{
    ssize_t bytes_sent = sendto(__listenerFD, buffer, SAMPQUERY_OUTGOING_LEN, 0, address, len);

    if (bytes_sent < 0)
    {
        return 0;
    }

    return 1;
}

struct sockaddr_in sampquery_serverAddr(void)
{
    return __serverAddress;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lo/lo.h"

#define N_ADDRS 20

lo_address server;
char *addresses[N_ADDRS];
int msg_count = 0;

int handler(const char *path, const char *types,
            lo_arg **argv, int argc, lo_message msg, void *user_data)
{
    // keep count and send a reply
    msg_count++;
    lo_send(server, addresses[msg_count % N_ADDRS], "i", msg_count);
    if (msg_count % 10000 == 0) {
        printf("client received %d messages\n", msg_count);
    }
    return 1;
}


int main()
{
    // create address for server
    server = lo_address_new("localhost", "8000");

    // create client
    lo_server client = lo_server_new("8001", NULL);

    // make addresses and register them with server
    for (int i = 0; i < N_ADDRS; i++) {
        char path[100];
        sprintf(path, "/benchmark/%d", i);
        addresses[i] = (char *) malloc(strlen(path));
        strcpy(addresses[i], path);
        lo_server_add_method(client, path, "i", &handler, NULL);
    }

    lo_send(server, addresses[msg_count % N_ADDRS], "i", msg_count);
    while (1) {
        lo_server_recv_noblock(client, 0);
    }
}

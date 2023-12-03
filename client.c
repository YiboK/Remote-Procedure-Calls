#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

#include "udp.h"
#include "helper.h"
#include "client.h"

struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[])
{
    struct rpc_connection rpc;
    rpc.seq_number = 0;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    srand(current_time.tv_usec);
    rpc.client_id = rand();
    // printf("id: %d\n", rpc.client_id);
    struct socket sock = init_socket(src_port);

    // code from instruction
    struct sockaddr_storage addr;
    socklen_t addrlen;
    populate_sockaddr(AF_INET, dst_port, dst_addr, &addr, &addrlen);
    rpc.dst_addr = *((struct sockaddr *)(&addr));
    rpc.dst_len = addrlen;

    rpc.recv_socket = sock;
    return rpc;
}

// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time)
{
    // check helper.h
    struct payload load;
    load.ack = 0;
    load.fcn_idx = 0;
    load.key = time;
    load.value = -1;
    load.client_id = rpc->client_id;
    load.seq_number = ++rpc->seq_number;

    struct packet_info packet;
    for (int i = 0; i < RETRY_COUNT; i++)
    {
        //printf("RPC_idle is working, seq_num: %d\n", load.seq_number);
        // send request
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&load, sizeof(load));
        // receive response
        packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        // printf("length %d \n", packet.recv_len);
        // check if the packet is valid
        if (packet.recv_len >= 0)
        {
            // "load" from server
            struct payload *result = (struct payload *)packet.buf;
            // printf("ack %d pkg id %d seq %d\n", result->ack, result->client_id, result->seq_number);
            if (rpc->client_id != result->client_id || result->seq_number < rpc->seq_number)
            { // the data is not for this client
                continue;
            }
            if (result->ack)
            {
                // if receive an acknowledgement, reset retry (according to piazza).
                // we do not check value in this case
                //printf("RPC_idle received ACK\n");
                sleep(1);
                i = -1;
            }
            else
            {
                //printf("RPC_idle completed\n");
                return;
            }
        }
    }
    perror("No response for RPC_idle\n");
    exit(1);
}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key)
{
    struct payload load;
    load.ack = 0;
    load.fcn_idx = 1;
    load.key = key;
    load.value = -1;
    load.client_id = rpc->client_id;
    // printf("seq #: %d before\n", rpc->seq_number);
    load.seq_number = ++rpc->seq_number;
    // printf("payload seq #: %d\n", load.seq_number);
    // printf("seq #: %d after\n", rpc->seq_number);

    struct packet_info packet;
    for (int i = 0; i < RETRY_COUNT; i++)
    {
        //printf("RPC_get id: %d,seq_num: %d\n", load.client_id, load.seq_number);
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&load, sizeof(load));
        packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (packet.recv_len >= 0)
        {
            struct payload *result = (struct payload *)packet.buf;
            if (rpc->client_id != result->client_id || result->seq_number < rpc->seq_number)
            {
                continue;
            }
            if (result->ack)
            {
                //printf("RPC_get id: %d received ACK\n", load.client_id);
                sleep(1);
                i = -1;
            }
            else
            {
                //printf("RPC_get id: %d completed\n", load.client_id);
                return result->value;
            }
        }
    }
    perror("No response for RPC_get\n");
    exit(1);
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value)
{
    struct payload load;
    load.ack = 0;
    load.fcn_idx = 2;
    load.key = key;
    load.value = value;
    load.client_id = rpc->client_id;
    // printf("seq #: %d before\n", rpc->seq_number);
    load.seq_number = ++rpc->seq_number;
    // printf("payload seq #: %d\n", load.seq_number);
    // printf("seq #: %d after\n", rpc->seq_number);

    struct packet_info packet;
    //printf("RPC_put id: %d is putting %d, seq_num: %d\n", load.client_id, load.value, load.seq_number);
    for (int i = 0; i < RETRY_COUNT; i++)
    {

        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&load, sizeof(load));
        packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (packet.recv_len >= 0)
        {
            struct payload *result = (struct payload *)packet.buf;
            if (rpc->client_id != result->client_id || result->seq_number < rpc->seq_number)
            {
                continue;
            }
            if (result->ack)
            {
                //printf("RPC_put id: %d received ACK\n", load.client_id);
                sleep(1);
                i = -1;
            }
            else
            {
                //printf("RPC_put id: %d completed\n", load.client_id);
                return result->value;
            }
        }
    }
    perror("No response for RPC_put\n");
    exit(1);
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc)
{
    close(rpc->recv_socket.fd);
    return;
}

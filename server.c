#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "helper.h"
#include "udp.h"
#include "server_functions.h"

// structs stored in call table
struct client_info
{
    int client_id;
    int last;
    //int working;
    int last_result; // -3, if the thread is running
};

// use to pass data to thread
struct thread
{
    struct payload *data;
    int table_idx;
    pthread_mutex_t *lock;
};

pthread_mutex_t locks[100];

pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

struct client_info calltable[100];
int table_len = 0; // track num of connnected client

void *handler(void *arg)
{
    // struct thread *thread_data = (struct thread *)malloc(sizeof(struct thread));
    // memcpy(thread_data, (struct thread *)arg, sizeof(struct thread));
    // struct payload *data= (struct payload *)malloc(sizeof(struct payload));
    // memcpy(data, thread_data->data, sizeof(struct payload));
    struct thread *thread_data = (struct thread *)arg;
    //printf("thread id in handler: %d, idx %d\n", thread_data->data->client_id, thread_data->table_idx);
    struct payload *data = thread_data->data;
    pthread_mutex_t *lock = thread_data->lock;
    int idx = data->fcn_idx;
    

    printf("handler with client %d and id %d and seq %d\n", thread_data->table_idx,
           calltable[thread_data->table_idx].client_id, thread_data->data->seq_number);

    if (idx == 0)
    {
        printf("sleeping for %d second\n", data->key);
        pthread_mutex_lock(lock);
        idle(data->key);
        // calltable[thread_data->table_idx].working = 1;
        //data->value = -2;
        calltable[thread_data->table_idx].last_result = -2;
        pthread_mutex_unlock(lock);
        printf("waked\n");
        
    }
    else if (idx == 1)
    {
        pthread_mutex_lock(lock);
        //data->value = get(data->key);
        //calltable[thread_data->table_idx].working = 1;
        calltable[thread_data->table_idx].last_result = get(data->key);
        pthread_mutex_unlock(lock);

        printf("GET %d get value %d\n", data->key, calltable[thread_data->table_idx].last_result);
    }
    else
    {
        printf("id %d PUT %d put at %d\n", data->client_id,data->value, data->key);
        pthread_mutex_lock(lock);
        //data->value = put(data->key, data->value);
        calltable[thread_data->table_idx].last_result = put(data->key, data->value);
        //calltable[thread_data->table_idx].working = 1;
        pthread_mutex_unlock(lock);
    }

    // pthread_mutex_lock(lock);
    // calltable[thread_data->table_idx].last_result = data->value;
    // //calltable[thread_data->table_idx].working = 0;
    // pthread_mutex_unlock(lock);
    // printf("true client %d, idx: %d\n", thread_data->data->client_id, thread_data->table_idx);
    // printf("client %d with id %d has last value %d\n", thread_data->table_idx,
           //calltable[thread_data->table_idx].client_id, calltable[thread_data->table_idx].last_result);

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("no port specified!\n");
        return -1;
    }

    for (int i = 0; i < 100; i++)
        pthread_mutex_init(&locks[i], NULL);

    struct socket sock = init_socket(atoi(argv[1]));
    struct packet_info packet;
    pthread_t child_threads[100];
    while (1)
    {
        int handled = 0; // if client has connected before, we don't need to add a new element to call table
        packet = receive_packet(sock);
        // printf("server is working\n");
        if (packet.recv_len < 0)
        {
            printf("No data in packet\n");
        }

        

        struct payload *data = (struct payload *)packet.buf; // the "load" from client

        // the data we send back to client
        struct payload *load = (struct payload *)malloc(sizeof(struct payload));
        memcpy(load, data, sizeof(struct payload));

        int cur_id = load->client_id;
        int cur_seq = load->seq_number; // "i" in project description

        // check if the client recorded before
        for (int i = 0; i < table_len; i++)
        {
            if (calltable[i].client_id == cur_id)
            {
                //printf("id:%d %d is an old client\n", cur_id, cur_seq);
                int last = calltable[i].last;

                if (last > cur_seq)
                {
                    handled = 1;
                }
                else if (last == cur_seq)
                {

                    if (calltable[i].last_result == -3) //|| calltable[i].working
                        // if thread is running, we send an acknowledgement
                        load->ack = 1;
                    else
                        // else send old value
                        load->ack = 0;
                    // reference line 65 in client.c; it is ok to send old value with ack
                    load->value = calltable[i].last_result;
                    send_packet(sock, packet.sock, packet.slen, (char *)load, sizeof(struct payload));
                    handled = 1;
                }
                else
                {
                    // upade last
                    calltable[i].last = cur_seq;
                    calltable[i].last_result = -3;
                    // struct thread thread_data = {load, i, &locks[i]};
                    struct thread *thread_data = (struct thread *)malloc(sizeof(struct thread));
                    thread_data->data = (struct payload *)malloc(sizeof(struct payload));
                    memcpy(thread_data->data, load, sizeof(struct payload));
                    thread_data->table_idx = i;
                    thread_data->lock = &locks[i];
                    //start a new thread
                    handled = 1;
                    pthread_create(&child_threads[i], NULL, handler, thread_data);
                    // pthread_join(child_threads[i], NULL);
                }
                break;
            }
        }

        if (handled)
            continue;

        // new client
        //printf("id:%d %d is a new client\n", cur_id, cur_seq);
        
        struct client_info new_client = {cur_id, cur_seq, -3};
        pthread_mutex_lock(&main_lock);
        int idx = table_len;
        table_len++;
        calltable[idx] = new_client;
        pthread_mutex_unlock(&main_lock);

        //printf("idx: %d\n", idx);
        //struct thread thread_data = {load, idx, &locks[idx]};
        //printf("thread_data id %d, load id %d\n", thread_data.data->client_id, load->client_id);
        struct thread *thread_data = (struct thread *)malloc(sizeof(struct thread));
        thread_data->data = (struct payload *)malloc(sizeof(struct payload));
        memcpy(thread_data->data, load, sizeof(struct payload));
        thread_data->table_idx = idx;
        thread_data->lock = &locks[idx];
        // printf("creating thread for client %d\n", table_len);
        pthread_create(&child_threads[idx], NULL, handler, thread_data);
        // pthread_join(child_threads[table_len], NULL);
        
        if (calltable[idx].last_result == -3)
            load->ack = 1;
        else
            load->ack = 0;
        send_packet(sock, packet.sock, packet.slen, (char *)load, sizeof(struct payload));

        // for (int i = 0; i < table_len; i++)
        //     printf("%d ", calltable[i].client_id);
        // printf("\n");
    }
    return 0;
}
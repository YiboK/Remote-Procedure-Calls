#ifndef HELPER_H
#define HELPER_H

struct payload{
    int ack; // whether this struct is acknowledgement or payload.
    int fcn_idx; // which function the client want to execute. 0-idle, 1-get, 2-put
    int key; // key for get & put. time for idle
    int value; // return value; -2 if the function is idle; also used to store value for put
    int client_id;
    int seq_number;
};

#endif
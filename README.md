# Remote-Procedure-Calls

A simplified version of a classic paper, [Implementing Remote Procedure Calls](https://web.eecs.umich.edu/~mosharaf/Readings/RPC.pdf).

## Remote Proceadure Calls
Remote Proceadure Calls (RPC) is a very popular way to simplify programming distributed systems. They allow for non-experts to be able to easily execute functions on a remote server without having to worry about the networking, scheduling, or management of these tasks. They also give very powerful at-most once semantics, meaning that if you call this functon it will run at most one time on the server. The client should also be able to notice that the function has not executed, it just may not able to resolve this. Under normal circumstances these functions will run exactly once. Note that this is different from the exactly once semantics of running a normal function.

One major limitation of these functions is that, because you are running remotely, it requires that all data be passed by value. Practically this limits how how much data can actually be transfered per function call.

## Description
A client will send the server a message containing a client id as well as a sequence number. The client id is uniquely randomly selected on startup (rand() is okay). The sequence number is a monotonically increasing number that is used to track if requests are duplicate. The client needs to send messages to the server and block until either the message times out or the it receives a message from the server. In the case of a timeout, the client will try sending the message to the server again. If the client receives no responses after 5 attempts, the client should exit with an error message.

The server will track clients that have connected to the server. When a client requests to have the server run a function it will execute the chosen function if the server has a definition for the function. To prevent duplicate requests and enforce the at-most once semantics, if it receives a message that is less than the current tracked sequence number for that client it will simply discard it. If it receives a message that is equal to the sequence number it will reply with either the old value that was returned from the function, or a message indicating that it is working on the current requests. If it receives a message with a sequence number that is greater than the current tracked sequence number this indicates a new request and it will start a thread to run this task that will reply independently to the client.

 

The RPC server should take one command line argument: \

**port**: The port that the server will bind itself to
server_functions.h and server_functions.c contain the implementations of the RPC stubs for the server to execute. If you use the given Makefile this will build all of the file with your server.c file automatically. This file contains the following implementations:
\
idle
This sleeps the thread for a given number of seconds. In practice this is just a wrapper for sleep.
get
This reads a value from a server local int array. If it is out of bounds it returns -1.
put
This sets a value from a server local int array. If it is out of bounds it returns -1.
Otherwise the RPC server implementation is completely up to you. It will be helpful for you to look and use udp.h and udp.c. This is the easiest way for you to receive and send communications to your clients. There is more information about this in the hints. It will also be helpful to understand the call table from the paper as well as you will likely implement something very similar in your implementation. Without duplicating the paper, the call table can be summarized as an array of structs the contains a unique identifier for you client, the last sequence number for that client, and the last result for that client. This table can then be searched to find the client context and implement the sequence number handling reiterated below:

message arrives with sequence number i:
i > last: new request - execute RPC and update call table entry
i = last: duplicate of last RPC or duplicate of in progress RPC. Either resend result or send acknowledgement that RPC is being worked on.
i < last: old RPC, discard message and do not reply
This may raise the question of what happens if the client or server crashes? The client will get a new client_id and the client will need to restart their requests. From a client perspective we will be unsure if the last message completed, but this is okay is it still fulfills our at-least once guarantee. On server crash we will lose the entire Call Table. In the paper there are solutions to this, but in this project we are not going to handle this scenario and we can assume that all call state is lost.

 

The server is required to have the following functionality:

Track up to 100 connected clients
Excecute idle, get, and put commands (given in the starter code)
Receiving packets from a client handling the execution
Execute the requested function if the sequence number is greater than what the server has tracked for the client. This should also begin running a new thread that will allow for multiple clients to connect at the same time
Resend results from the most recent RPC for every client if the sequence number is equal
Reply with an acknowlegement (ACK) for in progress requests
Ignore requests that are older than the most recent requests sequence number 
 

 

The client library will not have a main function and will integrate into provided test programs. The starter code for the client library (in client.h) will contain the interface that need to be implemented. These are the only functions that need to be implemented in client.c. 

RPC_init
This function will initialize the rpc_connection struct and do any other work you need to do to initialize your RPC connection with the server
RPC_idle
This function will initiate and block until the idle RPC is completed on the server
RPC_get
This function will  initiate and block until the get RPC is completed on the server
RPC_put
This function will  initiate and block until the put RPC is completed on the server
RPC_close
This function will do any cleanup that you need to do for your RPC variables
 

The client library should have the following functionality:

Sending idle, get, or put requests to the RPC server and blocking until response from the server or throwing an exception on no response.
Retrying RPC requests on a chosen (short) timeout interval up to 5 times
Ignore packets that are for other client ids, or to old sequence numbers
Delaying retrys for 1 second on receiving an ACK from the server
 

Messages between the server and client have no requirements, however there are a few things that will likely need to contain in them:

 sequence numbers: This is what you will use to determine if this is a rebroadcast of an old request, or a new request from the client
client_id: This is needed for the server to determine which call table entry to associate with the client, and for the client to determine if this message is actually intended for it.

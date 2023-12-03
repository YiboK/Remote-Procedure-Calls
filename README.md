# Remote-Procedure-Calls

A simplified version of a classic paper, [Implementing Remote Procedure Calls](https://web.eecs.umich.edu/~mosharaf/Readings/RPC.pdf).

## Remote Proceadure Calls
Remote Proceadure Calls (RPC) is a very popular way to simplify programming distributed systems. They allow for non-experts to be able to easily execute functions on a remote server without having to worry about the networking, scheduling, or management of these tasks. They also give very powerful at-most once semantics, meaning that if you call this functon it will run at most one time on the server. The client should also be able to notice that the function has not executed, it just may not able to resolve this. Under normal circumstances these functions will run exactly once. Note that this is different from the exactly once semantics of running a normal function.

One major limitation of these functions is that, because you are running remotely, it requires that all data be passed by value. Practically this limits how how much data can actually be transfered per function call.

## Description
![0002](https://github.com/YiboK/Remote-Procedure-Calls/assets/94937314/d4ed9207-cc9f-4627-9b8f-664f641d5592)


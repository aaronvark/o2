README.txt

These are test programs for o2, benchmarking and development.

broadcastclient.c - development code; see if broadcasting works
broadcastserver.c

clockmaster.c - test of O2 clock synchronization (there are no 
clockmaster.h   provisions here to test accuracy, only if it works)

lo_benchmark_client.c - a performance test similar to o2client/o2server
lo_benchmark_server.c

o2client.c - performance test; send messages back and forth between
o2server.c   client and server. Only expected to work on localhost.

tcpclient.c - o2client/o2server will eventually drop a message if
tcpserver.c   run on an unreliable network. These programs do the
              same test using tcp rather than udp so that they should
              work on a wireless connection.

tcppollclient.c - development code exercising poll() to get messages
tcppollserver.c


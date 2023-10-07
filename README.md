# Sotochat

A very simple chat server and client.

IPC: System V shm and UNIX datagram socket
Network protocol: TCP/IP

### Synopsis
`[exec] [IPv4] [port0] [port1] [-sc]`
- `exec`: executable
- `IPv4`: address of server to connect to
- `port0`: outgoing port
- `port1`: incoming port
- `s`, `c`: client, server modes

### Todo

#### Soon
- implement message overflow check (from previous project)
- check if the bind issue still occurs on different addresses
- close the incoming socket when exiting from client
- " with SIGTERM
- comment code more and add descriptions of libraries; my name

#### Later
- a message log
- more message metadata (date, time)
- proper TLV encoding in message buffer

#### Would be cool
- a form of encryption
- use ncurses to make a nice UI for the client 

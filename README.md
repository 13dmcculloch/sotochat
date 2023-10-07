# Sotochat

A very simple chat server and client.

IPC: System V
Protocol: TCP

### Synopsis
`[exec] [IPv4] [port0] [port1] [-sc]`
- `exec`: executable
- `IPv4`: address of server to connect to
- `port0`: outgoing port
- `port1`: incoming port
- `s`, `c`: client, server modes

### Todo
- implement message overflow check (from previous project)
- check if the bind issue still occurs on different addresses
- close the incoming socket when exiting from client
- " with SIGTERM

- a message log
- more message metadata (date, time)
- proper TLV encoding in message buffer

- a form of encryption

- use ncurses to make a nice UI for the client 

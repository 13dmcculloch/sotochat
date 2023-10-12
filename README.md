# sotochat

Simple TCP/IP chat server on sockets.

### Synopsys

`[exec] [addr] [port0] [port1] [-sc]`
- `addr` is the address of the server to connect to
- `port0` is the port for outgoing messages
- `port1` is the port for incoming messages
- `-s` opens in server mode
- `-c` opens in client mode (precedence over `s` in case of both)

### Description

This is just a another hobby project. It doesn't really do much, and the UI
is nonexistent. It is also buggy. It may be useful if the usual chat service
fails and a replacement is needed.

### Bugs

When the main server process terminates, the process that handles sending
global messages back to clients gets stuck in a loop. The main process
needs to kill this child process when it exits uncleanly. A server going
offline would be no issue, but an interrupted parent process would cause
this behaviour if the server remains online.

Due to the way messages are handled, only one client is allowed per IP.

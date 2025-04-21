# UDP File Transfer System (Linux)

A simple UDP-based file transfer implementation with a server and client program.

## Features

- Server listens for file requests from clients
- Client can request files from the server
- Simple packet acknowledgment system
- Progress display during file transfer
- Timeout handling to detect lost connections

## Compilation

Compile the server and client with g++:

```bash
g++ -o server server.cpp
g++ -o client client.cpp
```

## Usage

### Starting the Server

```bash
./server
```

The server will start and listen for file requests on the default IP (127.0.0.1) and port (5000).

### Using the Client

Request a file from the server:

```bash
./client get <filename>
```

For example:
```bash
./client get example.txt
```

## Protocol Details

The file transfer protocol uses the following messages:

1. Client sends: `GET:<filename>` to request a file
2. Server responds:
   - `ERROR:File not found` if the file doesn't exist
   - `SIZE:<filesize>` if the file exists
3. Client sends: `READY` to begin transfer
4. Server sends file in packets with format: `<packet_id>:<data>`
5. Client sends: `ACK:<packet_id>` for each packet received
6. Server sends: `END` when transfer is complete

## Configuration

You can modify the default settings in both server.cpp and client.cpp:

- SERVER_IP: The IP address to bind/connect to (default: 127.0.0.1)
- SERVER_PORT: The port number to use (default: 5000)
- BUFFER_SIZE: Size of buffer for data transfer (default: 1024)

## Notes

- This implementation uses UDP which doesn't guarantee delivery, so the code implements a simple acknowledgment system.
- Large files are split into multiple packets for transfer.
- For production use, consider implementing more robust error handling, retransmission logic, and security features. 
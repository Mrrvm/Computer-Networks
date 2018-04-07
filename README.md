# Computer-Networks
### Project summary

The project objective was to make a reliable ring of servers that provide a generic service to requesting clients.

For the clients to get the service, they ask the central server for one IP and Port of a server in the ring. And connect to it through UDP. The IP/Port given corresponds to the ring's dispatch server.

For a server to enter the ring, it follows the same procedure, and the IP/Port given is the one of the ring's startup server.

The servers follow a messaging protocol when doing certain actions:

- among each other (through TCP), a message travels the ring when
  - leaving the ring (token O)
  - entering the ring (token N)
  - stop being available (token S)
  - becoming available when the ring is unavailable (token D)
  - warning about the unavailability of the ring (token I)
  - confirming the ring's availability (token t)
  - connecting to startup server (NEW)

- with the central server (through UDP), a message is sent when
  - requesting the startup server IP/Port (GET_START)
  - becoming the startup server (SET_START)
  - becoming the dispatch server (SET_DS)
  - stop being the dispatch (WITHDRAW_DS)
  - stop being the startup (WITHDRAW_START)
  

Project/reqserv.c represents the client

Project/service.c represents the server(s)




/*
   Outline of the program:
   1. Create a Socket
   2. Bind the Socket to and address
   3. Listen on the Address
   4. Block on Accept until a connection is made
   5. Read on the connected Socket
   6. Respond
   7. Write back on the connected socket
   8. Close the connection
   9. Go back to blocking on accept
   */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> // for 'memset'
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h> // for close

int main(int argc, char *argv[])
{
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res;
  int status;
  int sockfd, client_fd;
  int b;
  int l;

  // Check we got correct user input
  if (argc != 2) {
    fprintf(stderr, "Please enter one argument\n");
    return 1;
  }

  // Load up address structs with getaddrinfo()

  memset(&hints, 0, sizeof hints); // Ensure hints is empty
  hints.ai_family = AF_UNSPEC;     // Either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Want a stream socket
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  // Ensure the getaddrinfo goes well

  if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  // Make a socket:

  sockfd = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
  if (sockfd == -1) {
    fprintf(stderr, "Something went wrong with socket\n");
    return 1; // Maybe think about errno here?
  }

  // Bind it to the port we pased in to getaddrinfo()

  b = bind(sockfd, res -> ai_addr, res -> ai_addrlen);
  if (b == -1) {
    fprintf(stderr, "Something went wrong with binding\n");
    return 1; // Maybe think about errno here?
  }

  // Get the server to start listening for connections

  l = listen(sockfd, 10); // backlog = 10

  if (l == -1) {
    fprintf(stderr, "Something went wrong with listening\n");
    return 1; // Maybe think about errno here?
  } 

  // Now we accept an incoming connection:

  addr_size = sizeof their_addr;
  client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
  if (client_fd == -1) {
    fprintf(stderr, "Something went wrong with accepting\n");
    return 1; // Maybe think about errno here?
  }

  // Hre we get info about who we got connection from
  // inet_ntop(their_addr.ss_famili

  // Now we are going to Read on the connected socket
  // We will respond to a variety of HTTP requests slowly adding more complexity
  
  char buffer[257] = {0}; // Create a buffer one longer than accept request
  int r;

  r = recv(client_fd, buffer, 256, 0); // Accept size one less that size of buffer
  
  // Now we deal with different requests, intially lets try GET
  char* f = buffer + 5; // GET /file.html ...
  *strchr(f, ' ') = 0; // Set the space to a null 
                       
  int opened_fd = open(f, O_RDONLY);

  sendfile(client_fd, opened_fd, 0, 256);

  sleep(5);

  close(opened_fd);
  close(client_fd);
  close(sockfd);

  return 0;
}

# System calls

This document goes over system calls (and other library calls) that allow you
to access network functionality. These let the kernel do all the work :)

The man pages are useful for these, however they are not useful on the order to
call these functions, this guide goes through each function in mostly the same
order you call them in a program.

1. getaddrinfo
2. socket
3. bind
4. connect
5. listen
6. accept
7. send/recv
8. sendto/recvfrom
9. close/shutdown
10. getpeername
11. gethostname

## 1 getaddrinfo

This function does a lot, and has lots of options. It's essence is quite simple
though: *it sets up `structs` you need later on*

This used to be done with `gethostbyname` to do DNS lookups, then loading info
by hand into a `structaddr_in` using that in calls.

```c
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int getaddrinfo(const char *node,   // e.g. "www.example.com" or IP
                const char *service,  // e.g. "http" or port number
                const struct addrinfo *hints,
                struct addrinfo **res);
```

You give this function three input parameters, and it gives you a pointer to a
linked-list, `res` of results.

node parameter -> host name or IP address

service parameter -> A port number (such as 80) or particular service

hints parameter -> points to a `struct addrinfo` filled out before

Here is a sample call if you want to listen on host's IP address, port 3490.
Note this doesn't do anything, it just sets up stuctures to use later.

```c
int status;
struct addrinfo hints;
struct addrinfo *servinfo;  // will point to the results

memset(&hints, 0, sizeof hints); // make sure the struct is empty
hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

if ((status = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
    fprintf(stderr, "gai error: %s\n", gai_strerror(status));
    exit(1);
}

// servinfo now points to a linked list of 1 or more
// struct addrinfos

// ... do everything until you don't need servinfo anymore ....

freeaddrinfo(servinfo); // free the linked-list
```

Here follows an short program that prints IP addresses for whatever host you
specify on the command line:

```c
/*
** showip.c
**
** show IP addresses for a host given on the command line
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // Either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);

    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list

    return 0;
}
```
### Description of showip

This calls `getaddrinfo()` on whatever we pass. This fills our linked list
pointed to by res, which we then iterate over and print stuff out.

Let's go over this slowly, and learn lots from it. First of all we include lots
of libraries for the functions we need. Next we pass `int argc` and
`char * argv[]` to our main. These are new to me! argc is the number of
arguments passed to our main, argv is an array of char pointers (strings).

Next we declare three `addrinfo` structs. Declare an integer status, and an
array of chars, ipstr (of length INET_ADDRSTRLEN) which I assume is a macro and
won't worry abou for now

Next we do some error handling if we do not have the correct number of inputs.
Note that argv[0] is always (usually) the programs name, and so we check if
argc is 2 (which means 1 user input). If not, we print to the standard error
and exit the program, returning 1.

Next we call `memset` (found in <string.h>) which fills the first `sizeof(hints)`
bytes, starting at `&hints` with the constant integer `0`. This just ensure it
truly is an empty struct.

Then we fill in two of the fields of the struct, specifying we don't care which
IP family (IPv4 or IPv6) and that we are using Stream Sockets (as opposed to
datagram sockets).

Next we do some more error checking, filling status with the result of
`getaddrinfo(argv[1], NULL, &hints, &res)`. This takes a node and/or service, a
`stuct addrinfo` in this case (&hints), and also the &res. This is where the
results are stored (the structures are filled with info used for other
functions). From the man page:

> The getaddrinfo() function allocates and initializes a linked list of addrinfo
> structures, one for each  network address that matches node and service,
> subject to any restrictions imposed by hints, and returns a pointer to the
> start of the list in res.  The items in the linked list are linked by the
> ai_next field.

It returns `0` if it succeeds, hence our check, and one of a couple error codes
if it fails. The `gai_strerror` function translates the error codes into a human
readable string, suitable for error reporting (hence why we print it to stderr)

Next we loop through our results, using the fact that `res` is a linked list.
We then declare a couple of variables we will fill with some info. We then check
what `ai_family` is in res (using arrow notation), split into IPv4 and IPv6
cases. If IPv4 we fill a `sockaddr_in` struct with info, about the address, and
then access this address and store it in `addr`.

Finally, we convert our `addr` into a string, and print it! This is done with
the `inet_ntop` function. This takes a network address structure, in the
`ai_family` family, it stores it in `ipstr` which has a restricted size of
`sizeof ipstr`.

Finally we 'free' the linked list.

## 2 socket

Here is the breakdown:

```c
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```

These arguments allow you to state what kind of socket you want:

* domain -> IPv4 or IPv6
* type -> stream or datagram
* protocol -> TCP or UDP

It used to be that people hardcoded these values. However, it is cleaner to use
values from a call to `getaddrinfo` and then feed these values directly into
socket().

```c
int s;
struct addrinfo hints, *res;

// do the lookup, supose that "hints" has been filled (doing something like the
// above example)
getaddrinfo("www.example.com", "http", &hints, &res);

// You should at this point do error checking, as well looping through the 'res'
// linked list, however we will assume the first one is good

s = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
```

socket returns a *socket descriptor* that is used in later system calls, or is
-1 on error. The socket isn't any use by itslef, it needs other system calls to
*do* anything useful.

## 3 bind

Once you have a socket, you may want to associate it with a 'port' on your local
machine. The port number is used by the kernel to match an incoming packet to
a certain process's socket descriptor (what we get when calling socket).

This only appears to be necessary if you are going to `listen`, if one just
wants to `connect` (because you are not the server) then this is potentially
unnecessary.

```c
#include <sys/type.h>
#include <sys/socket.h>

int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
```

Here's what the parameters mean:

* sockfd -> socket file descriptor (returned by `socket`)
* my_addr -> pointer to `struct sockaddr` contains info (such as port and IP)
* addrlen -> length in bytes of address

Let's see an example, that binds the socket to the host the program is running
on, port 3490.

```c
struct addrinfo hints, *res;
int sockfd;

// first, load up address structs with getaddrinfo():

memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

getaddrinfo(NULL, "3490", &hints, &res);

// make a socket:

sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

// bind it to the port we passed in to getaddrinfo():

bind(sockfd, res->ai_addr, res->ai_addrlen);
```

The `AI_PASSIVE` flag, tells the program to bind the IP of the host it's
running on. If you wanted to do a specific one, add it to the first argument
of `getaddrinfo`.

bind also returns -1 on error, and sets `errno` to the error's value.

One thing to watch out for is not to go too **low** with your port numbers.
All ports bellow 1024 are RESERVED. You can have any number up to 65535 
(2^16 - 1).

Finally, if you rerun a server and `bind` fails due to "Address already in use".
This is because a little bit of socket that was connected is still hanging
around in the kernel and hogging the port.

## 4 connect

This call is as follows

```c
#include <sys/types.h>
#include <sys/socket.h>

int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
```

Parameters:

* sockfd -> socket file descriptor
* serv_addr -> a `struct sockaddr` containing destination port and IP address
* addrlen -> length in bytes of above struct

All this info can be gotten from `getaddrinfo`

This is used to connect to a server (in this case you are the client)

## 5 listen

Suppose you want to host a server, wait for connections and handle them in some
way. This process has two steps, first you `listen` then you `accept`.

```c
int listen(int sockfd, int backlog);
```

sockfd is the usual socket file descriptor. `backlog` is the number of
connections allowed on the incoming queue. This is where connections wait until
they are accepted. Most systems silently limit this to about 20, but you get
away with maybe 5 or 10.

As usual it returns -1 if there is an error, and sets `errno` on error.

A reminder of the order of calls:  
getaddrinfo -> socket -> bind -> listen

## 6 accept

When someone tries to connect to your server that is listening, it gets put on
the queue. When you call `accept` you tell it to get the pending connection.
This returns a **new** socket file descriptor. The old socket file descriptor
is still listening for new connections, the new one is ready to send() and
recv()

```c
#include <sys/types.h>
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

The `addr` pointer points to a local struct which will be where information
about the incoming connection will go. The address length parameter should be
set to the size of the previous struct.

This does the usual error things.

## 7 send/recv

These two functions are for communicating over stream sockets.

> These are *blocking* calls. This means that `recv()` will *block* until there
> is some data to receive. This means the program stops there, on that system
> call, until someone sends you something (OS jargon for "stop" is "sleep").
> `send()` can also block, if the things you are sending are all jammed up.

Here is the send call:

```c
int send(int sockfd, const void *msg, int len, int flags);
```

sockfd is the descriptor you want to send data to (in lots of cases the one
got with accept()). `msg` is a pointer to the data you want to send, `len` being
its size in bytes. You can set `flags` to 0 if you don't want to deal with, or
look up the man page for more info.

`send` returns the number of bytes *actually sent out*. This may be less than
the number we told it send. If it is, it is on *us* to ensure the rest gets
sent. As usual, -1 is returned on error, and the whoe errno shabang happens.

```c
int recv(int socfd, void *buf, int len, int flags);
```

This is quite similar, buf being the buffer to read info into. `recv` can also
return 0, this means that the remote side has closed the connection.

## 8 sendto/recvfrom

Used for datagram, not going to worry about for now.

## 9 close/shutdown

You can just us the regular Unix file descriptor `close(sockfd);` to close
the socket, which prevents any more reads and writes.

If you want more control over how it closes, use the `shutdown` function, which
means you can cut off communication in a certain direction

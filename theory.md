# Theory

These notes are for my personal use only, taken from [Beej](https://beej.us/guide/bgnet/html/split/)

What are sockets?

They are a way to speak to other programs using standard Unix file descriptors.

## Files

There is a common saying that "everything in Unix is a file." What this means is
that when Unix programs do I/O, they do it by reading or writing to a *file
descriptor*.

A *file descriptor* is just an integer associated with an open file. This file
can be just about anything, including: a network connection, a FIFO, a pipe, a
terminal, a real on-the-disk file.

This file descriptor is gotten by a call to the `socket()` system routine!

## Sockets

There are tons of different types of sockets. The main two are DARPA Internet
addresses (Internet Sockets) and path names on a local node (Unix Sockets). We
will be dealing with Internet Sockets for this project

### Two Types of Internet Sockets

There are many more than two types of Internet sockets, but we will only talk
about two in this document. The first is "Stream Sockets" and the second type
is "Datagram Sockets" (sometimes called connectionless sockets).

*Stream sockets* are reliable, two-way connected communication streams. They are
used for the `telnet` and `ssh` applications as well as web browsers using the
Hypertext Transfer Protocol (HTTP).

### TCP

How do stream sockets achieve high level of data transmision quality? They use
a protocol called "The Transmission Control Protocol", known as [TCP](https://datatracker.ietf.org/doc/html/rfc793)
TCP makes sure your data arrives *sequentially* as well as *error-free*. TCP
is also the other half of TCP/IP where IP is [Internet Protocol](https://datatracker.ietf.org/doc/html/rfc791)
IP deals primarily with internet routing, and generally not data integrity.

### UDP

When you send a datagram, it may arrive out of order, and may not arrive.
They use IP for routing, but instead of TCP they use "User Datagram Protocol" or
[UDP](https://datatracker.ietf.org/doc/html/rfc768).

They are conectionless because you don't have to maintain an open connection as
you if using a stream socket. They are used when either a TCP stack is not
available or if a few dropped packets does not matter. Primary uses are
multiplayer games, streaming audio, video conferencing, etc.

Why would you ever use an unreliable protocol? *speed*. It's way easier to just
send a packet and not worry about connection and ensuring it arrived safely.

## Network Theory

Let's learn a little about *Data Encapsulation*. This states that; a packet is
born -> the packet is wrapped in a header by the first protocol (say TFTP) ->
the who thing is encapsulated again by the next protocol (say UDP) -> It is
encapsulated again by the next (IP) -> Then again by the final protocol on the
hardware (physical) layer (say Ethernet).

When another computer receives the packet it does this process in reverse to get
the data.

The *Layered Network Model* (ISO/OSI) describes a system of network
functionality.

* Application
* Presentation
* Session
* Transport
* Network
* Data Link
* Physical

This is extremely general, but you can see how the layers relate to the
encapsulation of data. Thankfully, the kernel does most of the work for us
and we just need to chose a method of encapsulating the data and send it out
with stream sockets!

## IP Addressess, v4 and v6

The way you've probably seen ip addresses is with The Internet Protocol Version
4 (IPv4). These address are made up of four bytes, and commonly written in "dots
and numbers form" like: 192.0.2.111.

Vint Cerf (the father of the internet) warned people that we were going to run
out of IPv4 addresses. IPv6 was born to solve this issue, and introduced *a lot*
more addresses. It's the differece between 32 and 128 bits, since these are
powers of 2, this is a lot!

We represent them (in hexadecimal) as follows:
2001:0db8:c9d2:aee5:73e3:934a:a5ae:9551

If we have IP address with lots of zeros in, we can compress them all between
two colons.

The address `::1` is the *loopback address* which means "the machine I'm running
on now". In IPv4, the loopback address is `127.0.0.1.`

There is also a IPv4-compatability mode, for example the IPv$ address can be
represented as follows in IPv6
192.0.2.33 -> ::ffff:192.0.2.33

### Subnets

For organisation, we sometimes split an IP address into the *network portion*
and the remainder is the *host portion*

With 192.0.2.12 we could say the first three bytes are the network, and the last
byte is the host

There is also some information about *classes* of subnets that may or may not be
useful to know.

### Port Numbers

Besides IP addresses, there is another address that is used by TCP, and by UDP.
This is the *port number* which is a 16-bit number. It can be thought of as the
locacl address for the connection.

Think of the IP address as the street address of a hotel and the port number as
the room number.

This is how a computer handles incoming mail and web services on a computer with
a single IP address. HTTP is port 80, telnet is 23, the game DOOM used port 666.

## Byte Order

There seem to be two different ways to order bytes, great :)

Everybody agrees that if you want to represent the two-byte hex number, say
b34f, you will store it in two sequential bytes b3 followed by 4f. This number,
stored with the *big end first*, is called *Big-Endian*.

The problem is that a *few* computers (Intel related ones) decide to store bytes
reversed, so b34f would be stored as the sequential bytes 4f followed by b3.
This is called *Little-Endian*.

Big-Endian is also sometimes called *Network Byte Order* since thats what is
preferred(?) by networks people?

If we want to do network programming we need our numbers to be in Network Byte
Order, how can we ensure this? We put our values through a special function that
will do the conversion if it has to.

| Function | Description           |
| -------- | --------------------- |
| htons()  | Host TO Network Short |
| htonl()  | Host TO Network Long  |
| ntohs()  | Network TO Host Short |
| ntohl()  | Network TO Host Long  |

In short, we will convert our numbers to Network Byte Order *before* they go out
and then convert them to Host Byte Order *after* they come in.

## 'struct s'

In this section we will cover the various data types used by the sockets
interface

### Socket Descriptor

This is an easy one, it has the type `int`

### struct addrinfo

This is more recent invention and is used to prep the socket address structures
for subsequent use.

It is also used in host name lookups and service name lookups

```c
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
    int              ai_protocol;  // use 0 for "any"
    size_t           ai_addrlen;   // size of ai_addr in bytes
    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
    char            *ai_canonname; // full canonical hostname

    struct addrinfo *ai_next;      // linked list, next node
};
```

When you load up this struct with `getaddrinfo` it returns a pointer to
a linked list of these structures, filled with useful data. It is useful
knowing what is inside this struct though! Especially since lots of old code
had to do this by hand.

### struct sockaddr

This hold socket address information for many types of sockets.

```c
struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};
```

`sa_family` can be a bunch of things, however we will only care about
`AF_INET` for IPv4 or `AF_INET6` for IPv6. `sa_data` contains a destination
address and port number for the socket.

### struct sockaddr_in

This is a parallel structure that deals **just with IPv4**

Importantly, a pointer to a `struct sockaddr_in` can be *cast* to a pointer to
a `struct sockaddr` and vice-versa.

```c
// (IPv4 only--see struct sockaddr_in6 for IPv6)

struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};
```

Note that this splits up the port and address and so is easier to work with than
`struct sockaddr`.

### struct in_addr

One step deeper! This is a filed in the `struct sockaddr_in`. What is this?
A **scary** union. At least it used to be.

```c
// (IPv4 only--see struct in6_addr for IPv6)

// Internet address (a structure for historical reasons)
struct in_addr {
    uint32_t s_addr; // that's a 32-bit int (4 bytes)
};
```

Won't look into IPv6, that can be a task for later!

### struct sockaddr_storage

Final structure (for now). This is designed to be large enough so that it can
hold both IPv4 and IPv6 structures. For some calls, you don't know whether you
will get an IPv4 or IPv6 address.

```c
struct sockaddr_storage {
    sa_family_t  ss_family;     // address family

    // all this is padding, implementation specific, ignore it:
    char      __ss_pad1[_SS_PAD1SIZE];
    int64_t   __ss_align;
    char      __ss_pad2[_SS_PAD2SIZE];
};
```

## Manipulating IP addresses

Lots of nice functions to manipulate IP addresses. No need to do things by hand

Suppose you have a `struct sockaddr_in` and an IP address you want to store into
it. Note that we need something in a `struct in_addr` format, not the typical
dot notation. We can use the function `inet_pton()` to convert our address to
the format it needs

```c
struct sockaddr_in sa;   // IPv4
struct sockaddr_in6 sa6; // IPv6

inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr));
```

> The old way of doing this used `inet_addr()` but this does not work with
> IPv6 (sad times)

Note that the above code is quite bad! There is no **error checking**. You can
check how `inet_pton` deals with error by checking the man pages, and make sure
to deal with them in actual code.

How about the other way round? You will want to use the function `inet_ntop`.
ntop in this case means "network to presentation." Here is an example for IPv4

```c
// IPv4:

char ip4[INET_ADDRSTRLEN];  // space to hold the IPv4 string
struct sockaddr_in sa;      // pretend this is loaded with something

inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);

printf("The IPv4 address is: %s\n", ip4);
```

When you call the function, you pass it: Address type, the addresss, a pointer
to a string to hold the result and the maximum length of that string.

Note the macro `INET_ADDSTRLEN` calculates this value.

## Private Networks

Lot's of places have a *firewall* that translates "internal" IP addresses to
"external" IP addresses using a process called *Network Address Translation" or
NAT.



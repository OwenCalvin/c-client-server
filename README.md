# A simple networking implementation in C
Basically it was a school project but it gives a basis to code server implementations in C

# client-server-tcp
```bash
$ cd client-server-tcp
$ make
$ ./server 4000
$ ./client 127.0.0.1 4000
$ make clean
```  
Just type some stuff inside the client console, it will be displayed on the server's one

# client-server-udp
```bash
$ cd client-server-udp
$ make
$ ./server 4000
$ ./client 127.0.0.1 4000
$ make clean
```  
Just type some stuff inside the client console, it will be displayed on the server's one

# client-http
An example to create a HTTP requester (or any other protocol)
```bash
$ cd client-http
$ make
$ make test
$ make clean
```

# Environnement
Tested on Unix based system (Mac OS)

# The project
**Networking school group:**
- [Owen Gombas](https://github.com/OwenCalvin)
- [David Darmanger](https://github.com/darmangerd)
- [Clément Brigliano](https://github.com/clms0u)

# See also
[makefiletutorial.com](https://makefiletutorial.com)
[getaddrinfo(3)](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
[tcp-server-client-implementation-in-c](https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/)

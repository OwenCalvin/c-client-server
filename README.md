# A simple networking implementation in C
Basically it was a school project but it gives a basis to code server implementations in C

# client-server-tcp
```bash
$ cd client-server-tcp
$ clang server.c -o build/server && ./build/server 4000
$ clang client.c -o build/client && ./build/client 127.0.0.1 4000
```  
Just type some stuff inside the client console, it will be displayed on the server's one

# client-server-udp
```bash
$ cd client-server-udp
$ clang server.c -o build/server && ./build/server 4000
$ clang client.c -o build/client && ./build/client 127.0.0.1 4000
```  
Just type some stuff inside the client console, it will be displayed on the server's one

# client-http
An example to create a HTTP requester (or any other protocol)
```bash
$ cd client-http
$ clang client.c -o build/client && ./build/client gandalf.teleinf.labinfo.eiaj.ch 80
```

# The project
**Networking school group:**
- [Owen Gombas](https://github.com/OwenCalvin)
- [David Darmanger](https://github.com/darmangerd)
- [Clément Brigliano](https://github.com/clms0u)

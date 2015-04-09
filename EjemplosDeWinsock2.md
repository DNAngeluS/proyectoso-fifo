Descripcion del contenido del download homonimo

# Introduccion #

Aca explica que hace exactamente cada uno de los dos procesos que encontre. Y se encuentran Levantados en el archivo de nombre homonimo.
El asunto es que en algunas partes usa stdlib.h y que es c++, por lo que las partes que usa ">>" y "<<" va a haber que cambiarlas por scanf y printf; al igual que algunas funciones  bool que habra que cambiarlas por int.

# Detalle #

**Threaded Server**

The threaded server is probably the simplest "real" server type to understand. A server program almost always needs to handle more than one connection at a time, and this is the simplest way to do that under Winsock. The reason is, since each connection gets its own thread, each thread can use simple blocking I/O on the socket. All other multi-connection server types use non-synchronous I/O of varying complexities in order to avoid thread overhead.

This server isn't that different from the basic single-connection server. The architectures are fairly different, but some of the code is identical between the two server programs.

In the basic server, the AcceptConnection() function is only two lines. The equivalent function in the threaded server, AcceptConnections(), is about 20 lines, because it sits in a loop waiting for connections, each of which it spins off into its own thread.

Each thread handles its connection more or less identically to the way the basic server handles each connection. In fact, the EchoIncomingPackets() is the same in each program. The only difference is that the main loop calls this function in the basic server, but it's called from the thread entry function in the threaded server. Similarly, the thread function shuts the connection down once the client stops sending packets; in the basic server, the main loop shuts client connections down.



**select()-Based Server**

This server uses the age-old Berkeley select() function to manage many connections within a single thread. This style of server has two big advantages over the multithreaded server: it doesn't have all the context switching overhead associated with threads, and the code is easy to port to nearly all flavors of Unix. The cost you pay for these benefits is that there's roughly twice as much code as in the threaded server.

The major difference between this server and the threaded server is that all the I/O in this version is handled in the AcceptConnections() function, rather than in a bunch of concurrent threads. In fact, AcceptConnection() is pretty much the main function in this version, whereas in the threaded server, it just accepts connections and passes them off to handler threads.

Notice that we have to keep a list of clients and an I/O buffer for each client in this version. That's the price of non-synchronous I/O: you have to keep a lot of state around so you can juggle between many connections within a single thread.

Also notice that we do not call WSAGetLastError() in this program, but instead call getsockopt() asking for the SO\_ERROR value. With select(), several sockets can go into an error state at once, so the "last" error is not necessarily the error value we're interested in. We have to find the last error that occured on a particular socket instead.
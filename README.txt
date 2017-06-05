Tomas Mantelato UNI: tm2779

Lab 7


Everything works as expected for both parts. If the server cannot open a tcp connection with the mdb-lookup-server, it will exit.
If the client asks for a directory without the '/' at the end, the server will respond with "Bad Request"

The code uses the signal library to see if the user cancels the server with control c. The signal handler function will close the socket to prevent memory error.






# XFireDB C API

The XFireDB C API can be used to interface with an
XFireDB server from a C/C++ program. An example of how to
connect to a server is given below.

	struct xfiredb_client *client;

	client = xfiredb_connect(host, port, XFIREDB_SOCK_STREAM |
			XFIREDB_AUTH | XFIREDB_SSL);
	if(!client)
	    return -1;
	
	if(xfiredb_auth_client(client, "user", "pass") != -XFIREDB_OK) {
	    xfiredb_disconnect(client);
	    return -1;
	}

	xfiredb_query(client, "set key1 \"Key 1 data\"");
	xfiredb_disconnect(client);

You can read the XFireDB CLI sources for a more detailed example.


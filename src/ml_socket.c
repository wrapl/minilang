#include "ml_socket.h"
#include "ml_object.h"
#include "ml_stream.h"
#include "ml_macros.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

ML_ENUM2(MLSocketTypeT, "socket::type",
	"Stream", SOCK_STREAM,
	"DGram", SOCK_DGRAM,
	"Raw", SOCK_RAW
);

ML_TYPE(MLSocketT, (MLStreamFdT), "socket");

ML_METHOD("listen", MLSocketT, MLIntegerT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	if (listen(Socket, ml_integer_value(Args[1])) < 0) {
		return ml_error("SocketError", "Error listening socket: %s", strerror(errno));
	}
	return Args[0];
}

extern ml_type_t MLSocketLocalT[];

ML_FUNCTION(MLSocketLocal) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSocketTypeT);
	int Socket = socket(PF_LOCAL, ml_enum_value_value(Args[0]), 0);
	if (Socket < 0) {
		return ml_error("SocketError", "Error creating socket: %s", strerror(errno));
	}
	return ml_fd_stream(MLSocketLocalT, Socket);
}

ML_TYPE(MLSocketLocalT, (MLSocketT), "socket::local",
	.Constructor = (ml_value_t *)MLSocketLocal
);

ML_METHOD("bind", MLSocketLocalT, MLStringT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	strncpy(Name.sun_path, ml_string_value(Args[1]), sizeof(Name.sun_path));
	Name.sun_path[sizeof(Name.sun_path) - 1] = 0;
	if (bind(Socket, (struct sockaddr *)&Name, SUN_LEN(&Name)) < 0) {
		return ml_error("SocketError", "Error binding socket: %s", strerror(errno));
	}
	return Args[0];
}

ML_METHOD("connect", MLSocketLocalT, MLStringT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_un Name;
	Name.sun_family = AF_LOCAL;
	strncpy(Name.sun_path, ml_string_value(Args[1]), sizeof(Name.sun_path));
	Name.sun_path[sizeof(Name.sun_path) - 1] = 0;
	if (connect(Socket, (struct sockaddr *)&Name, SUN_LEN(&Name)) < 0) {
		return ml_error("SocketError", "Error connecting socket: %s", strerror(errno));
	}
	return Args[0];
}

ML_METHOD("accept", MLSocketLocalT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_un Name;
	socklen_t Length;
	int Client = accept(Socket, (struct sockaddr *)&Name, &Length);
	if (Client < 0) {
		return ml_error("SocketError", "Error accepting socket: %s", strerror(errno));
	}
	return ml_fd_stream(MLSocketLocalT, Client);
}

extern ml_type_t MLSocketInetT[];

ML_FUNCTION(MLSocketInet) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSocketTypeT);
	int Socket = socket(PF_INET, ml_enum_value_value(Args[0]), 0);
	if (Socket < 0) {
		return ml_error("SocketError", "Error creating socket: %s", strerror(errno));
	}
	return ml_fd_stream(MLSocketInetT, Socket);
}

ML_TYPE(MLSocketInetT, (MLSocketT), "socket::inet",
	.Constructor = (ml_value_t *)MLSocketInet
);

static ml_value_t *host_address(const char *Name, struct in_addr *Address) {
	struct hostent Host, *Result;
	size_t BufferSize = 1024;
	char *Buffer = malloc(BufferSize);
	int Status, Error;
	while ((Status = gethostbyname_r(Name, &Host, Buffer, BufferSize, &Result, &Error)) == ERANGE) {
		BufferSize *= 2;
		Buffer = realloc(Buffer, BufferSize);
	}
	if (Status || !Result) {
		free(Buffer);
		return ml_error("HostnameError", "Error looking up hostname %s: %s", Name, strerror(Error));
	} else {
		*Address = *(struct in_addr *)Result->h_addr;
		free(Buffer);
		return NULL;
	}
}

ML_METHOD("bind", MLSocketInetT, MLIntegerT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(ml_integer_value(Args[1]));
	Name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(Socket, (struct sockaddr *)&Name, sizeof(Name)) < 0) {
		return ml_error("SocketError", "Error binding socket: %s", strerror(errno));
	}
	return Args[0];
}

ML_METHOD("bind", MLSocketInetT, MLStringT, MLIntegerT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(ml_integer_value(Args[2]));
	ml_value_t *Error = host_address(ml_string_value(Args[1]), &Name.sin_addr);
	if (Error) return Error;
	if (bind(Socket, (struct sockaddr *)&Name, sizeof(Name)) < 0) {
		return ml_error("SocketError", "Error binding socket: %s", strerror(errno));
	}
	return Args[0];
}

ML_METHOD("connect", MLSocketInetT, MLStringT, MLIntegerT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_in Name;
	Name.sin_family = AF_INET;
	Name.sin_port = htons(ml_integer_value(Args[2]));
	ml_value_t *Error = host_address(ml_string_value(Args[1]), &Name.sin_addr);
	if (Error) return Error;
	if (connect(Socket, (struct sockaddr *)&Name, sizeof(Name)) < 0) {
		return ml_error("SocketError", "Error connecting socket: %s", strerror(errno));
	}
	return Args[0];
}

ML_METHOD("accept", MLSocketInetT) {
	int Socket = ml_fd_stream_fd(Args[0]);
	struct sockaddr_in Name;
	socklen_t Length;
	int Client = accept(Socket, (struct sockaddr *)&Name, &Length);
	if (Client < 0) {
		return ml_error("SocketError", "Error accepting socket: %s", strerror(errno));
	}
	return ml_fd_stream(MLSocketInetT, Client);
}

void ml_socket_init(stringmap_t *Globals) {
#include "ml_socket_init.c"
	stringmap_insert(MLSocketT->Exports, "local", MLSocketLocalT);
	stringmap_insert(MLSocketT->Exports, "inet", MLSocketInetT);
	stringmap_insert(MLSocketT->Exports, "type", MLSocketTypeT);
	if (Globals) {
		stringmap_insert(Globals, "socket", MLSocketT);
	}
}

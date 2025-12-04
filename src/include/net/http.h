// Filename: src/include/net/http.h
#ifndef COME_NET_HTTP_H
#define COME_NET_HTTP_H

#include <sys/types.h>
#include <unistd.h>

// Forward declarations of structs defined in http.c
typedef struct generic_connection generic_connection;
typedef struct net_http_request net_http_request;
typedef struct net_http_response net_http_response;
typedef struct net_http_session net_http_session;

// --- Public API Functions ---

// net.http.new()
net_http_session* net_http_new(void* mem_ctx, int is_server_side);

// net.http.attach(conn) - Accepts a pointer to a generic connection (e.g., net_tcp_connection or net_tls_connection)
void net_http_attach(net_http_session* session, generic_connection* conn);

// net.http.request.send(content)
void net_http_request_send(net_http_request* req, const char* content);

// net.http.response.send(content)
void net_http_response_send(net_http_response* resp, const char* content);

// --- Event Registration Placeholders (Incoming READ -> READY) ---

// For Request object
void net_http_req_on_line_ready(net_http_request* req, void (*handler)(net_http_request*));
void net_http_req_on_header_ready(net_http_request* req, void (*handler)(net_http_request*));
void net_http_req_on_data_ready(net_http_request* req, void (*handler)(net_http_request*));
void net_http_req_on_ready(net_http_request* req, void (*handler)(net_http_request*));

// For Response object
void net_http_resp_on_line_ready(net_http_response* resp, void (*handler)(net_http_response*));
void net_http_resp_on_header_ready(net_http_response* resp, void (*handler)(net_http_response*));
void net_http_resp_on_data_ready(net_http_response* resp, void (*handler)(net_http_response*));
void net_http_resp_on_ready(net_http_response* resp, void (*handler)(net_http_response*));

// --- Event Registration Placeholders (Outgoing WRITE -> DONE) ---

// For Request object
void net_http_req_on_header_done(net_http_request* req, void (*handler)(net_http_request*));
void net_http_req_on_data_done(net_http_request* req, void (*handler)(net_http_request*));
void net_http_req_on_done(net_http_request* req, void (*handler)(net_http_request*));

// For Response object
void net_http_resp_on_header_done(net_http_response* resp, void (*handler)(net_http_response*));
void net_http_resp_on_data_done(net_http_response* resp, void (*handler)(net_http_response*));
void net_http_resp_on_done(net_http_response* resp, void (*handler)(net_http_response*));

#endif // COME_NET_HTTP_H

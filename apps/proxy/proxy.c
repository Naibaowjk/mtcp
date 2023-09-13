#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "cpu.h"
#include "http_parsing.h"
#include "netlib.h"
#include "debug.h"
#include "read_conf.h"

#define MAX_FLOW_NUM  (10000)

#define RCVBUF_SIZE (8*1024)
#define SNDBUF_SIZE (8*1024)

#define MAX_EVENTS (MAX_FLOW_NUM * 3)

#define HTTP_LEN 8096
#define URL_LEN 128
#define HOST_LEN 128

#define MAX_FILES 30

#define NAME_LIMIT 256
#define FULLNAME_LIMIT 512

#define TIMEVAL_TO_MSEC(t)		((t.tv_sec * 1000) + (t.tv_usec / 1000))
#define TIMEVAL_TO_USEC(t)		((t.tv_sec * 1000000) + (t.tv_usec))
#define TS_GT(a,b)				((int64_t)((a)-(b)) > 0)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define HT_SUPPORT FALSE

#ifndef MAX_CPUS
#define MAX_CPUS		16
#endif

int delta_port = 0;

// static int concurrency = 16;
/*----------------------------------------------------------------------------*/
struct wget_stat
{
	uint64_t waits;
	uint64_t events;
	uint64_t connects;
	uint64_t reads;
	uint64_t writes;
	uint64_t completes;

	uint64_t errors;
	uint64_t timedout;

	uint64_t sum_resp_time;
	uint64_t max_resp_time;
};
/*----------------------------------------------------------------------------*/
struct file_cache
{
	char name[NAME_LIMIT];
	char fullname[FULLNAME_LIMIT];
	uint64_t size;
	char *file;
};
/*----------------------------------------------------------------------------*/
struct server_vars
{

	int request_len;
	long int total_read, total_sent;
	uint8_t done;
	uint8_t rspheader_sent;
	uint8_t keep_alive;

	char recv_buf[HTTP_LEN];
	int recv_len;
	char send_buf[HTTP_LEN];
	int send_len;
};
/*----------------------------------------------------------------------------*/
struct wget_vars
{
	char send_buf[HTTP_LEN];
	char recv_buf[HTTP_LEN];
	int send_len;
	int recv_len;

	uint8_t keep_alive;
	uint8_t done;

	uint64_t total_sent;
	uint64_t total_read;
	struct timeval t_last;
};
/*----------------------------------------------------------------------------*/
struct thread_context
{
	mctx_t mctx;
	int ep;
	struct server_vars* svars;
	struct wget_vars* wvars;
	struct wget_stat stat;
	int target;
	int started;
	int errors;
	int incompletes;
	int done;
	int pending;
};
/*----------------------------------------------------------------------------*/
struct sockid_peer
{
	int client_sockid;  /* sockid for client */
	int server_sockid;  /* sockid for server */
	int type;		    /* type 0: client, type 1: server*/
};
/*----------------------------------------------------------------------------*/
static int num_cores;
static int core_limit;
static pthread_t app_thread[MAX_CPUS];
static int done[MAX_CPUS];
static char *conf_file = NULL;
static int backlog = -1;
/*----------------------------------------------------------------------------*/
const char *www_main;
static struct file_cache fcache[MAX_FILES];
static int nfiles;
/*----------------------------------------------------------------------------*/
static int finished;
/*----------------------------------------------------------------------------*/

static inline struct sockid_peer*
CreateConnection(struct thread_context* ctx, uint32_t daddr, uint16_t dport, int sv_sockid)
{
	mctx_t mctx = ctx->mctx;
	struct mtcp_epoll_event ev;
	struct sockaddr_in addr;
	struct sockid_peer* sp = (struct sockid_peer*)calloc(1, sizeof(struct sockid_peer));
	int sockid;
	int ret;

	

	sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		printf("Failed to create socket!\n");
		return NULL;
	}
	memset(&ctx->wvars[sockid], 0, sizeof(struct wget_vars));
	ret = mtcp_setsock_nonblock(mctx, sockid);
	if (ret < 0) {
		printf("Failed to set socket in nonblocking mode.\n");
		exit(-1);
	}
	addr.sin_family = AF_INET;

	addr.sin_addr.s_addr = config_dict->ip_dict[1].value;
	addr.sin_port = htons(1025 + (delta_port++));

	printf("[CreateConnection] sv_sockid:%d, dport:%u\n",sv_sockid, ntohs(addr.sin_port));

	ret = mtcp_bind(mctx, sockid, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
    	// 处理错误
		printf("Failed to bind to the client socket!\n");
		return NULL;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = daddr;
	addr.sin_port = dport;
	

	ret = mtcp_connect(mctx, sockid, 
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("mtcp_connect");
			mtcp_close(mctx, sockid);
			return NULL;
		}
	}

	ctx->started++;
	ctx->pending++;
	ctx->stat.connects++;

	sp->server_sockid = sv_sockid;
	sp->client_sockid = sockid;
	sp->type = 0;

	ev.events = MTCP_EPOLLOUT | MTCP_EPOLLIN;
	ev.data.ptr = (void*)sp;
	mtcp_epoll_ctl(mctx, ctx->ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);

	return sp;
}


static inline void 
CloseConnectionClient(struct thread_context* ctx, int sockid)
{
	mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
	mtcp_close(ctx->mctx, sockid);
	ctx->pending--;
	ctx->done++;

	/* 	
	assert(ctx->pending >= 0);
	while (ctx->pending < concurrency && ctx->started < ctx->target) {
		if (CreateConnection(ctx) < 0) {
			done[ctx->core] = TRUE;
			break;
		}
	} */
}

static char *
StatusCodeToString(int scode)
{
	switch (scode) {
		case 200:
			return "OK";
			break;

		case 404:
			return "Not Found";
			break;
	
		case 418:
			return "I'm a teepot";
			break;
	}

	return NULL;
}
/*----------------------------------------------------------------------------*/
void
CleanServerVariable(struct server_vars *sv)
{
	sv->recv_len = 0;
	sv->request_len = 0;
	sv->total_read = 0;
	sv->total_sent = 0;
	sv->done = 0;
	sv->rspheader_sent = 0;
	sv->send_len = 0;
	// sv->keep_alive = 0;
}

void
CleanClientVariable(struct wget_vars *wv)
{
	wv->recv_len = 0;
	wv->total_read = 0;
	wv->total_sent = 0;
	wv->done = 0;
	// wv->keep_alive = 0;
	wv->send_len = 0;
}
/*----------------------------------------------------------------------------*/
void 
CloseConnectionServer(struct thread_context *ctx, int sockid, struct server_vars *sv)
{
	mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
	mtcp_close(ctx->mctx, sockid);
}

/*----------------------------------------------------------------------------*/
static int 
SendUntilAvailable(struct thread_context *ctx, struct sockid_peer* sp)
{
	int ret;
	int sent;
	int len;
	struct server_vars* sv; 
	struct wget_vars* wv;
	int sockid;
	if (sp->type == 1)
	{
		printf("[SendUntilAvailble] Handle Send for server: sp->server_sockid = %d\n", sp->server_sockid);
		sockid = sp->server_sockid;
		sv = &ctx->svars[sp->server_sockid];
		sent = 0;
		ret = 1;
		while (ret > 0) {
			len = MIN(SNDBUF_SIZE, sv->send_len - sv->total_sent);
			if (len <= 0) {
				break;
			}
			ret = mtcp_write(ctx->mctx, sp->server_sockid,  
					sv->send_buf + sv->total_sent, len);
			if (ret < 0) {
				TRACE_APP("Connection closed with client.\n");
				break;
			}
			TRACE_APP("Socket %d: mtcp_write try: %d, ret: %d\n", sockid, len, ret);
			sent += ret;
			sv->total_sent += ret;
		}

		if (sv->total_sent >= sv->send_len) {
			struct mtcp_epoll_event ev;
			sv->done = TRUE;
			finished++;

			if (sv->keep_alive) {
				/* if keep-alive connection, wait for the incoming request */
				ev.events = MTCP_EPOLLIN;
				ev.data.ptr = (void*)sp;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sockid, &ev);

				CleanServerVariable(sv);
			} else {
				/* else, close connection */
				CloseConnectionServer(ctx, sockid, sv);
			}
		}

	}
	else
	{
		printf("[SendUntilAvailble] Handle Send for client: sp->client_sockid = %d\n", sp->client_sockid);
		sockid = sp->client_sockid;
		wv = &ctx->wvars[sockid];
		sent = 0;
		ret = 1;
		while (ret > 0) {
			len = MIN(SNDBUF_SIZE, wv->send_len - wv->total_sent);
			if (len <= 0) {
				break;
			}
			printf("wv->sendbuf:\n%s\n", wv->send_buf);
			ret = mtcp_write(ctx->mctx, sockid,  
					wv->send_buf + wv->total_sent, len);
			if (ret < 0) {
				printf("Connection closed with client.\n");
				break;
			}
			printf("Socket %d: mtcp_write try: %d, ret: %d\n", sockid, len, ret);
			sent += ret;
			wv->total_sent += ret;
		}

		if (wv->total_sent >= wv->send_len)
		{
			struct mtcp_epoll_event ev;
			wv->done = TRUE;
			finished++;

			if (wv->keep_alive)
			{
				ev.events = MTCP_EPOLLIN;
				ev.data.ptr = (void*)sp;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sockid, &ev);
				CleanClientVariable(wv);
			} 
			else 
			{
				/* should not close here */
				printf("should not be here: wv->keep_alive == 0!\n");
			}
		}
	}
	return sent;
}


/*----------------------------------------------------------------------------*/
static int
HandleReadEvent2(struct thread_context *ctx, struct sockid_peer* sp)
{
	struct sockid_peer* sp_server;
	struct sockid_peer* sp_client;
	struct mtcp_epoll_event ev_server, ev_client;
	struct server_vars* sv;
	struct wget_vars* wv;
	/* buf for mtcp_read */
	char buf[HTTP_LEN];
	int rd;
	/* change http url */
	char url[URL_LEN];
	char host[HOST_LEN];
	char url_new[URL_LEN] = "";
	char host_new[HOST_LEN];
	int url_posi, host_posi;
	int change_len;
	/* others */
	int scode;
	time_t t_now;
	char t_str[128];
	char keepalive_str[128];
    int i;
	int copy_len;

	printf("[HandleReadEvent] sp->type:%d, sp->client_sockid:%d, sp->server_sockid:%d\n", sp->type, sp->client_sockid, sp->server_sockid);
	
	/* init server_sp, client_sp */
	if (sp->type == 0) // client events
	{
		sp_client = sp;
		sp_server = (struct sockid_peer*)calloc(1, sizeof(struct sockid_peer));
		sp_server->type = 1;
		sp_server->server_sockid = sp->server_sockid;
		sp_server->client_sockid = sp->client_sockid;
		
		/* in client, server_sockid can directly use, don't need to check */
		wv = &ctx->wvars[sp->client_sockid];
		sv = &ctx->svars[sp->server_sockid];

		/* check the space we have in sv->send_len left */
		copy_len = HTTP_LEN - sv->send_len;
		rd = mtcp_read(ctx->mctx, sp_server->client_sockid, sv->send_buf + sv->send_len, copy_len);
		sv->send_len += rd;

		/* Handle Events*/
		ev_client.events = MTCP_EPOLLIN;
		ev_client.data.ptr = (void*)sp_client;
		mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_client->client_sockid, &ev_client);

		ev_server.events = MTCP_EPOLLIN | MTCP_EPOLLOUT;
		ev_server.data.ptr = (void*)sp_server;
		mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp->server_sockid, &ev_server);

		if (sv->keep_alive == FALSE)
		{
			mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_DEL, sp->client_sockid, NULL);
			mtcp_close(ctx->mctx, sp->client_sockid);
			printf("client closed! should not be there!");
		}
		return rd;

	}
	else				//server events
	{
		sp_server = sp;
		sp_client = (struct sockid_peer*)calloc(1, sizeof(struct sockid_peer));
		sp_client->type = 0;
		sp_client->client_sockid = sp->client_sockid;
		sp_client->server_sockid = sp->server_sockid;

		/* in server, we should check sp->client_id, in fault injection, not need to create connection*/
		sv = &ctx->svars[sp->server_sockid];



		/* handle fault_injection */
		if (config_dict->func_dict->fault_injection == 1)
		{
			
			/* Easy Handle, if has HTTP Header, send response */
			rd = mtcp_read(ctx->mctx, sp_server->server_sockid, sv->recv_buf, HTTP_LEN);
			sv->recv_len = rd;
			sv->request_len = find_http_header(sv->recv_buf, sv->recv_len);

			// find header
			if (sv->request_len > 0)
			{
				scode = 418;
				time(&t_now);
				strftime(t_str, 128, "%a, %d %b %Y %X GMT", gmtime(&t_now));
				/* setting keep_alive */
				sv->keep_alive = TRUE;
				if (http_header_str_val(sv->recv_buf, "Connection: ", 
						strlen("Connection: "), keepalive_str, 128)) 
				{	
					if (strstr(keepalive_str, "Keep-Alive")) 
					{
						sv->keep_alive = TRUE;
						sprintf(keepalive_str, "Keep-Alive");
					}
					else if (strstr(keepalive_str, "Close"))
					{
						sv->keep_alive = FALSE;
						sprintf(keepalive_str, "Close");
					}
					else sprintf(keepalive_str, "Keep-Alive");
				}
				sprintf(sv->send_buf, "HTTP/1.1 %d %s\r\n"
						"Date: %s\r\n"
						"Server: Webserver on Middlebox TCP (Ubuntu)\r\n"
						"Content-Length: %d\r\n"
						"Connection: %s\r\n\r\n", 
						scode, StatusCodeToString(scode), t_str, 0, keepalive_str);
				/* Handle Server Events */
				ev_server.events = MTCP_EPOLLIN | MTCP_EPOLLOUT;
				ev_server.data.ptr = (void*)sp_server;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_server->server_sockid, &ev_server);
				/* this is not sure to set */
				sv->rspheader_sent = TRUE;
			}
			return rd;
		}
		/* handle routing */
		else if (config_dict->func_dict->routing == 1)
		{
			/* should use client */
			if (sp_server->client_sockid == -1) copy_len = HTTP_LEN - 10;
			else 
			{
				wv = &ctx->wvars[sp_server->client_sockid];
				copy_len = HTTP_LEN - wv->send_len - 10; /* here -10 is using for changing url, maybe longer */
			}
			assert(copy_len >= 0);
			/* we should copy here because we need to change URL and HOST */
			rd = mtcp_read(ctx->mctx, sp_server->server_sockid, buf, copy_len);
			printf("[Routing] mtcp_read size copy_len(want): %d, rd(real): %d\n", copy_len, rd);
			/* check if recv 0 */
			if (rd <= 0)
			{
				return rd;
			}
			/* check if found header there, here must read all header */
			sv->request_len = find_http_header_noterm(buf, rd);
			
			/* find it*/
			if (sv->request_len > 0)
			{
				/* setting keep_alive */
				sv->keep_alive = TRUE;
				if (http_header_str_val(sv->recv_buf, "Connection: ", strlen("Connection: "), keepalive_str, 128)) 
				{	
						if (strstr(keepalive_str, "Keep-Alive")) sv->keep_alive = TRUE;
						else if (strstr(keepalive_str, "Close")) sv->keep_alive = FALSE;
						else sv->keep_alive = TRUE;
				}
				
				url_posi = http_get_url(buf, rd, url, URL_LEN);
				host_posi = http_get_host(buf, rd, host, HOST_LEN);

				/* find url */
				for (i = 0; i < config_dict->url_len; i++)
				{
					char url_fix[3];
					sprintf(url_fix, "s%d", i+1);
					if (strstr(url_fix, url) != NULL) break;
				}
				i += config_dict->len - config_dict->url_len;
				/* create connection */
				if (config_dict->func_dict->fault_injection == 0 && sp_server->client_sockid == -1)
				{
					sp_client = CreateConnection(ctx, config_dict->ip_dict[i].value, htons(8080), sp_server->server_sockid);
					sp_server->client_sockid = sp_client->client_sockid;
				}
				wv = &ctx->wvars[sp_server->client_sockid];
				wv->keep_alive = sv->keep_alive;
				sprintf(host_new, "%u.%u.%u.%u", 
					(config_dict->ip_dict[i].value & 0x000000FF),
					(config_dict->ip_dict[i].value & 0x0000FF00) >> 8,
					(config_dict->ip_dict[i].value & 0x00FF0000) >> 16,
            		(config_dict->ip_dict[i].value & 0xFF000000) >> 24);
				printf("[Routing] before send: send_len: %d\nsend_buf: \n%s\n", wv->send_len, wv->send_buf);
				change_len = http_change_host_url(wv->send_buf + wv->send_len, copy_len+10, buf, rd, 
									host_new, host_posi, strlen(host), 
									url_new, url_posi, strlen(url));
				printf("Write in wv->sendbuf: \n%s\n", wv->send_buf + wv->send_len);
				wv->send_len += change_len;
				printf("changed_len:%d, new wv->send_len:%d, wv->send_buf:\n%s\n", change_len, wv->send_len, wv->send_buf);
				/* server socket */
				ev_server.events = MTCP_EPOLLIN;
				ev_server.data.ptr = (void*)sp_server;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_server->server_sockid, &ev_server);

				/* client socket */
				ev_client.events = MTCP_EPOLLIN | MTCP_EPOLLOUT;
				ev_client.data.ptr = (void*)sp_client;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_server->client_sockid, &ev_client);
				return rd;
			}
			// not find
			else
			{
				wv = &ctx->wvars[sp_server->client_sockid];
				/* already copyed, memcpy to send_buf */
				memcpy(wv->send_buf + wv->send_len, (char*)buf, rd);
				printf("[HandleReadEvents - Routing] ready to send:\n%s\n", wv->send_buf + wv->send_len);
				wv->send_len += rd;

				/* server socket */
				ev_server.events = MTCP_EPOLLIN;
				ev_server.data.ptr = (void*)sp_server;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_server->server_sockid, &ev_server);

				/* client socket */
				ev_client.events = MTCP_EPOLLIN | MTCP_EPOLLOUT;
				ev_client.data.ptr = (void*)sp_client;
				mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_MOD, sp_server->client_sockid, &ev_client);
				return rd;
			}	
		}

	}
	return -1;
}
/*----------------------------------------------------------------------------*/
int 
AcceptConnection(struct thread_context *ctx, int listener)
{
	mctx_t mctx = ctx->mctx;
	struct server_vars *sv;
	struct mtcp_epoll_event ev;
	int c;
	struct sockid_peer* sp;

	c = mtcp_accept(mctx, listener, NULL, NULL);

	if (c >= 0) {
		if (c >= MAX_FLOW_NUM) {
			TRACE_ERROR("Invalid socket id %d.\n", c);
			return -1;
		}

		sv = &ctx->svars[c];
		CleanServerVariable(sv);
		TRACE_APP("New connection %d accepted.\n", c);
		sp = (struct sockid_peer*)calloc(1, sizeof(struct sockid_peer));
		sp->client_sockid = -1;
		sp->server_sockid = c;
		sp->type = 1;
		ev.events = MTCP_EPOLLIN;
		ev.data.ptr = (void*)sp;
		mtcp_setsock_nonblock(ctx->mctx, c);
		mtcp_epoll_ctl(mctx, ctx->ep, MTCP_EPOLL_CTL_ADD, c, &ev);
		TRACE_APP("Socket %d registered.\n", c);

	} else {
		if (errno != EAGAIN) {
			TRACE_ERROR("mtcp_accept() error %s\n", 
					strerror(errno));
		}
	}

	return c;
}
/*----------------------------------------------------------------------------*/
struct thread_context *
InitializeServerThread(int core)
{
	struct thread_context *ctx;
	int i;
	int ret;

	/* affinitize application thread to a CPU core */
#if HT_SUPPORT
	mtcp_core_affinitize(core + (num_cores / 2));
#else
	mtcp_core_affinitize(core);
#endif /* HT_SUPPORT */

	ctx = (struct thread_context *)calloc(1, sizeof(struct thread_context));
	if (!ctx) {
		TRACE_ERROR("Failed to create thread context!\n");
		return NULL;
	}

	/* create mtcp context: this will spawn an mtcp thread */
	ctx->mctx = mtcp_create_context(core);
	if (!ctx->mctx) {
		TRACE_ERROR("Failed to create mtcp context!\n");
		free(ctx);
		return NULL;
	}

	// for ( i = config_dict->len - config_dict->url_len; i < config_dict->len; i++)
	// {
	// 	ret = mtcp_init_rss(ctx->mctx, config_dict->ip_dict[1].value, 1, config_dict->ip_dict[i].value, htons(8080));
	// 	printf("init rss for server-%d: %d\n", i-1, ret);
	// }
	

	/* create epoll descriptor */
	ctx->ep = mtcp_epoll_create(ctx->mctx, MAX_EVENTS);
	if (ctx->ep < 0) {
		mtcp_destroy_context(ctx->mctx);
		free(ctx);
		TRACE_ERROR("Failed to create epoll descriptor!\n");
		return NULL;
	}

	/* allocate memory for server variables */
	ctx->svars = (struct server_vars *)
			calloc(MAX_FLOW_NUM, sizeof(struct server_vars));
	ctx->wvars = (struct wget_vars *)
			calloc(MAX_FLOW_NUM, sizeof(struct wget_vars));
	if (!ctx->svars || !ctx->wvars) {
		mtcp_close(ctx->mctx, ctx->ep);
		mtcp_destroy_context(ctx->mctx);
		free(ctx);
		TRACE_ERROR("Failed to create server_vars struct!\n");
		return NULL;
	}

	return ctx;
}
/*----------------------------------------------------------------------------*/
int 
CreateListeningSocket(struct thread_context *ctx)
{
	int listener;
	struct mtcp_epoll_event ev;
	struct sockaddr_in saddr;
	int ret;
	struct sockid_peer* sp;
	/* create socket and set it as nonblocking */
	listener = mtcp_socket(ctx->mctx, AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		TRACE_ERROR("Failed to create listening socket!\n");
		return -1;
	}
	ret = mtcp_setsock_nonblock(ctx->mctx, listener);
	if (ret < 0) {
		TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
		return -1;
	}

	/* bind to port 80 */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(80);
	ret = mtcp_bind(ctx->mctx, listener, 
			(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		TRACE_ERROR("Failed to bind to the listening socket!\n");
		return -1;
	}

	/* listen (backlog: can be configured) */
	ret = mtcp_listen(ctx->mctx, listener, backlog);
	if (ret < 0) {
		TRACE_ERROR("mtcp_listen() failed!\n");
		return -1;
	}
	
	/* wait for incoming accept events */
	sp = (struct sockid_peer*)calloc(1, sizeof(struct sockid_peer));
	sp->type = 1;
	sp->server_sockid = listener;
	sp->client_sockid = -1;
	ev.events = MTCP_EPOLLIN;
	ev.data.ptr = (void*)sp;
	mtcp_epoll_ctl(ctx->mctx, ctx->ep, MTCP_EPOLL_CTL_ADD, listener, &ev);

	return listener;
}
/*----------------------------------------------------------------------------*/
void *
RunServerThread(void *arg)
{
	int core = *(int *)arg;
	struct thread_context *ctx;
	mctx_t mctx;
	int listener;
	int ep;
	struct mtcp_epoll_event *events;
	int nevents;
	int i, ret;
	int do_accept;
	
	/* initialization */
	ctx = InitializeServerThread(core);
	if (!ctx) {
		TRACE_ERROR("Failed to initialize server thread.\n");
		return NULL;
	}
	mctx = ctx->mctx;
	ep = ctx->ep;

	events = (struct mtcp_epoll_event *)
			calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
	if (!events) {
		TRACE_ERROR("Failed to create event struct!\n");
		exit(-1);
	}

	listener = CreateListeningSocket(ctx);
	if (listener < 0) {
		TRACE_ERROR("Failed to create listening socket.\n");
		exit(-1);
	}

	while (!done[core]) {
		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
		if (nevents < 0) {
			if (errno != EINTR)
				perror("mtcp_epoll_wait");
			break;
		}

		do_accept = FALSE;
		printf("cpu: %d nevents: %d\n", core, nevents);
		for (i = 0; i < nevents; i++) {
			struct sockid_peer* sp = (struct sockid_peer*)events[i].data.ptr;
			printf("current events: %x\n, sp->type:%d, cid:%d, sid:%d\n", events[i].events, sp->type, sp->client_sockid, sp->server_sockid);
			int sockid = sp->type ? sp->server_sockid : sp->client_sockid;
			if (sockid == listener && sp->type == 1) {
				/* if the event is for the listener, accept connection */
				do_accept = TRUE;

			} 
			else if (events[i].events & MTCP_EPOLLERR) 
			{
				int err;
				socklen_t len = sizeof(err);

				/* error on the connection */
				TRACE_APP("[CPU %d] Error on socket %d\n", 
						core, sockid);
				if (mtcp_getsockopt(mctx, sockid, 
						SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0) {
					if (err != ETIMEDOUT) {
						fprintf(stderr, "Error on socket %d: %s\n", 
								sockid, strerror(err));
					}
				} else {
					perror("mtcp_getsockopt");
				}
				if (sp->type == 1)
					CloseConnectionServer(ctx, sockid, &ctx->svars[sockid]);
				else
					CloseConnectionClient(ctx, sockid);

			} 
			else if (events[i].events & MTCP_EPOLLIN) 
			{
				struct sockid_peer* sp= (struct sockid_peer*)events[i].data.ptr;
				ret = HandleReadEvent2(ctx, sp);

				if (ret == 0 && sp->type == 1) {
					/* connection closed by remote host */
					CloseConnectionServer(ctx, sp->server_sockid, 
							&ctx->svars[sp->server_sockid]);
				} else if (ret < 0 && sp->type == 1) {
					/* if not EAGAIN, it's an error */
					if (errno != EAGAIN) {
						CloseConnectionServer(ctx, sp->server_sockid, 
								&ctx->svars[sp->server_sockid]);
					}
				}

			} 
			else if (events[i].events & MTCP_EPOLLOUT) 
			{
				struct sockid_peer* sp= (struct sockid_peer*)events[i].data.ptr;
				printf("Handle EPOLLOUT:\n\ttype:%d server_sockid:%d, client_sockid:%d\n", sp->type, sp->server_sockid, sp->client_sockid);
				if (sp->type == 1)
				{
					// struct server_vars *sv = &ctx->svars[sp->server_sockid];
					// if (sv->rspheader_sent) {
					// 	SendUntilAvailable(ctx, sp);
					// } else {
					// 	TRACE_APP("Socket %d: Response header not sent yet.\n", 
					// 		sp->server_sockid);
					// }
					SendUntilAvailable(ctx, sp);
				}
				else
				{
					SendUntilAvailable(ctx, sp);
				}
			} 
			else 
			{
				assert(0);
			}
		}

		/* if do_accept flag is set, accept connections */
		if (do_accept) {
			while (1) {
				ret = AcceptConnection(ctx, listener);
				if (ret < 0)
					break;
			}
		}

	}

	/* destroy mtcp context: this will kill the mtcp thread */
	mtcp_destroy_context(mctx);
	pthread_exit(NULL);

	return NULL;
}
/*----------------------------------------------------------------------------*/
void
SignalHandler(int signum)
{
	int i;

	for (i = 0; i < core_limit; i++) {
		if (app_thread[i] == pthread_self()) {
			//TRACE_INFO("Server thread %d got SIGINT\n", i);
			done[i] = TRUE;
		} else {
			if (!done[i]) {
				pthread_kill(app_thread[i], signum);
			}
		}
	}
}
/*----------------------------------------------------------------------------*/
static void
printHelp(const char *prog_name)
{
	printf("%s -p <path_to_www/> -f <mtcp_conf_file> "
		     "[-N num_cores] [-c <per-process core_id>] [-h]\n",
		     prog_name);
	exit(EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
int 
main(int argc, char **argv)
{
	DIR *dir;
	struct dirent *ent;
	int fd;
	int ret;
	uint64_t total_read;
	struct mtcp_conf mcfg;
	int cores[MAX_CPUS];
	int process_cpu;
	int i, o;

	num_cores = GetNumCPUs();
	core_limit = num_cores;
	process_cpu = -1;
	dir = NULL;

	read_config();

	if (argc < 2) {
		printf("$%s directory_to_service\n", argv[0]);
		return FALSE;
	}

	while (-1 != (o = getopt(argc, argv, "N:f:p:c:b:h"))) {
		switch (o) {
		case 'p':
			/* open the directory to serve */
			www_main = optarg;
			dir = opendir(www_main);
			if (!dir) {
				printf("Failed to open %s.\n", www_main);
				perror("opendir");
				return FALSE;
			}
			break;
		case 'N':
			core_limit = mystrtol(optarg, 10);
			if (core_limit > num_cores) {
				printf("CPU limit should be smaller than the "
					     "number of CPUs: %d\n", num_cores);
				return FALSE;
			}
			/** 
			 * it is important that core limit is set 
			 * before mtcp_init() is called. You can
			 * not set core_limit after mtcp_init()
			 */
			mtcp_getconf(&mcfg);
			mcfg.num_cores = core_limit;
			mtcp_setconf(&mcfg);
			break;
		case 'f':
			conf_file = optarg;
			break;
		case 'c':
			process_cpu = mystrtol(optarg, 10);
			if (process_cpu > core_limit) {
				printf("Starting CPU is way off limits!\n");
				return FALSE;
			}
			break;
		case 'b':
			backlog = mystrtol(optarg, 10);
			break;
		case 'h':
			printHelp(argv[0]);
			break;
		}
	}
	
	if (dir == NULL) {
		printf("You did not pass a valid www_path!\n");
		exit(EXIT_FAILURE);
	}

	nfiles = 0;
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0)
			continue;
		else if (strcmp(ent->d_name, "..") == 0)
			continue;

		snprintf(fcache[nfiles].name, NAME_LIMIT, "%s", ent->d_name);
		snprintf(fcache[nfiles].fullname, FULLNAME_LIMIT, "%s/%s",
			 www_main, ent->d_name);
		fd = open(fcache[nfiles].fullname, O_RDONLY);
		if (fd < 0) {
			perror("open");
			continue;
		} else {
			fcache[nfiles].size = lseek64(fd, 0, SEEK_END);
			lseek64(fd, 0, SEEK_SET);
		}

		fcache[nfiles].file = (char *)malloc(fcache[nfiles].size);
		if (!fcache[nfiles].file) {
			printf("Failed to allocate memory for file %s\n", 
				     fcache[nfiles].name);
			perror("malloc");
			continue;
		}

		TRACE_INFO("Reading %s (%lu bytes)\n", 
				fcache[nfiles].name, fcache[nfiles].size);
		total_read = 0;
		while (1) {
			ret = read(fd, fcache[nfiles].file + total_read, 
					fcache[nfiles].size - total_read);
			if (ret < 0) {
				break;
			} else if (ret == 0) {
				break;
			}
			total_read += ret;
		}
		if (total_read < fcache[nfiles].size) {
			free(fcache[nfiles].file);
			continue;
		}
		close(fd);
		nfiles++;

		if (nfiles >= MAX_FILES)
			break;
	}

	finished = 0;

	/* initialize mtcp */
	if (conf_file == NULL) {
		printf("You forgot to pass the mTCP startup config file!\n");
		exit(EXIT_FAILURE);
	}

	ret = mtcp_init(conf_file);
	if (ret) {
		printf("Failed to initialize mtcp\n");
		exit(EXIT_FAILURE);
	}

	mtcp_getconf(&mcfg);
	if (backlog > mcfg.max_concurrency) {
		printf("backlog can not be set larger than CONFIG.max_concurrency\n");
		return FALSE;
	}

	/* if backlog is not specified, set it to 4K */
	if (backlog == -1) {
		backlog = 4096;
	}
	
	/* register signal handler to mtcp */
	mtcp_register_signal(SIGINT, SignalHandler);

	TRACE_INFO("Application initialization finished.\n");

	for (i = ((process_cpu == -1) ? 0 : process_cpu); i < core_limit; i++) {
		cores[i] = i;
		done[i] = FALSE;
		
		if (pthread_create(&app_thread[i], 
				   NULL, RunServerThread, (void *)&cores[i])) {
			perror("pthread_create");
			printf("Failed to create server thread.\n");
				exit(EXIT_FAILURE);
		}
		if (process_cpu != -1)
			break;
	}
	
	for (i = ((process_cpu == -1) ? 0 : process_cpu); i < core_limit; i++) {
		pthread_join(app_thread[i], NULL);

		if (process_cpu != -1)
			break;
	}
	
	mtcp_destroy();
	closedir(dir);
	return 0;
}

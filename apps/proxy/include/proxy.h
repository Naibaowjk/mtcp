#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
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

#define RCVBUF_SIZE (2*1024)
#define SNDBUF_SIZE (8*1024)

#define MAX_EVENTS (MAX_FLOW_NUM * 3)

#define HTTP_HEADER_LEN 1024
#define URL_LEN 128

#define MAX_FILES 30

#define NAME_LIMIT 256
#define FULLNAME_LIMIT 512

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

/*-----------------------------client poll----------------------------*/
#define MAX_CONNS 100
#define INITIAL_POOL_SIZE 1
#define EPOLL_EVENTS 10
#define PORT 8080

struct wget_vars
{
	int request_sent;

	char response[HTTP_HEADER_LEN];
    char send_buf[HTTP_HEADER_LEN];
    int send_size;
	int resp_len;
	int headerset;
	uint64_t recv;
	uint64_t write;

	struct timeval t_start;
	struct timeval t_end;
};
/*----------------------------------------------------------------------------*/
/*------------------------client_pool----------------------------*/
#ifndef HTTP_HEADER_LEN
#define HTTP_HEADER_LEN 1024
#endif // HTTP_HEADER_LEN      

#ifndef INIT_FREE_SIZE
#define INIT_FREE_SIZE 4
#endif // INIT_FREE_SIZE


#define KEEP_ALIVE_TIMEOUT 15  // this value not be checked
struct connection 
{
    int sockfd;
    int used; // 0 if not used, 1 if used
    connection_t next;
    struct wget_vars* client_vars;
};
typedef struct connection* connection_t;


struct free_connection_pool
{
    uint32_t daddr;
    uint16_t port;
    int free_size;

    connection_t head;
    connection_t tail;
};
typedef struct free_connection_pool* free_connection_pool_t;

struct connection_pool
{
    uint32_t service_count;
    free_connection_pool_t free_pool_list;
};

typedef struct connection_pool* connection_pool_t;


connection_pool_t initialize_pool();
int CreateConnection(struct thread_context* ctx, );

/*----------------------------------------------------------------------------*/
struct server_vars
{
	char request[HTTP_HEADER_LEN];
	int recv_len;
	int request_len;
	long int total_read, total_sent;
	uint8_t done;
	uint8_t rspheader_sent;
	uint8_t keep_alive;

	uint8_t is_find_header; /* if find the header already */
	char url[HTTP_HEADER_LEN];
};


/*----------------------------------------------------------------------------*/
struct thread_context
{
	mctx_t mctx;
	int ep;
	struct server_vars *svars;
	connection_pool_t conn_pool;
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

static int finished;


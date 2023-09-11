






connection* get_connection() {
    for (int i = 0; i < MAX_CONNS; i++) {
        if (!conn_pool[i].used) {
            if (conn_pool[i].sockfd == -1) {
                // Lazy initialization of the connection
                conn_pool[i].sockfd = socket(AF_INET, SOCK_STREAM, 0);
                // TODO: Add error handling
            }
            conn_pool[i].used = 1;
            return &conn_pool[i];
        }
    }
    // If we reach here, the pool is exhausted. You can choose to expand the pool, 
    // wait for a connection to free up, or return an error.
    return NULL;  // Simplified for this example
}



void release_connection(Connection* conn) {
    conn->used = 0;
}

#include "nic/e2etests/proxy/ntls.hpp"

pthread_t server_thread;
int shut_down = 0;

void *main_server(void*);

int main(int argv, char* argc[]) {
  char *portlist, *port;
  int  lport;

  setlinebuf(stdout);
  setlinebuf(stderr);

  if (argv != 3) {
    TLOG( "usage: ./tcp-server <tcp-port-list> <shutdown>\n");
    exit(-1);
  }
  portlist = argc[1];
  TLOG( "Server: Serving port-list %s\n", portlist);
  
  if(argv == 3) {
    shut_down = atoi(argc[2]);
  }
  TLOG("Server: shut_down: %d\n", shut_down);

  /*
   * Create a separate thread to handle each port for now.
   */
  port = strtok(portlist, ",");
  while (port != NULL) {
    lport = atoi(port);
    int rc = pthread_create(&server_thread, NULL, main_server, (void *)&lport);
    if (rc) {
        TLOG( "Error creating server %i\n", rc);
	exit(-1);
    }
    port = strtok(NULL, ",");
    sleep(1);
  }

  while (1) { 
	sleep(2);
  }

  return 0;
}


int OpenListener(int port)
{   int sd;
  struct sockaddr_in addr;

  sd = socket(PF_INET, SOCK_STREAM, 0);
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int optval = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

  if ( bind(sd, (const struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
      TLOG("can't bind port - %s", strerror(errno));
      abort();
    }
  if ( listen(sd, 10) != 0 )
    {
      TLOG("Can't configure listening port - %s", strerror(errno));
      abort();
    }
  TLOG( "Created listener on port %d!!\n", port);
  return sd;
}


void test_tcp(int sd)
{
  int bytes      = 0;
  int send_bytes = 0;
  int bytes_recv = 0;
  int bytes_sent = 0;

  char buf[16384];
  int i;
  do {

    bytes = recv(sd, buf, sizeof(buf), 0);/* get request */
    if ( bytes > 0 ) {
      bytes_recv += bytes;
      TLOG( "Server: Bytes recv: %i\n", bytes_recv);
    }
    else {
      ERR_print_errors_fp(stderr);
      break;
    }
    for (i = 0; i < bytes; i++) {
      printf( "%c", buf[i]);
    }
    TLOG( "  %s", buf);
    TLOG( "\n");
    send_bytes = send(sd, buf, bytes, 0);
    TLOG( "Server: Bytes sent: %i\n", send_bytes);
    bytes_sent += send_bytes; 
  } while (bytes > 0 );

}

void Servlet(int client)/* Serve the connection -- threadable */
{

  int sd = client;

  test_tcp(sd);

  do {
      sleep(5);
  } while(0);

  if(shut_down) {
    TLOG("Servlet: Shutdown and Closing socket %d\n", sd);
    shutdown(sd, SHUT_RDWR);
    close(sd);/* close connection */
  } else {
    close(sd);    
  }
  TLOG("Servlet: Done closing socket\n");

  exit(0);
}

void *main_server(void* arg)
{
  int port = *(int *)arg;
  int server = OpenListener(port);/* create server socket */
  do {
      struct sockaddr_in addr;
      unsigned int len = sizeof(addr);
      TLOG( "Waiting to accept!!\n");
      int client = accept(server, (struct sockaddr*) &addr, &len);/* accept connection as usual */
      if (client == -1) {
          TLOG("accept() failed: %s\n", strerror(errno));
	  exit(1);
      }
      TLOG("Server: Client connection accepted from sport %d\n", addr.sin_port);

      while (1) {
	Servlet(client);/* service connection in continuous loop */
      }
  } while(0);
  TLOG("Server: Closing server socket %d\n", server);
  close(server);/* close server socket */
  TLOG("Server: Done closing server socket\n");

  return NULL;
}

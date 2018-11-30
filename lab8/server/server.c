// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX 256

// Define variables:
struct sockaddr_in server_addr, client_addr, name_addr;
struct hostent *hp;

int mysock, client_sock; // socket descriptors
int serverPort;          // server port number
int r, length, n;        // help variables


char *parseInput(char input[]) {
    printf("in parseint\n\tinput: %s\n", input);
    int i = 0;
    char* tmpstr;
    char *tmp = strtok(input, " ");
    char **array = (char**)malloc(sizeof(char*)*20);
    while(tmp != NULL) {
        tmpstr = (char*)malloc(sizeof(char)*strlen(tmp));
        strcpy(tmpstr, tmp);
        printf("%s\t", tmp);
        array[i++] = tmpstr;
        tmp = strtok(NULL, " ");
    }
    array[i] = NULL;
    printf("exiting parseInt function!\n");
    printf("%s\n", array[0]);
    return array;
}

// Server initialization code:

int server_init(char *name)
{
    printf("==================== server init ======================\n");
    // get DOT name and IP address of this host

    printf("1 : get and show server host info\n");
    hp = gethostbyname(name);
    if (hp == 0)
    {
        printf("unknown host\n");
        exit(1);
    }
    printf("    hostname=%s  IP=%s\n",
           hp->h_name, inet_ntoa(*(long *)hp->h_addr));

    //  create a TCP socket by socket() syscall
    printf("2 : create a socket\n");
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    if (mysock < 0)
    {
        printf("socket call failed\n");
        exit(2);
    }

    printf("3 : fill server_addr with host IP and PORT# info\n");
    // initialize the server_addr structure
    server_addr.sin_family = AF_INET;                // for TCP/IP
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // THIS HOST IP address
    server_addr.sin_port = 0;                        // let kernel assign port

    printf("4 : bind socket to host info\n");
    // bind syscall: bind the socket to server_addr info
    r = bind(mysock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (r < 0)
    {
        printf("bind failed\n");
        exit(3);
    }

    printf("5 : find out Kernel assigned PORT# and show it\n");
    // find out socket port number (assigned by kernel)
    length = sizeof(name_addr);
    r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
    if (r < 0)
    {
        printf("get socketname error\n");
        exit(4);
    }

    // show port number
    serverPort = ntohs(name_addr.sin_port); // convert to host ushort
    printf("    Port=%d\n", serverPort);

    // listen at port with a max. queue of 5 (waiting clients)
    printf("5 : server is listening ....\n");
    listen(mysock, 5);
    printf("===================== init done =======================\n");
}

main(int argc, char *argv[])
{
    char *hostname;
    char line[MAX], copy[MAX], cwd[128];
    double arg1, arg2, sum;
    DIR *dir;
    struct dirent *ent;

    getcwd(cwd, 128);

    if (argc < 2)
        hostname = "localhost";
    else
        hostname = argv[1];

    server_init(hostname);

    // Try to accept a client request
    while (1)
    {
        printf("server: accepting new connection ....\n");

        // Try to accept a client connection as descriptor newsock
        length = sizeof(client_addr);
        client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
        if (client_sock < 0)
        {
            printf("server: accept error\n");
            exit(1);
        }
        printf("server: accepted a client connection from\n");
        printf("-----------------------------------------------\n");
        printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                ntohs(client_addr.sin_port));
        printf("-----------------------------------------------\n");

        // Processing loop: newsock <----> client
        while (1)
        {
            n = read(client_sock, line, MAX);
            if (n == 0)
            {
                printf("server: client died, server loops\n");
                close(client_sock);
                break;
            }
            if(!strcmp(line, "quit")) {
                exit(1);
            }
            strcat(line, " ECHO");
            // show the line string
            printf("server: read  n=%d bytes; line=[%s]\n", n, line);

            printf("going into parseinput\n");
            char** inputArray = parseInput(line);
            printf("\n\nout of parseint funtion\n\n");

            // example structure:
            // arg1 = atof(strtok(line, " "));
            // arg2 = atof(strtok(NULL, " "));
            // sum = arg1 + arg2;
            // char* string[MAX];
            // sprintf(string, "%lf", sum);
            // printf("arg1 = %f\narg2=%f\nsum=%f\n", arg1, arg2, sum);
            // // send the echo line to client
            // n = write(client_sock, string, MAX);

            // commands needed:
            // mkdir
            // rmdir
            // rm
            // cd
            // pwd
            // ls
            // get
            // put

            if (!strcmp(inputArray[0], "mkdir")) {
                mkdir(inputArray[1], 0755);
            } else if (!strcmp(inputArray[0], "rmdir")) {
                printf("in rmdir\n");
                rmdir(inputArray[1]);
            } else if (!strcmp(inputArray[0], "rm")) {
                printf("in rm\n");
                unlink(inputArray[1]);
            } else if (!strcmp(inputArray[0], "cd")) {
                printf("in cd\n");
		if (strcmp(inputArray[1], "")) {
			printf("Changing directory to %s",inputArray[1]);
			chdir(inputArray[1]);
			// changing CWD
			getcwd(cwd, sizeof(cwd));
            } else if (!strcmp(inputArray[0], "pwd")) {
                printf("in pwd\n");
                if (getcwd(cwd, sizeof(cwd))) {
                    printf("Current working dir: %s\n", cwd);
                    // are we just supposed to send this to the client?
                }
                printf("\n\nleaving pwd\n\n");
            } else if (!strcmp(inputArray[0], "ls")) {
                printf("in ls\n");
            } else if (!strcmp(inputArray[0], "get")) {
                printf("in get\n");
		sent_file(client_sock, inputArray[1]);
            } else if (!strcmp(inputArray[0], "put")) {
                printf("in put\n ");
		receive_file(client_sock, inputArray[1]);
            } 



            printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
            printf("server: ready for next request\n");
        }
    }
}

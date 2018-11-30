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

char *parseInput(char input[])
{
    printf("in parseint\n\tinput: %s\n", input);
    int i = 0;
    char *tmpstr;
    char *tmp = strtok(input, " ");
    char **array = (char **)malloc(sizeof(char *) * 20);
    while (tmp != NULL)
    {
        tmpstr = (char *)malloc(sizeof(char) * strlen(tmp));
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
    int n = 0, size;
    char *hostname;
    char line[MAX], copy[MAX], cwd[128], buf[MAX], str[MAX];
    double arg1, arg2, sum;
    DIR *dir;
    struct dirent *ent;
    struct stat sb;
    FILE *fp, *gp;

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
            printf("\n\ntop of program\n\n");
            n = read(client_sock, line, MAX);
            if (n == 0)
            {
                printf("server: client died, server loops\n");
                close(client_sock);
                break;
            }
            if (!strcmp(line, "quit"))
            {
                exit(1);
            }
            // show the line string
            printf("server: read  n=%d bytes; line=[%s]\n", n, line);

            printf("going into parseinput\n");
            char **inputArray = parseInput(line);
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

            if (!strcmp(inputArray[0], "mkdir"))
            {
                if (!mkdir(inputArray[1], 0755)) {
                    strcpy(line, "created directory");
                }
                else {
                    strcpy(line, "failed to create directory");
                }
            }
            else if (!strcmp(inputArray[0], "rmdir"))
            {
                printf("in rmdir\n");
                if(!rmdir(inputArray[1])) {
                    strcpy(line, "removed directory");
                } else {
                    strcpy(line, "failed to remove directory");
                }
            }
            else if (!strcmp(inputArray[0], "rm"))
            {
                printf("in rm\n");
                if (!unlink(inputArray[1])) {
                    strcpy(line, "removed file");
                } else {
                    strcpy(line, "failed to remove file");
                }
            }
            else if (!strcmp(inputArray[0], "cd"))
            {
                printf("in cd\n");
                if (strcmp(inputArray[1], ""))
                {
                    printf("Changing directory to %s", inputArray[1]);
                    chdir(inputArray[1]);
                    // changing CWD
                    getcwd(cwd, sizeof(cwd));
                }
                else
                {
                    printf("Error: No directory specified.\n");
                }
            }
            else if (!strcmp(inputArray[0], "pwd"))
            {
                printf("in pwd\n");
                if (getcwd(cwd, sizeof(cwd)))
                {
                    printf("Current working dir: %s\n", cwd);
                    // are we just supposed to send this to the client?
                    strcpy(line, cwd);
                }
            }
            else if (!strcmp(inputArray[0], "ls"))
            {
                printf("doing ls\n");
                //if no path given, ls off cwd, else ls off pathname
                if (!inputArray[1])
                {
                    printf("ls '.'\n");
                    printf("printing cwd\n");
                    dir = opendir(".");
                    printf("opened dir\n");
                    strcpy(line, "");
                    while ((ent = readdir(dir)))
                    {
                        printf("%s ", ent->d_name);
                        strcat(line, ent->d_name);
                        strcat(line, " ");
                    }
                    closedir(dir);
                    printf("\n");
                }
                else
                {
                    printf("ls %s\n", inputArray[1]);
                    printf("printing directory\n");
                    dir = opendir(inputArray[1]);
                    printf("opened dir\n");
                    strcpy(line, "");
                    while ((ent = readdir(dir)) != NULL)
                    {
                        printf("%s ", ent->d_name);
                        strcat(line, ent->d_name);
                        strcat(line, " ");
                    }
                    closedir(dir);
                    printf("\n");
                }
            }

            if (!strcmp(inputArray[0], "get"))
            {
                printf("in get\n");
                int size, fd, n;
                char buf[MAX] = {0};
                struct stat sb;

                // check that the file exists
                if (stat(inputArray[1], &sb) == 0)
                {
                    size = sb.st_size;
                }
                else
                {
                    size = 0;
                }

                // tell receiver how many bytes to ezpect during transfer
                sprintf(buf, "%d", size);
                write(client_sock, buf, MAX);

                if (size <= 0)
                {
                    return -1;
                }

                // open and read from file, send to receiver
                fd = open(inputArray[1], O_RDONLY);
                while (n = read(fd, buf, MAX))
                {
                    write(client_sock, buf, n);
                }
                close(fd);
                strcat(line, " ECHO"); 
                n = write(client_sock, line, MAX);
                printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
                printf("server: ready for next request\n");

            }
            else if (!strcmp(inputArray[0], "put"))
            {
                printf("in put\n ");
                int size, i = 0, fd, n;
                char buf[MAX] = {0};

                // Get size of the transfer from sender
                read(client_sock, buf, MAX);
                sscanf(buf, "%d", &size);

                if (size <= 0)
                    return -1;

                // write data from sender into specified file
                fd = open(inputArray[1], O_WRONLY | O_CREAT, 0655);
                while (i < size)
                {
                    n = read(client_sock, buf, MAX);
                    write(fd, buf, n);
                    i += n;
                }
                close(fd);
                strcat(line, " ECHO");
                n = write(client_sock, line, MAX);
                printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
                printf("server: ready for next request\n");
            }
            else
            {
                strcat(line, " ECHO");
                n = write(client_sock, line, MAX);
                printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
                printf("server: ready for next request\n");
            }
        }
    }
}

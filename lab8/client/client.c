// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX 256

// Define variables
struct hostent *hp;
struct sockaddr_in server_addr;

int server_sock, r;
int SERVER_IP, SERVER_PORT;

// clinet initialization code

int client_init(char *argv[])
{
    printf("======= client init ==========\n");

    printf("1 : get server info\n");
    hp = gethostbyname(argv[1]);
    if (hp == 0)
    {
        printf("unknown host %s\n", argv[1]);
        exit(1);
    }

    SERVER_IP = *(long *)hp->h_addr;
    SERVER_PORT = atoi(argv[2]);

    printf("2 : create a TCP socket\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        printf("socket call failed\n");
        exit(2);
    }

    printf("3 : fill server_addr with server's IP and PORT#\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = SERVER_IP;
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to server
    printf("4 : connecting to server ....\n");
    r = connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (r < 0)
    {
        printf("connect failed\n");
        exit(1);
    }

    printf("5 : connected OK to \007\n");
    printf("---------------------------------------------------------\n");
    printf("hostname=%s  IP=%s  PORT=%d\n",
           hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
    printf("---------------------------------------------------------\n");

    printf("========= init done ==========\n");
}

char **parseInput(char *input)
{
    int count = 0;
    char *str = NULL;
    char *tmpStr = NULL;
    // Allocate char* inputArr[20]
    char **inputArray = (char **)malloc(sizeof(char *) * 20);
    str = strtok(input, " ");
    while (str)
    {
        tmpStr = (char *)malloc(sizeof(char) * strlen(str));
        strcpy(tmpStr, str);
        inputArray[count] = tmpStr;
        count++;
        str = strtok(NULL, " ");
    }
    inputArray[count] = NULL;
    //return the parsed array
    return inputArray;
}

//get file from server
 int Get(char* filename, int server_sock)
 {
    //buffer to write
    char buf[MAX];
    int count, n;
    unsigned long size;
    //filestream for file to be sent
    FILE *fw;
    memset(buf, 0, MAX);
    

    printf("\nfilename is %s\n", filename);
    //read from socket
    read(server_sock, buf, MAX);
    printf("buf: %s\n", buf);
    //if BAD, return error
    if(strcmp(buf, "BAD") == 0)
    {
        printf("Invalid filename\n");
        return -1;
    }
    
    //get based off of KC's helper code
    count = 0;
    sscanf(buf, "%d", &size);

    //open the file
    fw = fopen(filename,"w");

    //go through the file, writing
    // while(count < size)
    // {
    //     n = read(server_sock, buf, MAX);
    //     write(fw, buf, n);
    //     count += n;
    // }
    fputs(buf, fw);
    //close file stream
    fclose(fw); 
    return n;
 }

int Put(char *filename, int server_sock)
{
    struct stat ss;
    char buf[MAX];
    //file stream for file to send
    FILE *fr;
    int total = 0, n = 0;

    //if filename is no good, return error
    if (-1 == stat(filename, &ss) || !S_ISREG(ss.st_mode))
    {
        sprintf(buf, "BAD");
        n = write(server_sock, buf, MAX);
        return -1;
    }

    //fill the buffer
    memset(buf, 0, MAX);
    sprintf(buf, "%lu", ss.st_size);
    printf("SIZE = %lu\n", (unsigned long)ss.st_size);

    //write to socket
    n = write(server_sock, buf, MAX);

    //open file for reading
    fr = fopen(filename, "r");

    //read until end of file
    while (n = fread(buf, 1, MAX, fr))
    {
        printf("client: wrote n=%d bytes; line=[%s]\n", n, buf);
        total += n;
        //write to socket
        write(server_sock, buf, n);
        memset(buf, 0, MAX);
    }

    //reset the buffer
    memset(buf, 0, MAX);
    //write the tail end of the file
    if (total < ss.st_size)
    {
        n = fread(buf, 1, (unsigned int)(ss.st_size - total), fr);
        printf("client: wrote n=%d bytes; line=(%s)\n", n, buf);
        total += n;
        write(server_sock, buf, n);
    }
    //close file stream
    fclose(fr);
    return n;
}

main(int argc, char *argv[])
{
    int n, c, i = 0, count = 0, size;
    char line[MAX], ans[MAX], copy[MAX], cwd[128], buf[MAX];
    char **token;
    char **inputArray;
    FILE *fp, *gp;
    DIR *dir;
    struct dirent *dir_entry;

    getcwd(cwd, 128);

    if (argc < 3)
    {
        printf("Usage : client ServerName SeverPort\n");
        exit(1);
    }

    client_init(argv);
    // sock <---> server
    printf("********  processing loop  *********\n");
    while (1)
    {
        printf("input a line : ");
        bzero(line, MAX);        // zero out line[ ]
        fgets(line, MAX, stdin); // get a line (end with \n) from stdin

        line[strlen(line) - 1] = 0; // kill \n at end
        if (line[0] == 0)           // exit if NULL line
            exit(0);

        strcpy(copy, line);

        inputArray = parseInput(copy);
        if (!strcmp(inputArray[0], "lcat"))
        {
            if (inputArray[1])
            {
                printf("printing\n");
                fp = fopen(inputArray[1], "r");
                if (fp != NULL)
                {
                    while ((c = fgetc(fp)) != EOF)
                    {
                        putchar(c);
                    }
                }
                fclose(fp);
                printf("\n");
            }
            else
            {
                printf("lcat Failed\n");
            }
        }
        else if (!strcmp(inputArray[0], "lpwd"))
        {
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf(stdout, "Current Working Directory: %s\n", cwd);
            else
                printf("lpwd Failed\n");
        }
        else if (!strcmp(inputArray[0], "lls"))
        {
            //if no path given, ls off cwd, else ls off pathname
            if (!inputArray[1])
            {
                printf("printing cwd\n");
                dir = opendir(".");
                while ((dir_entry = readdir(dir)) != NULL)
                    printf("%s ", dir_entry->d_name);
                closedir(dir);
                printf("\n");
            }
            else
            {
                printf("printing directory\n");
                dir = opendir(inputArray[1]);
                while ((dir_entry = readdir(dir)) != NULL)
                    printf("%s ", dir_entry->d_name);
                closedir(dir);
                printf("\n");
            }
        }
        else if (!strcmp(inputArray[0], "lcd"))
        {
            //if directory name present, chdir
            if (inputArray[1])
            {
                printf("Changing directory to %s\n", inputArray[1]);
                if (inputArray[1])
                    chdir(inputArray[1]);
                else
                    printf("no destination directory\n");
                //change current working directory
                getcwd(cwd, 128);
            }
            else
                printf("lcd Error: No Directory Specified\n");
        }
        else if (!strcmp(inputArray[0], "lmkdir"))
        {
            //check if directory name present, if so mkdir
            if (!inputArray[1])
                printf("missing directory name\n");
            else
            {
                printf("making directory\n");
                mkdir(inputArray[1], 0775);
            }
        }
        else if (!strcmp(inputArray[0], "lrmdir"))
        {
            printf("removing %s\n", inputArray[1]);
            //check if directory name present, if so rmdir
            if (inputArray[1])
                rmdir(inputArray[1]);
            else
                printf("missing directory name\n");
        }
        else if (!strcmp(inputArray[0], "lrm"))
        {
            //check if filename present, if so unlink
            if (inputArray[1])
                unlink(inputArray[1]);
            else
                printf("missing file name\n");
        }
        else if (!strcmp(inputArray[0], "get"))
        {
            n = write(server_sock, line, MAX);
            printf("client wrote n=%d bytes; line=(%s)\n", n, line);

            // Read a line from sock and show it
            n = read(server_sock, ans, MAX);
            Get(inputArray[1], server_sock);

            printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
        }
        else if (!strcmp(inputArray[0], "put"))
        {
            n = write(server_sock, line, MAX);
            printf("client: wrote n=%d bytes; line=(%s)\n", n, line);
            Put(inputArray[1], server_sock);

            // Read a line from sock and show it
            n = read(server_sock, ans, MAX);
            printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
        }
        else if (!strcmp(inputArray[0], "quit"))
        {
            exit(0);
        }
        else
        {
            // Send ENTIRE line to server
            n = write(server_sock, line, MAX);
            printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

            // Read a line from sock and show it
            printf("listening for server\n");
            n = read(server_sock, ans, MAX);
            printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
        }
    }
}

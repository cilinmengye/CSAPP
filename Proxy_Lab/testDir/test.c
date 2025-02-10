#include <stdio.h>
#include <string.h>
#define MAXLINE 8192

int main(int argc, char **argv)
{
    int i, j;
    char *uri = argv[1], hostname[MAXLINE], port[MAXLINE];
    hostname[0] = '\0';
    port[0] = '8', port[1] = '0', port[2] = '\0'; // 默认HTTP端口为80
    if (strstr(uri, "http://") != NULL) {
        for (i = 7; uri[i] != '\0' && uri[i] != '/' && uri[i] != ':'; i++) hostname[i - 7] = uri[i];
        hostname[i - 7] = '\0';
        if (uri[i] == ':') i++;
        for (j = 0; uri[i] != '\0' && uri[i] != '/'; i++, j++) port[j] = uri[i];
        if (j != 0) port[j] = '\0';
        strcpy(uri, uri + i);
        uri += i;
        if (strlen(uri) <= 0) uri[0] = '/', uri[1] = '\0';
    }
    printf("uri:%s\r\nhostname:%s\r\nport:%s\n", uri, hostname, port);
}
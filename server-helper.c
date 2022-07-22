#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BUFFER_LEN 4096
#define STATUS_OK 200
#define STATUS_NOT_FOUND 404
#define _XOPEN_SOURCE_EXTENDED 1

// returns the extension of the file
char *fileExtention(char *filepath){
    // checks for the last occurence of dot to find the file extn
    char *separtor = strrchr(filepath, '.');
    if(!separtor || separtor == filepath) return NULL;
    return separtor + 1;
}

// returns the mime type of the file extension
char *mimeType(char *extn){

    if(extn){
        // checks file extn and returns correct type
        if (strcmp(extn, "html") == 0)
            return "text/html";
        if (strcmp(extn, "jpg") == 0)
            return "image/jpeg";
        if (strcmp(extn, "css") == 0)
            return "text/css";
        if (strcmp(extn, "js") == 0)
            return "application/javascript";
    }
    // else returns octet-stream for all other types
    return "application/octet-stream";
}

// parses the request recieved by the server
int parseReq(int newsockfd, char* filepath, char* rootpath){

    // stores the request header
    char buffer[BUFFER_LEN]; 
    // stores the size of the request header
    int header_size = 0;
    // stores the request apart from header
    char discard[BUFFER_LEN];

    bzero(buffer, BUFFER_LEN);

    while(header_size < (BUFFER_LEN-1)) {
        // read the request one character at a time to check for 2CRLFs
        char ch;
        // n is the number of characters read, can be less than 1
        int n = recv(newsockfd, &ch, 1, 0);

        if (n<=0){
            return n;
        }
        if (ch == '\n'){
            break;
        }
        if (ch == '\r'){
            continue;
        }

        buffer[header_size] = ch;
        header_size++;        
    }
    // Null-terminate string
    buffer[header_size] = '\0';

    // read the rest of the request to be discarded
    recv(newsockfd, discard, BUFFER_LEN, 0);

    printf("header_size is %d\n",header_size);
    printf("%s\n", buffer);

    // extracts the http method
    char *method;
    method = strtok(buffer, " \r\n");
    printf("extracting method: %s\n",method);
    // exits if the request is not GET
    if (strncmp(method, "GET", strlen("GET")) != 0) {
        return -1;
    } 

    // extracts file path for a GET request
    int filepath_length;
    for(filepath_length = 4; filepath_length < header_size; filepath_length++){
        if(buffer[filepath_length] == ' '){
            break;
        }
        filepath[filepath_length-4] = buffer[filepath_length];
    }
    // Null-terminate string
    filepath[filepath_length-4] = '\0';

    return 1;
}

// sends the appropriate header for a request
void responseHeader(int newsockfd, char *filepath, int status){
    char *contentType;
    char *statusLine;

    // assigns the correct status and content type for a response header
    switch (status) {
        case 200:
            statusLine = "200 OK";
            contentType = mimeType(fileExtention(filepath));
            break;
        case 404:
            statusLine = "404 Not Found";
            contentType = mimeType("html");
            break;
    }

    // structures the response header
    char buffer[BUFFER_LEN];
    bzero(buffer, BUFFER_LEN);
    snprintf(buffer, BUFFER_LEN, ("HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n"),statusLine,contentType);
    printf("response header: %s", buffer);

    // sends the response header using a buffer
    char* ch = buffer;
    int bufferLen = strlen(buffer);
    while(bufferLen > 0){
        int n = send(newsockfd, ch, bufferLen,0);
        bufferLen -= n;
        ch += n;
    }
}

// serves the request by adding appropriate response body
// or file contents
void serveReq(int newsockfd, char *root, char *filepath){

    // the actual path of the file requested
    // rootpath + filepath(url)
    char path[BUFFER_LEN];
    snprintf(path, sizeof(path), "%s%s",root, filepath);
    printf("filepath is %s\n", path);

    // searches for the real path of the file
    char real_path[BUFFER_LEN];
    (void) realpath(path, real_path);

    // deals with path escape problem
    if ((strncmp(real_path, root, strlen(root))!=0)||
        (strncmp(real_path, path, strlen(filepath))!=0)){
            responseHeader(newsockfd, path, STATUS_NOT_FOUND);
        }

    // sending the file content using a buffer
    int bytes = 0;
    char buffer[BUFFER_LEN];

    FILE* file = fopen(path, "r");
    // if the file exist, sends the requested file with 200 status
    if (file) {
        // send response header
        responseHeader(newsockfd, path, STATUS_OK);

        // handle up to BUFFER_LEN bytes each time
        while ((bytes = fread(buffer, sizeof(*buffer), sizeof(buffer), file)) > 0) {
                send(newsockfd, buffer, bytes, 0);
        }
        fclose(file);
            
    }
    // file requested doesnt exist, response header sent with 404 status
    else{
        responseHeader(newsockfd, path, STATUS_NOT_FOUND);
    }
}
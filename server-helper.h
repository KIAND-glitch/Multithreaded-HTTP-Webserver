#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#define BUFFER_LEN 4096
#define STATUS_OK 200
#define STATUS_NOT_FOUND 404
#define REQUEST "GET\0"

char *mimeType(char *extn);
char *fileExtention(char *filepath);
int parseReq(int newsockfd, char* filepath, char* rootpath);
void responseHeader(int newsockfd, char *filepath, int status);
void serveReq(int newsockfd, char *root, char *filepath);

#endif

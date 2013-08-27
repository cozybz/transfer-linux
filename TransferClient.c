#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_CONN 5
#define PORT 9999

int main(int argc,char **argv)
{
    printf("client\n");
    if(argc != 2){
        printf("argument error!\n");
        exit(1);
    }

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(PORT);
    inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
        printf("connect error!\n");
        exit(1);
    }

    fd_set rset;
    char cbuf[BUFSIZ],sbuf[BUFSIZ];
    int clen=0,slen=0;



    while(1){
        FD_ZERO(&rset);
        FD_SET(sockfd,&rset);
        FD_SET(STDIN_FILENO,&rset);

        select(sockfd+1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(sockfd,&rset)){
            if((clen=read(sockfd,cbuf,BUFSIZ))==0){
                printf("connection end\n");
                exit(0);
            }else if(clen<0){
                perror("read");
                exit(0);
            }else{
                if(cbuf[0]=='/'){
                    printf("receive %s?[path] or n\n",cbuf);
                    clen = read(STDIN_FILENO,cbuf,BUFSIZ);
                    if(clen==2 && cbuf[0]=='n'){
                        write(sockfd,cbuf,clen);
                        printf("refuse file!\n");
                    }else{
                        int file;
                        char path[clen];
                        memcpy(path,cbuf,clen-1);
                        path[clen-1]='\0';
                        if((file=open(path,O_WRONLY|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR))==-1){
                            write(sockfd,"n\n",2);
                            printf("create file error!\n");
                        }else{
                            write(sockfd,"y\n",2);
                            printf("start recving!\n");

//                            int val = fcntl(sockfd,F_GETFL,0);
//                            fcntl(sockfd,F_SETFL,val|O_NONBLOCK);

                            while((clen = read(sockfd,cbuf,BUFSIZ))>0){
                                write(file,cbuf,clen);
                                if(clen<BUFSIZ)
                                    break;
                            }

//                            fcntl(sockfd,F_SETFL,val&~O_NONBLOCK);

                            printf("receive over!\n");
                        }


                    }
                }else
                    write(STDOUT_FILENO,cbuf,clen);
            }
        }

        if(FD_ISSET(STDIN_FILENO,&rset)){
            slen = read(STDIN_FILENO,sbuf,BUFSIZ);
            if(sbuf[0]=='/'){
                int file;
                char path[slen];
                memcpy(path,sbuf,slen-1);
                path[slen-1]='\0';

                if((file = open(path,O_RDONLY)) == -1){
                    printf("open error!\n");
                }else{
                    struct stat buf;
                    fstat(file,&buf);
                    if(S_ISDIR(buf.st_mode)){
                        printf("error,it should not be a dir!\n");
                    }else{
                        write(sockfd,path,slen);
                        slen = read(sockfd,sbuf,BUFSIZ);
                        if(slen==2 && sbuf[0]=='n'){
                            printf("remote refuse!\n");
                        }else{
                            printf("remote is recving!\n");
                            while((slen = read(file,sbuf,BUFSIZ))>0){
                                write(sockfd,sbuf,slen);
                            }
                            close(file);
                            printf("send over!\n");
                        }
                    }
                }
            }else
                write(sockfd,sbuf,slen);
        }
    }
    exit(0);
}


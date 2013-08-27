#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_CONN 5
#define PORT 9999

void sig_chld(int);

int main(void)
{
    printf("server\n");
    int servfd,clifd;
    pid_t childpid;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t clilen;
    signal(SIGCHLD,sig_chld);

    servfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(PORT);
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    bind(servfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    listen(servfd,MAX_CONN);


    while(1){

        clilen = sizeof(cliaddr);
        if((clifd = accept(servfd,(struct sockaddr *)&cliaddr,&clilen))<0){
            if(errno==EINTR)
                continue;
            else{
                perror("accept");
                exit(0);
            }
        }

        if((childpid=fork())==0){
            close(servfd);
            fd_set rset;
            char cbuf[BUFSIZ],sbuf[BUFSIZ];
            int clen=0,slen=0;


            while(1){
                FD_ZERO(&rset);
                FD_SET(clifd,&rset);
                FD_SET(STDIN_FILENO,&rset);

                select(clifd+1,&rset,NULL,NULL,NULL);

                if(FD_ISSET( clifd,&rset)){
                    if((clen=read(clifd,cbuf,BUFSIZ))==0){
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
                                write(clifd,cbuf,clen);
                                printf("refuse file!\n");
                            }else{
                                int file;
                                char path[clen];
                                memcpy(path,cbuf,clen-1);
                                path[clen-1]='\0';
                                if((file=open(path,O_WRONLY|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR))==-1){
                                    printf("create file error!\n");
                                    write(clifd,"n\n",2);
                                }else{
                                    write(clifd,"y\n",2);
                                    printf("start recving!\n");

//                                    int val = fcntl(clifd,F_GETFL,0);
//                                    fcntl(clifd,F_SETFL,val|O_NONBLOCK);

                                    while((clen = read(clifd,cbuf,BUFSIZ))>0){
                                        write(file,cbuf,clen);
                                        if(clen<BUFSIZ)
                                            break;
//                                        printf("clen%d bufzide:%d\n",clen,BUFSIZ);
                                    }

//                                    fcntl(clifd,F_SETFL,val&~O_NONBLOCK);

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
                                write(clifd,path,slen);
                                slen = read(clifd,sbuf,BUFSIZ);
                                if(slen==2 && sbuf[0]=='n'){
                                    printf("remote refuse!\n");
                                }else{
                                    printf("remote is recving!\n");
                                    while((slen = read(file,sbuf,BUFSIZ))>0){
                                        write(clifd,sbuf,slen);
                                    }
                                    close(file);
                                    printf("send over!\n");
                                }
                            }
                        }
                    }else
                        write(clifd,sbuf,slen);
                }
            }
            exit(0);
        }
        close(clifd);
    }
}

void sig_chld(int signo){
    while(waitpid(-1,NULL,WNOHANG)>0);
}









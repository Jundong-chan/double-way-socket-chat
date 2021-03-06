/*这是一个客户端使用程序，在服务器做好端口监听后*/
/*启用这个程序，即可进行连接，具体操作见说明*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      //hostent结构体定义在该头文件中
#include <pthread.h>

//下面给出hostent结构体的定义，以便了解
/*struct hostent    
{ 
  char * h_name; / *主机的正式名称* / 
  char ** h_aliases; / *别名列表* / 
  int h_addrtype; / *主机地址类型* / 
  int h_length; / *地址的长度* / 
  char ** h_addr_list; / *来自名称服务器的地址列表* / 
  #define h_addr h_addr_list [0] / *地址，以实现向后兼容* / 
};*/

//传给线程的参数结构
struct parament             
{
    int socketfd;       /*socket文件描述符*/
    char *servername;   /*服务器主机名*/
    char *clientname;   /*客户端主机名*/
};
struct parament init_pthread_parament(int fd,char *sername); //初始化线程参数结构体
void pthread_send(void *arg);   //发送消息线程
void pthread_recv(void *arg);   //接收消息线程
void error(const char *msg)     //出错处理
{
    perror(msg);
    exit(0);
}
//客户端不需要直到自己建立连接的端口号，connect会自动分配，只需要服务器开放这个服务的端口号就可以
int main(int argc, char *argv[])    //运行时一共传三个参数，1、clinet，2、服务器的主机名，3、服务器的端口号
{
    int sockfd, portno;      //socket文件描述符，服务器提供的端口，
    struct sockaddr_in serv_addr;   //sockaddr_in是包含了互联网地址的结构
    struct hostent *server;     //hostent记录了主机各种信息
    
    char buffer[256];       //用于缓存消息
    pthread_t pthsend,pthrecv;  //两个线程，发送数据和接收数据

    //如传入的参数少于3个，出错
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);     //第三个参数是端口号，将字符串转成数字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //1、创建socket
    if (sockfd < 0) 
        error("ERROR opening socket");
    //2、域名解析
    server = gethostbyname(argv[1]);        //hostent结构是gethostbyname()函数的返回值，该函数做到了域名解析，并返回该域名主机的相关信息。
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    //3、给serv_addr结构体赋值
    bzero((char *) &serv_addr, sizeof(serv_addr));     //清空结构体里的数据
    serv_addr.sin_family = AF_INET;     //AF_INET代表IPV4
    bcopy((char *)server->h_addr,               //将参数1的前n个字节复制到参数2中
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(portno);     //传入端口号
    //4、建立连接
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //connect()连接服务器
        error("ERROR connecting");
    //5、连接成功后，初始化线程的参数，并创建两个线程
    struct parament para;
    para=init_pthread_parament(sockfd,argv[1]);     //初始化多线程传入的参数的列表
    int ret;
    ret=pthread_create(&pthsend,NULL,(void *)pthread_send,(void*)&para);      
    if(ret!=0)
    {
        perror("can't create send thread\n");
    }
    ret=pthread_create(&pthsend,NULL,(void *)pthread_recv,(void*)&para); 
    if(ret!=0)
    {
        perror("can't create recev thread\n");
    }

    pthread_join(pthsend,NULL);      //等待子线程的结束
    close(sockfd);
    return 0;
}

void pthread_send(void *arg)
{
    struct parament par;
    int n;
    char buffer[256];  //缓冲区
    par=*((struct parament *)arg);     //传入的参数转回原来的类型
    printf("%s:welcome,please write your message\n",par.clientname);
    while(1)
    {
        bzero(buffer,256);
        buffer[0]='c';          //在消息头部放一个标志，s表示消息是服务器的
        fgets(buffer+1,254,stdin);        //stdin实现对终端输入的数据进行读入操作
        n = write(par.socketfd,buffer,strlen(buffer));    //写消息
        if (n < 0) 
            printf("ERROR writing to socket");
    }
}

void pthread_recv(void *arg)
{
    struct parament par;    //主线程传进的参数结构
    int n;
    char buffer[256];  //缓冲区
    par=*((struct parament *)arg);     //传入的参数转回原来的类型
   
    while(1)
    {
        bzero(buffer,256);
        n = read(par.socketfd,buffer,255);
        if (n < 0) 
            error("ERROR reading from socket");
        if(buffer[0]=='s')      //判断是从客户端发来的消息
        {
            printf("%s:%s\n",par.servername,buffer+1);
        }
    }
}

struct parament init_pthread_parament(int fd,char *sername)
{
    struct parament para;
    char hostnambuff[256];      //本机主机名 
    gethostname(hostnambuff,sizeof(hostnambuff));
    para.clientname=hostnambuff;
    para.servername=sername;
    para.socketfd=fd;
    return para;

}
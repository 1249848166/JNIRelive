#include <jni.h>
#include <string>

#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <android/log.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/signal.h>
#include <android/log.h>
#include <sys/un.h>
#define LOG_TAG "tuch"
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

void child_work();
int child_create_channel();
void child_listen();

const char* userid;
const char* PATH="data/data/mf.com.jnirelive/process.sock";
int client;
int clientfd;

extern "C"
JNIEXPORT void JNICALL
Java_mf_com_jnirelive_NDK_createReliveProcess(JNIEnv *env, jclass type, jstring userid_) {
    userid = env->GetStringUTFChars(userid_, 0);
    pid_t  pid=fork();
    if(pid<0){

    }else if(pid==0){//子进程
        child_work();
    }else if(pid>0){//父进程

    }
    env->ReleaseStringUTFChars(userid_, userid);
}

void child_work() {
    //开启服务端
    if(child_create_channel()){
        child_listen();
    }
}

//接收信息
void child_listen() {
    fd_set fds;
    struct timeval timeout={
            3,0
    };
    while(1){
        FD_ZERO(&fds);
        FD_SET(client,&fds);
        //选择客户端
        int r=select(client+1,&fds,NULL,NULL,&timeout);
        LOGE("读取消息前 %d",r);
        if(r>0){
            if(FD_ISSET(client,&fds)){//保证读到信息属于指定客户端
                char pkg[256]={0};
                int result=read(client,pkg, sizeof(pkg));
                execlp("am","am","startservice","--user",userid,
                       "mf.com.jnirelive/mf.com.jnirelive.ReliveService",(char*)NULL);
                break;
            }
        }
    }
}

//创建socket
int child_create_channel() {
    int listenfd=socket(AF_LOCAL,SOCK_STREAM,0);
    unlink(PATH);
    struct sockaddr_un addr;
    memset(&addr,0, sizeof(sockaddr_un));
    addr.sun_family=AF_LOCAL;
    strcpy(addr.sun_path,PATH);
    if(bind(listenfd,(const sockaddr *)&addr, sizeof(sockaddr_un))<0){
        LOGE("绑定错误");
        return 0;
    }
    listen(listenfd,5);//可以守护5个进程
    //保证进程连接成功
    while (1){
        if((clientfd=accept(listenfd,NULL,NULL))<0){//返回客户端地址
            if(errno==EINTR){
                continue;
            }else{
                LOGE("读取错误");
                return 0;
            }
        }
        client=clientfd;
        LOGE("连接成功 %d",client);
        break;
    }
    return 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_mf_com_jnirelive_NDK_connect(JNIEnv *env, jclass type) {
    int socked;
    while(1){
        LOGE("客户端父进程开始连接");
        socked=socket(AF_LOCAL,SOCK_STREAM,0);
        if(socked<0){
            LOGE("连接失败");
            return;
        }
        struct  sockaddr_un addr;
        memset(&addr,0,sizeof(sockaddr));
        addr.sun_family=AF_LOCAL;
        strcpy(addr.sun_path,PATH);
        if(connect(socked,(const sockaddr *)&addr,sizeof(addr))<0){
            LOGE("连接失败");
            close(socked);
            sleep(1);
            //重新连接
            continue;
        }
        LOGE("连接成功");
        break;
    }
}
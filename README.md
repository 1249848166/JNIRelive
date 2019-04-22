## 说明
双进程守护当用户将app在活动界面划掉后，进程死了，服务也就挂了。在linux层开启守护进程可以绕过安卓端规则。
创建ndk项目，然后创建一个服务，这个服务在后台执行想要执行的代码。

```java
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Process;
import android.util.Log;

import java.util.Timer;
import java.util.TimerTask;

public class ReliveService extends Service {

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        NDK.createReliveProcess(""+Process.myUid());//创建客户端进程
        NDK.connect();//连接客户端进程
        Timer timer=new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                while(true){
                    try {
                        Thread.sleep(2000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    Log.e("服务进程","正在运行");
                }
            }
        },0);
    }
}
```

写java和c交互的接口，这里需要两个方法：创建linux层的守护进程客户端，连接客户端

```java
public class NDK {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void createReliveProcess(String userid);//创建客户端
    public static native void connect();//连接客户端
}
```

实现linux层代码，复制粘贴就行

```java
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
```

原理是通过fork函数开启双进程，然后创建服务端socket（守护进程作为服务端），socket通过bind后，accept客户端（我们的服务进程），然后与客户端连接。当客户端被kill后调用方法execlp("am","am","startservice","--user",userid,
       "mf.com.jnirelive/mf.com.jnirelive.ReliveService",(char*)NULL);
重新启动。
测试这个方法比较靠谱，但是在安卓高版本依旧不行，但是适用比较高。除非向支付宝微信qq这样让手机厂商加白名单的方式可以保证永驻后台。不同Rom手机可能有不同效果，失败几率还是不小的。
package mf.com.jnirelive;

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

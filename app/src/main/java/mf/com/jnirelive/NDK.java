package mf.com.jnirelive;

public class NDK {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void createReliveProcess(String userid);
    public static native void connect();
}

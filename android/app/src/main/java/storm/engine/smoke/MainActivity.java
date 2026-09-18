package storm.engine.smoke;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("storm_android_smoke");
    }

    private SurfaceView surfaceView;

    private static native void nativeStart(android.view.Surface surface);
    private static native void nativeStop();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeStart(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // The native renderer queries the current ANativeWindow dimensions each frame.
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeStop();
    }

    @Override
    protected void onDestroy() {
        nativeStop();
        super.onDestroy();
    }
}

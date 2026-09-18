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
    private SurfaceHolder surfaceHolder;
    private boolean surfaceReady;

    private static native void nativeStart(android.view.Surface surface);
    private static native void nativeStop();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        surfaceView = new SurfaceView(this);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        setContentView(surfaceView);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        surfaceReady = true;
        if (!isFinishing()) nativeStart(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // Native code reads the current ANativeWindow size each frame.
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        surfaceReady = false;
        nativeStop();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (surfaceReady && surfaceHolder != null && !isFinishing()) {
            nativeStart(surfaceHolder.getSurface());
        }
    }

    @Override
    protected void onPause() {
        nativeStop();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        nativeStop();
        super.onDestroy();
    }
}

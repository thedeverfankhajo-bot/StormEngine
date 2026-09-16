package storm.engine.smoke;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static {
        System.loadLibrary("storm_android_smoke");
    }

    private static native String nativeRunSmoke();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView view = new TextView(this);
        view.setText(nativeRunSmoke());
        view.setTextSize(14.0f);
        setContentView(view);
    }
}

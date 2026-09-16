package storm.engine.smoke;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static { System.loadLibrary("storm_android"); }

    private static native String runGpuSmokeTest();

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        TextView view = new TextView(this);
        view.setText(runGpuSmokeTest());
        view.setTextSize(18.0f);
        setContentView(view);
    }
}

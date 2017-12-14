package imt.tangodepthmap;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.graphics.Point;
import android.hardware.Camera;
import android.hardware.display.DisplayManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SeekBar;

import java.io.File;

public class TangoDepthMapActivity extends AppCompatActivity {
    private static final String TAG = TangoDepthMapActivity.class.getSimpleName();

    // The minimum Tango Core version required from this application.
    private static final int MIN_TANGO_CORE_VERSION = 9377;

    // For all current Tango devices, color camera is in the camera id 0.
    private static final int COLOR_CAMERA_ID = 0;

    private GLSurfaceView mGLView;

    private SeekBar mDepthOverlaySeekbar;
    private CheckBox mDepthmapCheckbox;
    private CheckBox mRecordCheckbox;

    private String mPath;

    // Tango Service connection.
    ServiceConnection mTangoServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName name, IBinder service) {
            TangoJNINative.onTangoServiceConnected(service);
            setAndroidOrientation();
        }

        public void onServiceDisconnected(ComponentName name) {
            // Handle this if you need to gracefully shutdown/retry
            // in the event that Tango itself crashes/gets upgraded while running.
        }
    };

    private class DepthOverlaySeekbarListener implements SeekBar.OnSeekBarChangeListener {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress,
                                      boolean fromUser) {
            //Sets minimum value to 500cm or 0.5m
            TangoJNINative.setRenderingDistanceValue(progress + 500);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
        }
    }

    private class DepthmapCheckboxListener implements CheckBox.OnCheckedChangeListener {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            if (buttonView == mDepthmapCheckbox) {
                //If the checkbox is checked
                if (isChecked) {

                    //Display the depth image and the depth cursor, and hide the color image
                    TangoJNINative.setDepthAlphaValue(1.0f);
                    mDepthOverlaySeekbar.setVisibility(View.VISIBLE);
                } else {
                    //Display the color image and hide the depth image and the depth cursor
                    TangoJNINative.setDepthAlphaValue(0.0f);
                    mDepthOverlaySeekbar.setVisibility(View.GONE);
                }
            }
        }
    }

    private class RecordCheckboxListener implements CheckBox.OnCheckedChangeListener {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            if (buttonView == mRecordCheckbox) {
                //If the checkbox is checked
                if (isChecked) {

                    TangoJNINative.setRecordingMode(true);

                } else {

                    TangoJNINative.setRecordingMode(false);

                }
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, TAG);
        super.onCreate(savedInstanceState);

        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        DisplayManager displayManager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        if (displayManager != null) {
            displayManager.registerDisplayListener(new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {

                }

                @Override
                public void onDisplayChanged(int displayId) {
                    synchronized (this) {
                        setAndroidOrientation();
                    }
                }

                @Override
                public void onDisplayRemoved(int displayId) {
                }
            }, null);
        }

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_main);

        mDepthOverlaySeekbar = (SeekBar) findViewById(R.id.depth_overlay_alpha_seekbar);
        mDepthOverlaySeekbar.setOnSeekBarChangeListener(new DepthOverlaySeekbarListener());
        mDepthOverlaySeekbar.setVisibility(View.GONE);

        mDepthmapCheckbox = (CheckBox) findViewById(R.id.depthmap_checkbox);
        mDepthmapCheckbox.setOnCheckedChangeListener(new DepthmapCheckboxListener());
        mDepthmapCheckbox.setChecked(false);

        mRecordCheckbox = (CheckBox) findViewById(R.id.record_checkbox);
        mRecordCheckbox.setOnCheckedChangeListener(new RecordCheckboxListener());
        mRecordCheckbox.setChecked(false);

        // OpenGL view where all of the graphics are drawn
        mGLView = (GLSurfaceView) findViewById(R.id.surfaceview);

        // Configure OpenGL renderer
        mGLView.setEGLContextClientVersion(2);
        mGLView.setRenderer(new TangoDepthMapRenderer(this));

        File folder =  getExternalFilesDir(Storage.FOLDER_NAME);
        String path = Storage.getFilePath(folder);
        Log.i(TAG, path);

        File truc =  getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        mPath = truc.getAbsolutePath();

        TangoJNINative.onCreate(this, mPath);
    }

    @Override
    protected void onResume() {
        // We moved most of the onResume lifecycle calls to the surfaceCreated,
        // surfaceCreated will be called after the GLSurface is created.
        super.onResume();
        mGLView.onResume();
        TangoInitializationHelper.bindTangoService(this, mTangoServiceConnection);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLView.onPause();
        TangoJNINative.onPause();
        unbindService(mTangoServiceConnection);
    }

    public void surfaceCreated() {
        TangoJNINative.onGlSurfaceCreated();
    }

    // Pass device's camera sensor rotation and display rotation to native layer.
    // These two parameter are important for Tango to render video overlay and
    // virtual objects in the correct device orientation.
    private void setAndroidOrientation() {
        Display display = getWindowManager().getDefaultDisplay();
        Camera.CameraInfo colorCameraInfo = new Camera.CameraInfo();
        Camera.getCameraInfo(COLOR_CAMERA_ID, colorCameraInfo);

        TangoJNINative.onDisplayChanged(display.getRotation(), colorCameraInfo.orientation);
    }
}

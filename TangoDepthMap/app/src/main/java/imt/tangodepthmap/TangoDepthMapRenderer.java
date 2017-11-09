package imt.tangodepthmap;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Render onGlSurfaceDrawFrame all GL content to the glSurfaceView.
 */
public class TangoDepthMapRenderer implements GLSurfaceView.Renderer {

    // Render loop of the Gl context.
    public void onDrawFrame(GL10 gl) {
        TangoJniNative.onGlSurfaceDrawFrame();
    }

    // Called when the surface size changes.
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        TangoJniNative.onGlSurfaceChanged(width, height);
    }

    // Called when the surface is created or recreated.
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        TangoJniNative.onGlSurfaceCreated();
    }
}
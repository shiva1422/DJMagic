package com.kalasoft.djmagic;

import androidx.appcompat.app.AppCompatActivity;

import android.app.NativeActivity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;

import java.io.InputStream;

public class MainActivity extends NativeActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }


    Bitmap importImageFromAssets(String fileName)
    {
        Bitmap bitmap=null;
        try
        {
            InputStream is=getAssets().open(fileName);
            bitmap= BitmapFactory.decodeStream(is);//////checked if scaled to density;

        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return bitmap;

    }

    float[] getDisplayParams()
    {
        float[] displayParams = new float[8];
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(displayMetrics);
        displayParams[0] = displayMetrics.widthPixels;
        displayParams[1] = displayMetrics.heightPixels;
        displayParams[2] = displayMetrics.density;
        displayParams[3] = displayMetrics.densityDpi;
        displayParams[4] = displayMetrics.DENSITY_DEVICE_STABLE;
        displayParams[5] = displayMetrics.scaledDensity;
        displayParams[6] = displayMetrics.xdpi;
        displayParams[7] = displayMetrics.ydpi;
        return displayParams;

    }

    public void hideSystemUI() {

        // Enables regular immersive mode.
        // For "lean back" mode, remove SYSTEM_UI_FLAG_IMMERSIVE.
        // Or for "sticky immersive," replace it with SYSTEM_UI_FLAG_IMMERSIVE_STICKY

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {

                    View decorView = getWindow().getDecorView();
                    decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                            // Set the content to appear under the system bars so that the
                            // content doesn't resize when the system bars hide and show.
                            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            // Hide the nav bar and status bar
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN);
                } catch (Exception e) {
                    e.printStackTrace();
                }

            }
        });


    }

    public void showSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }

}
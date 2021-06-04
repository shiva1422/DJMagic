package com.kalasoft.djmagic;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.app.NativeActivity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;

import java.io.InputStream;

public class MainActivity extends NativeActivity {

    final int PERMISSION_REQUEST_STORAGE=1422,IMPORTVIDEO=1433;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        requestStoragePermission();
      //  openGallery();
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
    public void openGallery()
    {
        Intent galleryIntent=new Intent();
        galleryIntent.setType("video/*");
        galleryIntent.setAction(Intent.ACTION_GET_CONTENT);
        startActivityForResult(Intent.createChooser(galleryIntent,"select Video"),IMPORTVIDEO);
    }

    public void requestStoragePermission()
    {
        // Check if the  permission has been granted
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED&&ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED)
        {
            // Permission is already available, start work else rquest
        }
        else
        {
            //request permision
            // Permission has not been granted and must be requested.
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE)&&ActivityCompat.shouldShowRequestPermissionRationale(this,Manifest.permission.WRITE_EXTERNAL_STORAGE))
            {

            }
            else
            {
                // Snackbar.make(mLayout, R.string.camera_unavailable, Snackbar.LENGTH_SHORT).show();
                // Request the permission. The result will be received in onRequestPermissionResult().
                ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_STORAGE);
            }
        }
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
    {
        // BEGIN_INCLUDE(onRequestPermissionsResult)
        if (requestCode == PERMISSION_REQUEST_STORAGE) {
            // Request for camera permission.
            if (grantResults.length == 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Permission has been granted. Start camera preview Activity.
                Log.e("Permission", "Granted");
                //startCamera();
            } else {
                // Permission request was denied.

            }
        }


    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        Log.e("Activity Result","obtained");
        if(requestCode==IMPORTVIDEO && resultCode==RESULT_OK)
        {


        }
        /*else if(requestCode==&&requestCode==RESULT_OK)
        {
            Log.e("image ","intent result on save");
        }*/


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
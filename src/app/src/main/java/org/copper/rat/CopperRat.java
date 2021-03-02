package org.copper.rat;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.jar.Pack200;

import android.app.*;
import android.content.*;
import android.content.res.Resources;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.AbsoluteLayout;
import android.os.*;
import android.provider.OpenableColumns;
import android.util.DisplayMetrics;
import android.util.Log;
import android.graphics.*;
import android.media.*;
import android.hardware.*;

import org.libsdl.app.SDLActivity;

class ResourceTuple{
    public int id;
    public String path;
    ResourceTuple(int id, String path){
        this.id = id;
        this.path = path;
    }
}

public class CopperRat extends org.libsdl.app.SDLActivity {
    public double getScreenDensity(){
    	DisplayMetrics metric = getResources().getDisplayMetrics();
    	double ret = metric.densityDpi / 25.4;
    	return ret;
    }
    
    public int getScreenWidth(){
    	Display display = getWindowManager().getDefaultDisplay();
    	Point size = new Point();
    	display.getSize(size);
    	return size.x;
    }

    public int getScreenHeight(){
    	Display display = getWindowManager().getDefaultDisplay();
    	Point size = new Point();
    	display.getSize(size);
    	return size.y;
    }

    public static void initializeAppDirectory(){
        ArrayList<ResourceTuple> list = new ArrayList<ResourceTuple>();
        list.add(new ResourceTuple(R.raw.unifont, "unifont.dat"));
        list.add(new ResourceTuple(R.raw.button_load, "button_load.png"));
        list.add(new ResourceTuple(R.raw.button_next, "button_next.png"));
        list.add(new ResourceTuple(R.raw.button_pause, "button_pause.png"));
        list.add(new ResourceTuple(R.raw.button_play, "button_play.png"));
        list.add(new ResourceTuple(R.raw.button_previous, "button_previous.png"));
        list.add(new ResourceTuple(R.raw.button_seekback, "button_seekback.png"));
        list.add(new ResourceTuple(R.raw.button_seekforth, "button_seekforth.png"));
        list.add(new ResourceTuple(R.raw.button_stop, "button_stop.png"));

        Application app = SDLActivity.mSingleton.getApplication();
        String base = app.getFilesDir().getPath() + "/";

        for (ResourceTuple resourceTuple : list) {
            String path = base + resourceTuple.path;
            if (new File(path).exists()){
                Log.i("Resources", "Skipping file " + path);
                continue;
            }
            Log.i("Resources", "Copying file " + path);
            copyResourceToFileSystem(app, resourceTuple.id, resourceTuple.path);
        }
    }

    private static void copyResourceToFileSystem(Application app, int id, String destinationPath){
        InputStream stream;
        FileOutputStream file;
        try{
            stream = app.getResources().openRawResource(id);
            file = app.openFileOutput(destinationPath, Application.MODE_WORLD_READABLE);
            byte[] buffer = new byte[1<<12];
            int bytes = 0;
            while (true){
                int bytesRead = stream.read(buffer);
                if (bytesRead <= 0)
                    break;
                file.write(buffer, 0, bytesRead);
                bytes += bytesRead;
            }
            Log.i("Resources", "Written " + bytes + " bytes.");
        }catch (Exception e){
            Log.e("Resources", "There was an exception! " + e.toString());
        }
    }
}

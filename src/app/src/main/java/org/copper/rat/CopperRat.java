/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

package org.copper.rat;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.jar.Pack200;

import android.app.*;
import android.content.*;
import android.content.res.Resources;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
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

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.app.NotificationCompat;

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
    private static CopperRat instance;

    public static CopperRat getInstance(){
        return instance;
    }

    public CopperRat(){
        instance = this;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        createChannel();
        this.sendNotification();
        init_settings();
        init_player();
    }

    @Override
    protected void onDestroy() {
        stopService(this.i);
        if (this.notification != null) {
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            manager.cancelAll();
            this.notification = null;
        }
        super.onDestroy();
    }

    private void createChannel(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            createChannelO();
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    private void createChannelO(){
        String id = getString(R.string.channel_id);
        CharSequence name = getString(R.string.channel_name);
        String description = getString(R.string.channel_description);
        int importance = NotificationManager.IMPORTANCE_HIGH;
        NotificationChannel channel = new NotificationChannel(id, name, importance);
        channel.setDescription(description);
        channel.enableLights(false);
        channel.setLightColor(Color.RED);
        channel.enableVibration(false);
        NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.createNotificationChannel(channel);
    }

    public Notification notification = null;

    private void sendNotification(){
        try {
            if (this.notification != null)
                return;

            NotificationCompat.Builder builder = new NotificationCompat.Builder(this, getString(R.string.channel_id))
                    .setContentTitle("CopperRat")
                    .setContentText("CopperRat is playing music.")
                    .setSmallIcon(R.mipmap.ic_launcher2)
                    .setPriority(NotificationCompat.PRIORITY_MAX)
                    .setOngoing(true)
                    .setNotificationSilent();

            Intent notificationIntent = new Intent(this, CopperRat.class);
            PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);
            builder.setContentIntent(contentIntent);

            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            this.notification = builder.build();
            manager.notify(0, this.notification);
        } catch (Throwable t) {
            Log.e("Notification test", t.getMessage());
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.N)
    public String getExternalStoragePathN(){
        StorageManager sm = (StorageManager)getContext().getSystemService(Context.STORAGE_SERVICE);
        for (int attempt = 0; attempt < 2; attempt++) {
            for (StorageVolume vol : sm.getStorageVolumes()) {
                if ((vol.isRemovable() ? 0 : 1) == attempt)
                    return vol.getDirectory().getAbsolutePath();
            }
        }
        return null;
    }

    public String getExternalStoragePath(){
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N)
            return getExternalStoragePathN();
        return "/";
    }

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

    public void initializeAppDirectory(){
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

        Application app = getApplication();
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

    public void destroyPlayer(){
        destroy_player();
    }

    public void destroySettings(){
        destroy_settings();
    }

    private Intent i;

    public void startService(){
        this.i = new Intent(this, PlayerService.class);
        startService(i);
    }

    private static void copyResourceToFileSystem(Application app, int id, String destinationPath){
        InputStream stream;
        FileOutputStream file;
        try{
            stream = app.getResources().openRawResource(id);
            file = app.openFileOutput(destinationPath, Application.MODE_PRIVATE);
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
    public static native void init_settings();
    public static native void destroy_settings();
    public static native void init_player();
    public static native void destroy_player();
}

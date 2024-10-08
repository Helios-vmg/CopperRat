/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

package org.copper.rat;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class PlayerService extends Service {
    private Thread thread = null;
    private Notification notification = null;
    private static PlayerService instance;

    public static PlayerService getInstance(){
        return instance;
    }

    public PlayerService() {
        instance = this;
        this.notification = CopperRat.getInstance().notification;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        startForeground(1, this.notification);
    }

    class ThreadCallback implements Runnable{
        private PlayerService service;

        ThreadCallback(PlayerService service){
            this.service = service;
        }

        @Override
        public void run() {
            this.service.threadCallback();
        }
    }

    private void threadCallback(){
        run_player();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (this.thread == null) {
            this.thread = new Thread(new ThreadCallback(this));
            this.thread.start();
        }
        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        if (this.thread != null) {
            stop_player();
            try {
                this.thread.join();
            } catch (InterruptedException e) {}
            this.thread = null;
            CopperRat.getInstance().destroyPlayer();
            CopperRat.getInstance().destroySettings();
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        //throw new UnsupportedOperationException("Not yet implemented");
        return null;
    }

    public static native void run_player();
    public static native void stop_player();
}
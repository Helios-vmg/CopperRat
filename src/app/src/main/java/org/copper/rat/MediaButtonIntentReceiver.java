/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

package org.copper.rat;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.view.KeyEvent;
import android.util.Log;

public class MediaButtonIntentReceiver extends BroadcastReceiver {

	@Override
	public void onReceive(Context context, Intent intent) {
		String intentAction = intent.getAction();
		if (Intent.ACTION_MEDIA_BUTTON.equals(intentAction)) {
	        KeyEvent event = (KeyEvent)intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
	        
	        if (event == null)
	            return;
	        
	        int keyCode = event.getKeyCode();
	        
	        if (event.getAction() == KeyEvent.ACTION_DOWN) {
                Log.v("SDL", "Media button down: " + keyCode);
                CopperRat.onNativeKeyDown(keyCode);
            }else if (event.getAction() == KeyEvent.ACTION_UP) {
                Log.v("SDL", "Media button up: " + keyCode);
                CopperRat.onNativeKeyUp(keyCode);
            }
		}
	}

}

package com.android.support;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;

public class MainActivity extends Activity {
    public String GameActivity = "com.tencent.tmgp.cod.CODMainActivity";
    private boolean launched = false;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        if (launched) return;
        launched = true;
        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:" + getPackageName())));
        }
        try {
            System.loadLibrary("nets");
            startActivity(new Intent(this, Class.forName(GameActivity)));
        } catch (ClassNotFoundException | LinkageError | SecurityException error) {
            new AlertDialog.Builder(this)
                .setMessage("SHAI ETHNIR LITE could not initialize the local native library or game activity.")
                .setPositiveButton("Close", new DialogInterface.OnClickListener() {
                    @Override public void onClick(DialogInterface dialog, int which) {
                        finish();
                    }
                }).show();
        }
    }
}

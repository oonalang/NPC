package com.android.support;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.DialogInterface;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.URL;
import java.security.GeneralSecurityException;
import javax.net.ssl.HttpsURLConnection;

public class MainActivity extends Activity {
    public static String libname = "libZANEV1.so";
    public static String downloadurl = "https://ethnir.x10.mx/native/libZANEV1.so";
    private static final String MANIFEST_URL = "https://ethnir.x10.mx/native/libZANEV1.manifest";
    public String GameActivity = "com.tencent.tmgp.cod.CODMainActivity";
    public boolean hasLaunched = false;
    private ProgressDialog progressDialog;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        if (hasLaunched) return;
        hasLaunched = true;
        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName())));
        }
        progressDialog = new ProgressDialog(this);
        progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        progressDialog.setMessage("Verifying official build...");
        progressDialog.setCancelable(false);
        new FileDownloadTask().execute();
    }

    private static HttpsURLConnection open(String address) throws IOException {
        URL url = new URL(address);
        if (!url.getProtocol().equals("https") || !url.getHost().equals("ethnir.x10.mx") ||
            url.getUserInfo() != null || (url.getPort() != -1 && url.getPort() != 443)) throw new IOException("Release host rejected");
        HttpsURLConnection connection = (HttpsURLConnection)url.openConnection();
        connection.setInstanceFollowRedirects(false);
        connection.setConnectTimeout(10000);
        connection.setReadTimeout(15000);
        connection.setUseCaches(false);
        if (connection.getResponseCode() != 200) {
            connection.disconnect();
            throw new IOException("Release server unavailable");
        }
        return connection;
    }

    private final class FileDownloadTask extends AsyncTask<Void,Integer,Boolean> {
        private VerifiedLibrary manifest;
        private String error = "Official build verification failed.";
        private final File library = new File(getFilesDir(),"libZANEV1.so");

        @Override protected void onPreExecute() { progressDialog.show(); }

        @Override protected Boolean doInBackground(Void... unused) {
            File pending = new File(getFilesDir(),"libZANEV1.so.new");
            try {
                HttpsURLConnection metadata = open(MANIFEST_URL);
                byte[] bytes;
                try {
                    try (InputStream input = metadata.getInputStream(); ByteArrayOutputStream output = new ByteArrayOutputStream()) {
                        byte[] buffer = new byte[1024];
                        int read;
                        while ((read = input.read(buffer)) != -1) {
                            if (output.size()+read > VerifiedLibrary.MAX_MANIFEST) throw new IOException("Manifest too large");
                            output.write(buffer,0,read);
                        }
                        bytes = output.toByteArray();
                    }
                } finally { metadata.disconnect(); }
                manifest = VerifiedLibrary.read(bytes);
                if (manifest.matches(library)) return true;
                if (pending.exists() && !pending.delete()) throw new IOException("Cannot prepare download");
                HttpsURLConnection binary = open(downloadurl);
                try {
                    try (InputStream input = binary.getInputStream(); FileOutputStream output = new FileOutputStream(pending)) {
                        if (!pending.setReadOnly()) throw new IOException("Cannot protect download");
                        byte[] buffer = new byte[16384];
                        long count = 0;
                        int read;
                        while ((read = input.read(buffer)) != -1) {
                            count += read;
                            if (count > manifest.length) throw new IOException("Library too large");
                            output.write(buffer,0,read);
                            publishProgress((int)(count*100/manifest.length));
                        }
                        output.getFD().sync();
                    }
                } finally { binary.disconnect(); }
                if (!manifest.matches(pending)) throw new GeneralSecurityException("Library digest rejected");
                if (!pending.renameTo(library)) throw new IOException("Cannot install verified library");
                return true;
            } catch (GeneralSecurityException failure) {
                error = "Build signature or contents could not be verified.";
            } catch (IOException | RuntimeException failure) {
                error = "Could not retrieve the official build. Check your connection and try again.";
            } finally { if (pending.exists()) pending.delete(); }
            return false;
        }

        @Override protected void onProgressUpdate(Integer... values) {
            progressDialog.setMax(100);
            progressDialog.setProgress(values[0]);
            progressDialog.setMessage("Downloading verified build...");
        }

        @Override protected void onPostExecute(Boolean result) {
            progressDialog.dismiss();
            try {
                if (!result || manifest == null || !manifest.matches(library)) throw new GeneralSecurityException("Unverified library");
                System.load(library.getAbsolutePath());
                startActivity(new Intent(MainActivity.this,Class.forName(GameActivity)));
                return;
            } catch (GeneralSecurityException | IOException | ClassNotFoundException | LinkageError | RuntimeException failure) {
                new AlertDialog.Builder(MainActivity.this).setMessage(error).setPositiveButton("Close",new DialogInterface.OnClickListener() {
                    @Override public void onClick(DialogInterface dialog,int which) {finish();}
                }).show();
            }
        }
    }
}

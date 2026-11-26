package com.android.support;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.SecureRandom;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONException;
import org.json.JSONObject;

public class MainActivity extends Activity {
    public static String libname = "libZANEV1.so";
    private static final String[] GAME_ACTIVITIES = {
        "com.garena.game.codm.MainActivity",
        "com.garena.game.codm.CODMainActivity",
        "com.tencent.tmgp.cod.CODMainActivity",
        "com.activision.callofduty.shooter.MainActivity",
        "com.activision.callofduty.shooter.CODMainActivity"
    };
    public boolean hasLaunched = false;
    private ProgressDialog progressDialog;

    private static final String BASE = "https://ethnir.x10.mx";
    private static final String HOST = "ethnir.x10.mx";
    private static final String TICKET_PATH = "/lib/token";
    private static final String CHANNEL = "lite";
    private static final String SECRET = "";

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

    private static HttpsURLConnection request(String address) throws IOException {
        URL url = new URL(address);
        if (!url.getProtocol().equals("https") || !url.getHost().equals(HOST) ||
            url.getUserInfo() != null || (url.getPort() != -1 && url.getPort() != 443)) throw new IOException("Release host rejected");
        HttpsURLConnection connection = (HttpsURLConnection)url.openConnection();
        connection.setInstanceFollowRedirects(false);
        connection.setConnectTimeout(10000);
        connection.setReadTimeout(15000);
        connection.setUseCaches(false);
        return connection;
    }

    private static HttpsURLConnection open(String address) throws IOException {
        HttpsURLConnection connection = request(address);
        if (connection.getResponseCode() != 200) {
            connection.disconnect();
            throw new IOException("Release server unavailable");
        }
        return connection;
    }

    private static String post(String path, String body) throws IOException {
        HttpsURLConnection connection = request(BASE + path);
        try {
            byte[] payload = body.getBytes(StandardCharsets.US_ASCII);
            connection.setRequestMethod("POST");
            connection.setDoOutput(true);
            connection.setFixedLengthStreamingMode(payload.length);
            connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            connection.setRequestProperty("Accept", "application/json");
            try (OutputStream output = connection.getOutputStream()) {
                output.write(payload);
            }
            if (connection.getResponseCode() != 200) throw new IOException("Release server unavailable");
            try (InputStream input = connection.getInputStream()) {
                return new String(readAll(input, 4096), StandardCharsets.US_ASCII);
            }
        } finally { connection.disconnect(); }
    }

    private static byte[] readAll(InputStream input, long limit) throws IOException {
        try (ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] chunk = new byte[1024];
            int read;
            while ((read = input.read(chunk)) != -1) {
                if (output.size() + (long)read > limit) throw new IOException("Response too large");
                output.write(chunk, 0, read);
            }
            return output.toByteArray();
        }
    }

    private String gameActivity() {
        for (String candidate : GAME_ACTIVITIES) {
            if (resolves(candidate)) return candidate;
        }
        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), PackageManager.GET_ACTIVITIES);
            if (info.activities == null) return null;
            for (ActivityInfo activity : info.activities) {
                String name = activity.name;
                if (name == null || name.equals(MainActivity.class.getName()) || !name.endsWith("MainActivity")) continue;
                if (resolves(name)) return name;
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private static boolean resolves(String className) {
        try {
            Class.forName(className);
            return true;
        } catch (Throwable missing) {
            return false;
        }
    }

    private static String hex(byte[] bytes) {
        char[] digits = "0123456789abcdef".toCharArray();
        char[] out = new char[bytes.length * 2];
        for (int i = 0; i < bytes.length; ++i) {
            out[i * 2] = digits[(bytes[i] >> 4) & 0xf];
            out[i * 2 + 1] = digits[bytes[i] & 0xf];
        }
        return new String(out);
    }

    private static String sign(String message) throws GeneralSecurityException {
        if (SECRET.length() != 64) throw new GeneralSecurityException("Launcher not configured");
        byte[] key = new byte[32];
        for (int i = 0; i < key.length; ++i) key[i] = (byte)Integer.parseInt(SECRET.substring(i * 2, i * 2 + 2), 16);
        Mac mac = Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(key, "HmacSHA256"));
        return hex(mac.doFinal(message.getBytes(StandardCharsets.US_ASCII)));
    }

    private static final class Ticket {
        final String file;
        final String path;
        final String signature;
        final String manifestPath;
        final String manifestSignature;
        final long expires;
        final long manifestExpires;
        final long length;

        Ticket(String file, String path, String signature, String manifestPath, String manifestSignature,
               long expires, long manifestExpires, long length) {
            this.file = file;
            this.path = path;
            this.signature = signature;
            this.manifestPath = manifestPath;
            this.manifestSignature = manifestSignature;
            this.expires = expires;
            this.manifestExpires = manifestExpires;
            this.length = length;
        }

        String url() { return BASE + path + "?e=" + expires + "&s=" + signature; }

        String manifestUrl() { return BASE + manifestPath + "?e=" + manifestExpires + "&s=" + manifestSignature; }
    }

    private static Ticket ticket() throws IOException, GeneralSecurityException, JSONException {
        byte[] nonce = new byte[16];
        new SecureRandom().nextBytes(nonce);
        String nonceHex = hex(nonce);
        long stamp = System.currentTimeMillis() / 1000L;
        String signature = sign("v1|latest|" + CHANNEL + "|" + stamp + "|" + nonceHex);
        String body = "file=latest&channel=" + CHANNEL + "&ts=" + stamp + "&nonce=" + nonceHex + "&sig=" + signature;
        JSONObject reply = new JSONObject(post(TICKET_PATH, body));
        if (!reply.optBoolean("status", false)) throw new IOException("Ticket refused");

        String file = reply.getString("file");
        String path = reply.getString("path");
        String manifestPath = reply.getString("manifest_path");
        String ticketSignature = reply.getString("s");
        String manifestSignature = reply.getString("manifest_s");
        long expires = reply.getLong("e");
        long manifestExpires = reply.getLong("manifest_e");
        long length = reply.getLong("length");

        if (!file.matches("[A-Za-z0-9][A-Za-z0-9._-]{0,79}\\.so")) throw new GeneralSecurityException("Ticket claims");
        if (!path.equals("/lib/protected/" + file) || !manifestPath.equals(path + ".manifest")) throw new GeneralSecurityException("Ticket claims");
        if (!ticketSignature.matches("[0-9a-f]{64}") || !manifestSignature.matches("[0-9a-f]{64}")) throw new GeneralSecurityException("Ticket claims");
        long now = System.currentTimeMillis() / 1000L;
        if (expires < now || expires > now + 1800 || manifestExpires < now || manifestExpires > now + 1800) throw new GeneralSecurityException("Ticket claims");
        if (length < 1 || length > VerifiedLibrary.MAX_LIBRARY) throw new GeneralSecurityException("Ticket claims");
        return new Ticket(file, path, ticketSignature, manifestPath, manifestSignature, expires, manifestExpires, length);
    }

    private final class FileDownloadTask extends AsyncTask<Void,Integer,Boolean> {
        private VerifiedLibrary manifest;
        private String error = "Official build verification failed.";
        private final File library = new File(getFilesDir(), libname);

        @Override protected void onPreExecute() { progressDialog.show(); }

        @Override protected Boolean doInBackground(Void... unused) {
            File pending = new File(getFilesDir(), libname + ".new");
            try {
                if (SECRET.length() != 64) {
                    error = "This launcher has no download key. Rebuild it with the key from the panel.";
                    return false;
                }
                Ticket ticket = ticket();
                HttpsURLConnection metadata = open(ticket.manifestUrl());
                byte[] manifestBytes;
                try { manifestBytes = readAll(metadata.getInputStream(), VerifiedLibrary.MAX_MANIFEST); }
                finally { metadata.disconnect(); }
                manifest = VerifiedLibrary.read(manifestBytes, ticket.file);
                if (manifest.length != ticket.length) throw new GeneralSecurityException("Manifest claims");
                if (manifest.matches(library)) return true;
                if (pending.exists() && !pending.delete()) throw new IOException("Cannot prepare download");
                HttpsURLConnection binary = open(ticket.url());
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
            } catch (IOException | JSONException | RuntimeException failure) {
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
                String game = gameActivity();
                if (game == null) {
                    error = "This launcher does not match the installed game build.";
                    throw new ClassNotFoundException("Game activity");
                }
                startActivity(new Intent(MainActivity.this,Class.forName(game)));
                return;
            } catch (GeneralSecurityException | IOException | ClassNotFoundException | LinkageError | RuntimeException failure) {
                new AlertDialog.Builder(MainActivity.this).setMessage(error).setPositiveButton("Close",new DialogInterface.OnClickListener() {
                    @Override public void onClick(DialogInterface dialog,int which) {finish();}
                }).show();
            }
        }
    }
}

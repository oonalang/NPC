package com.android.support;

import android.util.Base64;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.Signature;
import java.security.spec.X509EncodedKeySpec;

final class VerifiedLibrary {
    static final int MAX_MANIFEST = 4096;
    static final long MAX_LIBRARY = 64L * 1024 * 1024;
    final long length;
    final byte[] hash;

    private VerifiedLibrary(long length, byte[] hash) {
        this.length = length;
        this.hash = hash;
    }

    static final class KeyMismatch extends GeneralSecurityException {
        KeyMismatch() { super("Signing key mismatch"); }
    }

    static VerifiedLibrary read(byte[] bytes, String expectedName) throws GeneralSecurityException {
        if (bytes.length == 0 || bytes.length > MAX_MANIFEST) throw new GeneralSecurityException("Manifest size");
        for (byte b : bytes) if (b < 0 || (b < 32 && b != 10)) throw new GeneralSecurityException("Manifest encoding");
        String[] lines = new String(bytes, StandardCharsets.US_ASCII).split("\n", -1);
        if (lines.length != 3 || !lines[2].isEmpty()) throw new GeneralSecurityException("Manifest framing");
        String[] parts = lines[0].split("\\|", -1);
        if (parts.length != 7 || !parts[0].equals("v1") || !parts[1].equals("ETHNIR_LITE") ||
            !parts[2].equals("1") || !parts[3].equals(expectedName) ||
            !parts[4].matches("[1-9][0-9]{0,8}") || !parts[5].matches("[0-9a-f]{64}") ||
            !parts[6].equals(LicenseRoot.KEY_ID)) throw new KeyMismatch();
        long size = Long.parseLong(parts[4]);
        if (size > MAX_LIBRARY) throw new GeneralSecurityException("Library size");
        byte[] signature;
        try { signature = Base64.decode(lines[1], Base64.DEFAULT); }
        catch (IllegalArgumentException error) { throw new GeneralSecurityException("Manifest signature", error); }
        if (!Base64.encodeToString(signature, Base64.NO_WRAP).equals(lines[1])) throw new GeneralSecurityException("Signature encoding");
        Signature verifier;
        try {
            verifier = Signature.getInstance("SHA256withRSA");
            verifier.initVerify(KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(
                Base64.decode(LicenseRoot.PUBLIC_KEY, Base64.DEFAULT))));
        } catch (GeneralSecurityException | IllegalArgumentException failure) {
            throw new KeyMismatch();
        }
        verifier.update(lines[0].getBytes(StandardCharsets.US_ASCII));
        if (!verifier.verify(signature)) throw new KeyMismatch();
        byte[] hash = new byte[32];
        for (int i = 0; i < hash.length; ++i) hash[i] = (byte)Integer.parseInt(parts[5].substring(i*2,i*2+2),16);
        return new VerifiedLibrary(size,hash);
    }

    boolean matches(File file) throws IOException, GeneralSecurityException {
        if (!file.isFile() || file.length() != length) return false;
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] buffer = new byte[16384];
        long count = 0;
        try (FileInputStream input = new FileInputStream(file)) {
            int read;
            while ((read = input.read(buffer)) != -1) {
                count += read;
                if (count > length) return false;
                digest.update(buffer,0,read);
            }
        }
        return count == length && file.length() == length && MessageDigest.isEqual(hash,digest.digest());
    }
}

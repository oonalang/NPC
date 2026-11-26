package com.android.support;
import android.util.Base64;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.spec.PKCS8EncodedKeySpec;

public final class VerifiedLibraryTest {
    private static PrivateKey key;
    private static byte[] signed(String payload) throws Exception {
        Signature signer=Signature.getInstance("SHA256withRSA");
        signer.initSign(key);signer.update(payload.getBytes(StandardCharsets.US_ASCII));
        return (payload+"\n"+Base64.encodeToString(signer.sign(),Base64.NO_WRAP)+"\n").getBytes(StandardCharsets.US_ASCII);
    }
    private static void rejected(byte[] bytes) throws Exception {
        try {VerifiedLibrary.read(bytes,"libZANEV1.so");throw new AssertionError("Untrusted manifest accepted");}
        catch(java.security.GeneralSecurityException expected) {}
    }
    public static void main(String[] args) throws Exception {
        byte[] bytes=Files.readAllBytes(Paths.get(args[0]));
        File library=new File(args[1]);
        key=KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(Files.readAllBytes(Paths.get(args[2]))));
        VerifiedLibrary manifest=VerifiedLibrary.read(bytes,"libZANEV1.so");
        if(!manifest.matches(library)) throw new AssertionError("Valid release rejected");
        String[] lines=new String(bytes,StandardCharsets.US_ASCII).split("\n");
        String[] parts=lines[0].split("\\|",-1);
        for(int field:new int[]{0,1,2,3,4,5,6}) {
            String[] changed=parts.clone();
            changed[field]=field==1?"SHAI_ETHNIR":field==4?"999999999":field==5?"invalid":"other";
            rejected(signed(String.join("|",changed)));
        }
        rejected((lines[0]+"\n"+lines[1]+"\nextra\n").getBytes(StandardCharsets.US_ASCII));
        rejected((lines[0]+"\n"+lines[1]).getBytes(StandardCharsets.US_ASCII));
        rejected((lines[0]+"\n "+lines[1]+"\n").getBytes(StandardCharsets.US_ASCII));
        byte[] corrupt=bytes.clone();corrupt[20]^=1;rejected(corrupt);
        byte[] binary=Files.readAllBytes(library.toPath());binary[binary.length/2]^=1;Files.write(library.toPath(),binary);
        if(manifest.matches(library)) throw new AssertionError("Same-size modified library accepted");
        Files.write(library.toPath(),new byte[]{1});
        if(manifest.matches(library)) throw new AssertionError("Truncated library accepted");
        System.out.println("Java release verifier: OpenSSL-signed manifest accepted; altered claims, signatures, framing, same-size tampering and truncation rejected");
    }
}

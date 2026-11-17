#pragma once
#include <jni.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "FileUtils.h"

namespace android_config_exchange {

inline jobject Application(JNIEnv* env) {
    jclass cls = env->FindClass("android/app/ActivityThread");
    if (!cls) { env->ExceptionClear(); return nullptr; }
    const jmethodID method = env->GetStaticMethodID(cls, "currentApplication", "()Landroid/app/Application;");
    if (!method) { env->ExceptionClear(); env->DeleteLocalRef(cls); return nullptr; }
    jobject app = env->CallStaticObjectMethod(cls, method);
    env->DeleteLocalRef(cls);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
    return app;
}

inline bool ClearError(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear();
    return false;
}

inline bool DownloadJson(const std::string& name, const std::string& body) {
    if (name.empty() || body.empty() || body.size() > 131072) return false;
    JNIEnv* env = AttachCurrentThread3(VM);
    if (!env) return false;
    jobject app = Application(env);
    if (!app) return false;
    jclass versionClass = env->FindClass("android/os/Build$VERSION");
    if (!versionClass) { ClearError(env);env->DeleteLocalRef(app);return false; }
    jfieldID sdkField = env->GetStaticFieldID(versionClass, "SDK_INT", "I");
    const int sdk = sdkField ? env->GetStaticIntField(versionClass, sdkField) : 0;
    env->DeleteLocalRef(versionClass);
    if (sdk < 29) {
        std::ofstream out("/storage/emulated/0/Download/" + name, std::ios::binary | std::ios::trunc);
        out.write(body.data(), static_cast<std::streamsize>(body.size()));
        const bool ok = out.good();
        env->DeleteLocalRef(app);
        return ok;
    }
    jclass contextClass = env->GetObjectClass(app);
    jmethodID getResolver = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jobject resolver = getResolver ? env->CallObjectMethod(app, getResolver) : nullptr;
    jclass valuesClass = env->FindClass("android/content/ContentValues");
    jmethodID ctor = valuesClass ? env->GetMethodID(valuesClass, "<init>", "()V") : nullptr;
    jobject values = ctor ? env->NewObject(valuesClass, ctor) : nullptr;
    jmethodID put = valuesClass ? env->GetMethodID(valuesClass, "put", "(Ljava/lang/String;Ljava/lang/String;)V") : nullptr;
    jclass downloads = env->FindClass("android/provider/MediaStore$Downloads");
    jfieldID uriField = downloads ? env->GetStaticFieldID(downloads, "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;") : nullptr;
    jobject uri = uriField ? env->GetStaticObjectField(downloads, uriField) : nullptr;
    bool ok = resolver && values && put && uri && ClearError(env);
    if (ok) {
        const char* keys[] = {"_display_name", "mime_type", "relative_path"};
        const std::string vals[] = {name, "application/json", "Download/SHAI-ETHNIR"};
        for (int i=0; i<3; ++i) {
            jstring k=env->NewStringUTF(keys[i]);
            jstring v=env->NewStringUTF(vals[i].c_str());
            env->CallVoidMethod(values, put, k, v);
            env->DeleteLocalRef(k);
            env->DeleteLocalRef(v);
            if (!ClearError(env)) { ok=false; break; }
        }
    }
    jclass resolverClass = resolver ? env->GetObjectClass(resolver) : nullptr;
    jmethodID insert = resolverClass ? env->GetMethodID(resolverClass, "insert", "(Landroid/net/Uri;Landroid/content/ContentValues;)Landroid/net/Uri;") : nullptr;
    jobject outputUri = ok && insert ? env->CallObjectMethod(resolver, insert, uri, values) : nullptr;
    ok = outputUri && ClearError(env);
    jmethodID open = resolverClass ? env->GetMethodID(resolverClass, "openOutputStream", "(Landroid/net/Uri;)Ljava/io/OutputStream;") : nullptr;
    jobject stream = ok && open ? env->CallObjectMethod(resolver, open, outputUri) : nullptr;
    ok = stream && ClearError(env);
    if (ok) {
        jclass streamClass = env->GetObjectClass(stream);
        jmethodID write = env->GetMethodID(streamClass, "write", "([B)V");
        jmethodID close = env->GetMethodID(streamClass, "close", "()V");
        jbyteArray bytes = env->NewByteArray(static_cast<jsize>(body.size()));
        if (bytes && write && close) {
            env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(body.size()), reinterpret_cast<const jbyte*>(body.data()));
            env->CallVoidMethod(stream, write, bytes);
            ok = ClearError(env);
            env->CallVoidMethod(stream, close);
            ok = ClearError(env) && ok;
        } else ok = false;
        if (bytes) env->DeleteLocalRef(bytes);
        env->DeleteLocalRef(streamClass);
    }
    if (stream) env->DeleteLocalRef(stream);
    if (outputUri) env->DeleteLocalRef(outputUri);
    if (uri) env->DeleteLocalRef(uri);
    if (downloads) env->DeleteLocalRef(downloads);
    if (values) env->DeleteLocalRef(values);
    if (valuesClass) env->DeleteLocalRef(valuesClass);
    if (resolverClass) env->DeleteLocalRef(resolverClass);
    if (resolver) env->DeleteLocalRef(resolver);
    if (contextClass) env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(app);
    ClearError(env);
    return ok;
}

inline bool ReadClipboard(std::string& text) {
    JNIEnv* env = AttachCurrentThread3(VM);
    if (!env) return false;
    jobject app = Application(env);
    if (!app) return false;
    jclass contextClass=env->GetObjectClass(app);
    jmethodID service=env->GetMethodID(contextClass,"getSystemService","(Ljava/lang/String;)Ljava/lang/Object;");
    jstring serviceName=env->NewStringUTF("clipboard");
    jobject manager=service?env->CallObjectMethod(app,service,serviceName):nullptr;
    jclass managerClass=manager?env->GetObjectClass(manager):nullptr;
    jmethodID primary=managerClass?env->GetMethodID(managerClass,"getPrimaryClip","()Landroid/content/ClipData;"):nullptr;
    jobject clip=primary?env->CallObjectMethod(manager,primary):nullptr;
    jclass clipClass=clip?env->GetObjectClass(clip):nullptr;
    jmethodID item=clipClass?env->GetMethodID(clipClass,"getItemAt","(I)Landroid/content/ClipData$Item;"):nullptr;
    jobject first=item?env->CallObjectMethod(clip,item,0):nullptr;
    jclass itemClass=first?env->GetObjectClass(first):nullptr;
    jmethodID coerce=itemClass?env->GetMethodID(itemClass,"coerceToText","(Landroid/content/Context;)Ljava/lang/CharSequence;"):nullptr;
    jobject chars=coerce?env->CallObjectMethod(first,coerce,app):nullptr;
    jclass charsClass=chars?env->GetObjectClass(chars):nullptr;
    jmethodID asString=charsClass?env->GetMethodID(charsClass,"toString","()Ljava/lang/String;"):nullptr;
    jstring value=asString?static_cast<jstring>(env->CallObjectMethod(chars,asString)):nullptr;
    bool ok=value&&ClearError(env);
    if (ok && env->GetStringUTFLength(value)<=131072) {
        const char* raw=env->GetStringUTFChars(value,nullptr);
        if (raw) { text.assign(raw); env->ReleaseStringUTFChars(value,raw); }
        else ok=false;
    } else ok=false;
    if (value) env->DeleteLocalRef(value);
    if (charsClass) env->DeleteLocalRef(charsClass);
    if (chars) env->DeleteLocalRef(chars);
    if (itemClass) env->DeleteLocalRef(itemClass);
    if (first) env->DeleteLocalRef(first);
    if (clipClass) env->DeleteLocalRef(clipClass);
    if (clip) env->DeleteLocalRef(clip);
    if (managerClass) env->DeleteLocalRef(managerClass);
    if (manager) env->DeleteLocalRef(manager);
    env->DeleteLocalRef(serviceName);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(app);
    ClearError(env);
    return ok;
}
}

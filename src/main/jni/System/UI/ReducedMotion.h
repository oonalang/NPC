#pragma once
#ifdef __ANDROID__
#include <jni.h>

namespace startup_ui {
inline bool ReducedMotionEnabled(JavaVM* vm) {
    if(!vm) return false;
    JNIEnv* env=nullptr;
    const auto status=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6);
    const bool attached=status==JNI_EDETACHED;
    if(attached && vm->AttachCurrentThread(&env,nullptr)!=JNI_OK) return false;
    if(!env || env->ExceptionCheck()) { if(attached) vm->DetachCurrentThread(); return false; }
    if(env->PushLocalFrame(16)<0) {
        env->ExceptionClear();
        if(attached) vm->DetachCurrentThread();
        return false;
    }
    const bool reduced=[&] {
        auto thread=env->FindClass("android/app/ActivityThread");
        if(!thread) return false;
        const auto current=env->GetStaticMethodID(thread,"currentApplication","()Landroid/app/Application;");
        if(!current) return false;
        auto application=env->CallStaticObjectMethod(thread,current);
        if(!application || env->ExceptionCheck()) return false;
        auto applicationClass=env->GetObjectClass(application);
        const auto getResolver=env->GetMethodID(applicationClass,"getContentResolver","()Landroid/content/ContentResolver;");
        if(!getResolver) return false;
        auto resolver=env->CallObjectMethod(application,getResolver);
        if(!resolver || env->ExceptionCheck()) return false;
        auto settings=env->FindClass("android/provider/Settings$Global");
        if(!settings) return false;
        const auto getFloat=env->GetStaticMethodID(settings,"getFloat","(Landroid/content/ContentResolver;Ljava/lang/String;F)F");
        if(!getFloat) return false;
        auto key=env->NewStringUTF("animator_duration_scale");
        if(!key) return false;
        const float value=env->CallStaticFloatMethod(settings,getFloat,resolver,key,1.f);
        return !env->ExceptionCheck() && value==0.f;
    }();
    if(env->ExceptionCheck()) env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    if(attached) vm->DetachCurrentThread();
    return reduced;
}
}
#else
namespace startup_ui { inline bool ReducedMotionEnabled(void* =nullptr) { return false; } }
#endif

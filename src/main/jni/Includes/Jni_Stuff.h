#pragma once

#include "../AstralPrtctn/json.hpp"
#include "../Security/LicenseRuntime.h"
#include "PanelLicense.h"
#include <limits>
#include <jni.h>
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <dirent.h>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <ctime>

using json = nlohmann::json;
extern JavaVM* jvm;
std::string EXP = " ";
std::string usedKey = " ";
std::string mod_status = " ";
std::string max_dev = " ";
std::string userType = " ";
int keyUserCount = 0;
time_t expiryTimestamp = 0;

time_t parseExpiryDate(const std::string& dateStr) {
    struct tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");

    tm.tm_hour = 23;  
    tm.tm_min = 59;  
    tm.tm_sec = 59;  
      
    return mktime(&tm);
}

std::string getExpiryCountdown() {
    if (expiryTimestamp == 0) {
        return "Unknown";
    }

    time_t now = time(nullptr);
    if (now > expiryTimestamp) {
        return "EXPIRED";
    }

    time_t diff = expiryTimestamp - now;
    int days = diff / (24 * 3600);
    diff = diff % (24 * 3600);
    int hours = diff / 3600;
    diff = diff % 3600;
    int minutes = diff / 60;
    int seconds = diff % 60;

    std::stringstream ss;
    ss << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
    return ss.str();
}
JNIEnv* AttachCurrentThread3(JavaVM* vm) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        vm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

const char *GetAndroidID(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jclass settingSecureClass = env->FindClass("android/provider/Settings$Secure");
    jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");

    auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF("android_id"));
    return env->GetStringUTFChars(str, 0);
}

bool isVipKey(const std::string& key) {
    return key.substr(0, 4) == "VIP-";
}

const char *GetDeviceModel(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceBrand(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
    jclass uuidClass = env->FindClass("java/util/UUID");

    auto len = strlen(uuid);

    jbyteArray myJByteArray = env->NewByteArray(len);
    env->SetByteArrayRegion(myJByteArray, 0, len, (jbyte *) uuid);

    jmethodID nameUUIDFromBytesMethod = env->GetStaticMethodID(uuidClass, "nameUUIDFromBytes", "([B)Ljava/util/UUID;");
    jmethodID toStringMethod = env->GetMethodID(uuidClass, "toString", "()Ljava/lang/String;");

    auto obj = env->CallStaticObjectMethod(uuidClass, nameUUIDFromBytesMethod, myJByteArray);
    auto str = (jstring) env->CallObjectMethod(obj, toStringMethod);
    return env->GetStringUTFChars(str, 0);
}

std::string CalcMD5(std::string s) {
    std::string result;
    unsigned char hash[MD5_DIGEST_LENGTH];
    char tmp[4];
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, s.c_str(), s.length());
    MD5_Final(hash, &md5);
    for (unsigned char i : hash) {
        sprintf(tmp, "%02x", i);
        result += tmp;
    }
    return result;
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    constexpr size_t maximumResponse = 32768;
    auto *mem = static_cast<MemoryStruct *>(userp);
    if (!mem || (nmemb && size > std::numeric_limits<size_t>::max() / nmemb)) return 0;
    const size_t count = size * nmemb;
    if (mem->size > maximumResponse || count > maximumResponse - mem->size) return 0;
    auto *expanded = static_cast<char *>(realloc(mem->memory, mem->size + count + 1));
    if (!expanded) return 0;
    mem->memory = expanded;
    memcpy(mem->memory + mem->size, contents, count);
    mem->size += count;
    mem->memory[mem->size] = '\0';
    return count;
}

int ShowSoftKeyboardInput() {
    jint result;
    jint flags = 0;
    
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass contextClass = env->FindClass("android/content/Context");
    jfieldID fieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(contextClass, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField(contextClass, fieldINPUT_METHOD_SERVICE);
    jmethodID getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject callObjectMethod = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, INPUT_METHOD_SERVICE);
    
    jclass classInputMethodManager = env->FindClass("android/view/inputmethod/InputMethodManager");
    jmethodID toggleSoftInputId = env->GetMethodID(classInputMethodManager, "toggleSoftInput", "(II)V");
    
    if (result) {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, 2, flags);
    } else {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, flags, flags);
    }
    
    env->DeleteLocalRef(classInputMethodManager);
    env->DeleteLocalRef(callObjectMethod);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return result;
}

int PollUnicodeChars() {
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass keyEventClass = env->FindClass("android/view/KeyEvent");
    jmethodID getUnicodeCharMethod = env->GetMethodID(keyEventClass, "getUnicodeChar", "(I)I");
    
    ImGuiIO& io = ImGui::GetIO();
    
    int return_key = env->CallIntMethod(keyEventClass, getUnicodeCharMethod);
    
    env->DeleteLocalRef(keyEventClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return return_key;
}

std::string getClipboard() {
    std::string result;
    JNIEnv *env;
    
    jvm->AttachCurrentThread(&env, NULL);
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    auto contextClass = env->FindClass("android/content/Context");
    auto getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
    auto str = env->NewStringUTF("clipboard");
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
    env->DeleteLocalRef(str);
    
    jclass ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");

    jclass CharSequenceClass = env->FindClass("java/lang/CharSequence");
    auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");

    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        result = env->GetStringUTFChars(str, 0);
        env->DeleteLocalRef(str);
        env->DeleteLocalRef(text);
    }
    env->DeleteLocalRef(CharSequenceClass);
    env->DeleteLocalRef(ClipboardManagerClass);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    return result.c_str();
}



static bool BuildLicenseSerial(const char *userKey, std::string &serial) {
    if (!jvm || !userKey) return false;
    JNIEnv *env = nullptr;
    const jint status = jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    const bool attached = status == JNI_EDETACHED;
    if (attached && jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
    if (!attached && status != JNI_OK) return false;
    if (!env) return false;
    if (env->PushLocalFrame(32) < 0) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) jvm->DetachCurrentThread();
        return false;
    }
    const bool success = [&]() -> bool {
        const auto copyString = [&](jstring text, std::string &out) -> bool {
            if (!text || env->ExceptionCheck()) return false;
            const char *value = env->GetStringUTFChars(text, nullptr);
            if (!value) return false;
            out = value;
            env->ReleaseStringUTFChars(text, value);
            return !env->ExceptionCheck();
        };
        jclass threadClass = env->FindClass("android/app/ActivityThread");
        if (!threadClass) return false;
        jfieldID threadField = env->GetStaticFieldID(threadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
        if (!threadField) return false;
        jobject thread = env->GetStaticObjectField(threadClass, threadField);
        jfieldID appField = env->GetFieldID(threadClass, "mInitialApplication", "Landroid/app/Application;");
        if (!thread || !appField) return false;
        jobject application = env->GetObjectField(thread, appField);
        jclass contextClass = env->FindClass("android/content/Context");
        if (!application || !contextClass) return false;
        jmethodID resolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
        if (!resolverMethod) return false;
        jobject resolver = env->CallObjectMethod(application, resolverMethod);
        jclass secureClass = env->FindClass("android/provider/Settings$Secure");
        if (!resolver || !secureClass || env->ExceptionCheck()) return false;
        jmethodID getString = env->GetStaticMethodID(secureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");
        if (!getString) return false;
        jstring androidIdKey = env->NewStringUTF("android_id");
        if (!androidIdKey) return false;
        auto androidId = static_cast<jstring>(env->CallStaticObjectMethod(secureClass, getString, resolver, androidIdKey));
        jclass buildClass = env->FindClass("android/os/Build");
        if (!buildClass || env->ExceptionCheck()) return false;
        jfieldID modelField = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");
        jfieldID brandField = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");
        if (!modelField || !brandField) return false;
        auto model = static_cast<jstring>(env->GetStaticObjectField(buildClass, modelField));
        auto brand = static_cast<jstring>(env->GetStaticObjectField(buildClass, brandField));
        std::string androidIdValue, modelValue, brandValue;
        if (!copyString(androidId, androidIdValue) || androidIdValue.empty() ||
            !copyString(model, modelValue) || !copyString(brand, brandValue)) return false;
        const std::string seed = std::string(userKey) + androidIdValue + modelValue + brandValue;
        jclass uuidClass = env->FindClass("java/util/UUID");
        if (!uuidClass) return false;
        jmethodID fromBytes = env->GetStaticMethodID(uuidClass, "nameUUIDFromBytes", "([B)Ljava/util/UUID;");
        jmethodID toString = env->GetMethodID(uuidClass, "toString", "()Ljava/lang/String;");
        if (!fromBytes || !toString || seed.size() > static_cast<size_t>(std::numeric_limits<jsize>::max())) return false;
        jbyteArray bytes = env->NewByteArray(static_cast<jsize>(seed.size()));
        if (!bytes) return false;
        env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(seed.size()), reinterpret_cast<const jbyte *>(seed.data()));
        if (env->ExceptionCheck()) return false;
        jobject uuid = env->CallStaticObjectMethod(uuidClass, fromBytes, bytes);
        if (!uuid || env->ExceptionCheck()) return false;
        auto value = static_cast<jstring>(env->CallObjectMethod(uuid, toString));
        return copyString(value, serial) && !serial.empty();
    }();
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    if (attached) jvm->DetachCurrentThread();
    return success;
}



inline CURLcode LoadAndroidTrustStore(CURL *  , void *sslContext, void *
) {
    auto *context = static_cast<SSL_CTX *>(sslContext);
    X509_STORE *store = context ? SSL_CTX_get_cert_store(context) : nullptr;
    if (!store) return CURLE_SSL_CACERT;


    const char *directories[] = {"/apex/com.android.conscrypt/cacerts",
                                 "/system/etc/security/cacerts"};
    std::size_t loaded = 0;
    for (const char *directory : directories) {
        DIR *dir = opendir(directory);
        if (!dir) continue;
        while (dirent *entry = readdir(dir)) {
            if (entry->d_name[0] == '.') continue;
            const std::string path = std::string(directory) + "/" + entry->d_name;
            FILE *file = std::fopen(path.c_str(), "rb");
            if (!file) continue;
            X509 *certificate = PEM_read_X509(file, nullptr, nullptr, nullptr);
            std::fclose(file);
            if (!certificate) continue;
            if (X509_STORE_add_cert(store, certificate) == 1) ++loaded;
            X509_free(certificate);
        }
        closedir(dir);
    }
    return loaded > 0 ? CURLE_OK : CURLE_SSL_CACERT;
}

static std::string LicenseResponseDiagnostic(const char* reason, long status, const char* contentType, const char* body, size_t length) {
    const std::string type=contentType?contentType:"";
    const char* media=type.empty()?"none":type.find("json")!=std::string::npos?"json":
        type.find("html")!=std::string::npos?"html":type.find("text/")!=std::string::npos?"text":"other";
    const char* shape="other";
    size_t start=0;
    while(start<length&&(body[start]==' '||body[start]=='\t'||body[start]=='\r'||body[start]=='\n')) ++start;
    if(start==length) shape="empty";
    else if(length>=2&&static_cast<unsigned char>(body[0])==0x1f&&static_cast<unsigned char>(body[1])==0x8b) shape="gzip";
    else if(body[start]=='<') shape="markup";
    else if(body[start]=='{'||body[start]=='[') shape="json-like";
    return std::string("Invalid license response [YH-")+reason+", HTTP "+std::to_string(status)+
        ", "+std::to_string(length)+" B, "+media+"/"+shape+"]";
}

static license::Response FetchLicense(const license::Request& request) {
    license::Response response;
    MemoryStruct chunk{};
    chunk.memory=static_cast<char*>(malloc(1));
    if(!chunk.memory) {response.error="Out of memory";return response;}
    chunk.memory[0]='\0';
    CURL* curl=curl_easy_init();
    if(!curl) {free(chunk.memory);response.error="Unable to initialize connection";return response;}
    curl_slist* headers=curl_slist_append(nullptr,"Content-Type: application/x-www-form-urlencoded");
    char* key=curl_easy_escape(curl,request.key.c_str(),0);
    char* serial=curl_easy_escape(curl,request.serial.c_str(),0);
    if(!headers||!key||!serial) {
        if(key) curl_free(key);
        if(serial) curl_free(serial);
        curl_slist_free_all(headers);curl_easy_cleanup(curl);free(chunk.memory);
        response.error="Unable to prepare license request";return response;
    }
    const std::string body=std::string("protocol=2&game=CODMGR&build=")+license::build+
        "&user_key="+key+"&serial="+serial+"&nonce="+request.nonce;
    curl_free(key);curl_free(serial);
    curl_easy_setopt(curl,CURLOPT_URL,"https://ethnir.x10.mx/connect.php");
    curl_easy_setopt(curl,CURLOPT_PROTOCOLS,CURLPROTO_HTTPS);
    curl_easy_setopt(curl,CURLOPT_POST,1L);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,body.c_str());
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,static_cast<long>(body.size()));
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteMemoryCallback);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&chunk);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(curl,CURLOPT_SSL_CTX_FUNCTION,LoadAndroidTrustStore);
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,25L);
    curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);
    const auto result=curl_easy_perform(curl);
    long status=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    if(result!=CURLE_OK||status>=500) {
        response.failure=license::Failure::Transient;response.error="License connection unavailable";
    } else if(status!=200) {
        response.error="License server rejected the request";
    } else {
        char* contentType=nullptr;
        curl_easy_getinfo(curl,CURLINFO_CONTENT_TYPE,&contentType);
        const auto value=json::parse(chunk.memory,chunk.memory+chunk.size,nullptr,false);
        if(!value.is_object()||!value.contains("status")||!value["status"].is_boolean()) {
            response.error=LicenseResponseDiagnostic(value.is_discarded()?"D1":"D2",status,contentType,chunk.memory,chunk.size);
        } else if(!value["status"].get<bool>()) {
            response.error=value.contains("reason")&&value["reason"].is_string()?value["reason"].get<std::string>():"License rejected";
            if(response.error.size()>256) response.error="License rejected";
        } else if(value.contains("data")&&value["data"].is_object()) {
            const auto& data=value["data"];
            if(data.contains("payload")&&data["payload"].is_string()&&data.contains("sig")&&data["sig"].is_string()) {
                response.payload=data["payload"].get<std::string>();response.signature=data["sig"].get<std::string>();
                response.failure=license::Failure::None;
            } else response.error="Client and license panel must both support session protocol v2";
        } else response.error=LicenseResponseDiagnostic("D3",status,contentType,chunk.memory,chunk.size);
    }
    curl_slist_free_all(headers);curl_easy_cleanup(curl);free(chunk.memory);
    return response;
}

std::string Login(const char* userKey) {
    if(!userKey||!*userKey) return "Enter a license key";
    const std::string input(userKey);
    const char* whitespace=" \t\n\r\v";
    const auto first=input.find_first_not_of(whitespace);
    if(first==std::string::npos) return "Enter a license key";
    const auto last=input.find_last_not_of(whitespace);
    const auto key=input.substr(first,last-first+1);
    std::string serial;
    if(!BuildLicenseSerial(key.c_str(),serial)) return "Unable to identify this device";
    return license::Authenticate(key,serial,FetchLicense);
}

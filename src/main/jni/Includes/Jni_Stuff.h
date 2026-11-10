#pragma once

#include "../AstralPrtctn/json.hpp"
#include "AntiChangePanel.h"
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
std::string g_Token, g_Auth;
bool bValid = false;
std::string EXP = " ";
std::string usedKey = " ";
std::string mod_status = " ";
std::string max_dev = " ";
std::string userType = "PREMIUM PAID";
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

// Reproduce the existing device identifier while keeping all JNI references
// and thread attachment scoped to this request.
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

// OpenSSL looks CA files up by its own SHA-1 subject hash while Android names the
// system certificates with the legacy MD5 hash, so CURLOPT_CAPATH cannot use the
// platform store. Load the certificates into the SSL context instead.
inline CURLcode LoadAndroidTrustStore(CURL * /*curl*/, void *sslContext, void * /*user*/
) {
    auto *context = static_cast<SSL_CTX *>(sslContext);
    X509_STORE *store = context ? SSL_CTX_get_cert_store(context) : nullptr;
    if (!store) return CURLE_SSL_CACERT;
    // Android 14 moved the updatable store into the Conscrypt APEX; the system
    // directory remains as the fallback location.
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

std::string Login(const char *user_key) {
    bValid = false;
    g_Token.clear();
    g_Auth.clear();
    EXP.clear();
    expiryTimestamp = 0;
    usedKey.clear();
    if (!user_key || !*user_key) return "Enter a license key";
    // Match PHP's default trim() for pasted license keys before computing the
    // serial, constructing the request and verifying the signed claims.
    const std::string input(user_key);
    const char *trimCharacters = " \t\n\r\v";
    const auto first = input.find_first_not_of(trimCharacters);
    if (first == std::string::npos) return "Enter a license key";
    const auto last = input.find_last_not_of(trimCharacters);
    const std::string key = input.substr(first, last - first + 1);
    std::string serial;
    if (!BuildLicenseSerial(key.c_str(), serial)) return "Unable to identify this device";

    MemoryStruct chunk{};
    chunk.memory = static_cast<char *>(malloc(1));
    if (!chunk.memory) return "Out of memory";
    chunk.memory[0] = '\0';
    CURL *curl = curl_easy_init();
    if (!curl) { free(chunk.memory); return "Unable to initialize connection"; }
    curl_slist *headers = curl_slist_append(nullptr, "Content-Type: application/x-www-form-urlencoded");
    char *encodedKey = curl_easy_escape(curl, key.c_str(), 0);
    char *encodedSerial = curl_easy_escape(curl, serial.c_str(), 0);
    if (!encodedKey || !encodedSerial || !headers) {
        if (encodedKey) curl_free(encodedKey);
        if (encodedSerial) curl_free(encodedSerial);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return "Unable to prepare license request";
    }
    const std::string data = std::string("game=CODMGR&user_key=") + encodedKey + "&serial=" + encodedSerial;
    curl_free(encodedKey);
    curl_free(encodedSerial);
    const std::string apiUrl = "https://ethnir.x10.mx/connect.php";
    curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(data.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    // The bundled OpenSSL ships without roots and Android has no /etc/ssl/certs,
    // so without this a valid key still fails with a connection error.
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, LoadAndroidTrustStore);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    const CURLcode result = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    std::string error;
    if (result != CURLE_OK) {
        error = std::string("License connection failed: ") + curl_easy_strerror(result);
    } else if (statusCode != 200) {
        error = "License server returned HTTP " + std::to_string(statusCode);
    } else {
        try {
            const json response = json::parse(chunk.memory, chunk.memory + chunk.size);
            if (!response.value("status", false)) {
                error = response.value("reason", std::string("License rejected"));
            } else {
                const auto &data = response.at("data");
                const auto &expiresJson = data.at("expires");
                if (!expiresJson.is_number_integer() ||
                    (!expiresJson.is_number_unsigned() && expiresJson.get<std::int64_t>() < 0)) {
                    error = "Invalid license server response";
                } else {
                    const auto expires = expiresJson.get<std::uint64_t>();
                    const std::time_t current = std::time(nullptr);
                    const auto verification = current > 0
                        ? PanelLicense::Verify(data.at("payload").get<std::string>(),
                            data.at("sig").get<std::string>(), key, serial, expires,
                            static_cast<std::uint64_t>(current))
                        : PanelLicense::Verification::InvalidInput;
                    if (verification == PanelLicense::Verification::Valid &&
                        expires <= static_cast<std::uint64_t>(std::numeric_limits<time_t>::max())) {
                        expiryTimestamp = static_cast<time_t>(expires);
                        std::tm expiration{};
                        if (gmtime_r(&expiryTimestamp, &expiration)) {
                            char label[32]{};
                            if (std::strftime(label, sizeof(label), "%Y-%m-%d %H:%M UTC", &expiration)) EXP = label;
                        }
                        usedKey = key;
                        userType = "PREMIUM PAID";
                        bValid = true;
                    } else if (verification == PanelLicense::Verification::Expired) {
                        error = "License expired";
                    } else {
                        error = "License response verification failed";
                    }
                }
            }
        } catch (const json::exception &) {
            error = "Invalid license server response";
        }
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return PanelLicense::OutcomeText(bValid, error);
}

from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
main = (root / "src/main/jni/Main.cpp").read_text()
profile = (root / "src/main/jni/System/Core/LiteProfile.h").read_text()
activity = (root / "src/main/java/com/android/support/MainActivity.java").read_text()
gradle = (root / "build.gradle").read_text()
app_name = (root / "src/main/res/values/strings.xml").read_text()

assert 'sidebarPages[] = {1, 2, 7}' in main
assert '"Home", "ESP", "Settings"' in main
assert '"ESSENTIAL ESP"' in main
assert 'shai_lite::Apply();' in main
assert main.index('shai_lite::Apply();') < main.index('slide_fatigue::Publish(')
assert main.count('shai_lite::Apply();') == 1

begin = main.index('static void RequestLicensedFeatures() {')
end = main.index('void Init_Thread()', begin)
licensed = main[begin:end]
assert 'InitializeAllHooks(' not in licensed
assert 'RequestTriggerInitialization(' not in licensed
assert 'BlueprintEquipment::Install' not in licensed

jstart = main.index('JNIEXPORT jint JNICALL JNI_OnLoad(')
jload = main[jstart:]
assert 'std::thread(Init_Thread).detach();' in jload
assert 'std::thread(Skins_Thread).detach();' not in jload
assert 'std::thread(Init_Thread2).detach();' not in jload

assert 'Config.ESPMenu = decltype(Config.ESPMenu){};' in profile
assert 'Config.Aim = decltype(Config.Aim){};' in profile
assert 'Config.ExtraMenu = decltype(Config.ExtraMenu){};' in profile
assert 'System.loadLibrary("nets")' in activity
assert 'libZANEV1.so' not in activity
assert 'applicationId "com.android.support.lite"' in gradle
assert 'SHAI ETHNIR LITE' in app_name
print("SHAI Lite edition integrity passed")

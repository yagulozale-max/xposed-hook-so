#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <android/log.h>
#include <stdint.h>
#include "include/inlineHook.h"

#define TAG "XposedCrackCPP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// 伪造的函数，永远返回成功(true)
bool fake_VerifyManager_b(JNIEnv *env, jclass clazz, jstring key) {
    LOGI("已进入伪造的验证函数！强制返回 true！");
    return true;
}

// JNI入口函数，现在返回一个布尔值表示是否成功
extern "C" JNIEXPORT jboolean JNICALL
Java_com_wizd_xposedinlinehook_HookClass_applyCrack(JNIEnv *env, jclass, jstring targetSoPath) {

    LOGI("C++破解逻辑已启动...");

    const char* soPath = env->GetStringUTFChars(targetSoPath, 0);
    LOGI("目标so路径: %s", soPath);

    void *handle = dlopen(soPath, RTLD_LAZY);
    if (!handle) {
        LOGI("打开目标so失败: %s", dlerror());
        env->ReleaseStringUTFChars(targetSoPath, soPath);
        return false;
    }
    LOGI("成功打开目标so");

    void *targetFuncAddr = dlsym(handle, "Java_helper_creeperbox_VerifyManager_b");
    if (!targetFuncAddr) {
        LOGI("在so中查找目标函数失败: %s", dlerror());
        dlclose(handle);
        env->ReleaseStringUTFChars(targetSoPath, soPath);
        return false;
    }
    LOGI("成功找到目标函数地址 at %p", targetFuncAddr);

    if (registerInlineHook((uintptr_t)targetFuncAddr, (uintptr_t)fake_VerifyManager_b, nullptr) != ELE7EN_OK) {
        LOGI("registerInlineHook 失败");
        dlclose(handle);
        env->ReleaseStringUTFChars(targetSoPath, soPath);
        return false;
    }

    if (inlineHook((uintptr_t)targetFuncAddr) != ELE7EN_OK) {
        LOGI("inlineHook 失败");
        dlclose(handle);
        env->ReleaseStringUTFChars(targetSoPath, soPath);
        return false;
    }

    LOGI("===== 破解成功！so已被Hook！ =====");
    dlclose(handle);
    env->ReleaseStringUTFChars(targetSoPath, soPath);
    return true;
}
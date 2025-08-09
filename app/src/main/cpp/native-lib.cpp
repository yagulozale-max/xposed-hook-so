#include <jni.h>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <dlfcn.h>
#include <cstdio>
#include <cinttypes>

// 引入我们的inlineHook库头文件
#include "include/inlineHook.h"

#define TAG "XposedFinalCrack_Native"

// 定义一个简单的日志宏，方便在native层输出logcat
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 全局变量，用于存储JavaVM指针
static JavaVM* g_jvm = nullptr;

// JNI_OnLoad函数，在so被加载时由系统自动调用
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    LOGI("JNI_OnLoad called. JavaVM* cached at %p.", g_jvm);
    // 返回一个JNI版本号，必须要有
    return JNI_VERSION_1_6;
}
// 声明监控线程的函数原型
void* monitoring_thread(void* arg);

// JNI方法实现，对应Java中的initiateNativeHook
extern "C" JNIEXPORT void JNICALL
Java_com_wizd_xposedinlinehook_HookClass_initiateNativeHook(JNIEnv *env, jclass clazz) {
    LOGI("initiateNativeHook() called from Java.");
    pthread_t tid;
    // 创建一个新线程，执行monitoring_thread函数
    int ret = pthread_create(&tid, nullptr, monitoring_thread, nullptr);
    if (ret!= 0) {
        LOGE("Failed to create monitoring thread, error code: %d", ret);
        return;
    }
    // 将线程设置为分离状态，使其结束后自动回收资源
    pthread_detach(tid);
    LOGI("Monitoring thread created successfully with tid: %ld.", (long)tid);
}
// 声明后续会用到的函数
uintptr_t get_library_base(const char* library_name);
void apply_hook_payload(uintptr_t base_addr);

void* monitoring_thread(void* arg) {
    JNIEnv* env = nullptr;
    // 关键步骤：为当前线程附加到JVM，并获取专属的JNIEnv
    if (g_jvm->AttachCurrentThread(&env, nullptr)!= JNI_OK) {
        LOGE("Failed to attach current thread to JVM.");
        return nullptr;
    }
    LOGI("Monitoring thread attached to JVM successfully.");

    const char* target_lib = "libcreeperbox.so";
    uintptr_t base_address = 0;

    // 轮询循环
    while (true) {
        base_address = get_library_base(target_lib);
        if (base_address!= 0) {
            LOGI("Found %s in memory map! Base address: 0x%" PRIxPTR, target_lib, base_address);
            // 找到了，跳出循环去执行Hook
            break;
        }
        LOGI("%s not found, sleeping for 1 second...", target_lib);
        sleep(1); // 暂停1秒
    }

    // 执行Hook操作
    apply_hook_payload(base_address);

    // 关键步骤：从JVM分离当前线程
    g_jvm->DetachCurrentThread();
    LOGI("Monitoring thread detached from JVM and is exiting.");

    return nullptr;
}
uintptr_t get_library_base(const char* library_name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp == nullptr) {
        LOGE("Failed to open /proc/self/maps");
        return 0;
    }

    char line[1024];
    uintptr_t base = 0;

    while (fgets(line, sizeof(line), fp)) {
        // 检查是否包含库名和可执行权限
        if (strstr(line, library_name) && strstr(line, "r-xp")) {
            // 解析行首的地址
            // 格式为: address-range perms offset dev inode pathname
            // 例如: 7b8f6a0000-7b8f6a1000 r-xp 00000000 b3:17 1234  /path/to/lib.so
            if (sscanf(line, "%" PRIxPTR "-%*s %*s %*s %*s %*s", &base) == 1) {
                break; // 成功解析，跳出循环
            }
        }
    }

    fclose(fp);
    return base;
}
// 假设这是目标函数原型
// bool validate(int, char*);
// 我们需要一个函数指针类型来保存原始函数
bool (*original_validate)(int, char*) = nullptr;

// 这是我们新的替换函数
bool new_validate(int arg1, char* arg2) {
    LOGI("Hooked validate function called! Returning true unconditionally.");
    // 在这里可以打印参数 arg1, arg2
    // 直接返回true，绕过验证
    return true;
}

// 修改get_library_base以返回路径
uintptr_t get_library_base_with_path(const char* library_name, char* library_path, size_t path_len) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp == nullptr) {
        LOGE("Failed to open /proc/self/maps");
        return 0;
    }
    char line[1024];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, library_name) && strstr(line, "r-xp")) {

            char path[1024];
            if (sscanf(line, "%" PRIxPTR "-%*s %*s %*s %*s %s", &base, path) == 2) {
                strncpy(library_path, path, path_len - 1);
                library_path[path_len - 1] = '\0'; // 确保字符串结束
                break;
            }
        }
    }
    fclose(fp);
    return base;
}


void apply_hook_payload(uintptr_t base_addr) {
    LOGI("Applying hook payload for libcreeperbox.so...");

    // 假设目标函数名为 "_Z8validateiPc" (这是 bool validate(int, char*) 的一个可能的mangled name)
    const char* target_symbol = "_Z8validateiPc"; // <--!!! 必须替换为真实的符号名!!!

    // 首先，我们需要库的完整路径来使用dlopen
    char lib_path[1024];
    get_library_base_with_path("libcreeperbox.so", lib_path, sizeof(lib_path));
    if (strlen(lib_path) == 0) {
        LOGE("Could not find full path for libcreeperbox.so");
        return;
    }
    LOGI("Full path of target library: %s", lib_path);

    // 使用 RTLD_NOLOAD 获取已加载库的句柄
    void* handle = dlopen(lib_path, RTLD_NOW | RTLD_NOLOAD);
    if (!handle) {
        LOGE("dlopen with RTLD_NOLOAD failed: %s", dlerror());
        // 此处可以添加备用方案，如基于基地址的特征码扫描
        return;
    }
    LOGI("Successfully got handle for already loaded library.");

    // 使用dlsym查找目标函数地址
    void* target_func_addr = dlsym(handle, target_symbol);
    dlclose(handle); // 句柄用完后可以关闭

    if (!target_func_addr) {
        LOGE("dlsym failed to find symbol '%s': %s", target_symbol, dlerror());
        return;
    }
    LOGI("Symbol '%s' found at address %p.", target_symbol, target_func_addr);

    // 调用inlineHook进行Hook
    if (registerInlineHook((uint32_t)target_func_addr, (uint32_t)new_validate, (uint32_t**)&original_validate) == ELE7EN_OK) {
        if (inlineHook((uint32_t)target_func_addr) == ELE7EN_OK) {
            LOGI("Inline hook for '%s' successfully applied!", target_symbol);
        } else {
            LOGE("inlineHook failed for '%s'.", target_symbol);
        }
    } else {
        LOGE("registerInlineHook failed for '%s'.", target_symbol);
    }
}
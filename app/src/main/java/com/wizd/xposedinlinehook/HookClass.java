package com.wizd.xposedinlinehook;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.os.Bundle;
import android.util.Log;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class HookClass implements IXposedHookLoadPackage {

    private static final String TAG = "XposedCrack";
    private static final String TARGET_GAME_PACKAGE_NAME = "com.netease.x19";
    private static final String TARGET_MODULE_PACKAGE_NAME = "helper.creeperbox";
    private static boolean hasHooked = false;

    // JNI函数，它会执行最终的so hook，并返回一个布尔值表示是否成功
    public static native boolean applyCrack(String targetSoPath);

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam lpparam) throws Throwable {
        if (!lpparam.packageName.equals(TARGET_GAME_PACKAGE_NAME)) {
            return;
        }

        // 在最早的时机注入
        XposedHelpers.findAndHookMethod("android.app.Application", lpparam.classLoader, "attach", Context.class, new XC_MethodHook() {
            @Override
            protected void afterHookedMethod(final MethodHookParam param) throws Throwable {
                if (hasHooked) return;
                hasHooked = true;

                final Context appContext = (Context) param.args[0];

                try {
                    // 1. 准备好【助手模块】的ClassLoader
                    ApplicationInfo targetModuleInfo = appContext.getPackageManager().getApplicationInfo(TARGET_MODULE_PACKAGE_NAME, 0);
                    final String targetModuleApkPath = targetModuleInfo.sourceDir;
                    final ClassLoader targetClassLoader = new PathClassLoader(targetModuleApkPath, appContext.getClassLoader());

                    // 2. 加载我们自己的so，做好潜伏准备
                    System.loadLibrary("crack");
                    Log.i(TAG, "破解模块so已潜伏。");

                    // 3. 锁定“登录”按钮的点击事件
                    XposedHelpers.findAndHookMethod(
                            "helper.creeperbox.v", // 目标类
                            targetClassLoader,
                            "onClick",             // 目标方法
                            DialogInterface.class, int.class,
                            new XC_MethodHook() {
                                @Override
                                protected void beforeHookedMethod(MethodHookParam hookParam) throws Throwable {
                                    Log.i(TAG, "登录按钮被点击，准备执行致命一击！");

                                    // 4. 致命一击：在点击的瞬间，才执行so hook
                                    // 我们需要找到目标so的真实路径
                                    String targetSoPath = targetModuleApkPath.replace("base.apk", "lib/arm64/libcreeperbox.so"); // 这是一个合理的猜测
                                    boolean crackSuccess = applyCrack(targetSoPath);

                                    if (!crackSuccess) {
                                        Log.e(TAG, "致命一击失败！so hook未能成功应用。");
                                    }
                                    // 无论成功与否，都让原始的onClick继续执行。
                                    // 如果成功，它将调用被我们篡改的so；如果失败，它将执行原始逻辑。
                                }
                            }
                    );
                } catch (Exception e) {
                    Log.e(TAG, "破解模块准备阶段失败: " + e.toString());
                }
            }
        });
    }
}
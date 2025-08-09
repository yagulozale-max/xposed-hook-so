package com.wizd.xposedinlinehook;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class HookClass implements IXposedHookLoadPackage {

    private static final String TAG = "XposedFinalCrack_Java";
    private static final String TARGET_PACKAGE_NAME = "com.netease.x19";

    // 声明一个本地方法，用于启动原生层的监控
    public static native void initiateNativeHook();

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam lpparam) throws Throwable {
        // 仅对目标游戏进程生效
        if (!lpparam.packageName.equals(TARGET_PACKAGE_NAME)) {
            return;
        }

        XposedBridge.log(TAG + ": v15.0 原生守望者 - 已进入目标游戏进程: " + lpparam.processName);

        // Hook Application的onCreate方法，这是一个可靠的早期执行点
        XposedHelpers.findAndHookMethod(
                "android.app.Application",
                lpparam.classLoader,
                "onCreate",
                new XC_MethodHook() {
                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        XposedBridge.log(TAG + ": Application.onCreate() hooked. Process: " + lpparam.processName);

                        // 确保只在主进程中加载和初始化一次
                        if (lpparam.processName.equals(TARGET_PACKAGE_NAME)) {
                            try {
                                XposedBridge.log(TAG + ": 准备加载原生库 libcrack.so...");
                                System.loadLibrary("crack");
                                XposedBridge.log(TAG + ": 原生库 libcrack.so 加载成功.");

                                XposedBridge.log(TAG + ": 准备调用 native 方法 initiateNativeHook() 启动监控线程...");
                                initiateNativeHook();
                                XposedBridge.log(TAG + ": native 方法 initiateNativeHook() 调用完毕.");

                            } catch (Throwable t) {
                                XposedBridge.log(TAG + ": 加载或初始化原生库时发生严重错误.");
                                XposedBridge.log(t);
                            }
                        }
                    }
                }
        );
    }
}
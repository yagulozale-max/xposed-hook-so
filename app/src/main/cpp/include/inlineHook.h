#ifndef _INLINEHOOK_H
#define _INLINEHOOK_H

#include <stdio.h>
#include <stdint.h> // 引入stdint.h以使用uintptr_t

#ifdef __cplusplus
extern "C" {
#endif

enum ele7en_status {
    ELE7EN_ERROR_UNKNOWN = -1,
    ELE7EN_OK = 0,
    ELE7EN_ERROR_NOT_INITIALIZED,
    ELE7EN_ERROR_NOT_EXECUTABLE,
    ELE7EN_ERROR_NOT_REGISTERED,
    ELE7EN_ERROR_NOT_HOOKED,
    ELE7EN_ERROR_ALREADY_REGISTERED,
    ELE7EN_ERROR_ALREADY_HOOKED,
    ELE7EN_ERROR_SO_NOT_FOUND,
    ELE7EN_ERROR_FUNCTION_NOT_FOUND
};

// 将所有 uint32_t 地址参数修改为 uintptr_t，它可以同时兼容32位和64位
enum ele7en_status registerInlineHook(uintptr_t target_addr, uintptr_t new_addr, uintptr_t **proto_addr);
enum ele7en_status inlineUnHook(uintptr_t target_addr);
void inlineUnHookAll();
enum ele7en_status inlineHook(uintptr_t target_addr);
void inlineHookAll();

#ifdef __cplusplus
}
#endif

#endif

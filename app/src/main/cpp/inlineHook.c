#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdint.h>

#include "include/relocate.h"
#include "include/inlineHook.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_START(addr)	(~(PAGE_SIZE - 1) & (addr))
#define SET_BIT0(addr)		((uintptr_t)addr | 1)
#define CLEAR_BIT0(addr)	((uintptr_t)addr & ~1)
#define TEST_BIT0(addr)		((uintptr_t)addr & 1)

#define bool    int
#define true    1
#define false   0

enum hook_status {
    REGISTERED,
    HOOKED,
};

struct inlineHookItem {
    uintptr_t target_addr;
    uintptr_t new_addr;
    uintptr_t **proto_addr;
    void *orig_instructions;
    int orig_boundaries[4];
    int trampoline_boundaries[20];
    int count;
    void *trampoline_instructions;
    int length;
    int status;
    int mode;
};

struct inlineHookInfo {
    struct inlineHookItem item[1024];
    int size;
};

static struct inlineHookInfo info = {0};

static bool isExecutableAddr(uintptr_t addr) {
    FILE *fp;
    char line[1024];
    uintptr_t start, end;

    fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) return false;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "r-xp") || strstr(line, "rwxp")) {
            sscanf(line, "%lx-%lx", &start, &end);
            if (addr >= start && addr <= end) {
                fclose(fp);
                return true;
            }
        }
    }
    fclose(fp);
    return false;
}

static struct inlineHookItem *findInlineHookItem(uintptr_t target_addr) {
    int i;
    for (i = 0; i < info.size; ++i) {
        if (info.item[i].target_addr == target_addr) {
            return &info.item[i];
        }
    }
    return NULL;
}

static struct inlineHookItem *addInlineHookItem() {
    if (info.size >= 1024) return NULL;
    return &info.item[info.size++];
}

static void deleteInlineHookItem(int pos) {
    info.item[pos] = info.item[info.size - 1];
    --info.size;
}

enum ele7en_status registerInlineHook(uintptr_t target_addr, uintptr_t new_addr, uintptr_t **proto_addr) {
    struct inlineHookItem *item;

    if (!isExecutableAddr(target_addr) || !isExecutableAddr(new_addr)) {
        return ELE7EN_ERROR_NOT_EXECUTABLE;
    }

    item = findInlineHookItem(target_addr);
    if (item != NULL) {
        return item->status == REGISTERED ? ELE7EN_ERROR_ALREADY_REGISTERED : ELE7EN_ERROR_ALREADY_HOOKED;
    }

    item = addInlineHookItem();
    item->target_addr = target_addr;
    item->new_addr = new_addr;
    item->proto_addr = proto_addr;

    item->length = TEST_BIT0(item->target_addr) ? 12 : 8;
    item->orig_instructions = malloc(item->length);
    memcpy(item->orig_instructions, (void *) CLEAR_BIT0(item->target_addr), item->length);

    item->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    relocateInstruction((uint32_t)target_addr, item->orig_instructions, item->length, item->trampoline_instructions, item->orig_boundaries, item->trampoline_boundaries, &item->count);

    item->status = REGISTERED;
    return ELE7EN_OK;
}

static void doInlineUnHook(struct inlineHookItem *item, int pos) {
    mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy((void *) CLEAR_BIT0(item->target_addr), item->orig_instructions, item->length);
    mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_EXEC);
    munmap(item->trampoline_instructions, PAGE_SIZE);
    free(item->orig_instructions);
    deleteInlineHookItem(pos);
    __builtin___clear_cache((char*)CLEAR_BIT0(item->target_addr), (char*)(CLEAR_BIT0(item->target_addr) + item->length));
}

enum ele7en_status inlineUnHook(uintptr_t target_addr) {
    int i;
    for (i = 0; i < info.size; ++i) {
        if (info.item[i].target_addr == target_addr && info.item[i].status == HOOKED) {
            doInlineUnHook(&info.item[i], i);
            return ELE7EN_OK;
        }
    }
    return ELE7EN_ERROR_NOT_HOOKED;
}

void inlineUnHookAll() {
    int i;
    for (i = 0; i < info.size; ++i) {
        if (info.item[i].status == HOOKED) {
            doInlineUnHook(&info.item[i], i--);
        }
    }
}

static void doInlineHook(struct inlineHookItem *item) {
    mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_WRITE | PROT_EXEC);

    if (item->proto_addr != NULL) {
        *(item->proto_addr) = (uintptr_t *) (TEST_BIT0(item->target_addr) ? SET_BIT0(item->trampoline_instructions) : (uintptr_t)item->trampoline_instructions);
    }

    if (TEST_BIT0(item->target_addr)) {
        int i = 0;
        if (CLEAR_BIT0(item->target_addr) % 4 != 0) {
            ((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xBF00;
        }
        ((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xF8DF;
        ((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xF000;
        ((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = (uint16_t)(item->new_addr & 0xFFFF);
        ((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = (uint16_t)(item->new_addr >> 16);
    } else {
        ((uint32_t *) (item->target_addr))[0] = 0xe51ff004;
        ((uint32_t *) (item->target_addr))[1] = (uint32_t)item->new_addr;
    }

    mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_EXEC);
    item->status = HOOKED;
    __builtin___clear_cache((char*)CLEAR_BIT0(item->target_addr), (char*)(CLEAR_BIT0(item->target_addr) + item->length));
}

enum ele7en_status inlineHook(uintptr_t target_addr) {
    struct inlineHookItem *item = findInlineHookItem(target_addr);
    if (item == NULL) return ELE7EN_ERROR_NOT_REGISTERED;
    if (item->status == HOOKED) return ELE7EN_ERROR_ALREADY_HOOKED;

    doInlineHook(item);
    return ELE7EN_OK;
}

void inlineHookAll() {
    int i;
    for (i = 0; i < info.size; ++i) {
        if (info.item[i].status == REGISTERED) {
            doInlineHook(&info.item[i]);
        }
    }
}

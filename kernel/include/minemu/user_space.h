#ifndef MINEMU_USER_SPACE_H
#define MINEMU_USER_SPACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "minemu/boot.h"
#include "minemu/mmu.h"

#define USER_IMAGE_BASE UINT32_C(0x00400000)
#define USER_STACK_BOTTOM UINT32_C(0x007ff000)
#define USER_STACK_TOP UINT32_C(0x00800000)

bool user_space_map_system_rom(const struct minemu_boot_info *boot_info);
bool user_space_load_module(const struct minemu_boot_info *boot_info, uint32_t *entry_vaddr);

bool user_range_readable(uint32_t address, uint32_t length);
bool user_range_writable(uint32_t address, uint32_t length);

bool copy_from_user(void *destination, uint32_t source, uint32_t length);
bool copy_to_user(uint32_t destination, const void *source, uint32_t length);

void minemu_enter_user(uint32_t entry_vaddr, uint32_t stack_top) __attribute__((noreturn));

#endif

#include "minemu/boot.h"
#include "minemu/trace.h"
#include "minemu/trap.h"
#include "minemu/uart.h"
#include "minemu/user_space.h"
#include "minemu/debug.h"

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    uart_init();

    if (!user_space_map_system_rom(boot_info)) {
        minemu_trace_event(UINT32_C(0xb007bad1));
        minemu_fail_stop();
    }

    uint32_t entry_vaddr = 0;
    if (!user_space_load_module(boot_info, &entry_vaddr)) {
        minemu_trace_event(UINT32_C(0xb007bad2));
        minemu_fail_stop();
    }
    minemu_enter_user(entry_vaddr, USER_STACK_TOP);
}

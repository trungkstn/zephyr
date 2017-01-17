/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call _Cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel.h>
#include <stdint.h>
#include <toolchain.h>
#include <linker-defs.h>
#include <nano_internal.h>

#ifdef CONFIG_ARMV6_M
static inline void relocate_vector_table(void) { /* do nothing */ }
#elif defined(CONFIG_ARMV7_M)
#ifdef CONFIG_XIP
static inline void relocate_vector_table(void)
{
	/* vector table is located at the the beginning of the flash */
	_scs_relocate_vector_table((void *)CONFIG_FLASH_BASE_ADDRESS);
}
#else
static inline void relocate_vector_table(void)
{
	/* vector table is already in SRAM, just point to it */
	_scs_relocate_vector_table((void *)CONFIG_SRAM_BASE_ADDRESS);
}
#endif
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMv6_M */

#ifdef CONFIG_FLOAT
static inline void enable_floating_point(void)
{
	/*
	 * Upon reset, the Co-Processor Access Control Register is 0x00000000.
	 * Enable CP10 and CP11 coprocessors to enable floating point.
	 */
	__scs.cpacr.val = (_SCS_CPACR_CP10_FULL_ACCESS |
				_SCS_CPACR_CP11_FULL_ACCESS);

	/*
	 * Upon reset, the FPU Context Control Register is 0xC0000000
	 * (both Automatic and Lazy state preservation is enabled).
	 * Disable lazy state preservation so the volatile FP registers are
	 * always saved on exception.
	 */
	__scs.fpu.ccr.val = (_SCS_FPU_CCR_ASPEN_ENABLE |
				_SCS_FPU_CCR_LSPEN_DISABLE);

	/*
	 * Although automatic state preservation is enabled, the processor
	 * does not automatically save the volatile FP registers until they
	 * have first been touched. Perform a dummy move operation so that
	 * the stack frames are created as expected before any task or fiber
	 * context switching can occur.
	 */
	__asm__ volatile(
		"vmov s0, s0;\n\t"
		"dsb;\n\t"
		"isb;\n\t"
		);
}
#else
static inline void enable_floating_point(void)
{
}
#endif

extern FUNC_NORETURN void _Cstart(void);
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void _PrepC(void)
{
	relocate_vector_table();
	enable_floating_point();
	_bss_zero();
	_data_copy();
	_Cstart();
	CODE_UNREACHABLE;
}

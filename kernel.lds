ENTRY(_start)

L1_PAGE_SIZE =  0x100000;
RAM_END =     0x10000000; /* highest address of physical ram */

SECTIONS
{
	/**
		Bootloader and init 
	**/
	. = 0x00008000;
	.init : {
		_init_start = .;
		*(.init)
		_init_end = .;
	}

	/** 
		Kernel
	**/
	. = ALIGN(L1_PAGE_SIZE);
	/* .text */
	.text_kernel : {
		_text_kernel_start = .;
		build/arch/*(.text)
		build/kernel/*(.text)
		build/lib/*(.text)
		_text_kernel_end = .;
	}
	/* .bss (globals) and .data(statics) */
	. = ALIGN(L1_PAGE_SIZE);
	.bss_kernel : {
		_bss_kernel_start = .;
		build/arch/*(.bss)
		build/arch/*(COMMON) /**/
		build/kernel/*(.bss)
		build/kernel/*(COMMON) /**/
		build/lib/*(.bss)
		build/lib/*(COMMON) /**/
		_bss_kernel_end = .;
	}
	.data_kernel : {
		_data_kernel_start = .;
		build/arch/*(.data)
		build/kernel/*(.data)
		build/lib/*(.data)
		_data_kernel_end = .;
	}
	. = ALIGN(L1_PAGE_SIZE);
	.ram_kernel : {
		_ram_kernel_start = .;
		. = . + L1_PAGE_SIZE;
		_ram_kernel_end = . - 8;
	}
	.orig_globals : {
		_orig_globals_start = .;
		. = . + L1_PAGE_SIZE;
		_orig_globals_end = . - 8;
	}

	/** 
		User
	**/
	. = ALIGN(L1_PAGE_SIZE);
	.text_user : { 
		_text_user_start = .;
		build/user/*(.text)
		_text_user_end = .;
	}

	/* Virtual process space 
	this is the virtual address space for processes and the
	pyhsical address space for the original global variables*/
	. = ALIGN(L1_PAGE_SIZE);
	_ram_user_start = .;
	.bss_user : {
		_bss_user_start = .;
		build/user/*(.bss)
		build/user/*(COMMON) /**/
		_bss_user_end = .;
	}
	.data_user : {
		_data_user_start = .;
		build/user/*(.data)
		_data_user_end = .;
	}
	_stack_user_start = .;
	. = ALIGN(L1_PAGE_SIZE);
	_stack_user_end = .;
	_ram_user_end = .;

	/* Physical process space */
	_phys_ram_user_start = .;
	. = RAM_END;
	_phys_ram_user_end = .;
}


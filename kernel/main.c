// MIT License, Copyright (c) 2020 Marvin Borner

#include <boot.h>
#include <cpu.h>
#include <fs.h>
#include <ide.h>
#include <interrupts.h>
#include <keyboard.h>
#include <load.h>
#include <mem.h>
#include <mouse.h>
#include <net.h>
#include <pci.h>
#include <random.h>
#include <serial.h>
#include <syscall.h>
#include <timer.h>

void kernel_main(struct vid_info *vid_info)
{
	heap_init(0x00f00000 + rand());

	boot_passed = vid_info;

	// Serial connection
	serial_install();
	serial_print("\nKernel was compiled at " __TIME__ " on " __DATE__ "\n");
	serial_print("Serial connected.\n");

	cpu_enable_features();
	cpu_print();

	// Install drivers
	vfs_install();
	device_install();
	ata_install();
	pci_install();
	interrupts_install();
	timer_install();
	keyboard_install();
	mouse_install();
	/* net_install(); */

	// Enable drivers
	sti();
	keyboard_reset();

	syscall_init();
	proc_init();

	idle();
}

#include <kernel/fs/load.h>
#include <kernel/system.h>
#include <kernel/lib/stdio.h>
#include <kernel/lib/lib.h>
#include <kernel/fs/ext2.h>

void load_binaries()
{
	font = (struct font *)read_file("/bin/font");

	log("Successfully loaded binaries");
}
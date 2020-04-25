#include <kernel/system.h>
#include <kernel/fs/elf.h>
#include <kernel/lib/stdio.h>
#include <kernel/memory/alloc.h>
#include <kernel/lib/lib.h>
#include <kernel/memory/paging.h>
#include <kernel/fs/ext2.h>

int is_elf(elf_header_t *header)
{
	if (header->ident[0] == ELF_MAG && header->ident[1] == 'E' && header->ident[2] == 'L' &&
	    header->ident[3] == 'F' && header->ident[4] == ELF_32 &&
	    header->ident[5] == ELF_LITTLE && header->ident[6] == ELF_CURRENT &&
	    header->machine == ELF_386 && (header->type == ET_REL || header->type == ET_EXEC)) {
		return 1;
	}
	return 0;
}

void elf_load(char *path)
{
	uint8_t *file = read_file(path);
	if (!file) {
		warn("File or directory not found: %s", file);
		return;
	}

	elf_header_t *header = (elf_header_t *)file;
	elf_program_header_t *program_header = (void *)header + header->phoff;

	if (!is_elf(header)) {
		warn("File not valid: %s", path);
		return;
	} else {
		debug("File is valid: %s", path);
	}

	uint32_t seg_begin, seg_end;
	for (int i = 0; i < header->phnum; i++) {
		if (program_header->type == 1) {
			seg_begin = program_header->vaddr;
			seg_end = seg_begin + program_header->memsz;

			for (uint32_t z = 0; z < seg_end - seg_begin; z += 4096)
				paging_map((uint32_t)seg_begin + z, (uint32_t)seg_begin + z,
					   PT_PRESENT | PT_RW | PT_USED | PT_ALL_PRIV);

			memcpy((void *)seg_begin, file + program_header->offset,
			       program_header->filesz);
			memset((void *)(seg_begin + program_header->filesz), 0,
			       program_header->memsz - program_header->filesz);

			// Code segment
			if (program_header->flags == PF_X + PF_R + PF_W ||
			    program_header->flags == PF_X + PF_R) {
				debug("Found code segment");
				// current_process->regs.eip = header->entry + seg_begin;
			}
		}
		program_header++;
	}
}
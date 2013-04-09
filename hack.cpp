/*
 * ,------.  ,--.               ,--.   ,--.
 * |  .-.  \ `--' ,---.  ,--,--.|  |-. |  | ,---.
 * |  |  \  :,--.(  .-' ' ,-.  || .-. '|  || .-. :
 * |  '--'  /|  |.-'  `)\ '-'  || `-' ||  |\   --.
 * `-------' `--'`----'  `--`--' `---' `--' `----'
 *
 *                          ,---.
 *           ,----.         |   |
 * ,--,--,--.'.-.  |,--.,--.|  .'
 * |        |  .' < |  ||  ||  |
 * |  |  |  |/'-'  |'  ''  '`--'
 * `--`--`--'`----'  `----' .--.
 *                          '--'
 *
 * Disable m3u - An iTunes plugin to disable m3u playlists processing
 *
 * Copyright (c) fG!, 2011, 2012, 2013 - reverser@put.as - http://reverse.put.as
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * hack.cpp
 *
 */

#include "hack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_types.h>
#include <mach/mach_vm.h>
#include <errno.h>
#include <sys/types.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

#define EXIT_ON_MACH_ERROR(msg, retval) \
if (kr != KERN_SUCCESS) { mach_error(msg ":" , kr); exit((retval)); }

#define DEBUG 1

// local functions
static void patchmemory(mach_vm_address_t address);
static void unpatchmemory(struct header_info *hi);
static int find_image(struct header_info *hi);
static void find_and_patch_addresses(struct header_info *hi);

#pragma mark The exported functions

void
disable_m3u_processing(struct header_info *hi)
{
    if (find_image(hi)) return;
    find_and_patch_addresses(hi);
    hi->active = 0;
}

void
enable_m3u_processing(struct header_info *hi)
{
    unpatchmemory(hi);
    hi->active = 0;
}

#pragma mark Internal helper functions

/*
 * patch addresses by modifying m3u string to m5u
 */
void
patchmemory(mach_vm_address_t address)
{
	kern_return_t kr = 0;
	mach_port_t myself = 0;
	// get a send right
	myself = mach_task_self();
#if DEBUG
	printf("[DEBUG] Changing memory protections...\n");
#endif
	// change memory protection
	kr = mach_vm_protect(myself, address, (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	
	// the new byte to write
	uint8_t opcode = 0x35;
	// number of bytes to write
	mach_vm_size_t len = 1;
#if DEBUG
	printf("[DEBUG] Patching bytes...\n");
#endif
	// and write the new byte (strings will be modified to m5u and m5u8)
	kr = mach_vm_write(myself, address+1, (vm_offset_t)&opcode, len);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
    // restore original protections
	kr = mach_vm_protect(myself, address, (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
}

/*
 * iterate over the patch_addresses array and restore original byte
 */
void
unpatchmemory(struct header_info *hi)
{
	kern_return_t kr = 0;
	mach_port_t myself = 0;
	// get a send right
	myself = mach_task_self();
#if DEBUG
	printf("[DEBUG] Changing memory protections...\n");
#endif
    int index = 0;
    while (hi->patch_addresses[index] != 0)
    {
        printf("Patching %p\n", (void*)hi->patch_addresses[index]);
        // change memory protection
        kr = mach_vm_protect(myself, hi->patch_addresses[index], (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
        EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
        // the new byte to write
        uint8_t opcode = 0x33;
        // number of bytes to write
        mach_vm_size_t len = 1;
#if DEBUG
        printf("[DEBUG] Unpatching bytes...\n");
#endif
        // and write the new byte (strings will be modified to m3u and m3u8)
        kr = mach_vm_write(myself, hi->patch_addresses[index]+1, (vm_offset_t)&opcode, len);
        EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
        // restore original protections
        kr = mach_vm_protect(myself, hi->patch_addresses[index], (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
        EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
        index++;
    }
    memset(hi->patch_addresses, '\0', 16*sizeof(mach_vm_address_t));
}


/*
 * iterate over __cstring section to find location of m3u and m3u8 strings, and patch'em!
 * could be better :-)
 */
void
find_and_patch_addresses(struct header_info *hi)
{
    // get pointer to the __cstring section
    const struct section_64 *cstring_section = getsectbynamefromheader_64(hi->mh, "__TEXT", "__cstring");
    // header values do not have ASLR slide so fix it
    mach_vm_address_t cstring_addr = cstring_section->addr + hi->aslr_slide;
	// search for the addresses of the right strings to be patched
    char *cstring_mem = (char*)cstring_addr;
    int index = 0;
    
	for (uint64_t w = 0 ; w < cstring_section->size; w++)
	{   
		// match m3u
		if (*(int*)cstring_mem == 0x0075336d)
		{
            mach_vm_address_t patch_addr = cstring_addr + w;
            hi->patch_addresses[index++] = patch_addr;
            patchmemory(patch_addr);
#if DEBUG
			printf("[DEBUG] Found m3u address to be patched %p\n", (void*)patch_addr);
#endif
		}
		// match first bytes of m3u8
		else if (*(int*)cstring_mem == 0x3875336d)
		{
            mach_vm_address_t patch_addr = cstring_addr + w;
            hi->patch_addresses[index++] = patch_addr;
            patchmemory(patch_addr);
#if DEBUG
			printf("[DEBUG] Found m3u8 address to be patched %p\n", (void*)patch_addr);
#endif
		}
		// move to the next byte in the array
		cstring_mem++;
	}
}


/*
 * find the memory address where the target was loaded to
 * this is due to full ASLR in Lion
 * we also use it for Snow Leopard to avoid opening the binary and parse the mach-o header
 */
int
find_image(struct header_info *hi)
{
    // get mach-o header of iTunes image (main binary is usually (always?) index 0)
    // XXX: this can be made error-proof by iterating all images and match iTunes
    const struct mach_header *mh = _dyld_get_image_header(0);
    intptr_t aslr_slide = _dyld_get_image_vmaddr_slide(0);
#if DEBUG
    printf("[DEBUG] Aslr slide is %p\n", (void*)aslr_slide);
#endif

    // compute iTunes image size
    uint64_t image_size = 0;
    if (mh->magic != MH_MAGIC_64) return 1;
    
    struct load_command *load_cmd = NULL;
    char *load_cmd_addr = (char*)mh + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < mh->ncmds; i++)
    {
        load_cmd = (struct load_command*)load_cmd_addr;
        if (load_cmd->cmd == LC_SEGMENT_64)
        {
            struct segment_command_64 *seg_cmd = (struct segment_command_64*)load_cmd;
            image_size += seg_cmd->vmsize;
        }
        load_cmd_addr += load_cmd->cmdsize;
    }
    
    hi->mh = (const struct mach_header_64*)mh;
    hi->image_size = image_size;
    hi->aslr_slide = aslr_slide;
    memset(hi->patch_addresses, '\0', 16*sizeof(mach_vm_address_t));
	return 0;
}

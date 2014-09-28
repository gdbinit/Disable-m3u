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
 * Copyright (c) fG!, 2011, 2012, 2013, 2014 - reverser@put.as - http://reverse.put.as
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

#define DEBUG 1

#define ERROR_MSG(fmt, ...) fprintf(stderr, "[ERROR] " fmt " \n", ## __VA_ARGS__)
#define OUTPUT_MSG(fmt, ...) fprintf(stdout, fmt " \n", ## __VA_ARGS__)
#if DEBUG == 0
#   define DEBUG_MSG(fmt, ...) do {} while (0)
#else
#   define DEBUG_MSG(fmt, ...) fprintf(stdout, "[DEBUG] " fmt "\n", ## __VA_ARGS__)
#endif

// local functions
static int patchmemory(mach_vm_address_t address);
static int unpatchmemory(struct header_info *hi);
static int find_image(struct header_info *hi);
static int find_and_patch_addresses(struct header_info *hi);

#pragma mark The exported functions

void
disable_m3u_processing(struct header_info *hi)
{
    if (find_image(hi) != 0)
    {
        ERROR_MSG("Failed to find valid image.");
        return;
    }
    
    if (find_and_patch_addresses(hi) != 0)
    {
        ERROR_MSG("Failed to find and patch addresses.");
        return;
    }
    hi->active = 1;
}

void
enable_m3u_processing(struct header_info *hi)
{
    if (unpatchmemory(hi) != 0)
    {
        ERROR_MSG("Failed to restore original bytes.");
        return;
    }
    hi->active = 0;
}

#pragma mark Internal helper functions

/*
 * patch addresses by modifying m3u string to m5u
 */
int
patchmemory(mach_vm_address_t address)
{
	kern_return_t kr = 0;
	// change memory protection
	kr = mach_vm_protect(mach_task_self(), address, (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS)
    {
        ERROR_MSG("Failed to change memory protection.");
        return -1;
    }
	// the new byte to write
	uint8_t opcode = 0x35;
	// number of bytes to write
	mach_vm_size_t len = 1;
	DEBUG_MSG("Patching bytes...\n");
	// and write the new byte (strings will be modified to m5u and m5u8)
	kr = mach_vm_write(mach_task_self(), address+1, (vm_offset_t)&opcode, len);
    if (kr != KERN_SUCCESS)
    {
        ERROR_MSG("Failed to write to memory.");
        return -1;
    }
    // restore original protections
	kr = mach_vm_protect(mach_task_self(), address, (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS)
    {
        ERROR_MSG("Failed to change memory protection.");
        return -1;
    }
    return 0;
}

/*
 * iterate over the patch_addresses array and restore original byte
 */
int
unpatchmemory(struct header_info *hi)
{
	kern_return_t kr = 0;
    int index = 0;
    while (hi->patch_addresses[index] != 0)
    {
        printf("Patching %p\n", (void*)hi->patch_addresses[index]);
        // change memory protection
        kr = mach_vm_protect(mach_task_self(), hi->patch_addresses[index], (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
        if (kr != KERN_SUCCESS)
        {
            ERROR_MSG("Failed to change memory protection.");
            return -1;
        }
        // the new byte to write
        uint8_t opcode = 0x33;
        // number of bytes to write
        mach_vm_size_t len = 1;
        DEBUG_MSG("Restoring original bytes...\n");
        // and write the new byte (strings will be modified to m3u and m3u8)
        kr = mach_vm_write(mach_task_self(), hi->patch_addresses[index]+1, (vm_offset_t)&opcode, len);
        if (kr != KERN_SUCCESS)
        {
            ERROR_MSG("Failed to write to memory.");
            return -1;
        }
        // restore original protections
        kr = mach_vm_protect(mach_task_self(), hi->patch_addresses[index], (mach_vm_size_t)8, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
        if (kr != KERN_SUCCESS)
        {
            ERROR_MSG("Failed to change memory protection.");
            return -1;
        }
        index++;
    }
    memset(hi->patch_addresses, '\0', 16*sizeof(mach_vm_address_t));
    return 0;
}

/*
 * iterate over __cstring section to find location of m3u and m3u8 strings, and patch'em!
 * could be better :-)
 */
int
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
            DEBUG_MSG("Found m3u address to be patched %p\n", (void*)patch_addr);
            if (patchmemory(patch_addr) != 0)
            {
                ERROR_MSG("Failed to patch m3u.");
                return -1;
            }
			
		}
		// match first bytes of m3u8
		else if (*(int*)cstring_mem == 0x3875336d)
		{
            mach_vm_address_t patch_addr = cstring_addr + w;
            hi->patch_addresses[index++] = patch_addr;
            DEBUG_MSG("Found m3u8 address to be patched %p\n", (void*)patch_addr);
            if (patchmemory(patch_addr) != 0)
            {
                ERROR_MSG("Failed to patch m3u8.");
                return -1;
            }
		}
		// move to the next byte in the array
		cstring_mem++;
	}
    return 0;
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
    DEBUG_MSG("Aslr slide is %p\n", (void*)aslr_slide);

    hi->mh = (const struct mach_header_64*)mh;
    hi->aslr_slide = aslr_slide;
    
    // compute iTunes image size
    uint64_t image_size = 0;
    if (mh->magic != MH_MAGIC_64)
    {
        return -1;
    }
    
    if (mh->ncmds == 0 || mh->sizeofcmds == 0)
    {
        ERROR_MSG("Invalid number of commands or size.");
    }
    
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
    
    if (image_size != 0)
    {
        hi->image_size = image_size;
        return 0;
    }
    else
    {
        /* failure */
        return -1;
    }
}

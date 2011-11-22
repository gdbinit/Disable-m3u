/*
 *  hack.cpp
 *  iTunesPlugIn
 *
 *  Created by fG! on 8/29/11.
 *  Copyright 2011 fG!. All rights reserved.
 *
 *  reverser@put.as - http://reverse.put.as
 *
 */

#include "hack.h"


#define DEBUG 1
uint8_t patchactivated = 0;
// variables to hold addresses of __cstring and __text
uint64_t cstringaddr;
uint64_t cstringsize;
uint64_t textbaseaddr;
mach_vm_address_t address1, address2;


void patchmemory(void)
{
	kern_return_t kr;
	mach_port_t myself = 0;
	// get a send right
	myself = mach_task_self();
#if DEBUG
	printf("[DEBUG] Changing memory protections...\n");
#endif
	// change memory protection
	kr = mach_vm_protect(myself, (mach_vm_address_t)address1, (mach_vm_size_t)4096, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	kr = mach_vm_protect(myself, (mach_vm_address_t)address2, (mach_vm_size_t)4096, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	
	// the new byte to write
	uint8_t opcode = 0x35;
	// number of bytes to write
	mach_vm_size_t len = 1;
#if DEBUG
	printf("[DEBUG] Patching bytes...\n");
#endif
	// and write the new byte (strings will be modified to m5u and m5u8)
	kr = mach_vm_write(myself, (mach_vm_address_t)address1+1, (vm_offset_t)&opcode, len); 
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	kr = mach_vm_write(myself, (mach_vm_address_t)address2+1, (vm_offset_t)&opcode, len);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
}

void unpatchmemory(void)
{
	kern_return_t kr;
	mach_port_t myself=0;
	// get a send right
	myself = mach_task_self();
#if DEBUG
	printf("[DEBUG] Changing memory protections...\n");
#endif	
	// change memory protection
	kr = mach_vm_protect(myself, (mach_vm_address_t)address1, (mach_vm_size_t)4096, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	kr = mach_vm_protect(myself, (mach_vm_address_t)address2, (mach_vm_size_t)4096, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	
	// the new byte to write
	uint8_t opcode = 0x33;
	// number of bytes to write
	mach_vm_size_t len = 1;
#if DEBUG
	printf("[DEBUG] Unpatching bytes...\n");
#endif	
	// and write the new byte (strings will be modified to m3u and m3u8)
	kr = mach_vm_write(myself, (mach_vm_address_t)address1+1, (vm_offset_t)&opcode, len); 
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	kr = mach_vm_write(myself, (mach_vm_address_t)address2+1, (vm_offset_t)&opcode, len);
	EXIT_ON_MACH_ERROR("mach_vm_protect", kr);
	patchactivated = 0;
}


void find_patchaddresses(mach_port_t myself, uint64_t imageaddress, uint64_t imagesize)
{
	kern_return_t kr;
	mach_msg_type_number_t bytesread;
	vm_offset_t cstringarray;
	
	// read the whole __TEXT segment into memory
	vm_offset_t executable;
	kr = mach_vm_read(myself, (mach_vm_address_t)imageaddress, (mach_vm_size_t)imagesize, &executable, &bytesread);
	EXIT_ON_MACH_ERROR("mach_vm_read", kr);
	
	// get the address of the __cstring and __text section
	get_textseg_vmaddr(executable, 0);
#if DEBUG
	printf("[DEBUG] cstringaddr %p, cstringsize %p, __text %p\n", (void*)cstringaddr, (void*)cstringsize, (void*)textbaseaddr);
#endif
	/* 
	 we need to compute the real address for Lion due to ASLR
	 imageaddress gives us the aslr'ed address of where program image is located (LC_SEGMENT_64/__TEXT)
	 (cstringaddr - textbaseaddr) computes the delta from cstring to __TEXT load cmd
	 which we can add to imageaddress to find the aslr'ed address of cstring segment
	 */
#if __LP64__
	cstringaddr = (cstringaddr - textbaseaddr) + imageaddress;
#endif
	// read the whole __cstring section
	kr = mach_vm_read(myself, (mach_vm_address_t)cstringaddr, (mach_vm_size_t)cstringsize, &cstringarray, &bytesread);
	EXIT_ON_MACH_ERROR("mach_vm_read", kr);
	
	// search for the addresses of the right strings to be patched
	
	uint32_t w;
	for (w=0 ; w < cstringsize; w++)
	{   
		// match m3u
		if (*(int*)cstringarray == 0x0075336d)
		{
			address1 = cstringaddr + w;
#if DEBUG
			printf("[DEBUG] Found 1st address to be patched %p\n", (void*)address1);
#endif
		}
		// match first bytes of m3u8
		else if (*(int*)cstringarray == 0x3875336d)
		{
			address2 = cstringaddr + w;
#if DEBUG
			printf("[DEBUG] Found 2nd address to be patched %p\n", (void*)address2);
#endif
		}
		// move to the next byte in the array
		(char*)cstringarray++;
	}
	// and deallocate
	kr = mach_vm_deallocate(mach_task_self(), cstringarray, cstringsize);
	EXIT_ON_MACH_ERROR("mach_vm_deallocate", kr);
	kr = mach_vm_deallocate(mach_task_self(), executable, imagesize);
	EXIT_ON_MACH_ERROR("mach_vm_deallocate", kr);	
}

/*
 find the memory address where the target was loaded to
 this is due to full ASLR in Lion
 we also use it for Snow Leopard to avoid opening the binary and parse the mach-o header
 */
uint64_t find_image(mach_port_t targettask, uint64_t *addr, uint64_t *size)
{
	kern_return_t kr;
	vm_address_t address = 0;
	vm_size_t lsize = 0;
	uint32_t depth = 1;
	mach_msg_type_number_t bytesread;
	vm_offset_t magicnumber;
	
	while (1) 
	{
		struct vm_region_submap_info_64 info;
		mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
		kr = vm_region_recurse_64(targettask, &address, &lsize, &depth, (vm_region_info_64_t)&info, &count);
		if (kr == KERN_INVALID_ADDRESS)
		{
			break;
		}
		if (info.is_submap)
		{
			depth++;
		}
		else 
		{
			//do stuff
#if DEBUG
			printf ("[DEBUG] Found region: %08lx to %08lx\n", address, address+lsize);
#endif
			// try to read first 4 bytes
			kr = mach_vm_read(targettask, (mach_vm_address_t)address, (mach_vm_size_t)4, &magicnumber, &bytesread);
			// avoid deferencing an invalid memory location (for example PAGEZERO segment)
			if (kr == KERN_SUCCESS & bytesread == 4)
			{
				// verify if it's a mach-o binary at that memory location
				if (*(unsigned int*)magicnumber == MH_MAGIC ||
					*(unsigned int*)magicnumber == MH_MAGIC_64)
				{
#if DEBUG
					printf("[DEBUG] found program image!\n");
#endif
					*addr = address;
					*size = lsize;
					break;
				}
			}
			address += lsize;
		}
	}
	return(0);
}


// this will get the __TEXT segment vmaddr of any mach-o executable
uint64_t get_textseg_vmaddr (vm_offset_t buf, int8_t cputype)
{
    // STEP 1: parse MACH-O HEADER so we know what to dump
    
	uint32_t intel_base = 0x0, narchs = 0x0;
	uint64_t x64_base = 0x0;
#if __LP64__
	uint64_t vmaddr;
#else
	uint32_t vmaddr;
#endif
	unsigned int i=0;
    
	struct fat_header *fh; /* fat header */
	struct fat_arch *fa; /* fat arch */
	
	/* do we have a fat binary?
	 * if that's the case, start parsing the fat header
	 */
	if (ntohl(*(unsigned int*)buf) == FAT_MAGIC) 
	{
		fh = (struct fat_header *)(buf);
		narchs = ntohl(fh->nfat_arch);
#if DEBUG
		printf( "[DEBUG]  found fat header with %x archs\n", narchs);
#endif DEBUG
        fa = (struct fat_arch *)(buf + sizeof(struct fat_header));
        // grab the offsets from the header			   
        for (i = 0; i < narchs; i++) 
        {
            switch (ntohl(fa->cputype)) 
            {
                case CPU_TYPE_I386:
                    intel_base = ntohl(fa->offset);
#if DEBUG 
                    printf("[DEBUG] intel offset: %p\n", (void*)intel_base);
#endif
                    break;
					// FIXME
                case CPU_TYPE_X86_64:
                    x64_base = ntohl(fa->offset);
#if DEBUG 
                    printf("[DEBUG] intel x64 offset: %p\n", (void*)x64_base);
#endif												
                    break;
                default:
                    break;
            }
			
            fa += ((sizeof(struct fat_arch) * i) + 1);
        }
		// retrieve the offset information for current cpu
		printf("[INFO] Reading and processing the mach-o header...\n");
#if __LP64__
		vmaddr = get_offset(buf, x64_base, CPU_TYPE_X86_64);
#else
		vmaddr = get_offset(buf, intel_base, CPU_TYPE_I386);
#endif
	}
	else 
	{ /* if not, check the mach header */
#if DEBUG
		printf("[DEBUG]  this is not a fat binary, checking arch ...\n");
#endif
		fflush(stderr);
		if (*(unsigned int*)buf == MH_MAGIC ||
			*(unsigned int*)buf == MH_MAGIC_64)
		{
            // retrieve the offset information for current cpu
#if __LP64__
			vmaddr = get_offset(buf, 0, CPU_TYPE_X86_64);
#else
			vmaddr = get_offset(buf, 0, CPU_TYPE_I386);
#endif
		}
	}
    
	if (vmaddr != 1)
	{
		return(vmaddr);
	}
	else 
	{
		return(1);
	}
    
}


// auxiliary function for get_textseg_vmaddr
uint64_t get_offset(vm_offset_t buf, uint64_t base, uint32_t arch)
{
#if DEBUG
	printf("[DEBUG] Start get_offset\n");
#endif
	uint32_t i,x;
	uint32_t ncmds, cmd;
#if __LP64__
	uint64_t vmaddr;
	
	struct mach_header_64 *machheader; /* mach header */
	struct load_command *loadcmd; /* load commands */
	struct segment_command_64 *segcmd; /* segment command */
	struct section_64 *sectioncmd;
	
	machheader = (struct mach_header_64 *)(buf + base);
	loadcmd = (struct load_command*)((char*)machheader + sizeof(struct mach_header_64));
	
#else
	uint32_t vmaddr;
	
	struct mach_header *machheader; /* mach header */
	struct load_command *loadcmd; /* load commands */
	struct segment_command *segcmd; /* segment command */
	struct section *sectioncmd; /* section */
	machheader = (struct mach_header *)(buf + (uint32_t)base);
	loadcmd = (struct load_command*)((char*)machheader + sizeof(struct mach_header));
#endif
    
	ncmds = machheader->ncmds;
#if DEBUG
	printf("[DEBUG] Nr of cmds %d\n", ncmds);
#endif
	
	for (i = 0; i < ncmds; i++) 
	{
		
		cmd = loadcmd->cmd;
#if __LP64__
		if (cmd != LC_SEGMENT_64)
			goto end;
		segcmd = (struct segment_command_64 *)loadcmd;
#else
		if (cmd != LC_SEGMENT)
			goto end;
		segcmd = (struct segment_command *)loadcmd;
#endif
		
		// skip other segments except __TEXT
#if DEBUG
		printf("[DEBUG] Segment name %s\n", segcmd->segname);
#endif
		if (strcmp(segcmd->segname, "__TEXT"))
            goto end;
        
        textbaseaddr = segcmd->vmaddr;
#if DEBUG
        printf("[DEBUG] textbaseaddr is %p\n", (void*)textbaseaddr);
#endif
        
#if __LP64__
		sectioncmd = (struct section_64*)((char*)segcmd + sizeof(struct segment_command_64));
#else
        sectioncmd = (struct section*)((char*)segcmd + sizeof(struct segment_command));
#endif
		
        for (x = 0; x < segcmd->nsects; x++)
        {
            if (strcmp(sectioncmd->sectname, "__cstring") == 0)
            {
#if DEBUG
				printf("[DEBUG] Found __cstring! %p\n", (void*)sectioncmd->addr);
#endif
                cstringaddr = sectioncmd->addr;
                cstringsize = sectioncmd->size;
                break;
            }
            sectioncmd++;
        }
    end:
        loadcmd = (struct load_command*)((char*)loadcmd + loadcmd->cmdsize);
		
	}
	return(vmaddr);
}

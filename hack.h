/*
 *  hack.h
 *  iTunesPlugIn
 *
 *  Created by fG! on 8/29/11.
 *  Copyright 2011 fG!. All rights reserved.
 *
 *  reverser@put.as - http://reverse.put.as
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/mach_types.h> 
#include <mach/mach_vm.h>
#include <errno.h>
#include <sys/types.h>
// stuff to parse mach-o header
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>

#define EXIT_ON_MACH_ERROR(msg, retval) \
if (kr != KERN_SUCCESS) { mach_error(msg ":" , kr); exit((retval)); }

void patchmemory(void);
void unpatchmemory(void);
uint64_t find_image(mach_port_t, uint64_t *, uint64_t *);
uint64_t get_textseg_vmaddr(vm_offset_t, int8_t);
uint64_t get_offset(vm_offset_t , uint64_t, uint32_t);
void find_patchaddresses(mach_port_t, uint64_t, uint64_t);

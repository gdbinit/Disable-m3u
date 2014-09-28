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
 * hack.h
 *
 */

#ifndef _itunes_hack_h
#define _itunes_hack_h

#include <mach/mach.h>

struct header_info
{
    uint8_t active;                  // is patch active
    const struct mach_header_64 *mh; // iTunes mach-o header address
    uint64_t image_size;             // image size
    intptr_t aslr_slide;             // iTunes ASLR slide
    mach_vm_address_t cstring_addr;  // location of __cstring section (with ASLR)
    mach_vm_address_t cfstring_addr;
    // XXX: use a fixed value here
    //      expected hits should be 4
    //      could be dynamically allocated to make it right
    mach_vm_address_t patch_addresses[16]; // patched addresses so we can restore
};

void disable_m3u_processing(struct header_info *hi);
void enable_m3u_processing(struct header_info *hi);

#endif
/* $Id: load.c,v 1.1.1.1 2006/06/29 06:43:25 cpu Exp $ */
/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren  (www.lindergren.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by Patrik Lindergren.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*  This code is based on code made freely available by Algorithmics, UK */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

#ifdef __mips__
#include <machine/cpu.h>
#endif


#include "flash.h"
#if (NMOD_FLASH_AMD + NMOD_FLASH_INTEL + NMOD_FLASH_SST == 0)
    #ifdef HAVE_FLASH
    #undef HAVE_FLASH
    #endif
#else
    #ifndef HAVE_FLASH
    #define HAVE_FLASH
    #endif
#endif

extern int errno;                       /* global error number */
extern char *heaptop;
#ifdef HAS_EC
extern void tgt_ecprogram(void *, int);
#endif

static int	bootfd;
static int	bootbigend;

int cmd_nload __P((int, char *[]));
static int nload __P((int, char **));

/* ------------------------------------------------------- */

const Optdesc         cmd_nload_opts[] =
{
	{"-s", "don't clear old symbols"},
	{"-b", "don't clear breakpoints"},
	{"-e", "don't clear exception handlers"},
	{"-a", "don't add offset to symbols"},
	{"-t", "load at top of memory"},
	{"-i", "ignore checksum errors"},
#ifdef HAVE_FLASH
	{"-f flash_addr -o load_addr offsetr", ""},
#endif
#ifdef HAS_EC
	{"-d", "load ec_firmware file to update ec flash"},
#endif
	{"-n", "don't load symbols"},
	{"-y", "only load symbols"},
	{"-v", "verbose messages"},
	{"-w", "reverse endianness"},
	{"-k", "prepare for kernel symbols"},
	{"-o<offs>", "load offset"},
	{"-r", "load raw file"},
	{"path", "path and filename"},
	{0}
};

static int
nload (argc, argv)
	int argc;
	char **argv;
{
	char path[256];
	char buf[DLREC+1];
	long ep;
	int n;
	extern int optind;
	extern char *optarg;
	int c, err;
	int flags;
	unsigned long offset;
    int ret;
#ifdef HAVE_FLASH
	void	    *flashaddr;
	size_t	    flashsize;
#endif

	flags = 0;
	optind = err = 0;
	offset = 0;
	while ((c = getopt (argc, argv, "sbeatif:nrvwyko:dc:")) != EOF) {
		switch (c) {
			case 's':
				flags |= SFLAG; break;
			case 'b':
				flags |= BFLAG; break;
			case 'e':
				flags |= EFLAG; break;
			case 'a':
				flags |= AFLAG; break;
			case 't':
				flags |= TFLAG; break;
			case 'i':
				flags |= IFLAG; break;
#ifdef HAVE_FLASH
			case 'f':
				if (!get_rsa ((unsigned long *)&flashaddr, optarg)) {
					err++;
				}
                //08-29 for 64 bits compile
                flashaddr =(UNCACHED_TO_PHYS(flashaddr));
                flashaddr =(PHYS_TO_UNCACHED(flashaddr));
                
				flags |= FFLAG; break;
#endif
#if notyet
			case 'u':
				flashaddr = (void *)BOOTROMBASE;
				//for 64bit compile
                flashaddr =(UNCACHED_TO_PHYS(flashaddr));
                flashaddr =(PHYS_TO_UNCACHED(flashaddr));
                
				flags |= UFLAG;
				flags |= FFLAG; break;
#endif
			case 'n':
				flags |= NFLAG; break;
			case 'y':
				flags |= YFLAG; break;
			case 'v':
				flags |= VFLAG; break;
			case 'w':
				flags |= WFLAG; break;
			case 'k':
				flags |= KFLAG; break;
			case 'r':
				flags |= RFLAG; break;
#ifdef HAS_EC
			case 'd':
				flags |= DFLAG;
				break;
#endif
			case 'o':
				if (!get_rsa ((u_int32_t *)&offset, optarg)) {
					err++;
				}
				break;
			default:
				err++;
				break;
		}
	}

	if (err) {
		return EXIT_FAILURE;
	}

	if (optind < argc) {
		strcpy(path, argv[optind++]);
	} 
	else {
		printf("boot what?\n");
	}

	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return EXIT_FAILURE;
	}

#ifdef HAVE_FLASH
	if (flags & FFLAG) {        
		tgt_flashinfo (flashaddr, &flashsize);
		if (flashsize == 0) {
			printf ("No FLASH at given address\n");
			return 0;
		}
		/* any loaded program will be trashed... */
		flags &= ~(SFLAG | BFLAG | EFLAG);
		flags |= NFLAG;		/* don't bother with symbols */
		/*
		 * Recalculate any offset given on command line.
		 * Addresses should be 0 based, so a given offset should be
		 * the actual load address of the file.
		 */
		offset = (unsigned long)heaptop - offset;
#if BYTE_ORDER == LITTLE_ENDIAN
		bootbigend = 0;
#else
		bootbigend = 1;
#endif
	}
#endif

#ifdef HAS_EC
	if( (flags & DFLAG) || (flags & CFLAG) ){
		flags &= ~(SFLAG | BFLAG | EFLAG);
		flags |= NFLAG;
		offset = (unsigned long)heaptop - offset;
	}
#endif

	dl_initialise (offset, flags);

	fprintf (stderr, "Loading file: %s ", path);
	errno = 0;
	n = 0;

	if (flags & RFLAG) {
	   ExecId id;

	   id = getExec("bin");
	   if (id != NULL) {
		   ep = exec (id, bootfd, buf, &n, flags);
	   }
	} else {
		ep = exec (NULL, bootfd, buf, &n, flags);
	}

	close (bootfd);
	putc ('\n', stderr);

	if (ep == -1) {
		fprintf (stderr, "%s: boot failed\n", path);
		return EXIT_FAILURE;
	}

	if (ep == -2) {
		fprintf (stderr, "%s: invalid file format\n", path);
		return EXIT_FAILURE;
	}

	if (!(flags & (FFLAG|YFLAG))) {
		printf ("Entry address is %08x\n", ep);
		/* Flush caches if they are enabled */
		if (md_cachestat())
			flush_cache (DCACHE | ICACHE, NULL);
		md_setpc(NULL, ep);
		if (!(flags & SFLAG)) {
		    dl_setloadsyms ();
		}
	}

#ifdef HAS_EC
	if( (flags & DFLAG) || (flags & CFLAG) ){
		extern long dl_minaddr;
		extern long dl_maxaddr;
		printf("Entry address for EC is 0x%8x, size is 0x%x\n", ep, (dl_maxaddr - dl_minaddr));
		tgt_ecprogram((void *)ep, (dl_maxaddr - dl_minaddr));
	}
#endif

#ifdef HAVE_FLASH
	if (flags & FFLAG) {
		extern long dl_minaddr;
		extern long dl_maxaddr;
		if (flags & WFLAG)
			bootbigend = !bootbigend;
		ret = tgt_flashprogram ((void *)flashaddr, 	   	/* address */
				dl_maxaddr - dl_minaddr, 	/* size */
				(void *)heaptop,		/* load */
				bootbigend);
	}
    
	if( (flags & FFLAG) || (flags & RFLAG) ){
        if(ret == TRUE) {
            printf("pmon has been sucessfully updated.\n");            
        }    
    }
    
#endif
	return EXIT_SUCCESS;
}

int
cmd_nload (argc, argv)
    int argc; char **argv;
{
    int ret;
    ret = spawn ("load", nload, argc, argv);
    return (ret & ~0xff) ? 1 : (signed char)ret;
}

/*
 *  Command table registration
 *  ==========================
 */
static const Cmd Cmds[] =
{
   {"Boot and Load"},
   {"load",	"[-beastifr][-o offs]",
   cmd_nload_opts,
   "load file",
   cmd_nload, 1, 16, 0},
   {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
   init_cmd()
{
   cmdlist_expand(Cmds, 1);
}

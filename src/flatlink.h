#ifndef __FLATLINK_H__
#define __FLATLINK_H__

#define VERSION		"Ver0.80 2023-12-19"

#define	MAX_ARGS	256		// maximum arguments
#define MAX_SEGENTS	0x0400		// maximum segment1
#define MAX_PUBS	0x4000		// maximum public name           per segment
#define MAX_FIXUPS	0x4000		// maximum fixup and extern name per segment

#define uchar		unsigned char
#define uint32		unsigned int

extern int verbose;

uchar *load_file(const char *file, size_t *size);

#define read_int16(p)		((p)[0] | (p)[1]<<8)
#define read_int32(p)		((p)[0] | (p)[1]<<8 | (p)[2]<<16 | (p)[3]<<24)
#define write_int16(p,x)	((p)[0]=((x) & 0xff), (p)[1]=(((x)>>8) & 0xff))
#define write_int32(p,x)	((p)[0]=((x) & 0xff), (p)[1]=(((x)>>8) & 0xff), (p)[2]=(((x)>>16) & 0xff), (p)[3]=(((x)>>24) & 0xff))

typedef struct _exp_info {
	int	set_offset;
	int	set_entry;
	int	strip_header;	// slim P3 header
	uint32	entry;		// entry point
	uint32	offset;
	uint32	stack;
	uint32	mindata;
	uint32	maxdata;
	uint32	size;
} ExpInfo;


void V_PRINT(const char *fmt, ...);
void VV_PRINT(const char *fmt, ...);
void WARN_PRINT(const char *fmt, ...);
void ERR_PRINT(const char *fmt, ...);

#define V_PRINT_COND	(verbose)
#define VV_PRINT_COND	(1<verbose)


#endif	// __FLATLINK_H__

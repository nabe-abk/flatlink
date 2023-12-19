
#include <stdio.h>
#include <stdlib.h>

#include "flatlink.h"
#include "memory_x.h"
#include "output.h"

///////////////////////////////////////////////////////////////////////////////
// output com file header
///////////////////////////////////////////////////////////////////////////////
int output_com_header(FILE *fp, ExpInfo *com) {

	if (com->entry != 0x100) {
		WARN_PRINT("Entry point is not 0100h: entry=%06X\n", com->entry);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// output exp file header
///////////////////////////////////////////////////////////////////////////////
int output_exp_header(FILE *fp, ExpInfo *exp) {

	uint32 header_size = exp->strip_header ? 0x80 : 0x200;
	uint32 image_size  = exp->size;

	uchar *buf = (uchar *)calloc_x(header_size, 1);		// header

	buf[0] = 'P';
	buf[1] = '3';
	buf[2] = 1;		// flat model

	write_int16(buf+0x04, header_size);			// header size
	write_int32(buf+0x06, header_size + image_size);	// file size
	write_int32(buf+0x26, header_size);			// image offset
	write_int32(buf+0x2A, image_size);			// image size

	uint32 padding = image_size & 15;
	if (padding) padding = 16 - padding;		// for stack align

	uint32 stack   = exp->stack;
	uint32 stack_x = padding + stack;
	uint32 esp     = exp->offset + image_size + stack_x;
	uint32 mindata = stack_x + exp->mindata;
	uint32 maxdata = stack_x + exp->maxdata;
	if (mindata < stack_x) mindata = 0xffffffff;	// overflow
	if (maxdata < stack_x) maxdata = 0xffffffff;	// overflow

	write_int32(buf+0x56, mindata);			// mindata
	write_int32(buf+0x5A, maxdata);			// maxdata
	write_int32(buf+0x5E, exp->offset);		// offset
	write_int32(buf+0x62, esp);			// esp value
	write_int32(buf+0x68, exp->entry);		// entry point
	write_int32(buf+0x74, image_size);		// image size

	uint32 byte = fwrite(buf, 1, header_size, fp);
	if (byte != header_size) {
		fclose(fp);		// error
	}

	return header_size;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "flatlink.h"
#include "memory_x.h"
#include "load_obj.h"
#include "output.h"

///////////////////////////////////////////////////////////////////////////////
// global variable
///////////////////////////////////////////////////////////////////////////////
int  verbose = 0;

char zero[17] = "\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0";

///////////////////////////////////////////////////////////////////////////////
// PRINT functions
///////////////////////////////////////////////////////////////////////////////
void V_PRINT(const char *fmt, ...) {
	if (! V_PRINT_COND) return;
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void VV_PRINT(const char *fmt, ...) {
	if (! VV_PRINT_COND) return;
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void WARN_PRINT(const char *fmt, ...) {
	printf("[WARNING] ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
void ERR_PRINT(const char *fmt, ...) {
	printf("[ERROR] ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void PRINT_seg_info(FILE *fp, Segment *seg, char *append) {
	fprintf(fp, "[%06X] %s: %s '%s' use%d align=%d size=%d%s\n",
		seg->base, seg->f_name,
		seg->name, seg->c_name,
		seg->use32 ? 32 : 16, seg->align, seg->size,
		append ? append : ""
	);
}
void PRINT_seg_info_ex(FILE *fp, Segment *seg, const char *fmt, uint32 val) {
	char buf[64];
	buf[0] = ' ';
	sprintf(buf+1, fmt, val);	// High-C V1.7 not impliment snprintf()
	PRINT_seg_info(fp, seg, buf);
}

///////////////////////////////////////////////////////////////////////////////
// load file to memory
///////////////////////////////////////////////////////////////////////////////
uchar *load_file(const char *file, size_t *size) {

	FILE* fp = fopen(file, "rb");
	if (!fp) {
		ERR_PRINT("Can not open: %s\n", file);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	fgetpos(fp, (fpos_t *)size);
	fseek(fp, 0, SEEK_SET);

	uchar *p = (uchar *)malloc_x(*size +1);

	fread(p, 1, *size, fp);
	fclose(fp);

	return (uchar *)p;
}

///////////////////////////////////////////////////////////////////////////////
// parse num
///////////////////////////////////////////////////////////////////////////////
uint32 parse_num(char *p) {
	int len = strlen(p);

	uint32 digit = 10;
	if (*(p+len-1) == 'h' || *(p+len-1) == 'H') {
		digit=16;	// hexadecimal
		len--;
	}

	uint32 num = 0;
	for(int i=0; i<len; i++) {
		int c = p[i];
		if ('A' <= c && c <= 'Z') c += 0x20;	// rewrite to lower

		     if ('0' <= c && c <= '9') c -= '0';
		else if (digit==16 && 'a' <= c && c <= 'f') c = c - 'a' + 10;
		else {
			ERR_PRINT("Illegal number: %s\n", p);
			exit(1);
		}
		num = num*digit + c;
	}
	return num;
}

///////////////////////////////////////////////////////////////////////////////
// parse num
///////////////////////////////////////////////////////////////////////////////
char *rewrite_ext(const char *org, const char *ext) {

	int len1 = strlen(org);		// "test.obj"
	int len2 = strlen(ext);		// ".exp"
	char *file = (char *)malloc_x(len1 + len2 +1);
	strcpy(file, org);

	int p = len1 -1;
	while(0 <= p) {
		if (file[p] == '.') break;
		p--;
	}
	if (p<0) p = len1;		// "file" name not exists "."

	for(int i=0; i<len2; i++) {
		file[p++] = ext[i];
	}
	file[p] = 0;

	return file;
}

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	char **args = (char **)calloc_x(MAX_ARGS+1, sizeof(char *));
	int  args_c = 0;
	int  i;		// for MetaWare High-C

	//---------------------------------------------------------------------
	// merge @file argument
	//---------------------------------------------------------------------
	for(i=1; i<argc; i++) {
		if (MAX_ARGS < args_c) break;
		char *arg = argv[i];
		if (*arg != '@') {
			args[args_c++]=arg;
			continue;
		}

		// @filename
		size_t size;
		char *p   = (char *)load_file(arg+1, &size);
		char *end = p+size;
		*end = 0;

		int in_str=0;
		int in_quote=0;
		for(; p<end; p++) {
			if (MAX_ARGS < args_c) break;

			if (*p == '\r' || *p == '\n') *p=' ';
			if (in_quote) {
				if (*p != '"') continue;
				*p = 0;		// null terminate string
				in_str  =0;
				in_quote=0;
				continue;
			}

			if (*p == ' ' || *p == '\t') {
				if (!in_str) continue;
				*p = 0;
				in_str = 0;
				continue;
			}
			if (*p == '"') {
				if (in_str) continue;

				// " is start of string
				in_str   = 1;
				in_quote = 1;
				args[args_c++] = p+1;
				continue;
			}

			if (in_str) continue;

			// start string
			in_str   = 1;
			args[args_c++] = p;
		}
	}
	if (MAX_ARGS < args_c) {
		ERR_PRINT("Too many arguments (max %d)\n", MAX_ARGS);
		exit(1);
	}

	//---------------------------------------------------------------------
	// parse argument
	//---------------------------------------------------------------------
	int files = 0;
	int help  = 0;

	int exp_mode  = -1;	// exp file mode flag
	char *outfile = 0;
	char *mapfile = 0;

	ExpInfo exp;
	memset(&exp, 0, sizeof(exp));
	exp.offset   = 0x1000;
	exp.stack    = 0x1000;
	exp.mindata  = 0x1000;
	exp.maxdata  = 0xffffffff;

	uint32	max_segs   = MAX_SEGENTS;
	uint32	max_pubs   = MAX_PUBS;
	uint32	max_fixups = MAX_FIXUPS;

	for(i=0; i<args_c; i++) {
		char *p = args[i];
		if (*p != '-') {
			args[files++] = args[i];
			continue;
		}

		if (!strcmp(p, "-v")) {
			verbose = 1;
			continue;
		}
		if (!strcmp(p, "-vv")) {
			verbose = 2;
			continue;
		}
		if (!strcmp(p, "-q")) {
			verbose = -1;
			continue;
		}
		if (!strcmp(p, "-h")) {
			help = 1;
			continue;
		}
		if (!strcmp(p, "-strip")) {
			exp.strip_header = 1;
			continue;
		}
		// -o file
		// -mindata xxxxx
		// -maxdata xxxxx
		char *val = args[++i];
		if (args_c <= i) {
			ERR_PRINT("No value specified: %s\n", p);
			exit(1);
		}

		if (!strcmp(p, "-o")) {
			outfile = val;
			continue;
		}
		if (!strcmp(p, "-f")) {
			if (!strcmp(val, "exp")) {
				exp_mode = 1;
				continue;
			}
			if (!strcmp(val, "com") || !strcmp(val, "bin")) {
				exp_mode = 0;
				continue;
			}
			ERR_PRINT("Unknown format: %s\n", val);
			exit(1);
		}
		if (!strcmp(p, "-m")) {
			mapfile = val;
			continue;
		}
		if (!strcmp(p, "-offset")) {
			exp.set_offset = 1;
			exp.offset     = parse_num(val);
			continue;
		}
		if (!strcmp(p, "-mindata")) {
			exp.mindata = parse_num(val);
			continue;
		}
		if (!strcmp(p, "-maxdata")) {
			exp.maxdata = parse_num(val);
			continue;
		}
		if (!strcmp(p, "-stack")) {
			exp.stack = parse_num(val);
			continue;
		}

		if (!strcmp(p, "-maxsegs")) {
			max_segs = parse_num(val);
			continue;
		}
		if (!strcmp(p, "-maxpubs")) {
			max_pubs = parse_num(val);
			continue;
		}
		if (!strcmp(p, "-maxfixups")) {
			max_fixups = parse_num(val);
			continue;
		}

		ERR_PRINT("Unknown argument: %s\n", p);
		exit(1);
	}

	if (verbose<0) fclose(stdout);
	printf("FlatLink - .EXP and .COM file linker - " VERSION " (C)nabe@abk\n");

	//---------------------------------------------------------------------
	// help
	//---------------------------------------------------------------------
	if (help || !files) {
		printf(
			"\nUsage: %s [@respons_file] [options] obj_file ...\n"
			"\n"
			"	-o file		output file. '.com' or '.bin' is set 'com' format\n"
			"	-f format	'exp' or 'com' or 'bin'\n"
			"	-m mapfile	link map file\n"
			"	-v		verbose\n"
			"	-vv		more verbose\n"
			"	-q		quiet (close stdout)\n"
			"	-h		view this help\n"
			"	-strip		strip exp file header\n"
			"	-offset  num	exp file's load offset  (default 1000h)\n"
			"	-stack   num	exp file's stack size   (default 1000h)\n"
			"	-mindata num	exp file's minimum heap (default 1000h)\n"
			"	-maxdata num	exp file's maximum heap\n"
			"\n"
			"	-maxsegs   num	maximum segments     (default " TO_STR(MAX_SEGENTS) ")\n"
			"	-maxpubs   num	maximum public names (default " TO_STR(MAX_PUBS)    ")\n"
			"	-maxfixups num	maximum fixups       (default " TO_STR(MAX_FIXUPS)  ")\n"
			, argv[0]
		);
		return 0;
	}

	//---------------------------------------------------------------------
	// auto set outfile and mapfile
	//---------------------------------------------------------------------
	// check .com or .bin file
	if (outfile && exp_mode == -1) {
		int len = strlen(outfile);
		if (4 <= len) {
			char ext[5];
			ext[4] = 0;
			for(i=0; i<4; i++) {
				int c = outfile[len-4+i];
				if ('A'<=c && c<='Z') c+=0x20;	// to lower case
				ext[i] = c;
			}
			if (!strcmp(ext, ".com") || !strcmp(ext, ".bin")){
				exp_mode = 0;
			}
		}
	}

	if (!outfile) outfile = rewrite_ext(args[0], exp_mode ? ".exp" : ".com");
	if (!mapfile) mapfile = rewrite_ext(outfile, ".map");

	if (!exp_mode && !exp.set_offset) exp.offset = 0x100;

	//---------------------------------------------------------------------
	// load object files
	//---------------------------------------------------------------------
	init_memory(max_segs, max_pubs, max_fixups);

	for(i=0; i<files; i++) {
		int flag=1;
		for(int j=0; j<i; j++) {
			// same file skip
			if (!strcmp(args[i], args[j])) {
				flag=0;
				break;
			}
		}
		if (flag) load_obj( args[i] );
	}

	if (check_duplicate_pub_name()) exit(2);

	//---------------------------------------------------------------------
	// Rearrange segments
	//---------------------------------------------------------------------
	int seg_c;
	Segment **segs;

	if (1) {
		Segment *org_segs = load_all_segs(&seg_c);
		if (!seg_c) {
			ERR_PRINT("Segment not found!");
			exit(3);
		}

		// class name to Upper Case
		for(i=0; i<seg_c; i++) {
			Segment *seg = &org_segs[i];
			char *p = seg->c_name;
			if (!p) continue;

			// class name to upper case
			while(*p) {
				if ('a'<= *p && *p<='z') *p -= 0x20;
				p++;
			}
		}


		int s_idx = 0;
		segs = (Segment **)calloc_x(sizeof(Segment *), seg_c);

		// sort and set pointer to *segs[]
		for(i=0; i<seg_c; i++) {
			Segment *seg = &org_segs[i];
			char *p = seg->c_name;
			if (!p || !*p || !strcmp(p, "CODE")) {	// null class or CODE class
				seg->index = -1;		// pickuped mark
				segs[s_idx++] = seg;
			}
		}
		for(i=0; i<seg_c; i++) {
			Segment *seg = &org_segs[i];
			if (!strcmp(seg->c_name, "DATA")) {
				seg->index = -1;	// pickuped mark
				segs[s_idx++] = seg;
			}
		}
		for(i=0; i<seg_c; i++) {
			Segment *seg = &org_segs[i];
			if (seg->index == -1) continue;
			ERR_PRINT("[%s] Segment class %s is not support! (name=%s)\n", seg->f_name, seg->c_name, seg->name);
			exit(4);
		}
	}

	//---------------------------------------------------------------------
	// fix com file data for compatible alink
	//---------------------------------------------------------------------
	int hack_com = 0;
	if (!exp_mode && exp.offset == 0x100) {
		Segment *seg = segs[0];
		if (0x100 <= seg->size) {
			hack_com = 1;
			uchar *p = seg->code;
			for(i=0; i<0x100; i++) {
				if (p[i]) {
					hack_com = 0;
					break;
				}
			}
		}
	}

	//---------------------------------------------------------------------
	// allocate segments
	//---------------------------------------------------------------------
	FILE *fmap  = fopen(mapfile, "w");
	uint32 base = hack_com ? 0 : exp.offset;

	for(i=0; i<seg_c; i++) {
		Segment *seg = segs[i];
		int add   = seg->align -1;
		int mask  = 0xffffffff ^ add;
		uint32 _base = base;
		base = (base + add) & mask;
		seg->base    = base;
		seg->padding = base - _base;

		if (seg->exist_entry) {
			uint32 entry = seg->entry + base;
			exp.set_entry++;
			exp.entry = entry;

			PRINT_seg_info_ex(stdout, seg, "entry=%06X", entry);
			PRINT_seg_info_ex(fmap,   seg, "entry=%06X", entry);
		} else {
			PRINT_seg_info(stdout, seg, 0);
			PRINT_seg_info(fmap,   seg, 0);
		}

		// set all pub names offset
		int pub_c     = seg->pub_c;
		PubName *pubs = seg->pubs;
		for(int j=0; j<pub_c; j++) {
			PubName *p = &pubs[j];
			p->offset += base;
			VV_PRINT("\t[%06X] pub name: %s\n", p->offset, p->name);
			fprintf(fmap, "%08X %s\n", p->offset, p->name);
		}

		base     += seg->size;
		exp.size += seg->size + seg->padding;
	}
	fclose(fmap);
	if (1 < exp.set_entry) {
		ERR_PRINT("There are multiple entry points!\n", exp.set_entry);
		exit(5);
	}

	if (!exp.set_entry) {
		exp.entry = exp.offset;
		WARN_PRINT("Not exist entry point, set default entry point %06Xh\n", exp.entry);
	}

	//---------------------------------------------------------------------
	// fixup segments
	//---------------------------------------------------------------------
	int not_found = 0;
	for(i=0; i<seg_c; i++) {
		Segment *seg  = segs[i];
		int fixup_c   = seg->fixup_c;
		Fixup *fixups = seg->fixups;
		uint32 base   = seg->base;

		if (V_PRINT_COND) PRINT_seg_info_ex(stdout, seg, "fixup=%d", fixup_c);

		for(int j=0; j<fixup_c; j++) {
			Fixup *fixup = &fixups[j];
			uchar *p      = seg->code + fixup->offset;	// fix point
			uint32 offset =      base + fixup->offset;	// code offset
			int    bits32 = fixup->bits32;

			Segment *ref_seg  = fixup->ref_seg;
			char *ref_name = "";
			uint32 val;
			uint32 _val;

			if (ref_seg) {
				uint32 current	= fixup->place ? fixup->place
						: (bits32 ? read_int32(p) : read_int16(p));
				val = _val = current + ref_seg->base;
			} else {
				ref_name = fixup->ref_name;
				PubName *pub = search_pub_name(ref_name, seg->f_name);
				if (!pub) {
					not_found++;
					continue;
				}
				val = _val = pub->offset;
			}
			if (fixup->relative) {	// need relative offset
				val = val - (offset + (bits32 ? 4 : 2));
			}

			if (bits32) {
				write_int32(p, val);
				VV_PRINT("\tfixup [%06X] = %08X for %08X: label=%s\n",         offset, val, _val, ref_name);
			} else {
				val &= 0xffff;
				write_int16(p, val);
				VV_PRINT("\tfixup [%06X] =     %04X for     %04X: label=%s\n", offset, val, _val, ref_name);
			}
		}
	}
	if (not_found) exit(6);

	//---------------------------------------------------------------------
	// hack com format
	//---------------------------------------------------------------------
	if (hack_com) {		// skip first 100h byte
		Segment *seg = segs[0];
		seg->size -= 0x100;
		seg->code  = seg->code + 0x100;
	}

	//---------------------------------------------------------------------
	// output
	//---------------------------------------------------------------------
	FILE *fp = fopen(outfile, "wb");
	if (!fp) {
		ERR_PRINT("Can not write \"%s\"\n", outfile);
		exit(7);
	}

	uint32 size=0;
	if (exp_mode) {
		if (!segs[0]->use32) WARN_PRINT("First segment is use16.\n");
		size = output_exp_header(fp, &exp);
	} else {
		if ( segs[0]->use32) WARN_PRINT("First segment is use32.\n");
		size = output_com_header(fp, &exp);
	}

	for(i=0; i<seg_c; i++) {
		Segment *seg = segs[i];

		int padding = seg->padding;
		size += padding;
		while(0 < padding) {
			fwrite(zero, 1, 16 < padding ? 16 : padding, fp);
			padding -= 16;
		}

		uint32 byte = fwrite(seg->code, 1, seg->size, fp);
		if (byte != seg->size) {
			ERR_PRINT("File write failed \"%s\"\n", outfile);
			fclose(fp);
			remove(outfile);
			exit(8);
		}
		size += byte;
	}
	fclose(fp);

	printf("Used %d KB of heap memory.\n", (get_total_alloc_memory() + 0x3ff) >> 10);
	printf("Write \"%s\" %d bytes.\n", outfile, size);

	return 0;
}



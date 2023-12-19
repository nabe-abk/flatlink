
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flatlink.h"
#include "memory_x.h"
#include "load_obj.h"

#define read_word(buf,p)  (p+=2, ( buf[p-2]     | (buf[p-1]<<8)))
#define read_rword(buf,p) (p+=2, ((buf[p-2]<<8) |  buf[p-1]    ))
#define read_dword(buf,p) (p+=4, ( buf[p-4]     | (buf[p-3]<<8) | (buf[p-2]<<16) | (buf[p-1]<<24)))
#define read_index(buf,p) ((buf[p] & 0x80) ? (p+=2,(((buf[p-2] & 0x7f)<<8) | buf[p-1])) : buf[p++])

///////////////////////////////////////////////////////////////////////////////
// variable
///////////////////////////////////////////////////////////////////////////////
int base_offset;
char *obj_filename;


///////////////////////////////////////////////////////////////////////////////
// [80h] header
///////////////////////////////////////////////////////////////////////////////
void parse_header(uchar *buf, int size) {
	int len = buf[0];
	buf[len+1] = 0;
	V_PRINT("header=%s\n", buf+1);
}

///////////////////////////////////////////////////////////////////////////////
// [88h] comment
///////////////////////////////////////////////////////////////////////////////
void parse_comment(uchar *buf, int size, int *is_pharlap) {
	buf[size] = 0;

	int p    = 0;
	int type = read_rword(buf, p);

	char *append = "";
	if (type == 0x80AA && !strncmp((char *)buf+p, "80386", 5)) {
		*is_pharlap=1;
		append=" // Phar Lap Easy OMF-386";
	}

	V_PRINT("comment %04X=%s%s\n", type, buf+p, append);
}

///////////////////////////////////////////////////////////////////////////////
// [8Ah, 8Bh] Module End
///////////////////////////////////////////////////////////////////////////////
void parse_module_end(uchar *buf, int size, int bits32) {
	int p=0;
	int type   = buf[p++];
	int main   = (type & 0x80) ? 1 : 0;
	int start  = (type & 0x40) ? 1 : 0;

	if (!start) {
		V_PRINT("module_end main=%d start=%d\n", main, start);
		return;
	}
	if (!(type & 1)) {
		WARN_PRINT("invalid start address type\n");
		return;
	}
	// read fixup record
	int fixdata  = buf[p++];
	int f_type   = (fixdata >> 4) & 7;
	int t_type   =  fixdata       & 3;
	int f_idx    = (!(fixdata & 0x80) && f_type<3) ? read_index(buf, p) : 0;
	int t_idx    =  !(fixdata & 0x08)              ? read_index(buf, p) : 0;

	uint32 place = 0;
	if (!(fixdata & 0x04)) {
		if (bits32)
			place = read_dword(buf, p);
		else
			place = read_word (buf, p);
	}

	V_PRINT("module_end main=%d start=%d frame=%d/%d target=%d/%d place=%06X\n",
		main, start, f_type, f_idx, t_type, t_idx, place
	);
	if (t_type != 0) {
		WARN_PRINT("start address target type=%d not support\n", t_type);
		return;
	}

	Segment *seg = load_seg(t_idx);
	seg->exist_entry = 1;
	seg->entry       = place;	// set "..start:" label
}

///////////////////////////////////////////////////////////////////////////////
// [8Ch] External Names Definition
///////////////////////////////////////////////////////////////////////////////
void parse_ext_names(uchar *buf, int size) {

	V_PRINT("ext_names\n");

	int p=0;
	while(p<size) {
		int _p     = p;
		int len    = buf[p++];
		char *name = (char *)buf+p;
		p+=len;
		int t_idx  = read_index(buf,p);
		buf[_p+len+1] = 0;	// string null terminate

		int c = add_ext_name(name);

		VV_PRINT("    [%04X] type=%d [%d] %s\n", base_offset + _p, t_idx, c, name);
	}
}

///////////////////////////////////////////////////////////////////////////////
// [90h, 91h] Public Names Definition
///////////////////////////////////////////////////////////////////////////////
void parse_pub_names(uchar *buf, int size, int bits32) {
	int p=0;
	int g_idx = read_index(buf, p);
	int s_idx = read_index(buf, p);
	int frame = 0;
	if (s_idx==0) frame = read_word(buf, p);	// Base Frame

	V_PRINT("pub_names Group=%d Segment=%d frame=%d\n", g_idx, s_idx, frame);
	if (!s_idx) {
		ERR_PRINT("PUB NAMES: Illegal target segment=%d\n",s_idx);
		exit(23);
	}

	Segment *seg = load_seg(s_idx);

	while(p<size) {
		int _p     = p;
		int len    = buf[p++];
		char *name = (char *)buf+p;
		p+=len;
		int offset = bits32 ? read_dword(buf,p) : read_word(buf,p);
		int t_idx  = read_index(buf,p);

		buf[_p+len+1] = 0;	// string null terminate

		VV_PRINT("    [%04X] offset=%06X type=%d %s\n", base_offset + _p, offset, t_idx, name);

		add_pub_name(seg, offset, name);
	}
}

///////////////////////////////////////////////////////////////////////////////
// [96h] List of names
///////////////////////////////////////////////////////////////////////////////
void parse_list_names(uchar *buf, int size) {
	V_PRINT("list_names:");
	init_list_name();

	int p=0;
	int len=buf[p];
	while(p<size) {
		p++;
		int next_len = buf[p+len];
		buf[p+len]=0;

		int idx = add_list_name((char *)buf+p);
		V_PRINT(" %d=%s", idx, len ? (char *)buf+p : "(null)");

		p  += len;
		len = next_len;
	}
	V_PRINT("\n");
}

///////////////////////////////////////////////////////////////////////////////
// [98h, 99h] Segment Definition
///////////////////////////////////////////////////////////////////////////////
static int AlignTable[] = {
	0,	// 0: absolute segment is not support
	1,	// 1
	2,	// 2
	16,	// 3
	256,	// 4, if bits32 is 4096
	4,	// 5
	1,	// not use
	1	// not use
};

void parse_segdef(uchar *buf, int size, int bits32) {
	int attr  = buf[0];
	int align = AlignTable[attr >> 5];
	if (bits32 && align==256) align=4096;
	int combi = (attr >> 2) & 7;
	int use32 =  attr & 1;

	int p=1;
	uint32 seglen;
	if (bits32) {
		seglen = read_dword(buf, p);
		if ((attr & 2) && seglen==0) seglen=0x0ffffffff;
	} else {
		seglen = read_word(buf, p);
		if ((attr & 2) && seglen==0) seglen=0x10000;
	}
	int n_idx = read_index(buf, p);
	int c_idx = read_index(buf, p);
	int o_idx = read_index(buf, p);

	Segment *seg = add_seg();

	V_PRINT("SegDef#%d align=%d combi=%d use32=%d len=%06Xh name=%s class=%s overlay=%d\n",
		seg->index,
		align, combi, use32, seglen, load_list_name(n_idx), load_list_name(c_idx), o_idx);

	seg->align  = align;
	seg->size   = seglen;
	seg->use32  = use32;
	seg->f_name = obj_filename;
	seg->name   = load_list_name(n_idx);
	seg->c_name = load_list_name(c_idx);
	seg->code   = (uchar *)calloc_x(1, seglen);
}

///////////////////////////////////////////////////////////////////////////////
// [9Ah] Group Definition
///////////////////////////////////////////////////////////////////////////////
void parse_grpdef(uchar *buf, int size) {
	int p=0;
	int gn_idx = read_index(buf, p);

	V_PRINT("GrpDef g_name=%d", gn_idx);

	int seg_idx=0;
	int err=0;
	while(p<size) {
		int type = buf[p++];
		int idx  = read_index(buf, p);
		if (type != 0xff) err=type;
		seg_idx = seg_idx ? seg_idx : idx;	// save group first segment

		V_PRINT(" %02X=%d", type, idx);
	}
	V_PRINT("\n");
	if (err) WARN_PRINT("Group Definition type=%02X is not support!\n", err);
}

///////////////////////////////////////////////////////////////////////////////
// [9Ch, 9Dh] Fixup
///////////////////////////////////////////////////////////////////////////////
void parse_fixup(uchar *buf, int size, int seg_idx, int bits32, int pharlap) {

	Segment *seg = load_seg(seg_idx);

	V_PRINT("FIXUP\n");

	int p=0;
	while(p<size) {
		int _p   = p;
		int attr = buf[p];

		if (attr & 0x80) {	// FIXUP
			int attr     = read_rword(buf, p);
			int relative = (attr & 0x4000) ? 0 : 1;		// need relative offset
			int location = (attr >> 10) & 15;
			int offset   = (attr & 0x3ff) + seg->prev_offset;

			int fixdata  = buf[p++];
			int f_type   = (fixdata >> 4) & 7;
			int t_type   =  fixdata       & 3;
			int f_idx    = (!(fixdata & 0x80) && f_type<3) ? read_index(buf, p) : 0;
			int t_idx    =  !(fixdata & 0x08)              ? read_index(buf, p) : 0;

			uint32 place = 0;
			if (!(fixdata & 0x04)) {
				if (bits32)
					place = read_dword(buf, p);
				else
					place = read_word (buf, p);
			}

			int bits32 = -1;
			if (location==1) bits32 = 0;
			if (location==5) bits32 = pharlap;	// PharLap is 32bit, other is 16bit
			if (location==9) bits32 = 1;

			Segment *ref_seg  = (t_type==0) ? load_seg(t_idx) : 0;
			char    *ref_name = (t_type==2) ? load_ext_name(t_idx) : 0;

			VV_PRINT("    [%04X] FIXUP seg=%d relative=%d offset=%06X location=%d(%d) frame=%d/%d target=%d/%02d place=%X ref_%s=%s\n",
				base_offset + _p, seg_idx, relative, offset, 
				location, bits32 ? 32 : 16,
				f_type, f_idx, t_type, t_idx, place,
				ref_seg  ? "seg" : "name",
				ref_seg  ? ref_seg->name : ref_name
			);

			if (bits32 == -1) {
				ERR_PRINT("[FIXUP] location type %d is not support! (support '1' or '5' or '9')\n", location);
				exit(20);
			}
			if (f_type != 0 && f_type != 1 && f_type != 5) {
				ERR_PRINT("[FIXUP] frame type F%d is not support! (support '0' or '1' or '5' only)\n", f_type);
				exit(20);
			}
			if (t_type != 0 && t_type != 2) {
				ERR_PRINT("[FIXUP] target type %d is not support! (support '0' or '2')\n", t_type);
				exit(20);
			}

			Fixup *fixup    = add_fixup(seg);
			fixup->offset   = offset;
			fixup->place    = place;
			fixup->bits32   = bits32;
			fixup->relative = relative;
			fixup->ref_seg  = ref_seg;
			fixup->ref_name = ref_name;

		} else {	// THREAD
			int is_frame = (attr & 0x40) ? 1 : 0;
			int method   = (attr >> 2) & 7;
			int thred    =  attr & 3;
			int index    = 0;

			if (!is_frame || method < 3) {	// target has index always
				index = buf[p+1];
				p+=2;
			} else {
				p+=1;
			}
			VV_PRINT("    [%04X] THREAD %s method=%d thred=%d index=%d\n",
				base_offset + _p, is_frame ? "frame " : "target", method, thred, index
			);

			WARN_PRINT("[FIXUP] THREAD is not support!\n");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// [A0h, A1h] Logical Enumerated Data
///////////////////////////////////////////////////////////////////////////////
int parse_ledata(uchar *buf, int size, int bits32) {
	int s_idx = buf[0];

	int p=1;
	int offset;
	if (bits32)
		offset = read_dword(buf,p);
	else
		offset = read_word (buf,p);

	int data_size = size-p;

	V_PRINT("LEDATA seg_idx=%d offset=%06X-%06X\n", s_idx, offset, offset + data_size -1);

	Segment *seg = load_seg(s_idx);
	if (seg->size < offset + data_size) {
		ERR_PRINT("LEDATA is out of segment size! (seg_size=%d)\n");
		exit(22);
	}

	memcpy(seg->code + offset, buf+p, data_size);
	seg->prev_offset = offset;

	return s_idx;
}

///////////////////////////////////////////////////////////////////////////////
// load object
///////////////////////////////////////////////////////////////////////////////
uchar *load_obj(char *file) {

	printf("load file: %s\n", file);
	obj_filename = file;

	size_t	size;
	uchar	*buf = load_file(file, &size);

	init_local_seg();
	init_ext_name();

	int pharlap=0;
	int seg_idx=0;
	for(int p=0; p<size;) {
		int    type =  buf[p];
		int    size = (buf[p+1] | buf[p+2]<<8) -1;
		uchar *data =  buf+p+3;
		data[size]  = 0;

		V_PRINT("[%04X] %02X: ", p, type);
		base_offset = p+3;			// for V_PRINT

		switch(type) {
			case T_HEADER:
				parse_header(data, size);
				break;

			case T_COMMENT:
				parse_comment(data, size, &pharlap);
				break;

			case T_MOD_END:
				parse_module_end(data, size, pharlap);
				break;
			case T_MOD_END32:
				parse_module_end(data, size, 1);
				break;


			case T_EXT_NAMES:
				parse_ext_names(data, size);
				break;

			case T_PUB_NAMES:
				parse_pub_names(data, size, pharlap);
				break;
			case T_PUB_NAMES32:
				parse_pub_names(data, size, 1);
				break;

			case T_LIST_NAMES:
				parse_list_names(data, size);
				break;

			case T_SEG_DEF:
				parse_segdef(data, size, pharlap);
				break;
			case T_SEG_DEF32:
				parse_segdef(data, size, 1);
				break;

			case T_GRP_DEF:
				parse_grpdef(data, size);
				break;

			case T_FIXUP:
				parse_fixup(data, size, seg_idx, pharlap, pharlap);
				break;
			case T_FIXUP32:
				parse_fixup(data, size, seg_idx,       1, pharlap);
				break;

			case T_LE_DATA:
				seg_idx = parse_ledata(data, size, pharlap);
				break;
			case T_LE_DATA32:
				seg_idx = parse_ledata(data, size, 1);
				break;

			case T_COMDEF:
			case T_COMDAT:
			case T_COMDAT32:
				break;	// not support

			case T_LI_DATA:
			case T_LI_DATA32:
				ERR_PRINT("Not support LIDATA block\n");
				exit(21);
				break;

			default:
				ERR_PRINT("Unknown block type: %02X\n", type);
				exit(22);
				break;
		}
		p += 4+size;
	}

	return buf;
}


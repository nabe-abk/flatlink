
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flatlink.h"
#include "memory_x.h"

///////////////////////////////////////////////////////////////////////////////
// variables
///////////////////////////////////////////////////////////////////////////////
uint32	max_seg    = MAX_SEGENTS;
uint32	max_pubs   = MAX_PUBS;
uint32	max_fixups = MAX_FIXUPS;

int	seg_c = 0;
int	local_seg_c;
int	list_name_c;
int	ext_name_c;

Segment **segs;
PubName	**pub_idx_1st;
PubName	**pub_idx_cur;
LsName	**list_names;
ExtName	**ext_names;

///////////////////////////////////////////////////////////////////////////////
// malloc
///////////////////////////////////////////////////////////////////////////////
void *malloc_x(size_t size) {
	void *p = malloc(size);
	if (!p) {
		fprintf(stderr, "memory allocation failed!\n");
		exit(10);
	}
	return p;
}

void *calloc_x(size_t num, size_t size) {
	void *p = calloc(num, size);
	if (!p) {
		fprintf(stderr, "memory allocation failed!\n");
		exit(10);
	}
	return p;
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
void *init_pointer_ary(size_t st_size, size_t count) {

	void **ary = (void **)calloc_x(count, sizeof(void *));	// pointer ary
	uchar *p   = (uchar *)calloc_x(count, st_size);		// struct buffer

	for(int i=0; i<count; i++) {
		ary[i] = p;
		p += st_size;
	}
	return ary;
}

void init_memory(int mul) {
	if (mul) {
		max_seg    *= mul;
		max_pubs   *= mul;
		max_fixups *= mul;
	}

	segs        = (Segment **)init_pointer_ary(sizeof(Segment), max_seg);
	list_names  = (LsName  **)init_pointer_ary(sizeof(LsName),  max_seg);
	ext_names   = (ExtName **)init_pointer_ary(sizeof(ExtName), max_fixups);
	pub_idx_1st = (PubName **)calloc_x(0x100, sizeof(PubName *));
	pub_idx_cur = (PubName **)calloc_x(0x100, sizeof(PubName *));
}

///////////////////////////////////////////////////////////////////////////////
// list of names
///////////////////////////////////////////////////////////////////////////////
void init_list_name() {
	list_name_c = 0;
}

int add_list_name(char *name) {
	if (max_seg <= list_name_c) {
		fprintf(stderr, "Too many list of names! (max=%d)\n", max_seg);
		exit(11);
	}

	LsName *p = list_names[list_name_c++];
	p->name  = name;

	return list_name_c;
}

char *load_list_name(int index) {
	return list_names[index-1]->name;
}

///////////////////////////////////////////////////////////////////////////////
// Extern names
///////////////////////////////////////////////////////////////////////////////
void init_ext_name() {
	ext_name_c = 0;
}

int add_ext_name(char *name) {
	if (max_seg <= ext_name_c) {
		fprintf(stderr, "Too many extern names! (max=%d)\n", max_fixups);
		exit(11);
	}

	ExtName *p = ext_names[ext_name_c++];
	p->name = name;

	return ext_name_c;
}

char *load_ext_name(int index) {
	return ext_names[index-1]->name;
}

///////////////////////////////////////////////////////////////////////////////
// segment
///////////////////////////////////////////////////////////////////////////////
void init_local_seg() {
	local_seg_c = seg_c;
}
int num_of_local_segs() {
	return seg_c - local_seg_c;
}

Segment *add_seg() {
	if (max_seg <= seg_c) {
		fprintf(stderr, "Too many segment! (max=%d)\n", max_seg);
		exit(11);
	}
	Segment *seg = segs[seg_c++];
	seg->entry = 0xffffffff;
	seg->index = seg_c - local_seg_c;

	return seg;
}

Segment *load_seg(int index) {
	return segs[local_seg_c + index -1];
}

Segment **load_all_segs(int *count) {
	*count = seg_c;
	return segs;
}

///////////////////////////////////////////////////////////////////////////////
// pub name
///////////////////////////////////////////////////////////////////////////////
int make_str_hash(uchar *p) {
	int h   = 0;
	int len = strlen((char *)p);

	for(int i=0; i<len; i++) {
		int j = i & 7;
		h ^= (p[i]<<j | p[i]>>(8-j));
	}
	return h & 0xff;
}

void add_pub_name(Segment *seg, int offset, char *name) {
	if (max_pubs <= seg->pub_c) {
		fprintf(stderr, "Too many public name! (max=%d)\n", max_pubs);
		exit(11);
	}
	if (!seg->pubs) {	// alloc memory
		seg->pubs = (PubName **)init_pointer_ary(sizeof(PubName), max_pubs);
	}

	PubName *pub = seg->pubs[ (seg->pub_c)++ ];
	pub->seg    = seg;
	pub->offset = offset;
	pub->name   = name;

	int h = make_str_hash((uchar *)name);	// name's hash

	PubName *p = pub_idx_cur[h];
	if (p) {
		p->next = pub;
	} else {
		pub_idx_1st[h] = pub;
	}
	pub_idx_cur[h] = pub;
}

int check_duplicate_pub_name() {
	int dup=0;
	for(int c=0; c<0x100; c++) {
		PubName *first = pub_idx_1st[c];
		PubName *p = first;
		while(p) {
			// printf("%02X %s\n", c, p->name);
			PubName *p2 = p->next;

			while(p2) {
				if (!strcmp(p->name, p2->name)) {
					ERR_PRINT("duplicate public name!\n"
						"	file=%s name=%s\n"
						"	file=%s name=%s\n",
						p ->seg->f_name, p ->name,
						p2->seg->f_name, p2->name
					);
					dup=1;
					break;
				}
				p2 = p2->next;
			}
			p = p->next;
		}
	}
	return dup;
}

PubName *search_pub_name(char *name, char *f_name) {

	int h = make_str_hash((uchar *)name);
	PubName *p = pub_idx_1st[h];
	while(p) {
		if (!strcmp(p->name, name)) return p;
		p = p->next;
	}

	ERR_PRINT("[%s] extern name \"%s\" not found!\n", f_name, name);
	return 0;	// not found
}

///////////////////////////////////////////////////////////////////////////////
// fixup
///////////////////////////////////////////////////////////////////////////////
Fixup *add_fixup(Segment *seg) {
	if (max_fixups <= seg->fixup_c) {
		fprintf(stderr, "Too many fixup! (max=%d)\n", max_fixups);
		exit(11);
	}
	if (!seg->fixups) {	// alloc memory
		seg->fixups = (Fixup **)init_pointer_ary(sizeof(Fixup), max_fixups);
	}
	return seg->fixups[ (seg->fixup_c)++ ];
}


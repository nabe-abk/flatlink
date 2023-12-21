
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flatlink.h"
#include "memory_x.h"

///////////////////////////////////////////////////////////////////////////////
// variables
///////////////////////////////////////////////////////////////////////////////
uint32	max_segs;
uint32	max_pubs;
uint32	max_fixups;
uint32  total_memory = 0;

int	seg_c   = 0;
int	pub_c   = 0;
int	fixup_c = 0;
int	local_seg_c;

Segment *all_segs;
PubName *all_pubs;
Fixup   *all_fixups;

// PubName's hash table
PubName	**pub_idx_1st;
PubName	**pub_idx_cur;

// Temporary buffer for load_obj()
int	list_name_c;
int	ext_name_c;
LsName	*list_names;
ExtName	*ext_names;


///////////////////////////////////////////////////////////////////////////////
// malloc
///////////////////////////////////////////////////////////////////////////////

void alloc_fail(size_t size) {
	fprintf(stderr, "memory allocation failed! try %ld KB, used %d KB\n",
		(size         + 0x3ff) >>10,
		(total_memory + 0x3ff) >>10
	);
	exit(10);
}

void *malloc_x(size_t size) {
	void *p = malloc(size);
	if (!p) alloc_fail(size);

	total_memory += size;
	return p;
}

void *calloc_x(size_t num, size_t size) {
	void *p = calloc(num, size);
	if (!p) alloc_fail(num*size);

	total_memory += num*size;
	return p;
}

uint32 get_total_alloc_memory() {
	return total_memory;
}

///////////////////////////////////////////////////////////////////////////////
// init
///////////////////////////////////////////////////////////////////////////////
void init_memory(uint32 _max_segs, uint32 _max_pubs, uint32 _max_fixups) {
	max_segs    = _max_segs;
	max_pubs    = _max_pubs;
	max_fixups  = _max_fixups;

	all_segs    = (Segment  *)calloc_x(sizeof(Segment), max_segs);
	all_pubs    = (PubName  *)calloc_x(sizeof(PubName), max_pubs);
	all_fixups  = (Fixup    *)calloc_x(sizeof(Fixup),   max_fixups);

	list_names  = (LsName   *)calloc_x(sizeof(LsName),  max_segs);
	ext_names   = (ExtName  *)calloc_x(sizeof(ExtName), max_fixups);
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
	if (max_segs <= list_name_c) {
		fprintf(stderr, "Too many list of names! (max=%d)\n", max_segs);
		exit(11);
	}

	LsName *p = &list_names[list_name_c++];
	p->name  = name;

	return list_name_c;
}

char *load_list_name(int index) {
	return list_names[index-1].name;
}

///////////////////////////////////////////////////////////////////////////////
// Extern names
///////////////////////////////////////////////////////////////////////////////
void init_ext_name() {
	ext_name_c = 0;
}

int add_ext_name(char *name) {
	if (max_segs <= ext_name_c) {
		fprintf(stderr, "Too many extern names! (max=%d)\n", max_segs);
		exit(11);
	}

	ExtName *p = &ext_names[ext_name_c++];
	p->name = name;

	return ext_name_c;
}

char *load_ext_name(int index) {
	return ext_names[index-1].name;
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
	if (max_segs <= seg_c) {
		fprintf(stderr, "Too many segment! (max=%d)\n", max_segs);
		exit(11);
	}
	Segment *seg = &all_segs[seg_c++];
	seg->index   = seg_c - local_seg_c;	// index in a obj file

	return seg;
}

Segment *load_seg(int index) {
	return &all_segs[local_seg_c + index -1];
}

Segment *load_all_segs(int *count) {
	*count = seg_c;
	return all_segs;
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
	if (max_pubs <= pub_c) {
		fprintf(stderr, "Too many public name! (max=%d)\n", max_pubs);
		exit(11);
	}
	if (!seg->pubs) {	// set memory
		seg->pubs = &all_pubs[ pub_c ];
	}

	PubName *pub = &all_pubs[ pub_c++ ];
	seg->pub_c++; 
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
	if (max_fixups <= fixup_c) {
		fprintf(stderr, "Too many fixup! (max=%d)\n", max_fixups);
		exit(11);
	}
	if (!seg->fixups) {	// alloc memory
		seg->fixups = &all_fixups[ fixup_c ];
	}
	seg->fixup_c++;
	return &all_fixups[ fixup_c++ ];
}


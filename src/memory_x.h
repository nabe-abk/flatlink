#ifndef __MEMORY_X_H__
#define __MEMORY_X_H__

///////////////////////////////////////////////////////////////////////////////
// struct
///////////////////////////////////////////////////////////////////////////////
typedef struct _seg Segment;

typedef struct _pub_names {
	uint32	offset;
	Segment *seg;			// exists segment
	char	*name;
	struct _pub_names *next;	// start character index
} PubName;

typedef struct _ext_names {
	char	*name;
} ExtName;

typedef struct _ls_names {
	char	*name;
} LsName;

typedef struct _fixups {
	uint32	offset;
	uint32	place;		// default value by FIXUP
	int	bits32;
	int	relative;	// relative offset for jmp
	Segment *ref_seg;	// reference segment
	char	*ref_name;	// refer extern
} Fixup;

struct _seg {
	int	index;		// segment local index
	int	use32;		// use32 or use16
	uint32	base;		// base offset
	uint32	size;		// segment size
	int	align;		// alignment
	int	padding;	// align padding (byte)

	int	exist_entry;	// exist start entry
	uint32	entry;		// start entry offset

	char	*f_name;	// file name
	char	*name;		// segment name
	char	*c_name;	// segment class name

	int	pub_c;		// public names counter
	PubName	**pubs;		// public names
	int	ext_c;		// extern names counter
	ExtName	**exts;		// extern names

	int	fixup_c;	// fixup names counter
	Fixup	**fixups;	// fixups

	int	prev_offset;	// previous LEDATA's offset
	uchar	*code;		// segment code data
};

///////////////////////////////////////////////////////////////////////////////
// functions
///////////////////////////////////////////////////////////////////////////////

void *malloc_x(size_t size);
void *calloc_x(size_t num, size_t size);
void init_memory(int mul);

void  init_list_name();
int    add_list_name(char *name);
char *load_list_name(int index);

void  init_ext_name();
int    add_ext_name(char *name);
char *load_ext_name(int index);

void init_local_seg();
int  num_of_local_segs();

Segment *add_seg();
Segment *load_seg(int index);
Segment **load_all_segs(int *count);

void add_pub_name(Segment *seg, int offset, char *name);
int check_duplicate_pub_name();
PubName *search_pub_name(char *name, char *f_name);

Fixup *add_fixup(Segment *seg);

#endif	// __F_MEMORY_H__

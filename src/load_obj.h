#ifndef __LOAD_OBJ_H__
#define __LOAD_OBJ_H__

#define T_HEADER	0x80
#define T_COMMENT	0x88
#define T_MOD_END	0x8A
#define T_MOD_END32	0x8B

#define T_EXT_NAMES	0x8C
#define T_PUB_NAMES	0x90
#define T_PUB_NAMES32	0x91

#define T_LIST_NAMES	0x96
#define T_SEG_DEF	0x98
#define T_SEG_DEF32	0x99
#define T_GRP_DEF	0x9A

#define T_FIXUP		0x9C
#define T_FIXUP32	0x9D
#define T_LE_DATA	0xA0
#define T_LE_DATA32	0xA1
#define T_LI_DATA	0xA2
#define T_LI_DATA32	0xA3

#define	T_COMDEF	0xB0
#define	T_COMDAT	0xC2
#define	T_COMDAT32	0xC3


uchar *load_obj(char *filename);


#endif	// __LOAD_OBJ_H__

/* core of videomap */
#ifdef RECURSIVE
#if !defined(DECL_ZOOMX)

/* for each DECL_ZOOMX.... */
#define DECL_ZOOMX	0
#include "vmapcore.c"
#undef DECL_ZOOMX
#define DECL_ZOOMX	1
#include "vmapcore.c"
#undef DECL_ZOOMX
#define DECL_ZOOMX	2
#include "vmapcore.c"
#undef DECL_ZOOMX
#define DECL_ZOOMX	3
#include "vmapcore.c"
#undef DECL_ZOOMX
#define DECL_ZOOMX	4
#include "vmapcore.c"
#undef DECL_ZOOMX
#define DECL_ZOOMX	8
#include "vmapcore.c"
#undef DECL_ZOOMX

#elif !defined(GET_UINT32_PROC)

/* for each GET_UINT32_PROC.... */
#define GET_UINT32_PROC get_uint32
#include "vmapcore.c"
#undef GET_UINT32_PROC
#define GET_UINT32_PROC get_uint32_flip
#include "vmapcore.c"
#undef GET_UINT32_PROC

#else

#define LINEPROC_NAME_INT1(a,b,c,d,e)	dl##a##_##b##_##c##d##_##e
#define LINEPROC_NAME_INT2(a,b,c,d,e)	LINEPROC_NAME_INT1(a, b, c, d, e)
#define LINEPROC_NAME					LINEPROC_NAME_INT2(DECL_BPP, DECL_ZOOMX, HAS_CHARPROC, HAS_ARTIFACT, GET_UINT32_PROC)

#define get_uint32_proc(x)		GET_UINT32_PROC(x)

#ifdef DRAWLINE_TABLE

	LINEPROC_NAME,

#else

static void LINEPROC_NAME(struct drawline_params *params)
{
	const UINT32 *v;
	UINT32 l;
	UINT16 *s;
	UINT16 p;
	UINT32 pl;
	UINT32 last_l;
	const UINT16 *pens;
	register int zoomx;
	int rowlongs;

	assert(params->zoomx > 0);
	assert((DECL_ZOOMX == 0) || (params->zoomx == DECL_ZOOMX));
	assert(HAS_CHARPROC || !params->charproc);
	assert(!HAS_CHARPROC || params->charproc);
	assert(params->videoram_pos);
	assert(params->charproc || params->pens);
	assert(params->scanline_data);
	assert((params->bytes_per_row % 4) == 0);
	assert(params->bytes_per_row > 0);

	v = (const UINT32 *) (params->videoram_pos + params->offset);
	rowlongs = params->bytes_per_row / 4;
	zoomx = DECL_ZOOMX ? DECL_ZOOMX : params->zoomx;
	pens = params->pens;

	last_l = params->border_value;
	s = params->scanline_data;
	do
	{
		if (HAS_ARTIFACT)
		{
			UINT32 this_l;

			this_l = get_uint32_proc(v++);
			this_l = BIG_ENDIANIZE_INT32(this_l);
	
			l = (last_l << 24) | (this_l >> 8);
			EMITBYTE(DECL_ZOOMX, 0, HAS_CHARPROC, (DECL_BPP), 24-8-2, 1);
			EMITBYTE(DECL_ZOOMX, 1, HAS_CHARPROC, (DECL_BPP), 16-8-2, 1);
			l = (this_l << 8) | ((rowlongs > 1) ? ((UINT8 *) v)[0] : params->border_value);
			EMITBYTE(DECL_ZOOMX, 2, HAS_CHARPROC, (DECL_BPP),  8+8-2, 1);
			EMITBYTE(DECL_ZOOMX, 3, HAS_CHARPROC, (DECL_BPP),  0+8-2, 1);
			last_l = this_l;
		}
		else if ((DECL_BPP) == 16)
		{
			l = get_uint32_proc(v++);
			l = LITTLE_ENDIANIZE_INT32(l);
			EMITBYTE(DECL_ZOOMX, 0, HAS_CHARPROC, (DECL_BPP), 0, 0);
			EMITBYTE(DECL_ZOOMX, 1, HAS_CHARPROC, (DECL_BPP), 16, 0);
		}
		else
		{
			l = get_uint32_proc(v++);
			EMITBYTE(DECL_ZOOMX, 0, HAS_CHARPROC, (DECL_BPP), SHIFT0, 0);
			EMITBYTE(DECL_ZOOMX, 1, HAS_CHARPROC, (DECL_BPP), SHIFT1, 0);
			EMITBYTE(DECL_ZOOMX, 2, HAS_CHARPROC, (DECL_BPP), SHIFT2, 0);
			EMITBYTE(DECL_ZOOMX, 3, HAS_CHARPROC, (DECL_BPP), SHIFT3, 0);
		}
		s += (HAS_CHARPROC ? 8 : 1) * (DECL_ZOOMX ? DECL_ZOOMX : zoomx) * (32 / DECL_BPP);
	}
	while(--rowlongs);
}

#endif /* DRAWLINE_TABLE */

#undef get_uint32_proc
#undef LINEPROC_NAME_INT1
#undef LINEPROC_NAME_INT2
#undef LINEPROC_NAME

#endif

#else /* !defined(RECURSIVE) */

#define RECURSIVE
#define HAS_CHARPROC 0
#define HAS_ARTIFACT 0
#define DECL_BPP 1
#include "vmapcore.c"
#undef DECL_BPP
#define DECL_BPP 2
#include "vmapcore.c"
#undef DECL_BPP
#define DECL_BPP 4
#include "vmapcore.c"
#undef DECL_BPP
#define DECL_BPP 8
#include "vmapcore.c"
#undef DECL_BPP
#undef HAS_CHARPROC
#undef HAS_ARTIFACT

#define HAS_CHARPROC 1
#define HAS_ARTIFACT 0
#define DECL_BPP 8
#include "vmapcore.c"
#undef DECL_BPP
#define DECL_BPP 16
#include "vmapcore.c"
#undef DECL_BPP
#undef HAS_CHARPROC
#undef HAS_ARTIFACT

#define HAS_CHARPROC 0
#define HAS_ARTIFACT 1
#define DECL_BPP 1
#include "vmapcore.c"
#undef DECL_BPP
#undef HAS_CHARPROC
#undef HAS_ARTIFACT
#undef RECURSIVE

#endif /* defined(RECURSIVE) */


/***************************************************************************

  Sony PSX
  ========
  Driver by smf

todo:
	cd
	spu
	root counters
    finish gpu
	finish dma
	finish interrupts
	gte
	controllers
	mdec
	action replay

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/mips/mips.h"
#include "includes/psx.h"

#define READ_LONG(a)			(*(UINT32 *)(a))
#define WRITE_LONG(a,d)			(*(UINT32 *)(a) = (d))

/* vidhrdw */

#define VRAM_WIDTH ( 1024 )
#define VRAM_HEIGHT ( 512 )
#define VRAM_SIZE ( VRAM_WIDTH * VRAM_HEIGHT * 2L )

static UINT16 *m_p_vram;
static UINT8 m_n_gpu_buffer_offset;
static UINT32 m_n_vramx;
static UINT32 m_n_vramy;
static UINT32 m_n_twy;
static UINT32 m_n_twx;
static UINT32 m_n_twh;
static UINT32 m_n_tww;
//static UINT8 m_b_dirty;
//static UINT8 m_b_dirtyline[ VRAM_HEIGHT ];

#define COORD_X( a ) ( a.w.l )
#define COORD_Y( a ) ( a.w.h )
#define SIZE_W( a ) ( a.w.l )
#define SIZE_H( a ) ( a.w.h )
#define BGR_B( a ) ( a.b.h2 )
#define BGR_G( a ) ( a.b.h )
#define BGR_R( a ) ( a.b.l )
#define TEXTURE_V( a ) ( a.b.h )
#define TEXTURE_U( a ) ( a.b.l )

struct FLATVERTEX
{
	PAIR n_coord;
};

struct GOURAUDVERTEX
{
	PAIR n_bgr;
	PAIR n_coord;
};

struct FLATTEXTUREDVERTEX
{
	PAIR n_coord;
	PAIR n_texture;
};

struct GOURAUDTEXTUREDVERTEX
{
	PAIR n_bgr;
	PAIR n_coord;
	PAIR n_texture;
};

union
{
	UINT32 n_entry[ 16 ];

	struct
	{
		PAIR n_bgr;
		PAIR n_coord;
		PAIR n_size;
	} FlatRectangle;

	struct
	{
		PAIR n_bgr;
		PAIR n_coord;
		PAIR n_texture;
		PAIR n_size;
	} FlatTexturedRectangle;

	struct
	{
		PAIR n_bgr;
		struct FLATVERTEX vertex[ 4 ];
	} FlatPolygon;

	struct
	{
		struct GOURAUDVERTEX vertex[ 4 ];
	} GouraudPolygon;

	struct
	{
		PAIR n_bgr;
		struct FLATTEXTUREDVERTEX vertex[ 4 ];
	} FlatTexturedPolygon;

	struct
	{
		struct GOURAUDTEXTUREDVERTEX vertex[ 4 ];
	} GouraudTexturedPolygon;

} m_packet;

static UINT32 m_n_gpustatus;

static PALETTE_INIT( psx )
{
	UINT32 n_r;
	UINT32 n_g;
	UINT32 n_b;
	UINT32 n_colour;

	for( n_colour = 0; n_colour < 0x10000; n_colour++ )
	{
		n_r = ( ( n_colour & 0x1f ) * 0xff ) / 0x1f;
		n_g = ( ( ( n_colour >> 5 ) & 0x1f ) * 0xff ) / 0x1f;
		n_b = ( ( ( n_colour >> 10 ) & 0x1f ) * 0xff ) / 0x1f;

		palette_set_color( n_colour, n_r, n_g, n_b );
	}
}

static VIDEO_START( psx )
{
	m_p_vram = auto_malloc( VRAM_SIZE );
	if( m_p_vram == NULL )
	{
		return 1;
	}

	m_n_gpustatus = 0x14802000;
	m_n_gpu_buffer_offset = 0;
	memset( m_p_vram, 0x00, VRAM_SIZE );
//	m_b_dirty = 0;
//	memset( m_b_dirtyline, 0, sizeof( m_b_dirtyline ) );

	return 0;
}

static VIDEO_UPDATE( psx )
{
	/*
	 * Todo: Other screen modes.
	 *
	 */

	UINT16 *p_vram;
	UINT16 *p_line;
	UINT32 n_y;
	UINT32 n_x;

//	if( m_b_dirty )
	{
		for( n_y = 0; n_y < bitmap->height; n_y++ )
		{
//			if( m_b_dirtyline[ n_y ] )
			{
				p_line = (UINT16 *)bitmap->line[ n_y ];
				p_vram = &m_p_vram[ n_y * VRAM_WIDTH ];
//				m_b_dirtyline[ n_y ] = 0;

				for( n_x = 0; n_x < bitmap->width; n_x++ )
				{
					*( p_line++ ) = *( p_vram++ );
				}
			}
		}
//		m_b_dirty = 0;
	}
}

static void FlatPolygon( int n_points )
{
	/* Draws a flat polygon. No clipping / offset / transparency / dithering & probably not pixel perfect. */

	UINT16 n_y;
	UINT16 n_x;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_cx1;
	PAIR n_cx2;
	INT32 n_dx1;
	INT32 n_dx2;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;

	UINT16 *p_vram;

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( m_packet.FlatPolygon.vertex[ n_point ].n_coord.d < m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.FlatPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.FlatPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_r.w.h = BGR_R( m_packet.FlatTexturedPolygon.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatTexturedPolygon.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatTexturedPolygon.n_bgr ); n_b.w.l = 0;

	n_dx1 = 0;
	n_dx2 = 0;

	n_y = COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord );
	p_vram = &m_p_vram[ n_y * VRAM_WIDTH ];

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
		}
		n_distance = n_cx2.w.h - n_cx1.w.h;
		if( n_distance > 0 )
		{
			n_x = n_cx1.w.h;
			while( n_distance != 0 )
			{
				p_vram[ n_x ] = ( ( n_r.w.h & 0xf8 ) >> 3 ) | ( ( n_g.w.h & 0xf8 ) << 2 ) | ( ( n_b.w.h & 0xf8 ) << 7 );
				n_x++;
				n_distance--;
			}
//			m_b_dirtyline[ n_y ] = 1;
		}
		n_cx1.d += n_dx1;
		n_cx2.d += n_dx2;
		p_vram += VRAM_WIDTH;
		n_y++;
	}
//	m_b_dirty = 1;
}

static void FlatTexturedPolygon( int n_points )
{
	/*
	 * Todo: shading
	 *       drawing environment
	 *       rendering options
	 *       texture options
	 */

	UINT32 n_tx;
	UINT32 n_ty;

	UINT32 n_clutx;
	UINT32 n_cluty;

	UINT16 n_y;
	UINT16 n_x;

	UINT32 n_bgr;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_u;
	PAIR n_v;

	PAIR n_cx1;
	PAIR n_cx2;
	PAIR n_cu1;
	PAIR n_cv1;
	PAIR n_cu2;
	PAIR n_cv2;
	INT32 n_du;
	INT32 n_dv;
	INT32 n_dx1;
	INT32 n_dx2;
	INT32 n_du1;
	INT32 n_dv1;
	INT32 n_du2;
	INT32 n_dv2;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;
	UINT16 *p_vram;
	UINT16 *p_clut;

	if( ( m_packet.FlatTexturedPolygon.n_bgr.d & 0xffffff ) != 0x808080 )
	{
		logerror( "textured 4 point polygon: shading not supported (%08x)\n", m_packet.FlatTexturedPolygon.n_bgr.d & 0xffffff );
	}

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord.d < m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.FlatTexturedPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.FlatTexturedPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_clutx = ( m_packet.FlatTexturedPolygon.vertex[ 0 ].n_texture.w.h & 0x3f ) << 4;
	n_cluty = ( m_packet.FlatTexturedPolygon.vertex[ 0 ].n_texture.w.h >> 6 ) & 0x1ff;
	p_clut = &m_p_vram[ n_cluty * VRAM_WIDTH + n_clutx ];

	n_tx = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x0f ) << 6;
	n_ty = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x10 ) << 3;

	n_r.w.h = BGR_R( m_packet.FlatTexturedPolygon.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatTexturedPolygon.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatTexturedPolygon.n_bgr ); n_b.w.l = 0;

	n_dx1 = 0;
	n_dx2 = 0;
	n_du1 = 0;
	n_du2 = 0;
	n_dv1 = 0;
	n_dv2 = 0;

	n_y = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord );
	p_vram = &m_p_vram[ n_y * VRAM_WIDTH ];

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_cu1.w.h = TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ); n_cu1.w.l = 0;
			n_cv1.w.h = TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ); n_cv1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
			n_du1 = (INT32)( ( TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ) << 16 ) - n_cu1.d ) / n_distance;
			n_dv1 = (INT32)( ( TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ) << 16 ) - n_cv1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_cu2.w.h = TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ); n_cu2.w.l = 0;
			n_cv2.w.h = TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ); n_cv2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
			n_du2 = (INT32)( ( TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ) << 16 ) - n_cu2.d ) / n_distance;
			n_dv2 = (INT32)( ( TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ) << 16 ) - n_cv2.d ) / n_distance;
		}
		n_distance = n_cx2.w.h - n_cx1.w.h;
		if( n_distance > 0 )
		{
			n_u.d = n_cu1.d;
			n_v.d = n_cv1.d;
			n_du = (INT32)( n_cu2.d - n_cu1.d ) / n_distance;
			n_dv = (INT32)( n_cv2.d - n_cv1.d ) / n_distance;
			n_x = n_cx1.w.h;
			while( n_distance != 0 )
			{
				n_bgr = p_clut[ ( ( m_p_vram[ ( n_ty + n_v.w.h ) * VRAM_WIDTH + n_tx + ( n_u.w.h >> 2 ) ] >> ( ( n_u.w.h & 0x03 ) << 2 ) ) & 0x0f ) ];
				if( n_bgr != 0 )
				{
					p_vram[ n_x ] = n_bgr;
				}
				n_x++;
				n_u.d += n_du;
				n_v.d += n_dv;
				n_distance--;
			}
//			m_b_dirtyline[ n_y ] = 1;
		}
		n_cx1.d += n_dx1;
		n_cu1.d += n_du1;
		n_cv1.d += n_dv1;
		n_cx2.d += n_dx2;
		n_cu2.d += n_du2;
		n_cv2.d += n_dv2;
		p_vram += VRAM_WIDTH;
		n_y++;
	}
//	m_b_dirty = 1;
}

static void GouraudPolygon( int n_points )
{
	/* Draws a shaded polygon. No clipping / offset / transparency / dithering & probably not pixel perfect. */

	UINT16 n_y;
	UINT16 n_x;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_cx1;
	PAIR n_cx2;
	PAIR n_cr1;
	PAIR n_cg1;
	PAIR n_cb1;
	PAIR n_cr2;
	PAIR n_cg2;
	PAIR n_cb2;
	INT32 n_dr;
	INT32 n_dg;
	INT32 n_db;
	INT32 n_dx1;
	INT32 n_dx2;
	INT32 n_dr1;
	INT32 n_dg1;
	INT32 n_db1;
	INT32 n_dr2;
	INT32 n_dg2;
	INT32 n_db2;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;

	UINT16 *p_vram;

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( m_packet.GouraudPolygon.vertex[ n_point ].n_coord.d < m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.GouraudPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.GouraudPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_dx1 = 0;
	n_dx2 = 0;
	n_dr1 = 0;
	n_dr2 = 0;
	n_dg1 = 0;
	n_dg2 = 0;
	n_db1 = 0;
	n_db2 = 0;

	n_y = COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord );
	p_vram = &m_p_vram[ n_y * VRAM_WIDTH ];

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_cr1.w.h = BGR_R( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cr1.w.l = 0;
			n_cg1.w.h = BGR_G( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cg1.w.l = 0;
			n_cb1.w.h = BGR_B( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cb1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
			n_dr1 = (INT32)( ( BGR_R( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cr1.d ) / n_distance;
			n_dg1 = (INT32)( ( BGR_G( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cg1.d ) / n_distance;
			n_db1 = (INT32)( ( BGR_B( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cb1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_cr2.w.h = BGR_R( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cr2.w.l = 0;
			n_cg2.w.h = BGR_G( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cg2.w.l = 0;
			n_cb2.w.h = BGR_B( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cb2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
			n_dr2 = (INT32)( ( BGR_R( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cr2.d ) / n_distance;
			n_dg2 = (INT32)( ( BGR_G( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cg2.d ) / n_distance;
			n_db2 = (INT32)( ( BGR_B( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cb2.d ) / n_distance;
		}
		n_distance = n_cx2.w.h - n_cx1.w.h;
		if( n_distance > 0 )
		{
			n_r.d = n_cr1.d;
			n_g.d = n_cg1.d;
			n_b.d = n_cb1.d;
			n_dr = (INT32)( n_cr2.d - n_cr1.d ) / n_distance;
			n_dg = (INT32)( n_cg2.d - n_cg1.d ) / n_distance;
			n_db = (INT32)( n_cb2.d - n_cb1.d ) / n_distance;
			n_x = n_cx1.w.h;
			while( n_distance != 0 )
			{
				p_vram[ n_x ] = ( ( n_r.w.h & 0xf8 ) >> 3 ) | ( ( n_g.w.h & 0xf8 ) << 2 ) | ( ( n_b.w.h & 0xf8 ) << 7 );
				n_x++;
				n_r.d += n_dr;
				n_g.d += n_dg;
				n_b.d += n_db;
				n_distance--;
			}
//			m_b_dirtyline[ n_y ] = 1;
		}
		n_cx1.d += n_dx1;
		n_cr1.d += n_dr1;
		n_cg1.d += n_dg1;
		n_cb1.d += n_db1;
		n_cx2.d += n_dx2;
		n_cr2.d += n_dr2;
		n_cg2.d += n_dg2;
		n_cb2.d += n_db2;
		p_vram += VRAM_WIDTH;
		n_y++;
	}
//	m_b_dirty = 1;
}

static void FlatRectangle( void )
{
	/* shading, clipping & dithering not supported. 
	( currently used for packet $02 & $60, but $02 doesn't use drawing environment / offset etc ) */
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_w;
	UINT32 n_h;
	UINT32 n_y;
	UINT16 *p_vram;
	UINT16 n_modvram;

	n_r.w.h = BGR_R( m_packet.FlatRectangle.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatRectangle.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatRectangle.n_bgr ); n_b.w.l = 0;

	n_y = COORD_Y( m_packet.FlatRectangle.n_coord );
	p_vram = &m_p_vram[ n_y * VRAM_WIDTH + COORD_X( m_packet.FlatRectangle.n_coord ) ];
	n_modvram = VRAM_WIDTH - SIZE_W( m_packet.FlatRectangle.n_size );

	n_h = SIZE_H( m_packet.FlatRectangle.n_size );
	while( n_h != 0 )
	{
		n_w = SIZE_W( m_packet.FlatRectangle.n_size );
		while( n_w != 0 )
		{
			*( p_vram++ ) = ( ( n_r.w.h & 0xf8 ) >> 3 ) | ( ( n_g.w.h & 0xf8 ) << 2 ) | ( ( n_b.w.h & 0xf8 ) << 7 );
			n_w--;
		}
		p_vram += n_modvram;
//		m_b_dirtyline[ n_y ] = 1;
		n_y++;
		n_h--;
	}
//	m_b_dirty = 1;
}

static void FlatTexturedRectangle( void )
{
	/* texture window, clipping & shading not supported. */
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_tx;
	UINT32 n_ty;

	UINT32 n_clutx;
	UINT32 n_cluty;

	UINT16 n_u;
	UINT16 n_v;
	UINT32 n_w;
	UINT32 n_h;
	UINT32 n_x;
	UINT32 n_y;
	UINT16 *p_vram;
	UINT16 *p_clut;
	UINT16 n_bgr;

	n_r.w.h = BGR_R( m_packet.FlatTexturedRectangle.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatTexturedRectangle.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatTexturedRectangle.n_bgr ); n_b.w.l = 0;

	n_clutx = ( m_packet.FlatTexturedRectangle.n_texture.w.h & 0x3f ) << 4;
	n_cluty = ( m_packet.FlatTexturedRectangle.n_texture.w.h >> 6 ) & 0x1ff;
	p_clut = &m_p_vram[ n_cluty * VRAM_WIDTH + n_clutx ];

	n_tx = ( m_n_gpustatus & 0x0f ) << 6;
	n_ty = ( m_n_gpustatus & 0x10 ) << 3;

	n_v = TEXTURE_V( m_packet.FlatTexturedRectangle.n_texture );
	n_y = COORD_Y( m_packet.FlatTexturedRectangle.n_coord );
	p_vram = &m_p_vram[ n_y * VRAM_WIDTH ];

	n_h = SIZE_H( m_packet.FlatTexturedRectangle.n_size );
	while( n_h != 0 )
	{
		n_x = COORD_X( m_packet.FlatTexturedRectangle.n_coord );
		n_u = TEXTURE_U( m_packet.FlatTexturedRectangle.n_texture );
		n_w = SIZE_W( m_packet.FlatTexturedRectangle.n_size );
		while( n_w != 0 )
		{
			n_bgr = p_clut[ ( ( m_p_vram[ ( n_ty + n_v ) * VRAM_WIDTH + n_tx + ( n_u >> 2 ) ] >> ( ( n_u & 0x03 ) << 2 ) ) & 0x0f ) ];
			if( n_bgr != 0 )
			{
				p_vram[ n_x ] = n_bgr;
			}
			n_u++;
			n_x++;
			n_w--;
		}
		p_vram += VRAM_WIDTH;
//		m_b_dirtyline[ n_y ] = 1;
		n_v++;
		n_y++;
		n_h--;
	}
//	m_b_dirty = 1;
}

static WRITE32_HANDLER( gpu_w )
{
	switch( offset )
	{
	case 0x00:
		m_packet.n_entry[ m_n_gpu_buffer_offset ] = data;
		switch( m_packet.n_entry[ 0 ] >> 24 )
		{
		case 0x00:
/*			logerror( "*?????\n" ); */
			break;
		case 0x01:
			m_n_vramx = 0;
			m_n_vramy = 0;
/*			logerror( "clear cache\n" ); */
			break;
		case 0x02:
			if( m_n_gpu_buffer_offset < 2 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
/*				logerror( "frame buffer rectangle draw %d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 2 ] & 0xffff, m_packet.n_entry[ 2 ] >> 16 ); */
				/* WRONG!!! */
				FlatRectangle();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x60:
			if( m_n_gpu_buffer_offset < 2 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
/*				logerror( "flat rectangle %d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 2 ] & 0xffff, m_packet.n_entry[ 2 ] >> 16 ); */
				FlatRectangle();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x64:
			if( m_n_gpu_buffer_offset < 3 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
/*				logerror( "flat textured rectangle%d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 3 ] & 0xffff, m_packet.n_entry[ 3 ] >> 16 ); */
				FlatTexturedRectangle();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0xa0:
			if( m_n_gpu_buffer_offset < 3 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				static UINT32 n_pixel;
				for( n_pixel = 0; n_pixel < 2; n_pixel++ )
				{
					m_p_vram[ ( m_n_vramy + ( m_packet.n_entry[ 1 ] >> 16 ) ) * VRAM_WIDTH + m_n_vramx + ( m_packet.n_entry[ 1 ] & 0xffff ) ] = data & 0xffff;
//					m_b_dirtyline[ m_n_vramy ] = 1;
					m_n_vramx++;
					if( m_n_vramx >= ( m_packet.n_entry[ 2 ] & 0xffff ) )
					{
						m_n_vramx = 0;
						m_n_vramy++;
						if( m_n_vramy >= ( m_packet.n_entry[ 2 ] >> 16 ) )
						{
/*							logerror( "send image to frame buffer\n" ); */
							m_n_gpu_buffer_offset = 0;
						}
					}
					data >>= 16;
				}
//				m_b_dirty = 1;
			}
			break;
		case 0xc0:
			if( m_n_gpu_buffer_offset < 2 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				m_n_gpustatus |= ( 1L << 27 );
			}
			break;
		case 0xe1:
/*			logerror( "draw mode %06x\n", m_packet.n_entry[ 0 ] & 0xffffff ); */
			m_n_gpustatus = ( m_n_gpustatus & 0xfffff800 ) | ( m_packet.n_entry[ 0 ] & 0x7ff );
			break;
		case 0xe2:
/*			logerror( "texture window setting %06x\n", m_packet.n_entry[ 0 ] & 0xffffff ); */
			m_n_twy = ( ( ( m_packet.n_entry[ 0 ] >> 15 ) & 0x1f ) << 3 );
			m_n_twx = ( ( ( m_packet.n_entry[ 0 ] >> 10 ) & 0x1f ) << 3 );
			m_n_twh = 256 - ( ( ( m_packet.n_entry[ 0 ] >> 5 ) & 0x1f ) << 3 );
			m_n_tww = 256 - ( ( m_packet.n_entry[ 0 ] & 0x1f ) << 3 );
			break;
		case 0xe3:
/*			logerror( "drawing area top left %d,%d\n", m_packet.n_entry[ 0 ] & 1023, ( m_packet.n_entry[ 0 ] >> 10 ) & 511 ); */
			break;
		case 0xe4:
/*			logerror( "drawing area bottom right %d,%d\n", m_packet.n_entry[ 0 ] & 1023, ( m_packet.n_entry[ 0 ] >> 10 ) & 511 ); */
			break;
		case 0xe5:
/*			logerror( "drawing offset %d,%d\n", m_packet.n_entry[ 0 ] & 2047, ( m_packet.n_entry[ 0 ] >> 11 ) & 511 ); */
			break;
		case 0xe6:
/*			logerror( "mask setting %d\n", m_packet.n_entry[ 0 ] & 3 ); */
			break;
		case 0x20:
			if( m_n_gpu_buffer_offset < 3 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				FlatPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x24:
			if( m_n_gpu_buffer_offset < 6 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				FlatTexturedPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}

		case 0x28:
			if( m_n_gpu_buffer_offset < 4 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				FlatPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x2c:
			if( m_n_gpu_buffer_offset < 8 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				FlatTexturedPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x30:
			if( m_n_gpu_buffer_offset < 5 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				GouraudPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x38:
			if( m_n_gpu_buffer_offset < 7 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				GouraudPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		default:
			logerror( "unknown gpu packet %08x\n", m_packet.n_entry[ 0 ] );
			m_n_gpu_buffer_offset = 1;
			break;
		}
		break;
	case 0x01:
		switch( data >> 24 )
		{
		case 0x00:
/*			logerror( "reset gpu\n" ); */
			break;
		case 0x01:
/*			logerror( "reset command buffer\n" ); */
			break;
		case 0x02:
/*			logerror( "reset irq\n" ); */
			break;
		case 0x03:
/*			logerror( "display enable %d\n", data & 1 ); */
			break;
		case 0x04:
/*			logerror( "dma setup %d\n", data & 3 ); */
			break;
		case 0x05:
/*			logerror( "start of display area %d %d\n", data & 1023, ( data >> 10 ) & 511 ); */
			break;
		case 0x06:
/*			logerror( "horizontal display range %d %d\n", data & 4095, ( data >> 12 ) & 4095 ); */
			break;
		case 0x07:
/*			logerror( "vertical display range %d %d\n", data & 1023, ( data >> 10 ) & 2047 ); */
			break;
		case 0x08:
/*			logerror( "display mode %02x\n", data & 0xff ); */
			break;
		case 0x09:
/*			logerror( "*????? %06x\n", data & 0xffffff ); */
			break;
		default:
			logerror( "unknown gpu command %08x\n", data );
			break;
		}
		break;
	default:
		logerror( "%08x: gpu_w( %08x, %08x, %08x ) unknown register\n", activecpu_get_pc(), offset, data, mem_mask );
		break;
	}
}

static READ32_HANDLER( gpu_r )
{
	switch( offset )
	{
	case 0x00:
		switch( m_packet.n_entry[ 0 ] >> 24 )
		{
		case 0xc0:
			if( ( m_n_gpustatus & ( 1L << 27 ) ) != 0 )
			{
				UINT32 n_pixel;
				PAIR data;

				data.d = 0;
				for( n_pixel = 0; n_pixel < 2; n_pixel++ )
				{
					data.w.l = data.w.h;
					data.w.h = m_p_vram[ ( m_n_vramy + ( m_packet.n_entry[ 1 ] >> 16 ) ) * VRAM_WIDTH + m_n_vramx + ( m_packet.n_entry[ 1 ] & 0xffff ) ];
					m_n_vramx++;
					if( m_n_vramx >= ( m_packet.n_entry[ 2 ] & 0xffff ) )
					{
						m_n_vramx = 0;
						m_n_vramy++;
						if( m_n_vramy >= ( m_packet.n_entry[ 2 ] >> 16 ) )
						{
							m_n_gpustatus &= ~( 1L << 27 );
							m_n_gpu_buffer_offset = 0;
						}
					}
				}
				return data.d;
			}
			break;
		}
		return 0;
	case 0x01:
		return m_n_gpustatus;
	default:
		logerror( "%08x: gpu_r( %08x, %08x ) unknown register\n", activecpu_get_pc(), offset, mem_mask );
		break;
	}
	return 0;
}

/* machine */

static UINT32 m_n_irqdata;
static UINT32 m_n_irqmask;

static WRITE32_HANDLER( irq_w )
{
	switch( offset )
	{
	case 0x00:
		m_n_irqdata = ( m_n_irqdata & mem_mask ) | ( m_n_irqdata & m_n_irqmask & data );
		if( ( m_n_irqdata & m_n_irqmask ) == 0 )
		{
			cpu_set_irq_line( 0, 0, CLEAR_LINE );
		}
		break;
	case 0x01:
		m_n_irqmask = ( m_n_irqmask & mem_mask ) | data;
		if( ( m_n_irqmask & ~( 0x1 | 0x08 | 0x20 ) ) != 0 )
		{
			logerror( "%08x: irq_w( %08x, %08x, %08x ) unknown irq\n", activecpu_get_pc(), offset, data, mem_mask );
		}
		if( ( m_n_irqdata & m_n_irqmask ) != 0 )
		{
			cpu_set_irq_line( 0, 0, ASSERT_LINE );
		}
		break;
	default:
		logerror( "%08x: irq_w( %08x, %08x, %08x ) unknown register\n", activecpu_get_pc(), offset, data, mem_mask );
		break;
	}
}

static READ32_HANDLER( memcard_r )
{
	return 0xffffffff;
}

static READ32_HANDLER( irq_r )
{
	switch( offset )
	{
	case 0x00:
		return m_n_irqdata;
	case 0x01:
		return m_n_irqmask;
	default:
		logerror( "unknown irq read %d\n", offset );
		break;
	}
	return 0;
}

void psx_irq_trigger( UINT32 data )
{
	m_n_irqdata |= data;
	if( ( m_n_irqdata & m_n_irqmask ) != 0 )
	{
		cpu_set_irq_line( 0, 0, ASSERT_LINE );
	}
}

void psx_irq_clear( UINT32 data )
{
	m_n_irqdata &= ~data;
	if( ( m_n_irqdata & m_n_irqmask ) == 0 )
	{
		cpu_set_irq_line( 0, 0, CLEAR_LINE );
	}
}

static INTERRUPT_GEN( psx_interrupt )
{
	psx_irq_trigger( 0x0001 );
}

static UINT32 m_n_dmabase[ 7 ];
static UINT32 m_n_dmablockcontrol[ 7 ];
static UINT32 m_n_dmachannelcontrol[ 7 ];
static UINT32 m_n_dpcp;
static UINT32 m_n_dicr;

static WRITE32_HANDLER( dma_w )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			m_n_dmabase[ n_channel ] = data;
			break;
		case 1:
			m_n_dmablockcontrol[ n_channel ] = data;
			break;
		case 2:
			if( ( data & ( 1L << 0x18 ) ) != 0 && ( m_n_dpcp & ( 1 << ( 3 + ( n_channel * 4 ) ) ) ) != 0 )
			{
				static UINT32 n_size;
				static UINT32 n_address;
				static UINT32 n_nextaddress;
				static unsigned char *p_ram;

				p_ram = memory_region( REGION_CPU1 );
				n_address = m_n_dmabase[ n_channel ] & 0xffffff;

				if( n_channel == 2 )
				{
					if( data == 0x01000401 )
					{
						do
						{
							n_nextaddress = READ_LONG( &p_ram[ n_address ] );
							n_address += 4;
							n_size = ( n_nextaddress >> 24 );
							while( n_size != 0 )
							{
								gpu_w( 0, READ_LONG( &p_ram[ n_address ] ), 0 );
								n_address += 4;
								n_size--;
							}
							n_address = ( n_nextaddress & 0xffffff );
						} while( n_address != 0xffffff );
						data &= ~( 1L << 0x18 );
					}
					else if( data == 0x01000201 )
					{
						n_size = ( m_n_dmablockcontrol[ n_channel ] >> 16 ) * ( m_n_dmablockcontrol[ n_channel ] & 0xffff );
						while( n_size != 0 )
						{
							gpu_w( 0, READ_LONG( &p_ram[ n_address ] ), 0 );
							n_address += 4;
							n_size--;
						}
						data &= ~( 1L << 0x18 );
					}
					else
					{
						logerror( "dma 2 unknown mode %08x\n", data );
					}
				}
				else if( n_channel == 6 && data == 0x11000002 )
				{
					n_size = m_n_dmablockcontrol[ n_channel ];
					while( n_size != 0 )
					{
						n_nextaddress = ( n_address - 4 ) & 0xffffff;
						WRITE_LONG( &p_ram[ n_address ], n_nextaddress );
						n_address = n_nextaddress;
						n_size--;
					}
					WRITE_LONG( &p_ram[ n_address ], 0xffffff );

					data &= ~( 1L << 0x18 );
				}
				else
				{
					logerror( "unknown dma channel / mode (%d / %08x)\n", n_channel, data );
				}
				if( ( m_n_dicr & ( 1 << ( 16 + n_channel ) ) ) != 0 )
				{
					m_n_dicr |= 0x80000000 | ( 1 << ( 24 + n_channel ) );
					psx_irq_trigger( 0x0008 );
				}
			}
			m_n_dmachannelcontrol[ n_channel ] = data;
			break;
		default:
			logerror( "dma_w( %04x, %08x, %08x ) Unknown dma channel register\n", offset, data, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			m_n_dpcp = ( m_n_dpcp & mem_mask ) | data;
			break;
		case 0x1:
			m_n_dicr = ( m_n_dicr & mem_mask ) | ( data & 0xffffff );
			break;
		default:
			logerror( "dma_w( %04x, %08x, %08x ) Unknown dma control register\n", offset, data, mem_mask );
			break;
		}
	}
}

static READ32_HANDLER( dma_r )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			return m_n_dmabase[ n_channel ];
		case 1:
			return m_n_dmablockcontrol[ n_channel ];
		case 2:
			return m_n_dmachannelcontrol[ n_channel ];
		default:
			logerror( "%08x: dma_r( %08x, %08x ) Unknown dma channel register\n", activecpu_get_pc(), offset, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			return m_n_dpcp;
		case 0x1:
			return m_n_dicr;
		default:
			logerror( "%08x: dma_r( %08x, %08x ) Unknown dma control register\n", activecpu_get_pc(), offset, mem_mask );
			break;
		}
	}
	return 0;
}

static WRITE32_HANDLER( spu_w )
{
	return;
}

static READ32_HANDLER( spu_r )
{
	return 0;
}

/* driver */

INPUT_PORTS_START( psx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

static MEMORY_READ32_START( psx_readmem )
	{ 0x00000000, 0x001fffff, MRA32_RAM },		/* ram */
	{ 0x1f800000, 0x1f8003ff, MRA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_r },
	{ 0x1f801070, 0x1f801077, irq_r },
	{ 0x1f801080, 0x1f8010ff, dma_r },
	{ 0x1f801100, 0x1f80112f, MRA32_NOP },	/* counters */
	{ 0x1f801800, 0x1f801803, psx_cd_r },
	{ 0x1f801810, 0x1f801817, gpu_r },
	{ 0x1f801c00, 0x1f801dff, spu_r },
	{ 0x80000000, 0x801fffff, MRA32_BANK2 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MRA32_BANK3 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA32_BANK4 },	/* bios */
MEMORY_END

static MEMORY_WRITE32_START( psx_writemem )
	{ 0x00000000, 0x001fffff, MWA32_RAM },	/* ram */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_w },
	{ 0x1f801070, 0x1f801077, irq_w },
	{ 0x1f801080, 0x1f8010ff, dma_w },
	{ 0x1f801100, 0x1f80112f, MWA32_NOP },	/* counters */
	{ 0x1f801800, 0x1f801803, psx_cd_w },
	{ 0x1f801810, 0x1f801817, gpu_w },
	{ 0x1f801c00, 0x1f801dff, spu_w },
	{ 0x1f802040, 0x1f802043, MWA32_NOP },	/* unknown */
	{ 0x80000000, 0x801fffff, MWA32_BANK2 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MWA32_BANK3 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM },	/* bios */
	{ 0xfffe0130, 0xfffe0133, MWA32_NOP },	/* unknown */
MEMORY_END

static MACHINE_INIT( psx )
{
	cpu_setbank( 1, memory_region( REGION_CPU1 ) + 0x200000 );
	cpu_setbank( 2, memory_region( REGION_CPU1 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER1 ) );

	/* gpu */
	m_n_gpustatus = 0x14802000;
	m_n_gpu_buffer_offset = 0;
	memset( m_p_vram, 0x00, VRAM_SIZE );
//	m_b_dirty = 1;
//	memset( m_b_dirtyline, 0xff, sizeof( m_b_dirtyline ) );

	/* irq */
	m_n_irqdata = 0;
	m_n_irqmask = 0;

	/* dma */
	m_n_dpcp = 0;
	m_n_dicr = 0;
}

static MACHINE_DRIVER_START( psx )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33868800) /* 33MHz ?? */
	MDRV_CPU_MEMORY(psx_readmem,psx_writemem)
	MDRV_CPU_VBLANK_INT(psx_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(psx)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx)
	MDRV_VIDEO_UPDATE(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

ROM_START( psx )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1000.bin",  0x0000000, 0x080000, CRC(3b601fc8) SHA1(343883a7b555646da8cee54aadd2795b6e7dd070) )
ROM_END

ROM_START( psxj22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5000.bin",  0x0000000, 0x080000, CRC(24fc7e17) SHA1(ffa7f9a7fb19d773a0c3985a541c8e5623d2c30d) ) /* bad 0x8c93a399 */
ROM_END

ROM_START( psxa22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1001.bin",  0x0000000, 0x080000, CRC(37157331) SHA1(10155d8d6e6e832d6ea66db9bc098321fb5e8ebf) )
ROM_END

ROM_START( psxe22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "dtlh3002.bin",  0x0000000, 0x080000, CRC(1e26792f) SHA1(b6a11579caef3875504fcf3831b8e3922746df2c) )
ROM_END

ROM_START( psxj30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5500.bin",  0x0000000, 0x080000, CRC(ff3eeb8c) SHA1(b05def971d8ec59f346f2d9ac21fb742e3eb6917) )
ROM_END

ROM_START( psxa30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7003.bin",  0x0000000, 0x080000, CRC(8d8cb7e4) SHA1(0555c6fae8906f3f09baf5988f00e55f88e9f30b) )
ROM_END

ROM_START( psxe30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5502.bin",  0x0000000, 0x080000, CRC(4d9e7c86) SHA1(f8de9325fc36fcfa4b29124d291c9251094f2e54) )
ROM_END

ROM_START( psxj40 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7000.bin",  0x0000000, 0x080000, CRC(ec541cd0) SHA1(77b10118d21ac7ffa9b35f9c4fd814da240eb3e9) )
ROM_END

ROM_START( psxa41 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7001.bin",  0x0000000, 0x080000, CRC(502224b6) SHA1(14df4f6c1e367ce097c11deae21566b4fe5647a9) )
ROM_END

ROM_START( psxe41 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7502.bin",  0x0000000, 0x080000, CRC(318178bf) SHA1(8d5de56a79954f29e9006929ba3fed9b6a418c1d) )
ROM_END

SYSTEM_CONFIG_START(psx)
SYSTEM_CONFIG_END

/*
The version number & release date is stored in ascii text at the end of every bios, except for scph1000.
There is also a BCD encoded date at offset 0x100, but this is set to 041211995 in every version apart
from scph1000 & scph7000 ( where it is 22091994 & 29051997 respectively ).
*/

/*		YEAR	NAME	PARENT	COMPAT	MACHINE INPUT	INIT	CONFIG  COMPANY 	FULLNAME */
COMPX( 1994,	psx,	0,		0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph1000)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1995,	psxj22,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph5000 J v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1995,	psxa22,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph1001/dtlh3000 A v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1995,	psxe22,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph1002/dtlh3002 E v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1996,	psxj30,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph5500 J v3.0 09/09/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1996,	psxa30,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph7003 A v3.0 11/18/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1997,	psxe30,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph5502 E v3.0 01/06/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1997,	psxj40,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph7000 J v4.0 08/18/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1997,	psxa41,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph7001 A v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
COMPX( 1997,	psxe41,	psx,	0,		psx,	psx,	0,		psx,	"Sony",		"Sony PSX (scph7502 E v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )

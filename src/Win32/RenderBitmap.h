/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __RENDERBITMAP_H__
#define __RENDERBITMAP_H__

typedef void (*RenderMethod)(struct osd_bitmap* pSrcBitmap,
                             UINT nSrcStartLine, UINT nSrcStartColumn,
                             UINT nNumLines, UINT nNumColumns,
                             BYTE* pDst, UINT nDstWidth);

extern RenderMethod SelectRenderMethod(BOOL bDouble, BOOL bVDouble, BOOL bHScanlines, BOOL bVScanlines,
                                       enum DirtyMode eDirtyMode, BOOL b16bit, BOOL bPalette16,
                                       const UINT32* p16BitLookup, BOOL bMMX);
extern RenderMethod SelectRenderMethodMMX(BOOL bDouble, BOOL bHScanlines, BOOL bVScanlines,
                                          enum DirtyMode eDirtyMode, BOOL b16bit, BOOL bPalette16,
                                          const UINT32* p16BitLookup);

#endif

; effect_asm.asm
;
; MMX assembly language video effect functions
;
; 2004 Richard Goedeken <SirRichard@fascinationsoftware.com>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;
; HISTORY:
;
;  2004-09-25
;   - added effect_6tap_render_16 function
;  2004-09-22:
;   - modified 6-tap filter to work with simplified effect.c code
;  2004-07-24:
;   - initial version, including the 6-tap sinc filter, and the scanline effect


bits 32
section .text
align 64

global effect_setpalette_asm
global effect_6tap_render_32
global effect_6tap_render_16
global effect_6tap_addline_32
global effect_6tap_addline_16
global effect_scan2_16_16
global effect_scan2_16_16_direct
global effect_scan2_16_32
global effect_scan2_32_32_direct

; these are defined in effect.c
extern _6tap2x_buf0
extern _6tap2x_buf1
extern _6tap2x_buf2
extern _6tap2x_buf3
extern _6tap2x_buf4
extern _6tap2x_buf5

;--------------------------------------------------------
;extern void effect_setpalette_asm(void *pvPalette);
effect_setpalette_asm:
  push eax
  mov eax, [esp+8]
  mov [pLookup], eax
  pop eax
  ret

;--------------------------------------------------------
;extern void effect_6tap_addline_32(const void *src0, unsigned count);
effect_6tap_addline_32:
  push ebp
  mov ebp, esp
  pushad

  ; first, move all of the previous lines up
  mov eax, [_6tap2x_buf0]
  mov ebx, [_6tap2x_buf1]
  mov [_6tap2x_buf0], ebx
  mov ebx, [_6tap2x_buf2]
  mov [_6tap2x_buf1], ebx
  mov ebx, [_6tap2x_buf3]
  mov [_6tap2x_buf2], ebx
  mov ebx, [_6tap2x_buf4]
  mov [_6tap2x_buf3], ebx
  mov ebx, [_6tap2x_buf5]
  mov [_6tap2x_buf4], ebx
  mov [_6tap2x_buf5], eax
  
  ; check to see if we have a new line or if we should just clear it
  mov esi, [ebp+8]
  test esi, esi
  jne direct_6tap_n1
  
  ; no new line (we are at bottom of image), so just clear the last line
  xor eax, eax
  mov ecx, [ebp+12]			; count
  shl ecx, 1
  mov edi, [_6tap2x_buf5]
  rep stosd
  jmp direct_6tap_done  

  ; we have a new line, so let's horizontally filter it
direct_6tap_n1:
  mov edi, [_6tap2x_buf5]
  ; just replicate the first two pixels
  mov eax, [esi]
  mov ebx, [esi+4]
  add esi, 8
  mov [edi], eax
  mov [edi+4], eax
  mov [edi+8], ebx
  mov [edi+12], ebx
  add edi, 16
  ; now start the main loop  
  mov ecx, [ebp+12]
  sub ecx, 5
  pxor mm7, mm7
  movq mm6, [QW_6tapAdd]
direct_6tap_loop1:
  movd mm0, [esi]
  movd [edi], mm0
  punpcklbw mm0, mm7
  movd mm1, [esi+4]
  punpcklbw mm1, mm7
  movd mm2, [esi-4]
  punpcklbw mm2, mm7
  movd mm3, [esi+8]
  punpcklbw mm3, mm7
  movd mm4, [esi-8]
  punpcklbw mm4, mm7
  movd mm5, [esi+12]
  punpcklbw mm5, mm7
  paddw mm0, mm1
  paddw mm2, mm3
  psllw mm0, 2
  paddw mm4, mm5
  psubw mm0, mm2
  movq mm5, mm0
  psllw mm0, 2
  add esi, 4
  paddw mm0, mm5
  paddw mm4, mm6
  paddw mm0, mm4
  psraw mm0, 5
  packuswb mm0, mm0
  movd [edi+4], mm0
  add edi, 8
  sub ecx, 1
  jne direct_6tap_loop1
  ; finally, replicate the last three pixels
  mov eax, [esi]
  mov ebx, [esi+4]
  mov ecx, [esi+8]
  mov [edi], eax
  mov [edi+4], eax
  mov [edi+8], ebx
  mov [edi+12], ebx
  mov [edi+16], ecx
  mov [edi+20], ecx

direct_6tap_done:
  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;extern void effect_6tap_addline_16(const void *src0, unsigned count);
effect_6tap_addline_16:
  push ebp
  mov ebp, esp
  pushad

  ; first, move all of the previous lines up
  mov eax, [_6tap2x_buf0]
  mov ebx, [_6tap2x_buf1]
  mov [_6tap2x_buf0], ebx
  mov ebx, [_6tap2x_buf2]
  mov [_6tap2x_buf1], ebx
  mov ebx, [_6tap2x_buf3]
  mov [_6tap2x_buf2], ebx
  mov ebx, [_6tap2x_buf4]
  mov [_6tap2x_buf3], ebx
  mov ebx, [_6tap2x_buf5]
  mov [_6tap2x_buf4], ebx
  mov [_6tap2x_buf5], eax

  ; check to see if we have a new line or if we should just clear it
  mov esi, [ebp+8]
  test esi, esi
  jne indirect_6tap_n1
  
  ; no new line (we are at bottom of image), so just clear the last line
  mov edx, [ebp+12]			; count
  shl edx, 1
  xor eax, eax
  mov ecx, edx
  mov edi, [_6tap2x_buf5]
  rep stosd
  jmp indirect_6tap_done

  ; we have a new line, so first we need to do the palette lookup
indirect_6tap_n1:
  push ebp
  mov ecx, [ebp+12]	;count
  xor edi, edi     	;index
  add ecx, 3
  mov ebp, [pLookup]	;lookup
  shr ecx, 2
  mov [uCount], ecx
tap6_lookup_loop:
  movzx eax, word [esi+edi*2]
  movzx ebx, word [esi+edi*2+2]
  movzx ecx, word [esi+edi*2+4]
  movzx edx, word [esi+edi*2+6]
  mov eax, [ebp+eax*4]
  mov ebx, [ebp+ebx*4]
  mov ecx, [ebp+ecx*4]
  mov edx, [ebp+edx*4]
  mov [PixLine+edi*4], eax
  mov [PixLine+edi*4+4], ebx
  mov [PixLine+edi*4+8], ecx
  mov [PixLine+edi*4+12], edx
  add edi, 4
  sub dword [uCount], 1
  jne tap6_lookup_loop
  pop ebp

  ; now let's horizontally filter it
  mov esi, PixLine
  mov edi, [_6tap2x_buf5]
  ; just replicate the first two pixels
  mov eax, [esi]
  mov ebx, [esi+4]
  add esi, 8
  mov [edi], eax
  mov [edi+4], eax
  mov [edi+8], ebx
  mov [edi+12], ebx
  add edi, 16
  ; now start the main loop  
  mov ecx, [ebp+12]
  sub ecx, 5
  pxor mm7, mm7
  movq mm6, [QW_6tapAdd]
indirect_6tap_loop1:
  movd mm0, [esi]
  movd [edi], mm0
  punpcklbw mm0, mm7
  movd mm1, [esi+4]
  punpcklbw mm1, mm7
  movd mm2, [esi-4]
  punpcklbw mm2, mm7
  movd mm3, [esi+8]
  punpcklbw mm3, mm7
  movd mm4, [esi-8]
  punpcklbw mm4, mm7
  movd mm5, [esi+12]
  punpcklbw mm5, mm7
  paddw mm0, mm1
  paddw mm2, mm3
  psllw mm0, 2
  paddw mm4, mm5
  psubw mm0, mm2
  movq mm5, mm0
  psllw mm0, 2
  add esi, 4
  paddw mm0, mm5
  paddw mm4, mm6
  paddw mm0, mm4
  psraw mm0, 5
  packuswb mm0, mm0
  movd [edi+4], mm0
  add edi, 8
  sub ecx, 1
  jne indirect_6tap_loop1
  ; finally, replicate the last three pixels
  mov eax, [esi]
  mov ebx, [esi+4]
  mov ecx, [esi+8]
  mov [edi], eax
  mov [edi+4], eax
  mov [edi+8], ebx
  mov [edi+12], ebx
  mov [edi+16], ecx
  mov [edi+20], ecx

indirect_6tap_done:
  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;extern void effect_6tap_32(void *dst0, void *dst1, unsigned count);
effect_6tap_render_32:
  push ebp
  mov ebp, esp
  pushad

  ; first we need to just copy the 3rd line into the first destination line
  mov ecx, [ebp+16]
  mov esi, [_6tap2x_buf2]
  mov edi, [ebp+8]
  shl ecx, 1
  rep movsd

  ; now we need to vertically filter for the second line
  mov ecx, [ebp+16]			; count
  push ebp
  shl ecx, 1
  mov ebp, [ebp+12]			; dst1
  mov [uCount], ecx
  pxor mm7, mm7
  movq mm6, [QW_6tapAdd]
  ; load the index registers
  mov eax, [_6tap2x_buf0]
  mov ebx, [_6tap2x_buf1]
  mov ecx, [_6tap2x_buf2]
  mov edx, [_6tap2x_buf3]
  mov esi, [_6tap2x_buf4]
  mov edi, [_6tap2x_buf5]
VFilter_6tap_loop1:
  movd mm0, [ecx]
  add ecx, 4
  punpcklbw mm0, mm7
  movd mm1, [edx]
  add edx, 4
  punpcklbw mm1, mm7
  movd mm2, [ebx]
  add ebx, 4
  punpcklbw mm2, mm7
  movd mm3, [esi]
  add esi, 4
  punpcklbw mm3, mm7
  movd mm4, [eax]
  add eax, 4
  punpcklbw mm4, mm7
  movd mm5, [edi]
  add edi, 4
  punpcklbw mm5, mm7
  paddw mm0, mm1
  paddw mm2, mm3
  psllw mm0, 2
  paddw mm4, mm5
  psubw mm0, mm2
  paddw mm4, mm6
  movq mm5, mm0
  psllw mm0, 2
  paddw mm0, mm5
  paddw mm0, mm4
  movq mm1, mm0
  psraw mm0, 5
  psraw mm1, 7
  psubw mm0, mm1
  packuswb mm0, mm0
  movd [ebp], mm0
  add ebp, 4
  sub dword [uCount], 1
  jne VFilter_6tap_loop1
  pop ebp

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;extern void effect_6tap_16(void *dst0, void *dst1, unsigned count);
effect_6tap_render_16:
  push ebp
  mov ebp, esp
  pushad

  ; first we need to just copy the 3rd line into the first destination line
  mov ecx, [ebp+16]			; count
  mov esi, [_6tap2x_buf2]
  mov edi, [ebp+8]			; dst0
  shl ecx, 1
  call ConvertPix32To16

  ; now we need to vertically filter for the second line
  ; but we have to store it in a temporary buffer because it's 32 bits
  mov ecx, [ebp+16]			; count
  push ebp
  shl ecx, 1
  mov ebp, PixLine
  mov [uCount], ecx
  pxor mm7, mm7
  movq mm6, [QW_6tapAdd]
  ; load the index registers
  mov eax, [_6tap2x_buf0]
  mov ebx, [_6tap2x_buf1]
  mov ecx, [_6tap2x_buf2]
  mov edx, [_6tap2x_buf3]
  mov esi, [_6tap2x_buf4]
  mov edi, [_6tap2x_buf5]
VFilter_6tap_16_loop1:
  movd mm0, [ecx]
  add ecx, 4
  punpcklbw mm0, mm7
  movd mm1, [edx]
  add edx, 4
  punpcklbw mm1, mm7
  movd mm2, [ebx]
  add ebx, 4
  punpcklbw mm2, mm7
  movd mm3, [esi]
  add esi, 4
  punpcklbw mm3, mm7
  movd mm4, [eax]
  add eax, 4
  punpcklbw mm4, mm7
  movd mm5, [edi]
  add edi, 4
  punpcklbw mm5, mm7
  paddw mm0, mm1
  paddw mm2, mm3
  psllw mm0, 2
  paddw mm4, mm5
  psubw mm0, mm2
  paddw mm4, mm6
  movq mm5, mm0
  psllw mm0, 2
  paddw mm0, mm5
  paddw mm0, mm4
  movq mm1, mm0
  psraw mm0, 5
  psraw mm1, 7
  psubw mm0, mm1
  packuswb mm0, mm0
  movd [ebp], mm0
  add ebp, 4
  sub dword [uCount], 1
  jne VFilter_6tap_16_loop1
  pop ebp

  ; now convert the filtered pixels from 32 bits to 16
  mov ecx, [ebp+16]			; count
  mov esi, PixLine
  shl ecx, 1
  mov edi, [ebp+12]			; dst1
  call ConvertPix32To16
  
  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
; IN:  esi == source  edi == destination  ecx == count
; OUT: trashed eax, ebx, ecx, edx, esi, edi
ConvertPix32To16:
  ; the idea here is to do 2 pixels at once
  push ebp
  mov ebp, ecx

cp16loop1:
  mov eax, [esi]
  mov ecx, [esi+4]
  add esi, 8
  mov ebx, eax
  mov edx, ecx
  shr ah, 2
  shr ch, 2
  and ebx, 0f80000h
  and edx, 0f80000h
  shr ax, 3
  shr cx, 3
  shr ebx, 8
  shr edx, 8
  or eax, ebx
  or ecx, edx
  mov [edi], ax
  mov [edi+2], cx
  add edi, 4
  sub ebp, 2
  jg cp16loop1

  pop ebp
  ret
  
;--------------------------------------------------------
;void effect_scan2_16_16 (void *dst0, void *dst1, const void *src, 
;                         unsigned count,
;			  struct sysdep_palette_struct *palette)
effect_scan2_16_16:
  push ebp
  mov ebp, esp
  pushad

  ; first, do the table lookup
  push ebp
  mov ecx, [ebp+20]	;count
  xor edi, edi     	;index
  mov esi, [ebp+16]	;src0
  add ecx, 3
  mov ebp, [pLookup]	;lookup
  shr ecx, 2
  mov [uCount], ecx
scan2_16_lookup_loop:
  movzx eax, word [esi+edi*2]
  movzx ebx, word [esi+edi*2+2]
  movzx ecx, word [esi+edi*2+4]
  movzx edx, word [esi+edi*2+6]
  mov eax, [ebp+eax*4]
  mov ebx, [ebp+ebx*4]
  mov ecx, [ebp+ecx*4]
  mov edx, [ebp+edx*4]
  mov [PixLine+edi*2], ax
  mov [PixLine+edi*2+2], bx
  mov [PixLine+edi*2+4], cx
  mov [PixLine+edi*2+6], dx
  add edi, 4
  sub dword [uCount], 1
  jne scan2_16_lookup_loop
  pop ebp

  ; now do the shading, 8 pixels at a time
  mov edi, [ebp+8]	;dst0
  mov edx, [ebp+12]	;dst1
  mov esi, PixLine
  mov ecx, [ebp+20]	;count
  and edi, 0fffffff8h	;align destination
  add ecx, 7
  and edx, 0fffffff8h	;align destination
  shr ecx, 3
  movq mm7, [QW_16QuartMask]
scan2_16_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  punpcklwd mm0, mm0
  punpckhwd mm1, mm1
  punpcklwd mm2, mm2
  punpckhwd mm3, mm3
  movq mm4, mm0
  movq mm5, mm1
  movq [edi], mm0
  psrlw mm4, 2
  movq [edi+8], mm1
  psrlw mm5, 2
  movq [edi+16], mm2
  pand mm4, mm7
  movq [edi+24], mm3
  pand mm5, mm7
  psubw mm0, mm4
  movq mm4, mm2
  psubw mm1, mm5
  movq mm5, mm3
  movq [edx], mm0
  psrlw mm4, 2
  movq [edx+8], mm1
  psrlw mm5, 2
  pand mm4, mm7
  pand mm5, mm7
  psubw mm2, mm4
  psubw mm3, mm5
  add esi, 16
  movq [edx+16], mm2
  add edi, 32
  movq [edx+24], mm3
  add edx, 32
  sub ecx, 1
  jne scan2_16_shade_loop

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;void effect_scan2_16_16_direct (void *dst0, void *dst1,
;                                const void *src, unsigned count,
;				 struct sysdep_palette_struct *palette)
;
effect_scan2_16_16_direct
  push ebp
  mov ebp, esp
  pushad

  ; now do the shading, 8 pixels at a time
  mov edi, [ebp+8]	;dst0
  mov edx, [ebp+12]	;dst1
  mov esi, [ebp+16]	;src0
  mov ecx, [ebp+20]	;count
  and edi, 0fffffff8h	;align destination
  add ecx, 7
  and edx, 0fffffff8h	;align destination
  shr ecx, 3
  movq mm7, [QW_16QuartMask]
scan2_16_direct_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  punpcklwd mm0, mm0
  punpckhwd mm1, mm1
  punpcklwd mm2, mm2
  punpckhwd mm3, mm3
  movq mm4, mm0
  movq mm5, mm1
  movq [edi], mm0
  psrlw mm4, 2
  movq [edi+8], mm1
  psrlw mm5, 2
  movq [edi+16], mm2
  pand mm4, mm7
  movq [edi+24], mm3
  pand mm5, mm7
  psubw mm0, mm4
  movq mm4, mm2
  psubw mm1, mm5
  movq mm5, mm3
  movq [edx], mm0
  psrlw mm4, 2
  movq [edx+8], mm1
  psrlw mm5, 2
  pand mm4, mm7
  pand mm5, mm7
  psubw mm2, mm4
  psubw mm3, mm5
  add esi, 16
  movq [edx+16], mm2
  add edi, 32
  movq [edx+24], mm3
  add edx, 32
  sub ecx, 1
  jne scan2_16_direct_shade_loop

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;void effect_scan2_16_32 (void *dst0, void *dst1,
;                         const void *src, unsigned count
;			  struct sysdep_palette_struct *palette)
;
effect_scan2_16_32:
  push ebp
  mov ebp, esp
  pushad

  ; first, do the table lookup
  push ebp
  mov ecx, [ebp+20]	;count
  xor edi, edi     	;index
  mov esi, [ebp+16]	;src0
  add ecx, 3
  mov ebp, [pLookup]	;lookup
  shr ecx, 2
  mov [uCount], ecx
scan2_lookup_loop:
  movzx eax, word [esi+edi*2]
  movzx ebx, word [esi+edi*2+2]
  movzx ecx, word [esi+edi*2+4]
  movzx edx, word [esi+edi*2+6]
  mov eax, [ebp+eax*4]
  mov ebx, [ebp+ebx*4]
  mov ecx, [ebp+ecx*4]
  mov edx, [ebp+edx*4]
  mov [PixLine+edi*4], eax
  mov [PixLine+edi*4+4], ebx
  mov [PixLine+edi*4+8], ecx
  mov [PixLine+edi*4+12], edx
  add edi, 4
  sub dword [uCount], 1
  jne scan2_lookup_loop
  pop ebp

  ; now do the shading, 8 pixels at a time
  mov edi, [ebp+8]	;dst0
  mov edx, [ebp+12]	;dst1
  mov esi, PixLine
  mov ecx, [ebp+20]	;count
  and edi, 0fffffff8h	;align destination
  add ecx, 3
  and edx, 0fffffff8h	;align destination
  shr ecx, 2
scan2_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  punpckldq mm0, mm0
  punpckhdq mm1, mm1
  movq [edi], mm0
  punpckldq mm2, mm2
  movq [edi+8], mm1
  punpckhdq mm3, mm3
  movq [edi+16], mm2
  movq mm4, mm0
  movq [edi+24], mm3
  movq mm5, mm1
  psrlq mm0, 2
  movq mm6, mm2
  psrlq mm1, 2
  pand mm0, [QW_32QuartMask]
  movq mm7, mm3
  psrlq mm2, 2
  psubw mm4, mm0
  movq mm0, [QW_32QuartMask]
  psrlq mm3, 2
  pand mm1, mm0
  movq [edx], mm4
  pand mm2, mm0
  psubw mm5, mm1
  pand mm3, mm0
  movq [edx+8], mm5
  psubw mm6, mm2
  psubw mm7, mm3
  movq [edx+16], mm6
  add esi, 16
  movq [edx+24], mm7
  add edi, 32
  add edx, 32
  sub ecx, 1
  jne scan2_shade_loop

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;void effect_scan2_32_32_direct(void *dst0, void *dst1, 
;                               const void *src, unsigned count
;				struct sysdep_palette_struct *palette)
;
effect_scan2_32_32_direct:
  push ebp
  mov ebp, esp
  pushad

  mov edi, [ebp+8]	;dst0
  mov edx, [ebp+12]	;dst1
  mov esi, [ebp+16]	;src0
  mov ecx, [ebp+20]	;count
  and edi, 0fffffff8h	;align destination
  add ecx, 3
  and edx, 0fffffff8h	;align destination
  shr ecx, 2
scan2_direct_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  punpckldq mm0, mm0
  punpckhdq mm1, mm1
  movq [edi], mm0
  punpckldq mm2, mm2
  movq [edi+8], mm1
  punpckhdq mm3, mm3
  movq [edi+16], mm2
  movq mm4, mm0
  movq [edi+24], mm3
  movq mm5, mm1
  psrlq mm0, 2
  movq mm6, mm2
  psrlq mm1, 2
  pand mm0, [QW_32QuartMask]
  movq mm7, mm3
  psrlq mm2, 2
  psubw mm4, mm0
  movq mm0, [QW_32QuartMask]
  psrlq mm3, 2
  pand mm1, mm0
  movq [edx], mm4
  pand mm2, mm0
  psubw mm5, mm1
  pand mm3, mm0
  movq [edx+8], mm5
  psubw mm6, mm2
  psubw mm7, mm3
  movq [edx+16], mm6
  add esi, 16
  movq [edx+24], mm7
  add edi, 32
  add edx, 32
  sub ecx, 1
  jne scan2_direct_shade_loop

  popad
  pop ebp
  emms
  ret

;____________________________________________________________________________
; Data_Block:
section .data
align 16

; memory pointers to various buffers
pLookup		dd	0

; global variables
uCount		dd	0

; MMX constants
align 32
QW_6tapAdd	dd	000100010h, 000000010h
QW_32QuartMask	dd	03f3f3f3fh, 03f3f3f3fh
QW_16QuartMask	dd	039e739e7h, 039e739e7h	; 0011 1001 1110 0111

;____________________________________________________________________________
; Uninitialized data
section .bss
align 64

PixLine		resd    4096

end

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
;  2004-07-24:
;   - initial version, including the 6-tap sinc filter, and the scanline effect


bits 32
section .text
align 64

global effect_6tap_clear_asm
global effect_6tap_render_32
global effect_6tap_addline_32_32_direct
global effect_6tap_addline_16_32
global effect_scan2_16_16
global effect_scan2_16_16_direct
global effect_scan2_16_32
global effect_scan2_32_32_direct

;--------------------------------------------------------
;extern void effect_6tap_clear(unsigned count);
effect_6tap_clear_asm:
  push eax
  push ecx
  push edi

  xor eax, eax
  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated1
  rep stosd

  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated2
  rep stosd

  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated3
  rep stosd

  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated4
  rep stosd

  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated5
  rep stosd

  mov ecx, [esp+16]			; count
  shl ecx, 1
  mov edi, Translated6
  rep stosd

  pop edi
  pop ecx
  pop eax
  ret

;--------------------------------------------------------
;extern void effect_6tap_addline_32_32_direct(const void *src0, unsigned count);
effect_6tap_addline_32_32_direct:
  push ebp
  mov ebp, esp
  pushad

  ; first, move all of the previous lines up
  mov edx, [ebp+12]			; count
  shl edx, 1
  mov ecx, edx
  mov esi, Translated2
  mov edi, Translated1
  rep movsd
  mov ecx, edx
  mov esi, Translated3
  mov edi, Translated2
  rep movsd
  mov ecx, edx
  mov esi, Translated4
  mov edi, Translated3
  rep movsd
  mov ecx, edx
  mov esi, Translated5
  mov edi, Translated4
  rep movsd
  mov ecx, edx
  mov esi, Translated6
  mov edi, Translated5
  rep movsd

  ; check to see if we have a new line or if we should just clear it
  mov esi, [ebp+8]
  test esi, esi
  jne direct_6tap_n1
  
  ; no new line (we are at bottom of image), so just clear the last line
  xor eax, eax
  mov ecx, edx
  mov edi, Translated6
  rep stosd
  jmp direct_6tap_done  

  ; we have a new line, so let's horizontally filter it
direct_6tap_n1:
  mov edi, Translated6
  mov ecx, [ebp+12]
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

direct_6tap_done:
  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;extern void effect_6tap_addline_16_32(const void *src0, unsigned count,
;                                      const void *lookup);
effect_6tap_addline_16_32:
  push ebp
  mov ebp, esp
  pushad

  ; first, move all of the previous lines up
  mov edx, [ebp+12]			; count
  shl edx, 1
  mov ecx, edx
  mov esi, Translated2
  mov edi, Translated1
  rep movsd
  mov ecx, edx
  mov esi, Translated3
  mov edi, Translated2
  rep movsd
  mov ecx, edx
  mov esi, Translated4
  mov edi, Translated3
  rep movsd
  mov ecx, edx
  mov esi, Translated5
  mov edi, Translated4
  rep movsd
  mov ecx, edx
  mov esi, Translated6
  mov edi, Translated5
  rep movsd

  ; check to see if we have a new line or if we should just clear it
  mov esi, [ebp+8]
  test esi, esi
  jne indirect_6tap_n1
  
  ; no new line (we are at bottom of image), so just clear the last line
  xor eax, eax
  mov ecx, edx
  mov edi, Translated6
  rep stosd
  jmp indirect_6tap_done

  ; we have a new line, so first we need to do the palette lookup
indirect_6tap_n1:
  push ebp
  mov ecx, [ebp+12]	;count
  xor edi, edi     	;index
  add ecx, 3
  mov ebp, [ebp+16]	;lookup
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
  mov edi, Translated6
  mov ecx, [ebp+12]
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
  mov esi, Translated3
  mov edi, [ebp+8]
  shl ecx, 1
  rep movsd

  ; now we need to vertically filter for the second line
  mov ecx, [ebp+16]
  mov edi, [ebp+12]
  xor esi, esi			; index
  shl ecx, 1
  pxor mm7, mm7
  movq mm6, [QW_6tapAdd]
VFilter_6tap_loop1:
  movd mm0, [Translated3+esi*4]
  punpcklbw mm0, mm7
  movd mm1, [Translated4+esi*4]
  punpcklbw mm1, mm7
  movd mm2, [Translated2+esi*4]
  punpcklbw mm2, mm7
  movd mm3, [Translated5+esi*4]
  punpcklbw mm3, mm7
  movd mm4, [Translated1+esi*4]
  punpcklbw mm4, mm7
  movd mm5, [Translated6+esi*4]
  punpcklbw mm5, mm7
  paddw mm0, mm1
  paddw mm2, mm3
  psllw mm0, 2
  paddw mm4, mm5
  psubw mm0, mm2
  paddw mm4, mm6
  movq mm5, mm0
  psllw mm0, 2
  add esi, 1
  paddw mm0, mm5
  paddw mm0, mm4
  movq mm1, mm0
  psraw mm0, 5
  psraw mm1, 7
  psubw mm0, mm1
  packuswb mm0, mm0
  movd [edi], mm0
  add edi, 4
  sub ecx, 1
  jne VFilter_6tap_loop1

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;void effect_scan2_16_16 (void *dst0, void *dst1, const void *src, 
;                         unsigned count, const void *lookup)
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
  mov ebp, [ebp+24]	;lookup
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
;                                const void *src, unsigned count)
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
;void effect_scan2_16_16 (void *dst0, void *dst1,
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
  mov ebp, [ebp+24]	;lookup
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
  add ecx, 7
  and edx, 0fffffff8h	;align destination
  shr ecx, 3
scan2_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  movq mm4, [esi+16]
  movq mm5, mm4
  movq mm6, [esi+24]
  movq mm7, mm6
  punpckldq mm0, mm0
  punpckhdq mm1, mm1
  movq [edi], mm0
  punpckldq mm2, mm2
  movq [edi+8], mm1
  punpckhdq mm3, mm3
  movq [edi+16], mm2
  punpckldq mm4, mm4
  movq [edi+24], mm3
  punpckhdq mm5, mm5
  movq [edi+32], mm4
  punpckldq mm6, mm6
  movq [edi+40], mm5
  punpckhdq mm7, mm7
  movq [edi+48], mm6
  psrlq mm0, 1
  movq [edi+56], mm7
  psrlq mm1, 1
  pand mm0, [QW_32HalfMask]
  psrlq mm2, 1
  movq [edx], mm0
  psrlq mm3, 1
  movq mm0, [QW_32HalfMask]
  psrlq mm4, 1
  pand mm1, mm0
  pand mm2, mm0
  movq [edx+8], mm1
  pand mm3, mm0
  psrlq mm5, 1
  movq [edx+16], mm2
  pand mm4, mm0
  psrlq mm6, 1
  movq [edx+24], mm3
  pand mm5, mm0
  psrlq mm7, 1
  movq [edx+32], mm4
  pand mm6, mm0
  movq [edx+40], mm5
  pand mm7, mm0
  movq [edx+48], mm6
  add esi, 32
  movq [edx+56], mm7
  add edi, 64
  add edx, 64
  sub ecx, 1
  jne scan2_shade_loop

  popad
  pop ebp
  emms
  ret

;--------------------------------------------------------
;void effect_scan2_16_16_direct(void *dst0, void *dst1, 
;                         const void *src, unsigned count)
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
  add ecx, 7
  and edx, 0fffffff8h	;align destination
  shr ecx, 3
scan2_direct_shade_loop:
  movq mm0, [esi]
  movq mm1, mm0
  movq mm2, [esi+8]
  movq mm3, mm2
  movq mm4, [esi+16]
  movq mm5, mm4
  movq mm6, [esi+24]
  movq mm7, mm6
  punpckldq mm0, mm0
  punpckhdq mm1, mm1
  movq [edi], mm0
  punpckldq mm2, mm2
  movq [edi+8], mm1
  punpckhdq mm3, mm3
  movq [edi+16], mm2
  punpckldq mm4, mm4
  movq [edi+24], mm3
  punpckhdq mm5, mm5
  movq [edi+32], mm4
  punpckldq mm6, mm6
  movq [edi+40], mm5
  punpckhdq mm7, mm7
  movq [edi+48], mm6
  psrlq mm0, 1
  movq [edi+56], mm7
  psrlq mm1, 1
  pand mm0, [QW_32HalfMask]
  psrlq mm2, 1
  movq [edx], mm0
  psrlq mm3, 1
  movq mm0, [QW_32HalfMask]
  psrlq mm4, 1
  pand mm1, mm0
  pand mm2, mm0
  movq [edx+8], mm1
  pand mm3, mm0
  psrlq mm5, 1
  movq [edx+16], mm2
  pand mm4, mm0
  psrlq mm6, 1
  movq [edx+24], mm3
  pand mm5, mm0
  psrlq mm7, 1
  movq [edx+32], mm4
  pand mm6, mm0
  movq [edx+40], mm5
  pand mm7, mm0
  movq [edx+48], mm6
  add esi, 32
  movq [edx+56], mm7
  add edi, 64
  add edx, 64
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
pU32dst0        dd      0
pU32dst1        dd      0
pU32src0        dd      0
pU32src1        dd      0
pU32src2        dd      0
pLookup		dd	0

; global variables
uCount		dd	0

; MMX constants
align 32
QW_6tapAdd	dd	000100010h, 000000010h
QW_32HalfMask	dd	07f7f7f7fh, 07f7f7f7fh
QW_16HalfMask	dd	07bef7befh, 07bef7befh  ; 0111 1011 1110 1111
QW_16QuartMask	dd	039e739e7h, 039e739e7h	; 0011 1001 1110 0111
QW_16EighthMask	dd	018e318e3h, 018e318e3h	; 0001 1000 1110 0011

;____________________________________________________________________________
; Uninitialized data
section .bss
align 64

PixLine		resd    2048
Translated1	resd	2048
Translated2	resd	2048
Translated3	resd	2048
Translated4	resd	2048
Translated5	resd	2048
Translated6	resd	2048

end

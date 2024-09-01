DEFAULT REL
SECTION .text

global ASM_PFX(DisableCache)
ASM_PFX(DisableCache):
  mov     rax, cr0
  bts     rax, 30
  btr     rax, 29
  mov     cr0, rax
  wbinvd
  ret

global ASM_PFX(EnableCache)
ASM_PFX(EnableCache):
  wbinvd
  mov     rax, cr0
  btr     rax, 29
  btr     rax, 30
  mov     cr0, rax
  ret

global ASM_PFX(ClearCache)
ASM_PFX(ClearCache):    
  call DisableCache
  call EnableCache
  ret
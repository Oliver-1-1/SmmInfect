#pragma once

#if defined(__GNUC__)
  #include <stdint.h>

  static inline UINT64 read_cr3(VOID) {
      UINT64 cr3;
      __asm__ __volatile__ (
          "mov %%cr3, %0" 
          : "=r" (cr3)
          :
          : "memory"
      );
      return cr3;
  }

  static inline UINT64 read_msr(UINT32 msr) {
      UINT32 low, high;
    
      __asm__ __volatile__ (
          "rdmsr"
          : "=a" (low), "=d" (high)
          : "c"  (msr)
          : "memory"
      );
    
      return ((UINT64)high << 32) | low;
  }

  #define READ_CR3()      read_cr3() 
  #define READ_MSR(msr)       read_msr(msr)

#elif defined(_MSC_VER)
  #include <intrin.h>

  #define READ_CR3()           __readcr3()
  #define READ_MSR()           __readmsr()
#endif

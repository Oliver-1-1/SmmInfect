#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <asm/io.h>

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);


extern long shellcode(long a, long b);

static void read_physical_memory(phys_addr_t phys_addr, uint8_t* buffer,  size_t size) {
    void *vaddr;

    vaddr = ioremap(phys_addr, size);
    if (!vaddr) {
        return;
    }
    memcpy(buffer, vaddr, size);
    iounmap(vaddr);
}

struct idt_desc {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));
uint64_t get_idt(void) {
    struct idt_desc idt;
    asm volatile ("sidt %0" : "=m"(idt)); 
    return idt.base;
}

typedef union _KIDTENTRY64
{
  union
  {
    struct
    {
      unsigned short OffsetLow;
      unsigned short Selector;
      struct 
      {
         unsigned short IstIndex : 3;
         unsigned short Reserved0 : 5;
         unsigned short Type : 5;
         unsigned short Dpl : 2;
         unsigned short Present : 1;
      };
       unsigned short OffsetMiddle;
       unsigned long OffsetHigh;
       unsigned long Reserved1;
    }; 
     uint64_t Alignment;
  };
} KIDTENTRY64, *PKIDTENTRY64;
typedef union
{
    struct
    {
        uint64_t reserved1 : 3;
        uint64_t page_level_write_through : 1;
        uint64_t page_level_cache_disable : 1;
        uint64_t reserved2 : 7;
        uint64_t address_of_page_directory : 36;
        uint64_t reserved3 : 16;
    };

    uint64_t flags;
} cr3;
typedef union
{
    struct
    {
        uint64_t present : 1;
        uint64_t write : 1;
        uint64_t supervisor : 1;
        uint64_t page_level_write_through : 1;
        uint64_t page_level_cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t reserved1 : 1;
        uint64_t must_be_zero : 1;
        uint64_t ignored_1 : 4;
        uint64_t page_frame_number : 36;
        uint64_t reserved2 : 4;
        uint64_t ignored_2 : 11;
        uint64_t execute_disable : 1;
    };

    uint64_t flags;
} pml4e_64;
uint64_t get_cr3(void)
{
    uint64_t _cr3;
    uint64_t _cr33;
    asm volatile("mov %%cr3, %%rax \n mov %%rax, %0" : "=m" (_cr33) :: "%rax");
    asm volatile("mov %%cr3, %0" : "=r"(_cr3));

    return _cr3;
}
typedef union
{
    struct
    {
        uint64_t present : 1;
        uint64_t write : 1;
        uint64_t supervisor : 1;
        uint64_t page_level_write_through : 1;
        uint64_t page_level_cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t reserved1 : 1;
        uint64_t large_page : 1;
        uint64_t ignored_1 : 4;
        uint64_t page_frame_number : 36;
        uint64_t reserved2 : 4;
        uint64_t ignored_2 : 11;
        uint64_t execute_disable : 1;
    };

    uint64_t flags;
} pdpte_64;

typedef union
{
    struct
    {
        uint64_t present : 1;
        uint64_t write : 1;
        uint64_t supervisor : 1;
        uint64_t page_level_write_through : 1;
        uint64_t page_level_cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t reserved1 : 1;
        uint64_t large_page : 1;
        uint64_t ignored_1 : 4;
        uint64_t page_frame_number : 36;
        uint64_t reserved2 : 4;
        uint64_t ignored_2 : 11;
        uint64_t execute_disable : 1;
    };

    uint64_t flags;
} pde_64;

typedef union
{
    struct
    {
        uint64_t present : 1;
        uint64_t write : 1;
        uint64_t supervisor : 1;
        uint64_t page_level_write_through : 1;
        uint64_t page_level_cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t dirty : 1;
        uint64_t pat : 1;
        uint64_t global : 1;
        uint64_t ignored_1 : 3;
        uint64_t page_frame_number : 36;
        uint64_t reserved1 : 4;
        uint64_t ignored_2 : 7;
        uint64_t protection_key : 4;
        uint64_t execute_disable : 1;
    };
    uint64_t flags;
} pte_64;
uint64_t TranslateVa(uint64_t addr){
      uint16_t pml4_idx = ((uint64_t)addr >> 39) & 0x1FF;
      uint16_t pdpt_idx = ((uint64_t)addr >> 30) & 0x1FF;
      uint16_t pd_idx   = ((uint64_t)addr >> 21) & 0x1FF;
      uint16_t pt_idx   = ((uint64_t)addr >> 12) & 0x1FF;
      uint16_t offset   = addr & 0xFFF;
      
      pml4e_64* PML4_Arr = (pml4e_64*)phys_to_virt(get_cr3() + 8 * pml4_idx);
      if(!PML4_Arr->present) return 0;
      pdpte_64* PDPTE_Arr = (pdpte_64*)phys_to_virt((PML4_Arr->page_frame_number << 12) + 8 * pdpt_idx);
      if(!PDPTE_Arr->present) return 0;
      pde_64* PDE_Arr = (pde_64*)phys_to_virt((PDPTE_Arr->page_frame_number << 12) + 8 * pd_idx);
      if(!PDE_Arr->present) return 0;
      
      if(PDE_Arr->large_page){
        PDE_Arr->write = 1;
        return ((uint64_t)(addr & 0x1FFFFF) + (*(uint64_t*)PDE_Arr & 0xFFFFFFFFF000));
        }
      
      pte_64* PTE_Arr = (pte_64*)phys_to_virt((PDE_Arr->page_frame_number << 12) + 8 * pt_idx);
      if(!PTE_Arr->present) return 0;
      PTE_Arr->write = 1;
      
      
      return (addr & 0xFFF) + (*(uint64_t*)PTE_Arr & 0xFFFFFFFFF000);
}

unsigned char bytepattern[] = { 0x8B, 0x04, 0x25, 0x69, 0x69, 0x69, 0x69, 0x48, 0x89, 0x45, 0xf0, 0x31,0xc0, 0x80, 0x3f, 0x00, 0xc7};
kallsyms_lookup_name_t get_kallsyms(unsigned long base)
{
	int i;
	int j;
	bool found = false;
	unsigned long kaddr = (unsigned long)base;

	for ( i = 0x0 ; i < 0x200000 ; i++ )
	{
		for(j = 0; j < sizeof(bytepattern); j++)
		{
		    if(bytepattern[j] == 0x69)
		    {
		        found = true;
		        continue;
		    }
		    unsigned char byte = 0;
		    read_physical_memory(virt_to_phys(kaddr + j), &byte, 1);
		    if(byte != bytepattern[j])
		    {
		        found = false;
		        break;
		    }
		    else
		    {
		        found = true;
		    }
	    
		}
		if(found == true)
		{
		  return kaddr - 0x10;
		}
		
		
		kaddr += 0x10;
	}

	return NULL;
}


void asm_replace(uint64_t base, uint64_t target, uint64_t value)
{
    for(uint32_t i = 0; i < 200; i++)
    {
      if(*(uint64_t*)(base+i) == target){
          *(uint64_t*)(base+i) = value;
          break;
        }
      
    }
}
const unsigned char* str = "_printk";
const unsigned char* str1 = "Zepta";
int main(void) {
    uint64_t idt_base = get_idt();
    PKIDTENTRY64 idt_entry = (PKIDTENTRY64)(idt_base + (0xE * 16));  // (#PF);
    uint64_t handler_addr = idt_entry->OffsetLow | (idt_entry->OffsetMiddle << 16) | (idt_entry->OffsetHigh << 32);

    // Align 2MB
    uint64_t kernel_entry = handler_addr & 0xFFFFFFFFFFF00000;
    kernel_entry -= 0x1400000;

    pr_info("Kernel_entry: 0x%llx\n", kernel_entry);
    kallsyms_lookup_name_t kallsyms = get_kallsyms(kernel_entry);

    //rw(add_numbers, 1);

    TranslateVa(shellcode);
    asm_replace(shellcode, 0x6969696969696969, str);
    asm_replace(shellcode, 0x6969696969696969, kallsyms);
    asm_replace(shellcode, 0x6969696969696969, str1);
    shellcode(1,1);

}
static int m_init(void)
{

  pr_info("module loaded\n");
  main();
  
  return 0;
}

static void m_exit(void)
{

  pr_info("module unloaded\n");
}

module_init(m_init);
module_exit(m_exit);

MODULE_LICENSE("GPL");


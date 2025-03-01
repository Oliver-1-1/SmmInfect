#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <asm/io.h>

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

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

// tested for kernel version: 6.8.0-52
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


void main(void) {
    uint64_t idt_base = get_idt();
    PKIDTENTRY64 idt_entry = (PKIDTENTRY64)(idt_base + (0xE * 16));  // (#PF);
    uint64_t handler_addr = idt_entry->OffsetLow | (idt_entry->OffsetMiddle << 16) | (idt_entry->OffsetHigh << 32);

    // Align 1MB
    uint64_t kernel_entry = handler_addr & 0xFFFFFFFFFFF00000;
    kernel_entry -= 0x1400000;

    pr_info("Kernel_entry: 0x%llx\n", kernel_entry);
    
    pr_info("Kallsyms_lookup_name: 0x%llx\n", get_kallsyms(kernel_entry));

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

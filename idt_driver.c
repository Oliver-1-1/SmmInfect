#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/init.h>

struct idt_desc {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));
uint64_t get_idt(void) {
    struct idt_desc idt;
    asm volatile ("sidt %0" : "=m"(idt)); 
    return idt.base;
}
int main(void) {
    uint64_t idt_base = get_idt();
    uint64_t *idt_entry = (uint64_t*)(idt_base + (0xE * 16));  // (#PF)
    uint64_t handler_addr = (idt_entry[0] & 0xFFFF) | ((idt_entry[0] >> 32) & 0xFFFF0000) | (idt_entry[1] & 0xFFFFFFFF00000000);
    pr_info("Interrupt handler: 0x%llx\n", handler_addr);

    // Align 1MB
    uint64_t kernel_entry = handler_addr & 0xFFFFFFFFFFF00000;
    pr_info("kernel_entry: 0x%llx\n", kernel_entry);
    
    // _text base = kernel_entry − 0x1400000;
    // Should be pretty easy to just search for kallsyms from here :)
    
    return 0;
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



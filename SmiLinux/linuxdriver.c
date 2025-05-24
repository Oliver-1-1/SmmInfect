#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/efi.h>

#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004

void smi_print(const char *fmt, unsigned long long val);
void trigger_smi(void);
extern void smi_linux(void);

void smi_print(const char *fmt, unsigned long long val)
{
  printk( pr_fmt(fmt), val);
}
EXPORT_SYMBOL(smi_print);

void trigger_smi(void)
{
  uint8_t data[8];
  efi_guid_t global_guid = {0x0};

  uint32_t attributes =  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;  
  if (!efi.get_variable) {
      pr_err("Efi get var NULL!\n");
      return;
  }  
  (void)efi.set_variable(L"ZeptaVar", &global_guid, attributes, sizeof(data), data);
}

static int m_init(void)
{
  pr_info("module loaded\n");
  smi_linux();
  return 0;
}

static void m_exit(void)
{
  pr_info("module unloaded\n");
}

module_init(m_init);
module_exit(m_exit);

MODULE_LICENSE("GPL");



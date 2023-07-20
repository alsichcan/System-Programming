#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>
#include <linux/pgtable.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;

struct packet {
        pid_t pid;
        unsigned long vaddr;
        unsigned long paddr;
};

static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
        // Implement read file operation
        struct packet *pckt;
        int ret;

        // Page Table
        pgd_t *pgd; // Top level page table entry
        p4d_t *p4d; // Next level page table entry
        pud_t *pud;
        pmd_t *pmd; 
        pte_t *pte; // Pate Table Entry

        pckt = (struct packet*) user_buffer;

        // Get pid of app & virtual address
        task = pid_task(find_get_pid(pckt -> pid), PIDTYPE_PID);

        // Get PHysical Address
        pgd = pgd_offset(task->mm, pckt->vaddr);
        p4d = p4d_offset(pgd, pckt->vaddr);
        pud = pud_offset(p4d, pckt->vaddr);
        pmd = pmd_offset(pud, pckt->vaddr);
        pte = pte_offset_kernel(pmd, pckt->vaddr);

        // Offset: 12bit
        pckt->paddr = (pte_val(*pte) & 0xFFFFFFFFFF000) | (pckt->vaddr & 0x0000000000000FFF);
        pte_unmap(pte);

        // Returns Physical address
        // int copy_to_user(void __user* to, const void* from, unsigned long n)
        ret = copy_to_user(user_buffer, pckt, sizeof(struct packet));
        
        return length;
}

static const struct file_operations dbfs_fops = {
        // Mapping file operations with your functions
        .read = read_output,
};

static int __init dbfs_module_init(void)
{
        // Implement init module

        dir = debugfs_create_dir("paddr", NULL);
        if (!dir) {
                printk("Cannot create paddr dir\n");
                return -1;
        }

        // Fill in the arguments below
        // struct dentry* debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops)
        // S_IRUSR: 읽기 권한
        output = debugfs_create_file("output", S_IRUSR, dir , NULL, &dbfs_fops);

	printk("dbfs_paddr module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module
        debugfs_remove_recursive(dir);

	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *inputdir, *ptreedir;
static struct task_struct *curr;

struct debugfs_blob_wrapper *blob;
static char *logs;

struct ptree_node{
        char* process_command;
        int process_id;
        struct ptree_node *next;
};

static void free_list(struct ptree_node* head){
        struct ptree_node *temp;
        while(head != NULL)
        {
                temp = head;
                head = head->next;
                kfree(temp);
        }
}

static ssize_t write_pid_to_input(struct file *fp, 
                                const char __user *user_buffer, 
                                size_t length, 
                                loff_t *position)
{
        pid_t input_pid;
        int size = 0;
        struct ptree_node *head;
        struct ptree_node *node;
        struct ptree_node *temp;

        // user_buffer에 담긴 pid를 읽어들이고 input_pid에 입력
        sscanf(user_buffer, "%u", &input_pid);
        
        // Find task_struct using input_pid. Hint: pid_task
        curr = pid_task(find_get_pid(input_pid), PIDTYPE_PID);

        // Tracing process tree from input_pid to init(1) process
        head = (struct ptree_node *) kmalloc(sizeof(struct ptree_node), GFP_KERNEL);

        while(1){
                // 새로운 ptree_node를 생성해 head에 삽입
                node = (struct ptree_node *) kmalloc(sizeof(struct ptree_node), GFP_KERNEL);

                node -> process_command = curr -> comm;
                node -> process_id = curr -> pid;
                node -> next = head -> next;
                head -> next = node;

                // curr 업데이트
                if(curr -> pid == 1) break;
                curr = curr -> parent;
        }

        // Make Output Format string: process_command (process_id)
        logs = kmalloc(sizeof(char) * 101010, GFP_KERNEL);

        temp = head -> next;

        while(temp != NULL){
                // snprintf: https://www.ibm.com/docs/ko/i/7.3?topic=functions-snprintf-print-formatted-data-buffer 참고
                size += snprintf(logs + size, 64, "%s (%d)\n", temp->process_command, temp->process_id);
                temp = temp -> next;
        }
        blob->data = logs;
        blob->size = (unsigned long)strlen(logs);

        free_list(head);

        return length;
}

static const struct file_operations dbfs_fops = {
        .write = write_pid_to_input,
};

static int __init dbfs_module_init(void)
{
        // Implement init module code

        dir = debugfs_create_dir("ptree", NULL);
        
        if (!dir) {
                printk("Cannot create ptree dir\n");
                return -1;
        }
        // struct dentry* debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops)
        // S_IWUSR: 쓰기 권한
        inputdir = debugfs_create_file("input", S_IWUSR, dir, NULL, &dbfs_fops);

        // Blob (Binary Large Object) 메모리 할당
        // kmalloc: kernel malloc
        blob = (struct debugfs_blob_wrapper *)kmalloc(sizeof(struct debugfs_blob_wrapper), GFP_KERNEL);

        // struct dentry* debufgs_create_blob(const char *name, umode_t mode, struct dentry *parent, struct debugfs_blob_wrapper *blob);
        // S_IRUSR: 읽기 권한
        ptreedir = debugfs_create_blob("ptree", S_IRUSR, dir, blob); // Find suitable debugfs API
	
	printk("dbfs_ptree module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module code
        
        // // Free kmalloc'd object
        kfree(logs);
        kfree(blob);

        // struct dentry* debufgs_remove_recursive(struct dentry *dentry)
        debugfs_remove_recursive(dir);
	
	printk("dbfs_ptree module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

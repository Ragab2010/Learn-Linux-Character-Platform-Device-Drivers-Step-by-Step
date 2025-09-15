#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>

MODULE_LICENSE("GPL");

static int print_init(void){
    dev_t device_number;
    char format_buffer[20];
    int format_buffer_len;
    device_number = MKDEV(160, 1);
    pr_info("print_init:called\n");
    pr_info("call format_dev_t \n");
    format_buffer_len=print_dev_t(format_buffer, device_number);
    format_buffer[format_buffer_len]=0;
    pr_info("the Device_number by print_dev_t is:%s\n" , format_buffer);
    //we use print_dev_t
    pr_info("the Device_number by format_dev_t  is:%s\n" , format_dev_t(format_buffer ,device_number));
    return 0;
}
static void print_exit(void){
    pr_info("print_exit:called\n");
}
module_init(print_init);
module_exit(print_exit);

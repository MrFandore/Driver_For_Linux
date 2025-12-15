#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/version.h>

#define DEVICE_NAME "mydriver"
#define CLASS_NAME "mydrv"
#define BUFFER_SIZE 1024
#define DEFAULT_TIMEOUT 5000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Simple Linux character driver");
MODULE_VERSION("1.0");

// Параметры модуля
static int debug = 0;
static int timeout_ms = DEFAULT_TIMEOUT;
static char *init_message = "Hello from driver!";

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-off, 1-on)");
module_param(timeout_ms, int, 0644);
MODULE_PARM_DESC(timeout_ms, "Timer timeout in milliseconds");
module_param(init_message, charp, 0644);
MODULE_PARM_DESC(init_message, "Initial driver message");

// Глобальные переменные
static int major_number;
static struct class* mydriver_class = NULL;
static struct device* mydriver_device = NULL;
static struct cdev mydriver_cdev;
static struct timer_list my_timer;

// Буфер данных
static char driver_buffer[BUFFER_SIZE];
static int buffer_used = 0;
static int read_count = 0;
static int write_count = 0;

// Прототипы функций
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

// Обработчик таймера
static void timer_callback(struct timer_list *t) {
    if (debug)
        printk(KERN_INFO "MYDRIVER: Timer triggered! Jiffies = %lu\n", jiffies);
    
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(timeout_ms));
}

// Структура file_operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};

// Чтение из устройства
static ssize_t dev_read(struct file *filp, char __user *buffer, 
                       size_t len, loff_t *offset) {
    int bytes_to_read;
    
    if (debug)
        printk(KERN_INFO "MYDRIVER: Reading: offset=%lld, len=%zu\n", *offset, len);
    
    if (*offset >= buffer_used)
        return 0;
    
    bytes_to_read = min((int)len, buffer_used - (int)*offset);
    
    if (copy_to_user(buffer, driver_buffer + *offset, bytes_to_read)) {
        printk(KERN_WARNING "MYDRIVER: Error in copy_to_user\n");
        return -EFAULT;
    }
    
    *offset += bytes_to_read;
    read_count++;
    
    if (debug)
        printk(KERN_INFO "MYDRIVER: Read %d bytes\n", bytes_to_read);
    
    return bytes_to_read;
}

// Запись в устройство
static ssize_t dev_write(struct file *filp, const char __user *buffer,
                        size_t len, loff_t *offset) {
    int bytes_to_write;
    
    if (debug)
        printk(KERN_INFO "MYDRIVER: Writing: offset=%lld, len=%zu\n", *offset, len);
    
    if (*offset >= BUFFER_SIZE)
        return -ENOSPC;
    
    bytes_to_write = min((int)len, BUFFER_SIZE - (int)*offset);
    
    if (copy_from_user(driver_buffer + *offset, buffer, bytes_to_write)) {
        printk(KERN_WARNING "MYDRIVER: Error in copy_from_user\n");
        return -EFAULT;
    }
    
    if (*offset + bytes_to_write > buffer_used)
        buffer_used = *offset + bytes_to_write;
    
    *offset += bytes_to_write;
    write_count++;
    
    if (debug)
        printk(KERN_INFO "MYDRIVER: Written %d bytes\n", bytes_to_write);
    
    return bytes_to_write;
}

// Открытие устройства
static int dev_open(struct inode *inodep, struct file *filep) {
    if (debug)
        printk(KERN_INFO "MYDRIVER: Device opened\n");
    return 0;
}

// Закрытие устройства
static int dev_release(struct inode *inodep, struct file *filep) {
    if (debug)
        printk(KERN_INFO "MYDRIVER: Device closed\n");
    return 0;
}

// IOCTL операции
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int size;
    int stats[3];
    
    switch(cmd) {
        case 0x01:  // GET_STATS
            if (debug)
                printk(KERN_INFO "MYDRIVER: IOCTL GET_STATS\n");
            
            stats[0] = read_count;
            stats[1] = write_count;
            stats[2] = buffer_used;
            
            if (copy_to_user((int __user *)arg, stats, sizeof(stats)))
                return -EFAULT;
            break;
            
        case 0x02:  // CLEAR_STATS
            if (debug)
                printk(KERN_INFO "MYDRIVER: IOCTL CLEAR_STATS\n");
            read_count = 0;
            write_count = 0;
            break;
            
        case 0x03:  // GET_BUFFER_SIZE
            if (debug)
                printk(KERN_INFO "MYDRIVER: IOCTL GET_BUFFER_SIZE\n");
            size = BUFFER_SIZE;
            if (copy_to_user((int __user *)arg, &size, sizeof(size)))
                return -EFAULT;
            break;
            
        default:
            printk(KERN_WARNING "MYDRIVER: Unknown IOCTL command: %u\n", cmd);
            return -EINVAL;
    }
    
    return 0;
}

// Инициализация драйвера
static int __init mydriver_init(void) {
    dev_t dev_num;
    int ret;
    
    printk(KERN_INFO "MYDRIVER: Initializing driver\n");
    printk(KERN_INFO "MYDRIVER: Parameters: debug=%d, timeout=%dms, message='%s'\n",
           debug, timeout_ms, init_message);
    
    // 1. Инициализация буфера
    strncpy(driver_buffer, init_message, BUFFER_SIZE - 1);
    driver_buffer[BUFFER_SIZE - 1] = '\0';
    buffer_used = strlen(init_message);
    
    // 2. Динамическое выделение major номера
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "MYDRIVER: Failed to allocate device number\n");
        return ret;
    }
    major_number = MAJOR(dev_num);
    
    // 3. Инициализация cdev
    cdev_init(&mydriver_cdev, &fops);
    mydriver_cdev.owner = THIS_MODULE;
    
    // 4. Добавление cdev в систему
    ret = cdev_add(&mydriver_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ALERT "MYDRIVER: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    // 5. Создание класса устройств - для ядра 6.x
    mydriver_class = class_create(CLASS_NAME);
    if (IS_ERR(mydriver_class)) {
        printk(KERN_ALERT "MYDRIVER: Failed to create class\n");
        cdev_del(&mydriver_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(mydriver_class);
    }
    
    // 6. Создание устройства в /dev
    mydriver_device = device_create(mydriver_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(mydriver_device)) {
        printk(KERN_ALERT "MYDRIVER: Failed to create device\n");
        class_destroy(mydriver_class);
        cdev_del(&mydriver_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(mydriver_device);
    }
    
    // 7. Инициализация таймера
    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(timeout_ms));
    
    printk(KERN_INFO "MYDRIVER: Driver successfully loaded! Major number = %d\n", major_number);
    printk(KERN_INFO "MYDRIVER: Device available as /dev/%s\n", DEVICE_NAME);
    
    return 0;
}

// Выгрузка драйвера
static void __exit mydriver_exit(void) {
    dev_t dev_num = MKDEV(major_number, 0);
    
    // Остановка таймера
    del_timer(&my_timer);
    
    // Удаление устройства и класса
    device_destroy(mydriver_class, dev_num);
    class_destroy(mydriver_class);
    
    // Удаление cdev
    cdev_del(&mydriver_cdev);
    
    // Освобождение номера устройства
    unregister_chrdev_region(dev_num, 1);
    
    printk(KERN_INFO "MYDRIVER: Statistics: reads=%d, writes=%d\n", read_count, write_count);
    printk(KERN_INFO "MYDRIVER: Driver unloaded\n");
}

module_init(mydriver_init);
module_exit(mydriver_exit);
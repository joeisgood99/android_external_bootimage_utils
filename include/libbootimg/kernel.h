#ifndef _f7d18500_9064_11e2_b0f1_5404a601fa9d
#define _f7d18500_9064_11e2_b0f1_5404a601fa9d 
#include <bootimage.h>
typedef struct kernel_image kernel_image;

#define KERNEL_COMPRESSION_GZIP 1
#define KERNEL_COMPRESSION_LZO 2
#define KERNEL_COMPRESSION_LZMA 3

#define KERNEL_ZIMAGE_MAGIC "\x18\x28\x6F\x01"
#define KERNEL_ZIMAGE_MAGIC_SIZE 4 

struct kernel_image {
    
    //
    unsigned char* start_addr ;
    unsigned char* ramdisk_addr ;
    unsigned char* config_addr ;
    
    
    char* version ;
    char* version_number ;
    int version_number_length ;
    
    unsigned compression_type;
    unsigned size;
    unsigned config_size;
    unsigned ramdisk_size;
    };

int load_kernel_image_from_memory(unsigned char* kernel_addr,unsigned kernel_size,kernel_image* image );
int print_kernel_info(kernel_image* kimage);
char *str_kernel_compression(int compression_type);
    

#endif
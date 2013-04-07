// internal program headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <utils.h>
#include <actions.h>
#include <bootimage.h>
#include <ramdisk.h>
#include <compression.h>
typedef struct create_action create_action;

struct create_action{
	
	char *		bootimage_filename	;
	char *  	header_filename 	;
	char *  	kernel_filename 	;
	char ** 	ramdisk_filenames 	;
	char *  	ramdisk_cpioname 	;
	char *  	ramdisk_imagename 	;
	unsigned char * ramdisk_directory 	;
	char *		second_filename		;
	unsigned char * output_directory 	;
	unsigned	ramdisk_filenames_count	;
};

int create_bootimage(create_action* action){
    
    // setup a new boot_image_struct to receive the new information
    errno = 0;
    boot_image bimage ;
    int ramdisk_processed = 0 ;
    unsigned char* ramdisk_data;
    
    // set the physical address defaults and other boot_image structure defaults
    set_boot_image_defaults(&bimage);
    if ( action->header_filename) {
	load_boot_image_header_from_disk(action->header_filename,&bimage);
    }
    
    if(action->kernel_filename){
        
	unsigned kernel_size = 0; 
	bimage.kernel_addr = read_item_from_disk(action->kernel_filename,&kernel_size);
	fprintf(stderr,"read_item_from_disk kernel:%d\n",errno) ;
	bimage.header->kernel_size = kernel_size;      

    }
    if(action->ramdisk_directory){
    
	unsigned cpio_ramdisk_size = 0; 
	
	unsigned char* cpio_data = pack_ramdisk_directory(action->ramdisk_directory,&cpio_ramdisk_size) ;
	if(cpio_data){
	    ramdisk_data = calloc(cpio_ramdisk_size,sizeof(char));
	    bimage.header->ramdisk_size = compress_gzip_memory(cpio_data,cpio_ramdisk_size,ramdisk_data,cpio_ramdisk_size);
	    bimage.ramdisk_addr = ramdisk_data;
	    
	    free(cpio_data);
	    fprintf(stderr,"ramdisk_directory:%s\n",action->ramdisk_directory);
	    ramdisk_processed = 1;
	}
    }
    
    if(action->ramdisk_cpioname && !ramdisk_processed){
	
	unsigned cpio_ramdisk_size = 0;     
	unsigned char* cpio_data = read_item_from_disk(action->ramdisk_cpioname,&cpio_ramdisk_size);
	
	ramdisk_data = calloc(cpio_ramdisk_size,sizeof(char));
	bimage.header->ramdisk_size = compress_gzip_memory(cpio_data,cpio_ramdisk_size,ramdisk_data,cpio_ramdisk_size);
	
	bimage.ramdisk_addr = ramdisk_data;
	
	free(cpio_data);
	ramdisk_processed = 1;
	
    }    
    if(action->ramdisk_imagename && !ramdisk_processed ){

	  bimage.ramdisk_addr =  read_item_from_disk(action->ramdisk_imagename,&bimage.header->ramdisk_size);
	  
    }
    
    if(action->second_filename){
	
	bimage.second_addr =  read_item_from_disk(action->second_filename,&bimage.header->second_size);
	if(bimage.header->second_size == 0 )  bimage.second_addr = NULL;
    }
    
    
    set_boot_image_padding(&bimage);
    set_boot_image_content_hash(&bimage);
    set_boot_image_offsets(&bimage);
    
    print_boot_image_info(&bimage);
        
   if(write_boot_image(action->bootimage_filename,&bimage)){
	fprintf(stderr,"write_boot_image failed %d %s\n",errno,strerror(errno));
    }
   
cleanup_bootimage:
    fprintf(stderr,"kernel_addr free\n");
    if(bimage.kernel_addr) free(bimage.kernel_addr);
    fprintf(stderr,"ramdisk_addr free\n");
    if(bimage.ramdisk_addr) free(bimage.ramdisk_addr);
    fprintf(stderr,"second_addr free\n");
    if(bimage.second_addr) free(bimage.second_addr);
    return errno;
}


// process_create_action - parse the command line switches
// although this code is repetitive we will favour readability
// over codesize ...... Ask me in 3 months time whether it was
// a good idea.
int process_create_action(int argc,char ** argv){
    // Initialize the action struct with NULL values
    create_action action;
    action.bootimage_filename 	= NULL 	;
    action.header_filename  	= NULL 	;
    action.kernel_filename  	= NULL 	;
    action.ramdisk_filenames  	= NULL 	;
    action.ramdisk_cpioname  	= NULL 	;
    action.ramdisk_imagename  	= NULL 	;
    action.ramdisk_directory  	= NULL 	;
    action.second_filename  	= NULL 	;
    action.output_directory  	= NULL 	;
    action.ramdisk_filenames_count	= 0	;
    

    FILE*file; int ramdisk_set = 0;
      
    while(argc > 0){
	    
	// check for a valid file name
	if(!action.bootimage_filename){
		
		//fclose(file);
		action.bootimage_filename = argv[0];
		fprintf(stderr,"action.bootimage_filename:%s\n",action.bootimage_filename);
		// set full extract if this is the last token 
		// or if the next token is NOT a switch. 
		
		if(argc == 1 || argv[1][0]!='-'){ 
		    fprintf(stderr,"extract all\n");
		    action.header_filename 	= "header";
		    action.kernel_filename 	= "kernel";
		    action.ramdisk_cpioname 	= "ramdisk.cpio";
		    action.ramdisk_imagename 	= "ramdisk.img";
		    action.ramdisk_directory 	= "ramdisk";
		    action.second_filename 	= "second";
		    // do we have an impiled output 
		    //if (argv[1]) action.output_directory = argv[1];

		}
		
		
	}else if(!strlcmp(argv[0],"--header") || !strlcmp(argv[0],"-h")){
		
		// we have an header, do we have a filename
		
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || argv[1][0]=='-'){
			action.header_filename = "header";
		}else{
			action.header_filename = argv[1];
			--argc; ++argv;
		}
		fprintf(stderr,"action.header_filename:%s\n",action.header_filename);
		
	}else if(!strlcmp(argv[0],"--kernel") || !strlcmp(argv[0],"-k")){
		
		// we have a kernel, do we have a filename
		
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || (argv[1][0]=='-')){
			action.kernel_filename = "kernel";
		}else{
			action.kernel_filename = argv[1];
			--argc; ++argv;
		}
		fprintf(stderr,"action.kernel_filename:%s\n",action.kernel_filename);
		
	}else if(!strlcmp(argv[0],"--second") || !strlcmp(argv[0],"-s")) {
		
		// the second bootloader. AFAIK there is only 1 device
		// in existance that
		
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || (argv[1][0]=='-')){
			action.second_filename = "second";
		}else{
			action.second_filename = argv[1];
			--argc; ++argv;
		}
		fprintf(stderr,"action.second_filename:%s\n",action.second_filename);
		
	}else if (!ramdisk_set){
    
	    if(!strlcmp(argv[0],"--cpio") || !strlcmp(argv[0],"-C")) {
		    
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || (argv[1][0]=='-')){
		    action.ramdisk_cpioname = "ramdisk.cpio";
		}else{
		    action.ramdisk_cpioname = argv[1];
		    --argc; ++argv;
		}
		ramdisk_set = 1 ;
		fprintf(stderr,"action.ramdisk_cpioname:%s\n",action.ramdisk_cpioname);
		
	    }else if(!strlcmp(argv[0],"--directory") || !strlcmp(argv[0],"-d")) {
		    
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || (argv[1][0]=='-')){
		    action.ramdisk_directory = "ramdisk";
		}else{
		    action.ramdisk_directory = argv[1];
		    --argc; ++argv;
		}
		ramdisk_set = 1 ;
		fprintf(stderr,"action.ramdisk_directory:%s\n",action.ramdisk_directory);
		    
	    }else if(!strlcmp(argv[0],"--image") || !strlcmp(argv[0],"-i")) {
		    
		// the ramdisk image as it is in the boot.img
		// this is normally a cpio.gz file but we need to 
		// check that later on.
		
		// use the default filename if this is the last token
		// or if the next token is a switch
		if(argc == 1 || (argv[1][0]=='-')){
		    action.ramdisk_imagename = "ramdisk.img";
		}else{
		    action.ramdisk_imagename = argv[1];
		    --argc; ++argv;
		}
		ramdisk_set = 1 ;
		fprintf(stderr,"action.ramdisk_imagename:%s\n",action.ramdisk_imagename);
		
	    } else if(!strlcmp(argv[0],"--files") || !strlcmp(argv[0],"-f")) {
		
		// ramdisk files. This is a variable length char array
		// containing a list of file to extract from the ramdisk
			
		// if this is the last token or if the next token is a switch
		// we need to create an array with 1 entry 
		
		// work out how much memory is required
		int targc = 0 ; 
		for(targc=0; targc < argc-1 ; targc++ ){
		    fprintf(stderr,"argv[%d] %s\n",targc,argv[targc]);
		    if(argv[targc+1] && argv[targc+1][0]=='-')
		      break;
		    else
		    action.ramdisk_filenames_count++;		
		    
		}
		fprintf(stderr,"action.ramdisk_filenames_count %d argc %d\n",action.ramdisk_filenames_count,argc);
		// allocate the memory and assign null to the end of the array
		action.ramdisk_filenames = calloc(action.ramdisk_filenames_count,sizeof(unsigned char*)) ;
		// populate the array with the values 
		for(targc =0 ; targc < action.ramdisk_filenames_count; targc++) {
		    argc--; argv++;
		    action.ramdisk_filenames[targc] = argv[0]; 
		    fprintf(stderr,"action.ramdisk_filenames[%d]:%s\n",targc,action.ramdisk_filenames[targc] );
		}
		ramdisk_set = 1 ;
	    }        
	}
        argc--; argv++ ;
    }
    // we must have at least a boot image to process
    if(!action.bootimage_filename){
	    fprintf(stderr,"no boot image:%s\n",action.bootimage_filename);
	    return EINVAL;
    }
   
	
    
    create_bootimage(&action);
       
    return 0;
}

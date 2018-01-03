#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD];
int next_open_fd = 0;


#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

/* YOUR CODE GOES HERE */

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2) {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;
  int inode_off;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));

  return OK;

}

int fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}
     
int fs_mkfs(int dev, int num_inodes) {
  int i;
  
  if (dev == 0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) {i++;}
  fsd.freemaskbytes = i / 8; 
  
  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR) {
    printf("fs_mkfs memget failed.\n");
    return SYSERR;
  }
  
  /* zero the free mask */
  for(i=0;i<fsd.freemaskbytes;i++) {
    fsd.freemask[i] = '\0';
  }
  
  fsd.inodes_used = 0;
  
  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));
  
  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  return 1;
}

void fs_print_fsd(void) {

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
  return OK;

}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void) {
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}




int fs_open(char *filename, int flags) {
  
   	int fd=-1,i=0,j;
   	for(i=0;i<fsd.root_dir.numentries;i++)
   	{
	if(strcmp(fsd.root_dir.entry[i].name,filename)==0)
		break;	
   	}

  	if(i==fsd.root_dir.numentries)
	{
		printf("No file with given name %s exists",filename);
		return SYSERR;
	}
	
	for(j=0;j<NUM_FD;j++)	
	{
		if(strcmp(fsd.root_dir.entry[i].name,oft[j].de->name)==0)
		{	
			fd = j;
			break;
		}
	}
	if(fd == -1) //fd has sentinel value so file not found
	{
		printf("\nEntry not found in File table");
		return SYSERR;
	}
	if(oft[fd].state ==  FSTATE_OPEN)
	{
		printf("\nFile is already in open state");
                return SYSERR;
	}
	
	// Getting iNode for the file
	struct inode in;
	int get_inode_status = fs_get_inode_by_num(0, oft[fd].in.id, &in);
	
	// Checking if error in completion of fs_get_inode_by_num
	if(get_inode_status == SYSERR)
	{
		printf("fs_open :: Error In fs_get_inode_by_num\n");
		return SYSERR;		
	}
	
	// Reinitializing file table entries
	oft[fd].state = FSTATE_OPEN;
	oft[fd].fileptr = 0;
	oft[fd].in = in;
	oft[fd].de = &fsd.root_dir.entry[i]; //index i got above i from dir entry
	
	// Returning file table index from oft table
  	return fd;
}

int fs_close(int fd) {

	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_close :: Invalid File\n");
		return SYSERR;
	}

if(oft[fd].state == FSTATE_CLOSED)
{
	printf("\nFile already closed");
	return SYSERR;
}
// Setting file state to be close
  oft[fd].state = FSTATE_CLOSED;
  oft[fd].fileptr = 0;
  return OK;
}

/*Allocates a free inode to the file,a new open file table entry is created to refer to this file */
int fs_create(char *filename, int mode) {
	if(mode != O_CREAT)
	{
		printf("Invalid Mode for creation of a file\n");
		return SYSERR;
	}
	
	int i = 0;

	for(i=0;i<fsd.root_dir.numentries;i++)
	{
		if(strcmp(fsd.root_dir.entry[i].name,filename)==0)
		{
		printf("\nFile with given name exists already\n");
		return SYSERR;
		}
	}
	if(fsd.inodes_used> fsd.ninodes)
	{
 	printf("No inodes available");
	return SYSERR;
	}

	//Create an inode
	struct inode in;
	int get_inode_status = fs_get_inode_by_num(0, ++fsd.inodes_used, &in);
	
	if(get_inode_status == SYSERR)
	{
	printf("fs_create :: Error In fs_get_inode_by_num\n");
	return SYSERR;		
	}


	in.id = fsd.inodes_used;
	in.type = INODE_TYPE_FILE;
	in.nlink = 0;
	in.device = 0;
	in.size = 0;

	int put_inode_status = fs_put_inode_by_num(0, i, &in);
	
	// Checking if error in completion of fs_put_inode_by_num
	if(put_inode_status == SYSERR)
	{
	printf("fs_create :: Error In fs_put_inode_by_num\n");
	return SYSERR;
	}

	strcpy(fsd.root_dir.entry[fsd.root_dir.numentries].name,filename);
	fsd.root_dir.entry[fsd.root_dir.numentries].inode_num = i; 

	oft[fsd.inodes_used].state = FSTATE_OPEN;
	oft[fsd.inodes_used].fileptr = 0; //what to set?
	oft[fsd.inodes_used].de = &fsd.root_dir.entry[fsd.root_dir.numentries++];
	oft[fsd.inodes_used].in = in;

	return fsd.inodes_used;
}

int fs_write(int fd, void *buf, int nbytes) {
 	
	if(nbytes == 0)
	{
	printf("\nZero Length File..Write Failed\n");
	return SYSERR;
	}
	int i = 0;
	 if(fd < 0 || fd > NUM_FD)
	{
		printf("\nWrite Failed..Invalid File");
		return SYSERR;
	}
	
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("\nFile not open for writing");
		return SYSERR;
	}
	// Loop over and clear till iNode size is not zero
	struct inode temp;
	if((oft[fd].in.size) > 0)
	{
		temp = oft[fd].in;
		while(oft[fd].in.size > 0)
		{
			// Clearing previously written non empty blocks
			if(fs_clearmaskbit(temp.blocks[(oft[fd].in.size)-1]) != OK)
			{
				printf("fs_write::Error In Clearing Block %d\n",(oft[fd].in.size)-1);
				return SYSERR;
			}
			oft[fd].in.size--;
		}
	}



	int blocksToWrite = nbytes/MDEV_BLOCK_SIZE;
	
	// add 1 more block if nbytes not an exact divisor of block size
	if((nbytes % MDEV_BLOCK_SIZE) != 0)
	{
		blocksToWrite++;
	}
		

	int bytesToWrite = nbytes;
	int inode_block_count = 0;
        int blockNum = 0;

	for(blockNum = FIRST_INODE_BLOCK + NUM_INODE_BLOCKS;(inode_block_count < blocksToWrite)&&(blockNum < fsd.nblocks);blockNum++)
	{
		
		if(fs_getmaskbit(blockNum) == 0) //i.e block is free to write
		{
			// Filling block cache with NULLs to avoid garbage in o/p
			memset(block_cache, NULL, MDEV_BLOCK_SIZE);
			
			// Writing blocks from block cache
			if(bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
			{
				printf("Error In Clearing Block. Write Failed %d\n", blockNum);
				return SYSERR;
			}
			
			
			int minBytes =  bytesToWrite > MDEV_BLOCK_SIZE ? MDEV_BLOCK_SIZE : bytesToWrite;
			
			memcpy(block_cache, buf, minBytes);
			
			// Writing memory and checking if operation produced some error
			if(bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
			{
				printf("fs_write :: Error In Writing Block %d\n", blockNum);
				return SYSERR;
			}
			
			// Resetting buf with new value
			buf = (char*) buf + minBytes;
			
			// Subtracting the number of bytes already written
			bytesToWrite = bytesToWrite - minBytes;
		
			// Setting bit mask for block num
			fs_setmaskbit(blockNum);
			
			oft[fd].fileptr += minBytes; //imp			
			oft[fd].in.blocks[inode_block_count++] = blockNum;


		}
	}	
	
	oft[fd].in.size = blocksToWrite;
	int put_inode_status = fs_put_inode_by_num(0, oft[fd].in.id, &oft[fd].in);
	
	if(put_inode_status == SYSERR)
	{
		printf("Error In fs_put_inode_by_num\n");
		return SYSERR;
	}
	// Resetting fileptr to new size of file
	oft[fd].fileptr = nbytes;

	return nbytes;
}


int fs_seek(int fd, int offset)
{
	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_seek :: Invalid File\n");
		return SYSERR;
	}
	
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_seek :: File Not Open\n");
		return SYSERR;
	}
	
	// Returning & setting fileptr to new position
	oft[fd].fileptr = oft[fd].fileptr + offset;
	
	return oft[fd].fileptr;
}

int fs_read(int fd, void *buf, int nbytes)
{	

	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_read :: Invalid File\n");
		return SYSERR;
	}
	// Throw error if file is in closed state
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_read :: File Not Open for reading.\n");
		return SYSERR;
	}

	if(nbytes <=0)
	{
		printf("fs_read::Invalid Length Of Read Buffer\n");
		return SYSERR;	
	}
	
	if(oft[fd].in.size == 0)
	{
		printf("fs_read :: Empty File to Read\n");
		return SYSERR;
	}

	nbytes += oft[fd].fileptr;
		
	int blocksToRead = nbytes / MDEV_BLOCK_SIZE;
	
	// If nbytes is not exact divisor then add one more block
	if((nbytes % MDEV_BLOCK_SIZE) != 0)
	{
		blocksToRead++;
	}
	

	blocksToRead = blocksToRead > oft[fd].in.size ? oft[fd].in.size : blocksToRead;
	
	// Finding first block to read
	int blockNum = (oft[fd].fileptr / MDEV_BLOCK_SIZE);
		
	// Clearing the given buffer i.e setting with NULL to avoid garbage	
	memset(buf, NULL, (MDEV_BLOCK_SIZE * MDEV_NUM_BLOCKS));
	

	int bytesRead = 0;
	
	// Setting file offset
	int offset = (oft[fd].fileptr % MDEV_BLOCK_SIZE);
	
	// Reading blocks till blocks to read
	for(; blockNum < blocksToRead; blockNum++, offset = 0)
	{
		// Clear block cache
		memset(block_cache, NULL, MDEV_BLOCK_SIZE+1);
		
		// Reading the given block
		if(bs_bread(0, oft[fd].in.blocks[blockNum], offset, block_cache, MDEV_BLOCK_SIZE - offset) == SYSERR)
		{
			printf("fs_read :: Error In Reading File\n");
			return SYSERR;
		}

		// Copy the bytes read in buffer
		strcpy((buf+bytesRead), block_cache);//store content of block_cache from position ahead of bytesRead in buffer
		
		// Resetting no of bytes read to current  length of buffer
		bytesRead = strlen(buf);
	}
	
	// Resetting fileptr to new value
	oft[fd].fileptr = bytesRead;
	
	// Returning number of bytes read
	return bytesRead;
}


#endif /* FS */
																																	

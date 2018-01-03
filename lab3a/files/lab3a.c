/* 
 *	NAME: Brandon Hua, Ryan Ohara
 *	EMAIL: huabrandon0@gmail.com, ryanohara@g.ucla.edu
 *	ID: 804595738, 404846404
 *
 *	Notes:	"...in the images we give you, the block and fragment sizes will be the same"
 *			"...in the images we give you, there will only be one group"
 *			4.1.2. rec_len: "...there cannot be any directory entry spanning multiple data blocks"
 *			Table 3-20: shows that inode index i corresponds to inode number i+1
 *			Prof. Kampe on Piazza: "The test images for P3A will not include any corruption"
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "ext2_fs.h"

#define SUPERBLOCK_OFFSET 1024

/* INODE: MODE FLAGS */
#define EXT2_S_IFLNK	0xA000	//symbolic link
#define EXT2_S_IFREG	0x8000	//regular file
#define EXT2_S_IFDIR	0x4000	//directory

char filetype(__u16 mode);

int main(int argc, char *argv[])
{	
	int img_fd;
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s FILE\n", argv[0]);
		exit(1);
	}
	else
	{
		img_fd = open(argv[1], O_RDONLY);
		if (img_fd == -1)
		{
			fprintf(stderr, "Unable to open file '%s': %s\n",
				argv[1], strerror(errno));
			exit(2);
		}
	}
	
	/* SUPERBLOCK SUMMARY */
	struct ext2_super_block sb;
	ssize_t rc = pread(img_fd, &sb, sizeof(sb), SUPERBLOCK_OFFSET);
	if (rc == -1)
	{
		fprintf(stderr, "Error while reading in superblock: %s\n",
			strerror(errno));
		exit(2);
	}
	
	__u32 block_size = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;
	
	printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
		sb.s_blocks_count,
		sb.s_inodes_count,
		block_size,
		sb.s_inode_size,
		sb.s_blocks_per_group,
		sb.s_inodes_per_group,
		sb.s_first_ino
		);
	
	/* GROUP SUMMARY */
	const off_t GD_OFFSET = SUPERBLOCK_OFFSET + block_size;
	struct ext2_group_desc gd; //Only one group, so only read in one group descriptor
	rc = pread(img_fd, &gd, sizeof(gd), GD_OFFSET);
	if (rc == -1)
	{
		fprintf(stderr, "Error while reading in group description: %s\n",
			strerror(errno));
		exit(2);
	}
	
	printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
		0, //Only one group, so constant 0 for first and only group.
		sb.s_blocks_count,
		sb.s_inodes_per_group,
		gd.bg_free_blocks_count,
		gd.bg_free_inodes_count,
		gd.bg_block_bitmap,
		gd.bg_inode_bitmap,
		gd.bg_inode_table
		);
	
	/* FREE BLOCK ENTRIES */
	const off_t DB_OFFSET = block_size * gd.bg_block_bitmap;
	
	off_t i;
	//loop to read one byte at a time
	for (i = 0; i < block_size && i*8 < sb.s_blocks_count; i++)
	{
		char db_byte;
		rc = pread(img_fd, &db_byte, sizeof(db_byte), DB_OFFSET + i);
		if (rc == -1)
		{
			fprintf(stderr, "Error while reading in block bitmap: %s\n",
				strerror(errno));
			exit(2);
		}
		
		//loop to check each of the 8 bits
		off_t j;
		for (j = 0; j < 8 && i*8 + j < sb.s_blocks_count; j++)
		{
			if ((db_byte >> j & 0x01) == 0) //bit is free if == 0
				printf("BFREE,%d\n", 1 + i*8 + j); //starts @ block #1 so add 1
		}
	}
	
	/* FREE INODE ENTRIES */
	const off_t IB_OFFSET = block_size * gd.bg_inode_bitmap;
	
	//loop to read one byte at a time
	for (i = 0; i < block_size && i*8 < sb.s_inodes_count; i++)
	{
		char ib_byte;
		rc = pread(img_fd, &ib_byte, sizeof(ib_byte), IB_OFFSET + i*sizeof(ib_byte));
		if (rc == -1)
		{
			fprintf(stderr, "Error while reading in inode bitmap: %s\n",
				strerror(errno));
			exit(2);
		}
		
		//loop to check each of the 8 bits
		off_t j;
		for (j = 0; j < 8 && i*8 + j < sb.s_inodes_count; j++)
		{
			if ((ib_byte >> j & 0x01) == 0) //bit is free if == 0
				printf("IFREE,%d\n", 1 + i*8 + j); //starts @ inode #1 so add 1
		}
	}
	
	/* INODE SUMMARY */
	const off_t IT_OFFSET = block_size * gd.bg_inode_table;
	struct ext2_inode it[(sb.s_inodes_count <= sb.s_inodes_per_group) ? sb.s_inodes_count: sb.s_inodes_per_group]; //24 for trivial.img
	rc = pread(img_fd, &it, sizeof(it), IT_OFFSET);
	if (rc == -1)
	{
		fprintf(stderr, "Error while reading in inode table: %s\n",
			strerror(errno));
		exit(2);
	}
	
	//loop to analyze each valid inode
	for (i = 0; i < sb.s_inodes_per_group && i < sb.s_inodes_count; i++)
	{
		if (it[i].i_mode != 0 && it[i].i_links_count != 0)
		{	
			time_t ctime = it[i].i_ctime;
			time_t mtime = it[i].i_mtime;
			time_t atime = it[i].i_atime;
			char ctime_str[18], mtime_str[18], atime_str[18];
			strftime(ctime_str, sizeof(ctime_str), "%D %T", gmtime(&ctime));
			strftime(mtime_str, sizeof(mtime_str), "%D %T", gmtime(&mtime));
			strftime(atime_str, sizeof(atime_str), "%D %T", gmtime(&atime));
			
			printf("INODE,%u,%c,%o,%u,"
				"%u,%u,%s,%s,%s,"
				"%u,%u,%u,%u,%u,"
				"%u,%u,%u,%u,%u,"
				"%u,%u,%u,%u,%u,"
				"%u,%u\n",
				1 + i, //starts @ 1
				filetype(it[i].i_mode),
				it[i].i_mode & 07777,
				it[i].i_uid,
				it[i].i_gid,
				it[i].i_links_count,
				ctime_str,
				mtime_str,
				atime_str,
				it[i].i_size,
				it[i].i_blocks,
				it[i].i_block[0],
				it[i].i_block[1],
				it[i].i_block[2],
				it[i].i_block[3],
				it[i].i_block[4],
				it[i].i_block[5],
				it[i].i_block[6],
				it[i].i_block[7],
				it[i].i_block[8],
				it[i].i_block[9],
				it[i].i_block[10],
				it[i].i_block[11],
				it[i].i_block[12],
				it[i].i_block[13],
				it[i].i_block[14]
				);
				
			/* DIRECTORY ENTRIES, INDIRECT BLOCK REFERENCES */
			switch(filetype(it[i].i_mode))
			{
				off_t j,k,l;
				case 'd':
					;
					/* DIRECTORY ENTRIES */
					for (j = 0; j < EXT2_N_BLOCKS; j++) //Note: EXT2_N_BlOCKS = 15 (number of direct/indirect blocks)
					{
						const off_t DE_HEAD_OFFSET = block_size * it[i].i_block[j];
						const off_t DE_TAIL_OFFSET = block_size * (it[i].i_block[j] + 1);
						
						off_t DE_CURR_OFFSET = DE_HEAD_OFFSET; //list iterator
						struct ext2_dir_entry de;
						rc = pread(img_fd, &de, sizeof(de), DE_CURR_OFFSET);
						if (rc == -1)
						{
							fprintf(stderr, "Error while reading in directory entry head: %s\n",
								strerror(errno));
							exit(2);
						}
						
						if (de.inode != 0)
						{
							do
							{
								printf("DIRENT,%u,%u,%d,%u,%u,'%s'\n",
									i + 1, //parent inode number
									DE_CURR_OFFSET - DE_HEAD_OFFSET,
									de.inode, //why isn't this i+1?
									de.rec_len,
									de.name_len,
									de.name
									);
								
								DE_CURR_OFFSET += de.rec_len;
								rc = pread(img_fd, &de, sizeof(de), DE_CURR_OFFSET);
								if (rc == -1)
								{
									fprintf(stderr, "Error while reading in directory entry: %s\n",
										strerror(errno));
									exit(2);
								}
								
							} while(DE_CURR_OFFSET != DE_TAIL_OFFSET && de.rec_len != 0);
						}
					}
					//Note: not having a break here is intended.
				case 'f':
					;
					/* SINGLY-INDIRECT BLOCKS */
					if (it[i].i_block[12] != 0)
					{
						off_t IND_OFFSET = block_size * it[i].i_block[12];
						__u32 block_ids[256]; //Note: block_size/(4 bytes) = 256 block addresses
						rc = pread(img_fd, block_ids, sizeof(block_ids), IND_OFFSET);
						if (rc == -1)
						{
							fprintf(stderr, "Error while reading in indirect block: %s\n",
								strerror(errno));
							exit(2);
						}
						
						//loop through all non-zero block ids
						for (j = 0; j < 256; j++)
						{
							if (block_ids[j] != 0)
							{
								printf("INDIRECT,%u,%d,%u,%u,%u\n",
									i + 1, //inode number of the owning file
									1, //1 level of indirection
									12 + j, //blocks 13-268, indexed by 12-267
									it[i].i_block[12], //block number of the indirect block that contains the block ids
									block_ids[j] //the actual block id
									);
							}
						}
					}
					
					/* DOUBLY-INDIRECT BLOCKS */
					if (it[i].i_block[13] != 0)
					{
						off_t DOUBLY_IND_OFFSET = block_size * it[i].i_block[13];
						__u32 ind_block_ids[256];
						rc = pread(img_fd, ind_block_ids, sizeof(ind_block_ids), DOUBLY_IND_OFFSET);
						if (rc == -1)
						{
							fprintf(stderr, "Error while reading in doubly-indirect block: %s\n",
								strerror(errno));
							exit(2);
						}
						
						//loop through all indirect blocks w/non-zero addresses
						for (j = 0; j < 256; j++)
						{
							if (ind_block_ids[j] != 0)
							{
								printf("INDIRECT,%u,%d,%u,%u,%u\n",
									i + 1, //inode number of the owning file
									2, //2 levels of indirection
									268 + j*256, //blocks 269-65804, indexed by 268-65803 (in increments of 256)
									it[i].i_block[13], //block number of the doubly-indirect block that references the indirect block ids
									ind_block_ids[j] //the actual indirect block id
									);
								
								off_t IND_OFFSET = block_size * ind_block_ids[j];
								__u32 block_ids[256];
								rc = pread(img_fd, block_ids, sizeof(block_ids), IND_OFFSET);
								if (rc == -1)
								{
									fprintf(stderr, "Error while reading in indirect block: %s\n",
										strerror(errno));
									exit(2);
								}
								
								//loop through all non-zero block ids
								for (k = 0; k < 256; k++)
								{
									if (block_ids[k] != 0)
									{
										printf("INDIRECT,%u,%d,%u,%u,%u\n",
											i + 1, //inode number of the owning file
											1, //probably should be 1 and not 2 levels of indirection
											268 + j*256 + k, //blocks 269 + j*256 through 269 + j*256 + 255, indexed by 268 + ...
											ind_block_ids[j], //block number of the indirect block that contains the block ids
											block_ids[k]
											);
									}
								}
							}
						}
					}
					
					/* TREBLY-INDIRECT BLOCKS */
					if (it[i].i_block[14] != 0)
					{
						off_t TREBLY_IND_OFFSET = block_size * it[i].i_block[14];
						__u32 doubly_ind_block_ids[256];
						rc = pread(img_fd, doubly_ind_block_ids, sizeof(doubly_ind_block_ids), TREBLY_IND_OFFSET);
						if (rc == -1)
						{
							fprintf(stderr, "Error while reading in trebly-indirect block: %s\n",
								strerror(errno));
							exit(2);
						}
						
						//loop through all doubly-indirect blocks w/non-zero addresses
						for (j = 0; j < 256; j++)
						{
							if (doubly_ind_block_ids[j] != 0)
							{
								printf("INDIRECT,%u,%d,%u,%u,%d\n",
									i + 1, //inode number of the owning file
									3, //3 levels of indirection
									65804 + j*65536, //blocks 65805-end, indexed by 65804-end (in increments of 65536)
									it[i].i_block[14], //block number of the trebly-indirect block that references the doubly-indirect block ids
									doubly_ind_block_ids[j] //the actual doubly-indirect block id
									);
								
								off_t DOUBLY_IND_OFFSET = block_size * doubly_ind_block_ids[j];
								__u32 ind_block_ids[256];
								rc = pread(img_fd, ind_block_ids, sizeof(ind_block_ids), DOUBLY_IND_OFFSET);
								if (rc == -1)
								{
									fprintf(stderr, "Error while reading in doubly-indirect block: %s\n",
										strerror(errno));
									exit(2);
								}
								
								//loop through all indirect blocks w/non-zero addresses
								for (k = 0; k < 256; k++)
								{
									if (ind_block_ids[k] != 0)
									{
										printf("INDIRECT,%u,%d,%u,%u,%u\n",
											i + 1, //inode number of the owning file
											2, //2 levels of indirection
											65804 + j*65536 + k*256, //blocks 65805 + j*65536 through 65805 + j*65536 + 65535, indexed by 65804 + ... (in increments of 256)
											doubly_ind_block_ids[j], //block number of the doubly-indirect block that references the indirect block ids
											ind_block_ids[k] //the actual indirect block id
											);
										
										off_t IND_OFFSET = block_size * ind_block_ids[k];
										__u32 block_ids[256];
										rc = pread(img_fd, block_ids, sizeof(block_ids), IND_OFFSET);
										if (rc == -1)
										{
											fprintf(stderr, "Error while reading in indirect block: %s\n",
												strerror(errno));
											exit(2);
										}
										
										//loop through all non-zero block ids
										for (l = 0; l < 256; l++)
										{
											if (block_ids[l] != 0)
											{
												printf("INDIRECT,%u,%d,%u,%u,%u\n",
													i + 1, //inode number of the owning file
													1, //probably should be 1 and not 3 levels of indirection
													65804 + j*65536 + k*256 + l, //blocks 65805 + j*65536 + k*256 through 65805 + j*65536 + k*256 + 255, indexed by 65804 + ...
													ind_block_ids[k], //block number of the indirect block that contains the block ids
													block_ids[l]
													);
											}
										}
									}
								}
							}
						}
					}
					
					break;
				default:
					break;
			}
		}
	}
	exit(0);
}

char filetype(__u16 mode)
{
	switch (mode & 0xC000)
	{
		case EXT2_S_IFLNK:	//symbolic link
			return 's';
		case EXT2_S_IFREG:	//regular file
			return 'f';
		case EXT2_S_IFDIR:	//directory
			return 'd';
		default:
			return '?';
	}
}

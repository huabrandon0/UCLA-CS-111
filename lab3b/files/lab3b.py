#! /usr/bin/env python3

import sys
import re

if len(sys.argv) != 2:
	print("wrong number of arguments", file=sys.stderr)
	sys.exit(1)

f = open(sys.argv[1], 'r')
lines = f.read().splitlines()
f.close()

exit_code = 0

#line_sb = []
#line_group = []
lines_inode = []
lines_indirect = []
lines_dirent = []
bfree_list = []
ifree_list = []
	
for line in lines:
	if re.search('SUPERBLOCK', line):
		line_sb = line.split(',')
	elif re.search('GROUP', line):
		line_group = line.split(',')
	elif re.search('INODE', line):
		lines_inode.append(line.split(','))
	elif re.search('INDIRECT', line):
		lines_indirect.append(line.split(','))
	elif re.search('DIRENT', line):
		lines_dirent.append(line.split(','))
	elif re.search('BFREE', line):
		bfree_list.append(int(line[6:]))
	elif re.search('IFREE', line):
		ifree_list.append(int(line[6:]))
		
############################
# BLOCK CONSISTENCY AUDITS #
############################

#	Minimum and maximum indices of block ids:
#		min = end of inodes = first inode block + inode size * number of inodes / block size
#		max = number of blocks in group - 1
block_min = int(line_group[8]) + int(int(line_sb[4])*int(line_group[3])/int(line_sb[3]))
block_max = int(line_group[2]) - 1

#	Data structure to maintain the "state" of each block.
blocks_state = ['UNREFERENCED']*int(line_group[2])

#	Examining the inodes for inconsistencies, while assigning states to each block.
#	States:
#		UNREFERENCED	default block state; block is not referenced at all
#		USED			block is legal and referenced only once
#		DUPLICATE		block is legal but referenced more than once
#	Pseudo-states (not actually assigned states):
#		RESERVED		block is below block min (error message printed immediately)
#		INVALID			block is above block max (error message printed immediately)
for line in lines_inode:
	for i in range(12, 27):
		if i == 24:
			block_type = 'INDIRECT BLOCK'
			block_offset = '12'
		elif i == 25:
			block_type = 'DOUBLE INDIRECT BLOCK'
			block_offset = '268'
		elif i == 26:
			block_type = 'TRIPPLE INDIRECT BLOCK'
			block_offset = '65804'
		else:
			block_type = 'BLOCK'
			block_offset = str(i - 12)
				
		if 0 < int(line[i]) < block_min:
			exit_code = 2
			print('RESERVED' + \
				' ' + block_type + ' ' + line[i] + \
				' IN INODE ' + line[1] + \
				' AT OFFSET ' + block_offset)
		elif int(line[i]) > block_max:
			exit_code = 2
			print('INVALID' + \
				' ' + block_type + ' ' + line[i] + \
				' IN INODE ' + line[1] + \
				' AT OFFSET ' + block_offset)
		elif int(line[i]) != 0:
			if blocks_state[int(line[i])] == 'UNREFERENCED':
				blocks_state[int(line[i])] = 'USED'
			elif blocks_state[int(line[i])] == 'USED':
				blocks_state[int(line[i])] = 'DUPLICATE'
		# else int(line[i]) must be 0 or between min and max,
		# which is consistent in either case

#	Examining the indirect blocks.
for line in lines_indirect:
	if line[2] == 1:
		block_type = 'BLOCK'
	elif line[2] == 2:
		block_type = 'INDIRECT BLOCK'
	else: # line[2] == 3
		block_type = 'DOUBLE INDIRECT BLOCK'

	if 0 < int(line[5]) < block_min:
		exit_code = 2
		print('RESERVED' + \
			' ' + block_type + ' ' + line[5] + \
			' IN INODE ' + line[1] + \
			' AT OFFSET ' + line[3])
	elif int(line[5]) > block_max:
		exit_code = 2
		print('INVALID' + \
			' ' + block_type + ' ' + line[5] + \
			' IN INODE ' + line[1] + \
			' AT OFFSET ' + line[3])
	elif int(line[5]) != 0:
		if blocks_state[int(line[5])] == 'UNREFERENCED':
			blocks_state[int(line[5])] = 'USED'
		elif blocks_state[int(line[5])] == 'USED':
			blocks_state[int(line[5])] = 'DUPLICATE'
			
#	Finalizing the block states based on the block free list.
#	Added states:
#		ON FREE LIST		block is UNREFERENCED and on free list (consistent)
#		ALLOCATED			block is USED/DUPLICATE and on free list (inconsistent)
#	Note 1: A block remains UNREFERENCED if it is not on the free list
#	Note 2: this assumes no invalid/reserved blocks will be on the free list
#	Note 3: this also assumes a legal block can only be in one state (and therefore
#		only one print statement per block is made)
for block_num in bfree_list:
	if blocks_state[block_num] == 'UNREFERENCED':
		blocks_state[block_num] = 'ON FREE LIST' #'ON FREE LIST' means the block is consistent
	if blocks_state[block_num] in ['USED', 'DUPLICATE']:
		blocks_state[block_num] = 'ALLOCATED'

#	Report inconsistencies found.
for line in lines_inode:
	for i in range(12, 27):
		if block_min <= int(line[i]) <= block_max:
			if i == 24:
				block_type = 'INDIRECT BLOCK'
				block_offset = '12'
			elif i == 25:
				block_type = 'DOUBLE INDIRECT BLOCK'
				block_offset = '268'
			elif i == 26:
				block_type = 'TRIPPLE INDIRECT BLOCK'
				block_offset = '65804'
			else:
				block_type = 'BLOCK'
				block_offset = str(i - 12)
				
			if blocks_state[int(line[i])] == 'DUPLICATE':
				exit_code = 2
				print('DUPLICATE' + \
					' ' + block_type + ' ' + line[i] + \
					' IN INODE ' + line[1] + \
					' AT OFFSET ' + block_offset)
			elif blocks_state[int(line[i])] == 'ALLOCATED':
				exit_code = 2
				print('ALLOCATED' + \
					' ' + block_type + ' ' + line[i] + \
					' ON FREELIST')

for line in lines_indirect:
	if block_min <= int(line[5]) <= block_max:
		if line[2] == 1:
			block_type = 'BLOCK'
		elif line[2] == 2:
			block_type = 'INDIRECT BLOCK'
		else: #line[2] == 3
			block_type = 'DOUBLE INDIRECT BLOCK'

		if blocks_state[int(line[5])] == 'DUPLICATE':
			exit_code = 2
			print('DUPLICATE' + \
				' ' + block_type + ' ' + line[5] + \
				' IN INODE ' + line[1] + \
				' AT OFFSET ' + line[3])
		elif blocks_state[int(line[5])] == 'ALLOCATED':
			exit_code = 2
			print('ALLOCATED' + \
				' ' + block_type + ' ' + line[5] + \
				' ON FREELIST')

for block_num in range(block_min, block_max + 1):
	if blocks_state[block_num] == 'UNREFERENCED':
		exit_code = 2
		print('UNREFERENCED BLOCK ' + str(block_num))
			
###########################
# INODE ALLOCATION AUDITS #
###########################

#	Note: The following code assumes inode mode is inode type.
#	From TA Huang on Piazza:
#			The "type" of 0 here means the '664' field.
#			If the inode here is not allocated, it should be 0.

#	Assigning a state to each inode based on its mode field.
#	States:
#		UNALLOCATED		default state; mode = 0
#		ALLOCATED		mode != 0
#	Note: The code assumes inode numbers are not repeated.
inodes_state = ['UNALLOCATED']*(int(line_group[3]) + 1)
for i in range (1, int(line_sb[7])): # reserved inodes are ALLOCATED
	inodes_state[i] = 'ALLOCATED'
for line in lines_inode:
	if int(line[3]) != 0:
		inodes_state[int(line[1])] = 'ALLOCATED'

#	Report inconsistencies found.
for i in range(1, int(line_group[3]) + 1):
	if inodes_state[i] == 'UNALLOCATED' and not i in ifree_list:
		exit_code = 2
		print('UNALLOCATED' + \
		' INODE ' + str(i) + \
		' NOT ON FREELIST')
	elif inodes_state[i] == 'ALLOCATED' and i in ifree_list:
		exit_code = 2
		print('ALLOCATED' + \
		' INODE ' + str(i) + \
		' ON FREELIST')

################################
# DIRECTORY CONSISTENCY AUDITS #
################################

inode_min = 1
inode_max = int(line_group[3]) # number of inodes in the group

#	Getting the reference counts of each inode referred to by
#	directory entries. While doing so, also checking the validity
#	and allocation status of each referenced inode.
inodes_ref_counts = [0]*(int(line_group[3]) + 1)
for line in lines_dirent:
	if int(line[3]) < inode_min or int(line[3]) > inode_max:
		exit_code = 2
		print('DIRECTORY INODE ' + line[1] + \
			' NAME ' + line[6] + \
			' INVALID INODE ' + line[3])
	elif inodes_state[int(line[3])] == 'UNALLOCATED':
		exit_code = 2
		print('DIRECTORY INODE ' + line[1] + \
			' NAME ' + line[6] + \
			' UNALLOCATED INODE ' + line[3])
	else:
		inodes_ref_counts[int(line[3])] += 1

#	Report inconsistencies (non-matching link counts).
for line in lines_inode:
	if int(line[6]) != inodes_ref_counts[int(line[1])]:
		exit_code = 2
		print('INODE ' + line[1] + \
			' HAS ' + str(inodes_ref_counts[int(line[1])]) + ' LINKS'\
			' BUT LINKCOUNT IS ' + line[6])
		
#	Checking the validity of the '.' and '..' links for
#	each directory inode.
for line_i in lines_inode:
	if line_i[2] == 'd':
		for line_d in lines_dirent:
			if line_d[1] == line_i[1]: # dirent lines of this directory inode
				if line_d[6] == '\'.\'':
					line_itself = line_d
				if line_d[6] == '\'..\'':
					line_parent = line_d
		
		# Parent and child inode number of '.' directory entry should be equal.
		if line_itself[3] != line_i[1]:
			exit_code = 2
			print('DIRECTORY INODE ' + line_i[1] + \
				' NAME \'.\' LINK TO INODE ' + line_itself[3] + \
				' SHOULD BE ' + line_i[1])
				
		# Obtaining the "should-be" parent inode number.
		# Finding the dirent line that refers to the parent inode of '..' as its child.
		if int(line_i[1]) != 2: # not root directory
			for line_d in lines_dirent:
				if line_d[3] == line_parent[1] and line_d[6] != '\'.\'':
					line_actualparent = line_d
		else:
			line_actualparent = line_itself
		
		# Inode of the parent directory does not match the '..' directory entry link.
		if line_parent[3] != line_actualparent[1]:
			exit_code = 2
			print('DIRECTORY INODE ' + line_i[1] + \
			' NAME \'..\' LINK TO INODE ' + line_parent[3] + \
			' SHOULD BE ' + line_actualparent[1])
			
sys.exit(exit_code)

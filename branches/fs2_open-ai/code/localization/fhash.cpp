/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/localization/fhash.cpp $
 * $Revision: 2.4.2.1 $
 * $Date: 2007-07-28 00:37:35 $
 * $Author: Goober5000 $
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.4  2005/05/12 17:49:13  taylor
 * use vm_malloc(), vm_free(), vm_realloc(), vm_strdup() rather than system named macros
 *   fixes various problems and is past time to make the switch
 *
 * Revision 2.3  2004/07/26 20:47:36  Kazan
 * remove MCD complete
 *
 * Revision 2.2  2004/07/12 16:32:52  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.1  2002/08/01 01:41:06  penguin
 * The big include file move
 *
 * Revision 2.0  2002/06/03 04:02:24  penguin
 * Warpcore CVS sync
 *
 * Revision 1.1  2002/05/02 18:03:09  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 5     2/23/99 9:25a Dave
 * Stubbed out a bunch of stuff to get cfile and lcl functions in.
 * 
 * 4     12/01/98 4:46p Dave
 * Put in targa bitmap support (16 bit).
 *  
 * $NoKeywords: $
 */

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "globalincs/pstypes.h"
#include "localization/fhash.h"



// -----------------------------------------------------------------------------------------------
// HASH DEFINES/VARS
//

// hash node
typedef struct fhash_node {
	char *str;							// allocated dynamically
	int id;								// can be -1
	fhash_node *next, *prev;			// for chaining in an individual hash table entry (non-circular, doubly linked list)
} fhash_node;

// hash table itself (with chained nodes)
#define HASH_TABLE_SIZE				253					// works better when not a power of 2, and is prime
fhash_node *Hash_table_fred[HASH_TABLE_SIZE];

// if the hash table is active
int Fhash_active = 0;


// -----------------------------------------------------------------------------------------------
// HASH FORWARD DECLARATIONS
//

// hash a string. return an index into the hash table where it should be inserted
int fhash_get_hash_index(char *str);

// insert a string into hash table index N, will take care of allocating/chaining everything
void fhash_insert(char *str, int id, int n);


// -----------------------------------------------------------------------------------------------
// HASH FUNCTIONS
//

// initialize the hash table
void fhash_init()
{
	memset(Hash_table_fred, 0, sizeof(fhash_node *) * HASH_TABLE_SIZE);
}

// set the hash table to be active for parsing
void fhash_activate()
{	
	Fhash_active = 1;	
}

// set the hash table to be inactive for parsing
void fhash_deactivate()
{	
	Fhash_active = 0;
}

// if the hash table is active
int fhash_active()
{
	return Fhash_active;
}

// flush out the hash table, freeing up everything
void fhash_flush()
{
	int idx;
	fhash_node *moveup, *backup;

	// go through each element
	for(idx=0; idx<HASH_TABLE_SIZE; idx++){
		if(Hash_table_fred[idx] != NULL){
			moveup = Hash_table_fred[idx];
			while(moveup != NULL){
				// next element
				backup = moveup;
				moveup = moveup->next;

				// free up this element
				if(backup->str != NULL){
					vm_free(backup->str);
				}
				vm_free(backup);
			}

			// null this element
			Hash_table_fred[idx] = NULL;
		}
	}
}

// add a string with the given id# to the has table
void fhash_add_str(char *str, int id)
{
	int hash_index;

	// if the hash table isn't active, don't bother
	Assert(Fhash_active);
	if(!Fhash_active){
		return;
	}

	// determine where the string goes in the has table
	Assert(str != NULL);
	if(str == NULL){
		return;
	}
	hash_index = fhash_get_hash_index(str);

	// insert into the hash table
	fhash_insert(str, id, hash_index);
}

// determine if the passed string exists in the table
// returns : -2 if the string doesn't exit, or >= -1 as the string id # otherwise
int fhash_string_exists(char *str)
{
	int hash_index;
	fhash_node *moveup;

	Assert(str != NULL);
	if(str == NULL){
		return -2;
	}

	// get the hash index for this string
	hash_index = fhash_get_hash_index(str);

	// if there are no entries, it doesn't exist
	if(Hash_table_fred[hash_index] == NULL){
		return -2;
	}

	// otherwise compare against all strings in this code
	moveup = Hash_table_fred[hash_index];
	while(moveup != NULL){
		// do a string compare on this item
		Assert(moveup->str != NULL);
		if(moveup->str != NULL){
			if(!strcmp(moveup->str, str)){
				return moveup->id;
			}
		}
		
		// next item
		moveup = moveup->next;
	}

	// didn't find it
	return -2;
}


// -----------------------------------------------------------------------------------------------
// HASH FORWARD DEFINITIONS
//

// hash a string. return an index into the hash table where it should be inserted
int fhash_get_hash_index(char *str)
{
	int accum = 0;
	int idx, str_len;
	int ret;

	// add up the string
	str_len = strlen(str);
	for(idx=0; idx<str_len; idx++){
		accum += str[idx];
	}

	ret = abs(accum) % 253;	
	return ret;
}

// insert a string into hash table index N, will take care of allocating/chaining everything
void fhash_insert(char *str, int id, int n)
{
	fhash_node *new_node;
	fhash_node *moveup;

	// allocate the new node
	new_node = (fhash_node*)vm_malloc(sizeof(fhash_node));
	Assert(new_node);
	if(new_node == NULL){
		return;
	}

	// fill in the node
	new_node->str = vm_strdup(str);
	new_node->id = id;
	new_node->next = NULL;
	new_node->prev = NULL;

	// if this hash index is NULL, just assign it
	if(Hash_table_fred[n] == NULL){
		Hash_table_fred[n] = new_node;
	} else {
		moveup = Hash_table_fred[n];
		while(moveup->next != NULL){
			moveup = moveup->next;
		}
		new_node->prev = moveup;
		moveup->next = new_node;
	}
}

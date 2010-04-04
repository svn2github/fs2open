/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

#include "globalincs/systemvars.h"
#include "parse/parselo.h"
#include "weapon/muzzleflash.h"
#include "particle/particle.h"
#include "graphics/2d.h"
#include "math/vecmat.h"
#include "object/object.h"


// ---------------------------------------------------------------------------------------------------------------------
// MUZZLE FLASH DEFINES/VARS
// 

// muzzle flash info - read from a table
typedef struct mflash_blob_info {
	char name[MAX_FILENAME_LEN];
	int anim_id;
	float offset;
	float radius;

	mflash_blob_info( const mflash_blob_info& mbi )
	{
		strcpy_s( name, mbi.name );
		anim_id = mbi.anim_id;
		offset = mbi.offset;
		radius = mbi.radius;
	}

	mflash_blob_info() :
		anim_id( -1 ),
		offset( 0.0 ),
		radius( 0.0 )
	{ 
		name[ 0 ] = '\0';
	}

	void operator=( const mflash_blob_info& r )
	{
		strcpy_s( name, r.name );
		anim_id = r.anim_id;
		offset = r.offset;
		radius = r.radius;
	}
} mflash_blob_info;

typedef struct mflash_info {
	char name[MAX_FILENAME_LEN];
	int	 used_this_level;
	SCP_vector<mflash_blob_info> blobs;

	mflash_info() 
		: used_this_level( 0 )
	{ 
		name[ 0 ] = '\0';
	}

	mflash_info( const mflash_info& mi )
	{
		strcpy_s( name, mi.name );
		used_this_level = mi.used_this_level;
		blobs = mi.blobs;
	}

	void operator=( const mflash_info& r )
	{
		strcpy_s( name, r.name );
		used_this_level = r.used_this_level;
		blobs = r.blobs;
	}
} mflash_info;

SCP_vector<mflash_info> Mflash_info;


//#define MAX_MFLASH				50

// Stuff for missile trails doesn't need to be saved or restored... or does it?
/*
typedef struct mflash {	
	struct	mflash * prev;
	struct	mflash * next;

	ubyte		type;																			// muzzle flash type
	int		blobs[MAX_MFLASH_BLOBS];												// blobs
} mflash;

int Num_mflash = 0;
mflash Mflash[MAX_MFLASH];

mflash Mflash_free_list;
mflash Mflash_used_list;
*/

// ---------------------------------------------------------------------------------------------------------------------
// MUZZLE FLASH FUNCTIONS
// 

void parse_mflash_tbl(char *filename)
{
	int rval;
	uint i;

	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("TABLES: Unable to parse '%s'!  Error code = %i.\n", filename, rval));
		return;
	}

	read_file_text(filename, CF_TYPE_TABLES);
	reset_parse();		

	// header
	required_string("#Muzzle flash types");

	while ( optional_string("$Mflash:") ) {
		mflash_info mflash;
		bool override_mflash = false;

		required_string("+name:");
		stuff_string(mflash.name, F_NAME, MAX_FILENAME_LEN);

		if (optional_string("+override"))
			override_mflash = true;

		// read in all blobs
		while ( optional_string("+blob_name:") ) {
			mflash_blob_info mblob;

			stuff_string(mblob.name, F_NAME, MAX_FILENAME_LEN);

			required_string("+blob_offset:");
			stuff_float(&mblob.offset);

			required_string("+blob_radius:");
			stuff_float(&mblob.radius);

			mflash.blobs.push_back(mblob);
		}

		for (i = 0; i < Mflash_info.size(); i++) {
			if ( !stricmp(mflash.name, Mflash_info[i].name) ) {
				if (override_mflash) {
					Mflash_info[i] = mflash;
				}
				break;
			}
		}

		// no matching name exists so add as new
		if (i == Mflash_info.size()) {
			Mflash_info.push_back(mflash);
		}
		// a mflash of the same name exists, don't add it again
		else {
			if (!override_mflash) {
				Warning(LOCATION, "Muzzle flash \"%s\" already exists!  Using existing entry instead.", mflash.name);
			}
		}
	}

	// close
	required_string("#end");
}

// initialize muzzle flash stuff for the whole game
void mflash_game_init()
{
	// parse main table first
	parse_mflash_tbl("mflash.tbl");

	// look for any modular tables
	parse_modular_table(NOX("*-mfl.tbm"), parse_mflash_tbl);
}

void mflash_mark_as_used(int index)
{
	if (index < 0)
		return;

	Assert( index < (int)Mflash_info.size() );

	Mflash_info[index].used_this_level++;
}

void mflash_page_in(bool load_all)
{
	uint i, idx;
	int num_frames, fps;

	// load up all anims
	for ( i = 0; i < Mflash_info.size(); i++) {
		// skip if it's not used
		if ( !load_all && !Mflash_info[i].used_this_level )
			continue;

		// blobs
		for ( idx = 0; idx < Mflash_info[i].blobs.size(); idx++) {
			Mflash_info[i].blobs[idx].anim_id = bm_load_animation(Mflash_info[i].blobs[idx].name, &num_frames, &fps, NULL, 1);
			Assert( Mflash_info[i].blobs[idx].anim_id >= 0 );
			bm_page_in_xparent_texture( Mflash_info[i].blobs[idx].anim_id );
		}
	}
}

// initialize muzzle flash stuff for the level
void mflash_level_init()
{
	uint i, idx;

	/*
	Num_mflash = 0;
	list_init( &Mflash_free_list );
	list_init( &Mflash_used_list );

	// Link all object slots into the free list
	for (i=0; i<MAX_MFLASH; i++)	{
		memset(&Mflash[i], 0, sizeof(mflash));
		list_append(&Mflash_free_list, &Mflash[i] );
	}
	*/

	// reset all anim usage for this level
	for ( i = 0; i < Mflash_info.size(); i++) {
		for ( idx = 0; idx < Mflash_info[i].blobs.size(); idx++) {
			Mflash_info[i].used_this_level = 0;
		}
	}
}

// shutdown stuff for the level
void mflash_level_close()
{
	uint i, idx;

	// release all anims
	for ( i = 0; i < Mflash_info.size(); i++) {
		// blobs
		for ( idx = 0; idx < Mflash_info[i].blobs.size(); idx++) {
			if ( Mflash_info[i].blobs[idx].anim_id < 0 )
				continue;

			bm_release( Mflash_info[i].blobs[idx].anim_id );
			Mflash_info[i].blobs[idx].anim_id = -1;
		}
	}
}

// create a muzzle flash on the guy
void mflash_create(vec3d *gun_pos, vec3d *gun_dir, physics_info *pip, int mflash_type, object *local)
{	
	// mflash *mflashp;
	mflash_info *mi;
	mflash_blob_info *mbi;
	particle_info p;
	uint idx;

	// standalone server should never create trails
	if(Game_mode & GM_STANDALONE_SERVER){
		return;
	}

	// illegal value
	if ( (mflash_type < 0) || (mflash_type >= (int)Mflash_info.size()) )
		return;

	/*
	if (Num_mflash >= MAX_MFLASH ) {
		#ifndef NDEBUG
		mprintf(("Muzzle flash creation failed - too many trails!\n" ));
		#endif
		return;
	}

	// Find next available trail
	mflashp = GET_FIRST(&Mflash_free_list);
	Assert( mflashp != &Mflash_free_list );		// shouldn't have the dummy element

	// remove trailp from the free list
	list_remove( &Mflash_free_list, mflashp );
	
	// insert trailp onto the end of used list
	list_append( &Mflash_used_list, mflashp );

	// store some stuff
	mflashp->type = (ubyte)mflash_type;	
	*/

	// create the actual animations	
	mi = &Mflash_info[mflash_type];

	if (local != NULL) {
		for (idx = 0; idx < mi->blobs.size(); idx++) {
			mbi = &mi->blobs[idx];

			// bogus anim
			if (mbi->anim_id < 0)
				continue;

			// fire it up
			memset(&p, 0, sizeof(particle_info));
			vm_vec_scale_add(&p.pos, gun_pos, gun_dir, mbi->offset);
			vm_vec_zero(&p.vel);
			//vm_vec_scale_add(&p.vel, &pip->rotvel, &pip->vel, 1.0f);
			p.rad = mbi->radius;
			p.type = PARTICLE_BITMAP;
			p.optional_data = mbi->anim_id;
			p.attached_objnum = OBJ_INDEX(local);
			p.attached_sig = local->signature;
			particle_create(&p);
		}
	} else {
		for (idx = 0; idx < mi->blobs.size(); idx++) {
			mbi = &mi->blobs[idx];

			// bogus anim
			if (mbi->anim_id < 0)
				continue;

			// fire it up
			memset(&p, 0, sizeof(particle_info));
			vm_vec_scale_add(&p.pos, gun_pos, gun_dir, mbi->offset);
			vm_vec_scale_add(&p.vel, &pip->rotvel, &pip->vel, 1.0f);
			p.rad = mbi->radius;
			p.type = PARTICLE_BITMAP;
			p.optional_data = mbi->anim_id;
			p.attached_objnum = -1;
			p.attached_sig = 0;
			particle_create(&p);
		}
	}

	// increment counter
	// Num_mflash++;		
}

// process muzzle flash stuff
void mflash_process_all()
{
	/*
	mflash *mflashp;

	// if the timestamp has elapsed recycle it
	mflashp = GET_FIRST(&Mflash_used_list);

	while ( mflashp!=END_OF_LIST(&Mflash_used_list) )	{			
		if((mflashp->stamp == -1) || timestamp_elapsed(mflashp->stamp)){
			// delete it from the list!
			mflash *next_one = GET_NEXT(mflashp);

			// remove objp from the used list
			list_remove( &Mflash_used_list, mflashp );

			// add objp to the end of the free
			list_append( &Mflash_free_list, mflashp );

			// decrement counter
			Num_mflash--;

			Assert(Num_mflash >= 0);
			
			mflashp = next_one;			
		} else {	
			mflashp = GET_NEXT(mflashp);
		}
	}
	*/
}

void mflash_render_all()
{
}

// lookup type by name
int mflash_lookup(char *name)
{	
	uint idx;

	// look it up
	for (idx = 0; idx < Mflash_info.size(); idx++) {
		if ( !stricmp(name, Mflash_info[idx].name) )
			return idx;
	}

	// couldn't find it
	return -1;	
}

/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 

/*
 * $Logfile: /Freespace2/code/Hud/HudArtillery.cpp $
 * $Revision: 2.14.2.2 $
 * $Date: 2007-09-02 02:07:41 $
 * $Author: Goober5000 $
 *
 *
 * $Log: not supported by cvs2svn $
 * Revision 2.14.2.1  2006/09/11 01:15:04  taylor
 * fixes for stuff_string() bounds checking
 *
 * Revision 2.14  2005/10/14 07:22:24  Goober5000
 * removed an unneeded parameter and renamed some stuff
 * --Goober5000
 *
 * Revision 2.13  2005/10/10 17:21:04  taylor
 * remove NO_NETWORK
 *
 * Revision 2.12  2005/07/22 10:18:38  Goober5000
 * CVS header tweaks
 * --Goober5000
 *
 * Revision 2.11  2005/07/13 03:15:51  Goober5000
 * remove PreProcDefine #includes in FS2
 * --Goober5000
 *
 * Revision 2.10  2005/04/05 05:53:17  taylor
 * s/vector/vec3d/g, better support for different compilers (Jens Granseuer)
 *
 * Revision 2.9  2005/03/25 06:57:34  wmcoolmon
 * Big, massive, codebase commit. I have not removed the old ai files as the ones I uploaded aren't up-to-date (But should work with the rest of the codebase)
 *
 * Revision 2.8  2005/03/02 21:24:44  taylor
 * more network/inferno goodness for Windows, takes care of a few warnings too
 *
 * Revision 2.7  2004/09/17 07:11:02  Goober5000
 * moved ssm stuff to header file so it would work in FRED
 * --Goober5000
 *
 * Revision 2.6  2004/08/23 04:00:15  Goober5000
 * ship-tag and ship-untag
 * --Goober5000
 *
 * Revision 2.5  2004/07/26 20:47:32  Kazan
 * remove MCD complete
 *
 * Revision 2.4  2004/07/12 16:32:49  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 2.3  2004/03/05 09:02:03  Goober5000
 * Uber pass at reducing #includes
 * --Goober5000
 *
 * Revision 2.2  2003/03/19 12:29:02  unknownplayer
 * Woohoo! Killed two birds with one stone!
 * Fixed the 'black screen around dialog boxes' problem and also the much more serious freezing problem experienced by Goober5000. It wasn't a crash, just an infinite loop. DX8 merge is GO! once again :)
 *
 * Revision 2.1  2002/08/01 01:41:05  penguin
 * The big include file move
 *
 * Revision 2.0  2002/06/03 04:02:23  penguin
 * Warpcore CVS sync
 *
 * Revision 1.3  2002/05/13 15:11:03  mharris
 * More NO_NETWORK ifndefs added
 *
 * Revision 1.2  2002/05/03 22:07:08  mharris
 * got some stuff to compile
 *
 * Revision 1.1  2002/05/02 18:03:08  mharris
 * Initial checkin - converted filenames and includes to lower case
 *  
 * 
 * 5     6/01/99 8:35p Dave
 * Finished lockarm weapons. Added proper supercap weapons/damage. Added
 * awacs-set-radius sexpression.
 * 
 * 4     4/28/99 11:36p Dave
 * Tweaked up subspace missile strike a bit,
 * 
 * 3     4/28/99 11:13p Dave
 * Temporary checkin of artillery code.
 * 
 * 2     4/20/99 6:39p Dave
 * Almost done with artillery targeting. Added support for downloading
 * images on the PXO screen.
 * 
 * 1     4/20/99 12:00a Dave
 * 
 * 
 * $NoKeywords: $
 */


#include "hud/hudartillery.h"
#include "parse/parselo.h"
#include "weapon/weapon.h"
#include "math/vecmat.h"
#include "globalincs/linklist.h"
#include "io/timer.h"
#include "fireball/fireballs.h"
#include "object/object.h"
#include "ai/ai.h"
#include "globalincs/alphacolors.h"
#include "network/multi.h"


// -----------------------------------------------------------------------------------------------------------------------
// ARTILLERY DEFINES/VARS
//
// Goober5000 - moved to hudartillery.h

// -----------------------------------------------------------------------------------------------------------------------
// ARTILLERY FUNCTIONS
//

// test code for subspace missile strike -------------------------------------------

// ssm_info, like ship_info etc.
int Ssm_info_count = 0;
ssm_info Ssm_info[MAX_SSM_TYPES];

// list of active/free strikes
int Num_ssm_strikes = 0;
ssm_strike Ssm_strikes[MAX_SSM_STRIKES];
ssm_strike Ssm_free_list;
ssm_strike Ssm_used_list;

// Goober5000
int ssm_info_lookup(char *name)
{
	if(name == NULL)
		return -1;

	for (int i = 0; i < Ssm_info_count; i++)
		if (!stricmp(name, Ssm_info[i].name))
			return i;

	return -1;
}

// game init
void ssm_init()
{	
	int rval;
	ssm_info bogus, *s;
	char weapon_name[NAME_LENGTH];

	if ((rval = setjmp(parse_abort)) != 0) {
		mprintf(("TABLES: Unable to parse '%s'!  Error code = %i.\n", "ssm.tbl", rval));
		return;
	}

	read_file_text("ssm.tbl");
	reset_parse();

	// parse the table
	Ssm_info_count = 0;
	while(!optional_string("#end")){
		// another ssm definition
		if(optional_string("$SSM:")){
			// pointer to info struct
			if(Ssm_info_count >= MAX_SSM_TYPES){
				s = &bogus;
			} else {
				s = &Ssm_info[Ssm_info_count];
			}

			// name
			stuff_string(s->name, F_NAME, NAME_LENGTH);

			// stuff data
			required_string("+Weapon:");
			stuff_string(weapon_name, F_NAME, NAME_LENGTH);
			required_string("+Count:");
			stuff_int(&s->count);
			required_string("+WarpRadius:");
			stuff_float(&s->warp_radius);
			required_string("+WarpTime:");
			stuff_float(&s->warp_time);
			required_string("+Radius:");
			stuff_float(&s->radius);
			required_string("+Offset:");
			stuff_float(&s->offset);

			// see if we have a valid weapon
			s->weapon_info_index = -1;
			s->weapon_info_index = weapon_name_lookup(weapon_name);
			if(s->weapon_info_index >= 0){
				// valid
				Ssm_info_count++;
			}
		}
	}
}

void ssm_get_random_start_pos(vec3d *out, vec3d *start, matrix *orient, int ssm_index)
{
	vec3d temp;
	ssm_info *s = &Ssm_info[ssm_index];

	// get a random vector in the circle of the firing plane
	vm_vec_random_in_circle(&temp, start, orient, s->radius, 1);

	// offset it a bit
	vm_vec_scale_add(out, &temp, &orient->vec.fvec, s->offset);
}

// level init
void ssm_level_init()
{
	int i;

	Num_ssm_strikes = 0;
	list_init( &Ssm_free_list );
	list_init( &Ssm_used_list );

	// Link all object slots into the free list
	for (i=0; i<MAX_SSM_STRIKES; i++)	{
		list_append(&Ssm_free_list, &Ssm_strikes[i] );
	}
}

// start a subspace missile effect
void ssm_create(object *target, vec3d *start, int ssm_index, ssm_firing_info *override, int team)
{	
	ssm_strike *ssm;		
	matrix dir;
	int idx;

	if (Num_ssm_strikes >= MAX_SSM_STRIKES ) {
		#ifndef NDEBUG
		mprintf(("Ssm creation failed - too many ssms!\n" ));
		#endif
		return;
	}

	// sanity
	Assert(target != NULL);
	if(target == NULL){
		return;
	}
	Assert(start != NULL);
	if(start == NULL){
		return;
	}
	if((ssm_index < 0) || (ssm_index >= MAX_SSM_TYPES)){
		return;
	}

	// Find next available trail
	ssm = GET_FIRST(&Ssm_free_list);
	Assert( ssm != &Ssm_free_list );		// shouldn't have the dummy element

	// remove trailp from the free list
	list_remove( &Ssm_free_list, ssm );
	
	// insert trailp onto the end of used list
	list_append( &Ssm_used_list, ssm );

	// increment counter
	Num_ssm_strikes++;	

	// Init the ssm data

	// override in multiplayer
	if(override != NULL){
		ssm->sinfo = *override;
	}
	// single player or the server
	else {
		// forward orientation
		vec3d temp;

        vm_vec_sub(&temp, &target->pos, start);
        vm_vec_normalize(&temp);

		vm_vector_2_matrix(&dir, &temp, NULL, NULL);

		// stuff info
		ssm->sinfo.ssm_index = ssm_index;
		ssm->sinfo.target = target;
        ssm->sinfo.ssm_team = team;

		for(idx=0; idx<Ssm_info[ssm_index].count; idx++){
			ssm->sinfo.delay_stamp[idx] = timestamp(200 + (int)frand_range(-199.0f, 1000.0f));
			ssm_get_random_start_pos(&ssm->sinfo.start_pos[idx], start, &dir, ssm_index);
		}

		// if we're the server, send a packet
		if(MULTIPLAYER_MASTER){
			//
		}
	}

	// clear timestamps, handles, etc
	for(idx=0; idx<MAX_SSM_COUNT; idx++){
		ssm->done_flags[idx] = 0;
		ssm->fireballs[idx] = -1;
	}
}

// delete a finished ssm effect
void ssm_delete(ssm_strike *ssm)
{
	// remove objp from the used list
	list_remove( &Ssm_used_list, ssm );

	// add objp to the end of the free
	list_append( &Ssm_free_list, ssm );

	// decrement counter
	Num_ssm_strikes--;

	nprintf(("General", "Recycling SSM, %d left", Num_ssm_strikes));
}

// process subspace missile stuff
void ssm_process()
{
	int idx, finished;
	ssm_strike *moveup, *next_one;
	ssm_info *si;
    int weapon_objnum;
	
	// process all strikes	
	moveup=GET_FIRST(&Ssm_used_list);
	while ( moveup!=END_OF_LIST(&Ssm_used_list) )	{		
		// get the type
		if(moveup->sinfo.ssm_index < 0){
			continue;
		}
		si = &Ssm_info[moveup->sinfo.ssm_index];

		// check all the individual missiles
		finished = 1;
		for(idx=0; idx<si->count; idx++){
			// if this guy is not marked as done
			if(!moveup->done_flags[idx]){
				finished = 0;				

				// if he already has the fireball effect
				if(moveup->fireballs[idx] >= 0){
					// if the warp effect is half done, fire the missile
					if((1.0f - fireball_lifeleft_percent(&Objects[moveup->fireballs[idx]])) >= 0.5f){
						// get an orientation
						vec3d temp;
						matrix orient;

						vm_vec_sub(&temp, &moveup->sinfo.target->pos, &moveup->sinfo.start_pos[idx]);
						vm_vec_normalize(&temp);
						vm_vector_2_matrix(&orient, &temp, NULL, NULL);

						// fire the missile and flash the screen
						weapon_objnum = weapon_create(&moveup->sinfo.start_pos[idx], &orient, si->weapon_info_index, -1, -1, 1);

                        Weapons[Objects[weapon_objnum].instance].team = moveup->sinfo.ssm_team;
                        Weapons[Objects[weapon_objnum].instance].homing_object = moveup->sinfo.target;
                        Weapons[Objects[weapon_objnum].instance].target_sig = moveup->sinfo.target->signature;

						// this makes this particular missile done
						moveup->done_flags[idx] = 1;
					}
				} 
				// maybe create his warpin effect
				else if((moveup->sinfo.delay_stamp[idx] >= 0) && timestamp_elapsed(moveup->sinfo.delay_stamp[idx])){
					// get an orientation
					vec3d temp;
					matrix orient;

                    vm_vec_sub(&temp, &moveup->sinfo.target->pos, &moveup->sinfo.start_pos[idx]);
					vm_vec_normalize(&temp);
					vm_vector_2_matrix(&orient, &temp, NULL, NULL);
					moveup->fireballs[idx] = fireball_create(&moveup->sinfo.start_pos[idx], FIREBALL_WARP_EFFECT, -1, si->warp_radius, 0, &vmd_zero_vector, si->warp_time, 0, &orient);
				}
			}
		}
		if(finished){
			next_one = GET_NEXT(moveup);			
			ssm_delete(moveup);															
			moveup = next_one;
			continue;
		}
		
		moveup=GET_NEXT(moveup);
	}	
}


// test code for subspace missile strike -------------------------------------------

// level init
void hud_init_artillery()
{
}

// update all hud artillery related stuff
void hud_artillery_update()
{
}

// render all hud artillery related stuff
void hud_artillery_render()
{
	// render how long the player has been painting his target	
	if((Player_ai != NULL) && (Player_ai->artillery_objnum >= 0)){
		gr_set_color_fast(&Color_bright_blue);
		gr_printf(10, 50, "%f", Player_ai->artillery_lock_time);
	}
}

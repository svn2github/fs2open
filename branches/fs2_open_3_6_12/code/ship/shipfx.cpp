/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/




#include "ship/shipfx.h"
#include "ship/ship.h"
#include "object/object.h"
#include "fireball/fireballs.h"
#include "debris/debris.h"
#include "weapon/weapon.h"
#include "gamesnd/gamesnd.h"
#include "io/timer.h"
#include "render/3d.h"			// needed for View_position, which is used when playing a 3D sound
#include "hud/hudmessage.h"
#include "math/fvi.h"
#include "gamesequence/gamesequence.h"
#include "lighting/lighting.h"
#include "globalincs/linklist.h"
#include "particle/particle.h"
#include "weapon/muzzleflash.h"
#include "demo/demo.h"
#include "ship/shiphit.h"
#include "object/objectsnd.h"
#include "playerman/player.h"
#include "weapon/shockwave.h"
#include "parse/parselo.h"
#include "object/objectdock.h"
#include "network/multi.h"
#include "network/multiutil.h"
#include "network/multimsgs.h"
#include "parse/scripting.h"
#include "asteroid/asteroid.h"



#ifndef NDEBUG
extern float flFrametime;
extern int Framecount;
#endif

extern int Cmdline_tbp;

#define SHIP_CANNON_BITMAP			"argh"
int Ship_cannon_bitmap = -1;

int Player_engine_wash_loop = -1;

extern float splode_level;

void shipfx_remove_submodel_ship_sparks(ship *shipp, int submodel_num)
{
	Assert(submodel_num != -1);

	// maybe no active sparks on submodel
	if (shipp->num_hits == 0) {
		return;
	}
	
	for (int spark_num=0; spark_num<shipp->num_hits; spark_num++) {
		if (shipp->sparks[spark_num].submodel_num == submodel_num) {
			shipp->sparks[spark_num].end_time = timestamp(1);
		}
	}
}

// Check if subsystem has live debris and create
// DKA: 5/26/99 make velocity of debris scale according to size of debris subobject (at least for large subobjects)
void shipfx_subsystem_maybe_create_live_debris(object *ship_obj, ship *ship_p, ship_subsys *subsys, vec3d *exp_center, float exp_mag)
{
	// initializations
	polymodel *pm = model_get(Ship_info[ship_p->ship_info_index].model_num);
	int submodel_num = subsys->system_info->subobj_num;
	submodel_instance_info *sii = &subsys->submodel_info_1;

	object *live_debris_obj;
	int i, num_live_debris, live_debris_submodel;

	// get number of live debris objects to create
	num_live_debris = pm->submodel[submodel_num].num_live_debris;
	if (num_live_debris <= 0) {
		return;
	}

	ship_model_start(ship_obj);

	// copy angles
	angles copy_angs = pm->submodel[submodel_num].angs;
	angles zero_angs = {0.0f, 0.0f, 0.0f};

	// make sure the axis point is set
	vec3d model_axis, world_axis, rotvel, world_axis_pt;
	matrix m_rot;	// rotation for debris orient about axis

	if(pm->submodel[submodel_num].movement_type == MOVEMENT_TYPE_ROT) {
		if ( !sii->axis_set ) {
			model_init_submodel_axis_pt(sii, pm->id, submodel_num);
		}

		// get the rotvel
		void model_get_rotating_submodel_axis(vec3d *model_axis, vec3d *world_axis, int modelnum, int submodel_num, object *obj);
		model_get_rotating_submodel_axis(&model_axis, &world_axis, pm->id, submodel_num, ship_obj);
		vm_vec_copy_scale(&rotvel, &world_axis, sii->cur_turn_rate);

		model_find_world_point(&world_axis_pt, &sii->pt_on_axis, pm->id, submodel_num, &ship_obj->orient, &ship_obj->pos);

		vm_quaternion_rotate(&m_rot, vm_vec_mag((vec3d*)&sii->angs), &model_axis);
	} else {
		//fix to allow non rotating submodels to use live debris
		vm_vec_zero(&rotvel);
		vm_set_identity(&m_rot);
		vm_vec_zero(&world_axis_pt);
	}

	// create live debris pieces
	for (i=0; i<num_live_debris; i++) {
		live_debris_submodel = pm->submodel[submodel_num].live_debris[i];
		vec3d start_world_pos, start_model_pos, end_world_pos;

		// get start world pos
		vm_vec_zero(&start_world_pos);
		model_find_world_point(&start_world_pos, &pm->submodel[live_debris_submodel].offset, pm->id, live_debris_submodel, &ship_obj->orient, &ship_obj->pos );

		// convert to model coord of underlying submodel
		// set angle to zero
		pm->submodel[submodel_num].angs = zero_angs;
		world_find_model_point(&start_model_pos, &start_world_pos, pm, submodel_num, &ship_obj->orient, &ship_obj->pos);

		// rotate from submodel coord to world coords
		// reset angle to current angle
		pm->submodel[submodel_num].angs = copy_angs;
		model_find_world_point(&end_world_pos, &start_model_pos, pm->id, submodel_num, &ship_obj->orient, &ship_obj->pos);

		int fireball_type = fireball_ship_explosion_type(&Ship_info[ship_p->ship_info_index]);
		if(fireball_type < 0) {
			fireball_type = FIREBALL_EXPLOSION_MEDIUM;
		}
		// create fireball here.
		fireball_create(&end_world_pos, fireball_type, FIREBALL_MEDIUM_EXPLOSION, OBJ_INDEX(ship_obj), pm->submodel[live_debris_submodel].rad);

		// create debris
		live_debris_obj = debris_create(ship_obj, pm->id, live_debris_submodel, &end_world_pos, exp_center, 1, exp_mag);

		// only do if debris is created
		if (live_debris_obj) {
			// get radial velocity of debris
			vec3d delta_x, radial_vel;
			vm_vec_sub(&delta_x, &end_world_pos, &world_axis_pt);
			vm_vec_crossprod(&radial_vel, &rotvel, &delta_x);

			if (Ship_info[ship_p->ship_info_index].flags & SIF_KNOSSOS_DEVICE) {
				// set velocity to cross center of knossos device
				vec3d rand_vec, vec_to_center;

				float vel_mag = vm_vec_mag_quick(&radial_vel) * 1.3f * (0.9f + 0.2f*frand());
				vm_vec_normalized_dir(&vec_to_center, &world_axis_pt, &end_world_pos);
				vm_vec_rand_vec_quick(&rand_vec);
				vm_vec_scale_add2(&vec_to_center, &rand_vec, 0.2f);
				vm_vec_scale_add2(&live_debris_obj->phys_info.vel, &vec_to_center, vel_mag);

			} else {
				// Get rotation of debris object
				matrix copy = live_debris_obj->orient;
				vm_matrix_x_matrix(&live_debris_obj->orient, &copy, &m_rot);

				// Add radial velocity (at least as large as exp velocity)
				vec3d temp_vel;	// explosion velocity with ship_obj velocity removed
				vm_vec_sub(&temp_vel, &live_debris_obj->phys_info.vel, &ship_obj->phys_info.vel);

				// find magnitudes of radial and temp velocity
				float vel_mag = vm_vec_mag(&temp_vel);
				float rotvel_mag = vm_vec_mag(&radial_vel);

				if (rotvel_mag > 0.1) {
					float scale = (1.2f + 0.2f * frand()) * vel_mag / rotvel_mag;
					// always add *at least* rotvel
					if (scale < 1) {
						scale = 1.0f;
					}

					if (exp_mag > 1) {	// whole ship going down
						scale = exp_mag;
					}

					if (Ship_info[ship_p->ship_info_index].flags & SIF_KNOSSOS_DEVICE) {
						scale = 1.0f;
					}

					vm_vec_scale_add2(&live_debris_obj->phys_info.vel, &radial_vel, scale);
				}

				// scale up speed of debris if ship_obj > 125, but not for knossos
				if (ship_obj->radius > 250 && !(Ship_info[ship_p->ship_info_index].flags & SIF_KNOSSOS_DEVICE)) {
					vm_vec_scale(&live_debris_obj->phys_info.vel, ship_obj->radius/250.0f);
				}
			}

			shipfx_debris_limit_speed(&Debris[live_debris_obj->instance], ship_p);
		}
	}

	ship_model_stop(ship_obj);
}

void set_ship_submodel_as_blown_off(ship *shipp, char *name)
{
	int found =	FALSE;

	// go through list of ship subsystems and find name
	ship_subsys	*pss = NULL;
	for (pss=GET_FIRST(&shipp->subsys_list); pss!=END_OF_LIST(&shipp->subsys_list); pss=GET_NEXT(pss)) {
		if ( subsystem_stricmp(pss->system_info->subobj_name, name) == 0) {
			found = TRUE;
			break;
		}
	}

	// set its blown off flag to TRUE
	Assert(found);
	if (found) {
		pss->submodel_info_1.blown_off = 1;
	}
}


// Create debris for ship submodel which has live debris (at ship death)
// when ship submodel has not already been blown off (and hence liberated live debris)
void shipfx_maybe_create_live_debris_at_ship_death( object *ship_obj )
{
	// if ship has live debris, detonate that subsystem now
	// search for any live debris

	ship *shipp = &Ships[ship_obj->instance];
	polymodel *pm = model_get(Ship_info[shipp->ship_info_index].model_num);

	// no subsystems -> no live debris.
	if (Ship_info[shipp->ship_info_index].n_subsystems == 0) {
		return;
	}

	int live_debris_submodel = -1;
	for (int idx=0; idx<pm->num_debris_objects; idx++) {
		if (pm->submodel[pm->debris_objects[idx]].is_live_debris) {
			live_debris_submodel = pm->debris_objects[idx];

			// get submodel that produces live debris
			int model_get_parent_submodel_for_live_debris( int model_num, int live_debris_model_num );
			int parent = model_get_parent_submodel_for_live_debris(pm->id, live_debris_submodel);
			Assert(parent != -1);

			// set model values only once (esp blown off)
			ship_model_start(ship_obj);

			// check if already blown off  (ship model set)
			if ( !pm->submodel[parent].blown_off ) {
		
				// get ship_subsys for live_debris
				// Go through all subsystems and look for submodel the subsystems with "parent" submodel.
				ship_subsys	*pss = NULL;
				for ( pss = GET_FIRST(&shipp->subsys_list); pss != END_OF_LIST(&shipp->subsys_list); pss = GET_NEXT(pss) ) {
					if (pss->system_info->subobj_num == parent) {
						break;
					}
				}

				Assert (pss != NULL);
				if (pss != NULL) {
					if (pss->system_info != NULL) {
						vec3d exp_center, tmp = ZERO_VECTOR;
						model_find_world_point(&exp_center, &tmp, pm->id, parent, &ship_obj->orient, &ship_obj->pos );

						// if not blown off, blow it off
						shipfx_subsystem_maybe_create_live_debris(ship_obj, shipp, pss, &exp_center, 3.0f);

						// now set subsystem as blown off, so we only get one copy
						pm->submodel[parent].blown_off = 1;
						set_ship_submodel_as_blown_off(&Ships[ship_obj->instance], pss->system_info->subobj_name);
					}
				}
			}
		}
	}

	// clean up
	ship_model_stop(ship_obj);

}

void shipfx_blow_off_subsystem(object *ship_obj,ship *ship_p,ship_subsys *subsys, vec3d *exp_center)
{
	vec3d subobj_pos;
	int model_num = Ship_info[ship_p->ship_info_index].model_num;

	model_subsystem	*psub = subsys->system_info;

	get_subsystem_world_pos(ship_obj, subsys, &subobj_pos);

/*
	if ( psub->turret_gun_sobj > -1 )
		debris_create( ship_obj, model_num, psub->turret_gun_sobj, &subobj_pos, exp_center, 0, 1.0f );

	if ( psub->subobj_num > -1 )
		debris_create( ship_obj, model_num, psub->subobj_num, &subobj_pos, exp_center, 0, 1.0f );
*/
	// get rid of sparks on submodel that is destroyed
	shipfx_remove_submodel_ship_sparks(ship_p, psub->subobj_num);

	// create debris shards
	shipfx_blow_up_model(ship_obj, model_num, psub->subobj_num, 50, &subobj_pos );

	// create live debris objects, if any
	// TODO:  some MULITPLAYER implcations here!!
	shipfx_subsystem_maybe_create_live_debris(ship_obj, ship_p, subsys, exp_center, 1.0f);
	
	int fireball_type = fireball_ship_explosion_type(&Ship_info[ship_p->ship_info_index]);
	if(fireball_type < 0) {
		fireball_type = FIREBALL_EXPLOSION_MEDIUM;
	}
	// create first fireball
	fireball_create( &subobj_pos, fireball_type, FIREBALL_MEDIUM_EXPLOSION, OBJ_INDEX(ship_obj), psub->radius );
}


void shipfx_blow_up_hull(object *obj, int model, vec3d *exp_center)
{
	int i;
	polymodel * pm;
	ushort sig_save;


	pm = model_get(model);
	if (!pm) return;

	// in multiplayer, send a debris_hull_create packet.  Save/restore the debris signature
	// when in misison only (since we can create debris pieces before mission starts)
	sig_save = 0;

	if ( (Game_mode & GM_MULTIPLAYER) && (Game_mode & GM_IN_MISSION) ) {
		sig_save = multi_get_next_network_signature( MULTI_SIG_DEBRIS );
		multi_set_network_signature( (ushort)(Ships[obj->instance].arrival_distance), MULTI_SIG_DEBRIS );
	}

	bool try_live_debris = true;
	for (i=0; i<pm->num_debris_objects; i++ )	{
		if (! pm->submodel[pm->debris_objects[i]].is_live_debris) {
			vec3d tmp = ZERO_VECTOR;		
			model_find_world_point(&tmp, &pm->submodel[pm->debris_objects[i]].offset, model, 0, &obj->orient, &obj->pos );
			debris_create( obj, model, pm->debris_objects[i], &tmp, exp_center, 1, 3.0f );
		} else {
			if ( try_live_debris ) {
				// only create live debris once
				// this creates *all* the live debris for *all* the currently live subsystems.
				try_live_debris = false;
				shipfx_maybe_create_live_debris_at_ship_death(obj);
			}
		}
	}

	// restore the ship signature to it's original value.
	if ( (Game_mode & GM_MULTIPLAYER) && (Game_mode & GM_IN_MISSION) ) {
		multi_set_network_signature( sig_save, MULTI_SIG_DEBRIS );
	}
}


// Creates "ndebris" pieces of debris on random verts of the the "submodel" in the 
// ship's model.
void shipfx_blow_up_model(object *obj,int model, int submodel, int ndebris, vec3d *exp_center )
{
	int i;

	// if in a multiplayer game -- seed the random number generator with a value that will be the same
	// on all clients in the game -- the net_signature of the object works nicely -- since doing so should
	// ensure that all pieces of debris will get scattered in same direction on all machines
	if ( Game_mode & GM_MULTIPLAYER )
		srand( obj->net_signature );

	// made a change to allow anyone but multiplayer client to blow up hull.  Clients will do it when
	// they get the create packet
	if ( submodel == 0 ) {
		shipfx_blow_up_hull(obj,model, exp_center );
	}

	for (i=0; i<ndebris; i++ )	{
		vec3d pnt1, pnt2;

		// Gets two random points on the surface of a submodel
		submodel_get_two_random_points(model, submodel, &pnt1, &pnt2 );

		vec3d tmp, outpnt;

		vm_vec_avg( &tmp, &pnt1, &pnt2 );
		model_find_world_point(&outpnt, &tmp, model,submodel, &obj->orient, &obj->pos );

		debris_create( obj, -1, -1, &outpnt, exp_center, 0, 1.0f );
	}
}


// =================================================
//          SHIP WARP IN EFFECT CODE
// =================================================


// Given an ship, find the radius of it as viewed from the front.
static float shipfx_calculate_effect_radius( object *objp, int warp_dir )
{
	float rad;

	// if docked, we need to calculate the overall cross-sectional radius around the z-axis (longitudinal axis)
	if (object_is_docked(objp))
	{
		rad = dock_calc_max_cross_sectional_radius_perpendicular_to_axis(objp, Z_AXIS);
	}
	// if it's not docked, we can save a lot of work by just using width and height
	else
	{
		//WMC - see if a radius was specified
		ship_info *sip = &Ship_info[Ships[objp->instance].ship_info_index];
		if(warp_dir == WD_WARP_IN && sip->warpin_radius > 0.0f)
		{
			return sip->warpin_radius;
		}
		else if(warp_dir == WD_WARP_OUT && sip->warpout_radius > 0.0f)
		{
			return sip->warpout_radius;
		}

		float w, h;
		polymodel *pm = model_get(Ship_info[Ships[objp->instance].ship_info_index].model_num);

		w = pm->maxs.xyz.x - pm->mins.xyz.x;
		h = pm->maxs.xyz.y - pm->mins.xyz.y;
	
		if ( w > h )
			rad = w / 2.0f;
		else
			rad = h / 2.0f;
	}

	return rad*3.0f;
}

// How long the stage 1 & stage 2 of warp in effect lasts.
// There are different times for small, medium, and large ships.
// The appropriate values are picked depending on the ship's
// radius.
#define SHIPFX_WARP_DELAY	(2.0f)		// time for warp effect to ramp up before ship moves into it.

// Give object objp, calculate how long it should take the
// ship to go through the warp effect and how fast the ship
// should go.   For reference,  capital ship of 2780m 
// should take 7 seconds to fly through.   Fighters of 30, 
// should take 1.5 seconds to fly through.

#define LARGEST_RAD 1390.0f 
#define LARGEST_RAD_TIME 7.0f

#define SMALLEST_RAD 15.0f
#define SMALLEST_RAD_TIME 1.5f

float shipfx_calculate_warp_time(object *objp, int warp_dir)
{
	if(objp->type == OBJ_SHIP)
	{
		ship_info *sip = &Ship_info[Ships[objp->instance].ship_info_index];

		//Warpin time defined
		if( (warp_dir == WD_WARP_IN) && (sip->warpin_time > 0.0f)) {
			return (float)sip->warpin_time/1000.0f;
		//Warpout time defined
		} else if( (warp_dir == WD_WARP_OUT) && (sip->warpout_time > 0.0f)) {
			return (float)sip->warpout_time/1000.0f;
		//Warpin defined
		} else if ( (warp_dir == WD_WARP_IN) && (sip->warpin_speed != 0.0f) ) {
			return ship_class_get_length(sip) / sip->warpin_speed;
		//Warpout defined
		} else if ( (warp_dir == WD_WARP_OUT) && (sip->warpout_speed != 0.0f) ) {
			return ship_class_get_length(sip) / sip->warpout_speed;
		//Player warpout defined
		} else if ( (warp_dir == WD_WARP_OUT) && (objp == Player_obj) && (sip->warpout_player_speed != 0.0f) ) {
			return ship_class_get_length(sip) / sip->warpout_player_speed;
		//Player warpout not defined
		} else if ( (warp_dir == WD_WARP_OUT) && (objp == Player_obj) ) {
			return ship_class_get_length(sip) / PLAYER_WARPOUT_SPEED;
		}

	}
	// Find rad_percent from 0 to 1, 0 being smallest ship, 1 being largest
	float rad_percent = (objp->radius-SMALLEST_RAD) / (LARGEST_RAD-SMALLEST_RAD);
	if ( rad_percent < 0.0f ) {
		rad_percent = 0.0f;
	} else if ( rad_percent > 1.0f )	{
		rad_percent = 1.0f;
	}
	float rad_time = rad_percent*(LARGEST_RAD_TIME-SMALLEST_RAD_TIME) + SMALLEST_RAD_TIME;

	return rad_time;
}

float shipfx_calculate_warp_dist(object *objp)
{
	float length;

	Assert(objp->type == OBJ_SHIP);
	if (objp->type != OBJ_SHIP) {
		length = 2.0f * objp->radius;
	} else {
		length = ship_class_get_length(&Ship_info[Ships[objp->instance].ship_info_index]);
	}
	return length;
}

// This is called to actually warp this object in
// after all the flashy fx are done, or if the flashy 
// fx don't work for some reason.
void shipfx_actually_warpin(ship *shipp, object *objp)
{
	//shipp->warp_stage = 0;

	shipp->flags &= (~SF_ARRIVING_STAGE_1);
	shipp->flags &= (~SF_ARRIVING_STAGE_2);

	// let physics in on it too.
	objp->phys_info.flags &= (~PF_WARP_IN);
}

// Validate reference_objnum
int shipfx_special_warp_objnum_valid(int objnum)
{
	object *special_objp;

	// must be a valid object
	if ((objnum < 0) || (objnum >= MAX_OBJECTS))
		return 0;

	special_objp = &Objects[objnum];

	// must be a ship
	if (special_objp->type != OBJ_SHIP)
		return 0;

	// must be a knossos
	if (!(Ship_info[Ships[special_objp->instance].ship_info_index].flags & SIF_KNOSSOS_DEVICE))
		return 0;

	return 1;
}

// JAS - code to start the ship doing the warp in effect
// This also starts the animating 3d effect playing.
// There are two modes, stage 1 and stage 2.   Stage 1 is
// when the ship just invisibly waits for a certain time
// period after the effect starts, and then stage 2 begins,
// where the ships flies through the effect at a set
// velocity so it gets through in a certain amount of
// time.
void shipfx_warpin_start( object *objp )
{
	//float effect_time, effect_radius;
	ship *shipp = &Ships[objp->instance];

	if (shipp->flags & SF_ARRIVING)
	{
		mprintf(( "Ship is already arriving!\n" ));
		Int3();
		return;
	}

	// post a warpin event
	if(Game_mode & GM_DEMO_RECORD)
	{
		demo_POST_warpin(objp->signature, shipp->flags);
	}

	// docked ships who are not dock leaders don't use the warp effect code
	// (the dock leader takes care of the whole group)
	if (object_is_docked(objp) && !(shipp->flags & SF_DOCK_LEADER))
	{
		return;
	}

	//WMC - Check if scripting handles this.
	Script_system.SetHookObject("Self", objp);
	if(Script_system.IsConditionOverride(CHA_WARPIN, objp))
	{
		Script_system.RunCondition(CHA_WARPIN, 0, NULL, objp);
		Script_system.RemHookVar("Self");
		return;
	}

	// if there is no arrival warp, then skip the whole thing
	if (shipp->flags & SF_NO_ARRIVAL_WARP)
	{
		shipfx_actually_warpin(shipp,objp);
		return;
	}

	shipp->warpin_effect->warpStart();
	/*
	shipp->warp_effect_fvec = objp->orient.vec.fvec;

	// maybe special warpin
	if(sip->warpin_type == WT_DEFAULT)
	{
		// VALIDATE special_warpin_objnum
		if (shipp->special_warpin_objnum >= 0)
		{
			if (!shipfx_special_warp_objnum_valid(shipp->special_warpin_objnum))
			{
				shipp->special_warpin_objnum = -1;
			}
		}

		// only move warp effect pos if not special warp in.
		if (shipp->special_warpin_objnum >= 0)
		{
			Assert(!(Game_mode & GM_MULTIPLAYER));
			polymodel *pm;
			pm = model_get(sip->model_num);
			vm_vec_scale_add(&shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, -pm->mins.xyz.z);
		}
		else
		{
			vm_vec_scale_add( &shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, objp->radius );
		}


		// start the warp-in effect here ------------------------------------------------------------

		
		// Effect time is 'SHIPFX_WARP_DELAY' (1.5 secs) seconds to start, 'shipfx_calculate_warp_time' 
		// for ship to go thru, and 'SHIPFX_WARP_DELAY' (1.5 secs) to go away.
		effect_time = shipfx_calculate_warp_time(objp, WD_WARP_IN) + SHIPFX_WARP_DELAY + SHIPFX_WARP_DELAY;
		effect_radius = shipfx_calculate_effect_radius(objp, WD_WARP_IN);

		int warp_objnum = -1;
		if (shipp->special_warpin_objnum >= 0)
		{
			// cap radius to size of knossos
			effect_radius = MIN(effect_radius, 0.8f*Objects[shipp->special_warpin_objnum].radius);

			// make sure that the warp effect will always have a forward orient facing the exit direction of the warpin ship - taylor
			matrix knossos_orient = Objects[shipp->special_warpin_objnum].orient;
			if ( (knossos_orient.vec.fvec.xyz.z < 0.0f) && (objp->orient.vec.fvec.xyz.z > 0.0f) ||
				(knossos_orient.vec.fvec.xyz.z > 0.0f) && (objp->orient.vec.fvec.xyz.z < 0.0f) )
			{
				knossos_orient.vec.uvec.xyz.y = -knossos_orient.vec.uvec.xyz.y;
				knossos_orient.vec.fvec.xyz.z = -knossos_orient.vec.fvec.xyz.z;
			}

			warp_objnum = fireball_create(&shipp->warp_effect_pos, FIREBALL_KNOSSOS, FIREBALL_WARP_EFFECT, shipp->special_warpin_objnum, effect_radius, 0, NULL, effect_time, shipp->ship_info_index, &knossos_orient);
		}
		else
		{
			warp_objnum = fireball_create(&shipp->warp_effect_pos, FIREBALL_WARP, FIREBALL_WARP_EFFECT, OBJ_INDEX(objp), effect_radius, 0, NULL, effect_time, shipp->ship_info_index);
		}

		//WMC - bail
		// JAS: This must always be created, if not, just warp the ship in
		if (warp_objnum < 0)
		{
			shipfx_actually_warpin(shipp,objp);
			return;
		}

		// maybe negate if special warp effect
		if (shipp->special_warpin_objnum >= 0)
		{
			if (vm_vec_dotprod(&shipp->warp_effect_fvec, &objp->orient.vec.fvec) < 0)
			{
				vm_vec_negate(&shipp->warp_effect_fvec);
			}
		}
	}
	else if (sip->warpin_type == WT_IN_PLACE_ANIM || sip->warpin_type == WT_SWEEPER)
	{
		//Set radius
		shipp->warp_radius = sip->warpin_radius;
		if(shipp->warp_radius <= 0.0f)
			shipp->warp_radius = objp->radius;

		if(shipp->warp_anim < 0)
			shipp->warp_anim = bm_load_either(sip->warpin_anim, &shipp->warp_anim_nframes, &shipp->warp_anim_fps, 1);

		//WMC - bail
		if (shipp->warp_anim < 0)
		{
			shipfx_actually_warpin(shipp,objp);
			return;
		}

		if(sip->warpin_type == WT_SWEEPER)
		{
			vm_vec_scale_add(&shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, objp->radius);
		}
	}
	else
	{
		//WMC - bail
		shipfx_actually_warpin(shipp,objp);
		return;
	}

	// flag as warping
	if(sip->warpin_type == WT_IN_PLACE_ANIM) {
		shipp->start_warp_time = timestamp();
		int total_time = fl2i(((float)shipp->warp_anim_nframes / (float)shipp->warp_anim_fps) * 1000.0f);
		shipp->final_warp_time = timestamp(total_time);
		shipp->flags |= SF_ARRIVING_STAGE_1;
	} else if(sip->warpin_type == WT_SWEEPER) {
		shipp->warp_stage = 1;
		shipp->warp_height = 0;
		shipp->warp_width = shipp->warp_radius;
		shipp->start_warp_time = timestamp();
		shipp->final_warp_time = timestamp(500);
		shipp->flags |= SF_ARRIVING_STAGE_1;
	} else {
		shipp->final_warp_time = timestamp(fl2i(SHIPFX_WARP_DELAY*1000.0f));
		shipp->flags |= SF_ARRIVING_STAGE_1;
	}
	*/
	Script_system.RunCondition(CHA_WARPIN, 0, NULL, objp);
	Script_system.RemHookVar("Self");
}

void shipfx_warpin_frame( object *objp, float frametime )
{
	ship *shipp;

	shipp = &Ships[objp->instance];

	if ( shipp->flags & SF_DYING ) return;

	shipp->warpin_effect->warpFrame(frametime);
	/*
	if(sip->warpin_type == WT_DEFAULT)
	{
		if ( shipp->flags & SF_ARRIVING_STAGE_1 )	{
			if ( timestamp_elapsed(shipp->final_warp_time) ) {

				// let physics know the ship is going to warp in.
				objp->phys_info.flags |= PF_WARP_IN;

				// done doing stage 1 of warp, so go on to stage 2
				shipp->flags &= (~SF_ARRIVING_STAGE_1);
				shipp->flags |= SF_ARRIVING_STAGE_2;

				float warp_time = shipfx_calculate_warp_time(objp, WD_WARP_IN);
				float speed = shipfx_calculate_warp_dist(objp) / warp_time;		// How long it takes to move through warp effect

				// Make ship move at velocity so that it moves two radii in warp_time seconds.
				vec3d vel;
				vel = objp->orient.vec.fvec;
				vm_vec_scale( &vel, speed );
				objp->phys_info.vel = vel;
				objp->phys_info.desired_vel = vel;
				objp->phys_info.prev_ramp_vel.xyz.x = 0.0f;
				objp->phys_info.prev_ramp_vel.xyz.y = 0.0f;
				objp->phys_info.prev_ramp_vel.xyz.z = speed;
				objp->phys_info.forward_thrust = 0.0f;		// How much the forward thruster is applied.  0-1.

				shipp->final_warp_time = timestamp(fl2i(warp_time*1000.0f));
			} 
		} else if ( shipp->flags & SF_ARRIVING_STAGE_2 )	{
			if ( timestamp_elapsed(shipp->final_warp_time) ) {
				// done doing stage 2 of warp, so turn off arriving flag
				shipfx_actually_warpin(shipp,objp);

				// notify physics to slow down
				if (Ship_info[shipp->ship_info_index].flags & SIF_SUPERCAP) {
					// let physics know this is a special warp in
					objp->phys_info.flags |= PF_SPECIAL_WARP_IN;
				}
			}
		}
	}
	else if(sip->warpin_type == WT_IN_PLACE_ANIM)
	{
		//WMC - This is handled by code in ship_render

		//WMC - ship appears after warpin_speed milliseconds
		if ( timestamp_elapsed(shipp->start_warp_time + sip->warpin_time )) {
			shipfx_actually_warpin(shipp,objp);
		}
	}
	else if(sip->warpin_type == WT_SWEEPER)
	{
		if ( timestamp_elapsed(shipp->final_warp_time )) {
			shipp->warp_stage++;
			shipp->start_warp_time = timestamp();
			if(shipp->warp_stage == 2)
			{
				objp->phys_info.flags |= PF_WARP_IN;
				shipp->flags &= (~SF_ARRIVING_STAGE_1);
				shipp->flags |= SF_ARRIVING_STAGE_2;

				//obj_snd_assign(OBJ_INDEX(objp), SND_BEAM_LOOP, &vmd_zero_vector, 1);

				shipp->final_warp_time = timestamp(sip->warpout_time);
			}
			else if(shipp->warp_stage == 3)
			{
				shipp->final_warp_time = timestamp(500);

				//snd_play_3d( &Snds[SND_CAPITAL_WARP_IN], &objp->pos, &View_position, shipp->warp_radius);
			}
		}
		float progress = ((float)timestamp() - (float)shipp->start_warp_time)/((float)shipp->final_warp_time - (float)shipp->start_warp_time);
		if(shipp->warp_stage == 1)
		{
			shipp->warp_height = shipp->warp_radius * progress;
		}
		else if(shipp->warp_stage == 2)
		{
			vm_vec_scale_add(&shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, shipp->warp_radius*2.0f*(0.5f-progress));
		}
		else if(shipp->warp_stage == 3)
		{
			shipp->warp_height = shipp->warp_radius * (1.0f-progress);
		}
		else
		{
			//obj_snd_delete_type(OBJ_INDEX(objp), -1, NULL);
			shipfx_actually_warpin(shipp, objp);
		}
	}
	*/
}
 
// This is called to actually warp this object out
// after all the flashy fx are done, or if the flashy fx
// don't work for some reason.  OR to skip the flashy fx.
void shipfx_actually_warpout(int shipnum)
{
	//Ships[shipnum].warp_stage = 0;
	// Once we get through effect, make the ship go away
	ship_actually_depart(shipnum);
}

// compute_special_warpout_stuff();
int compute_special_warpout_stuff(object *objp, float *speed, float *warp_time, vec3d *warp_pos)
{
	object	*sp_objp = NULL;
	ship_info	*sip;
	int		valid_reference_ship = FALSE, ref_objnum;
	vec3d	facing_normal, vec_to_knossos, center_pos;
	float		dist_to_plane;

	// knossos warpout only valid in single player
	if (Game_mode & GM_MULTIPLAYER) {
		mprintf(("special warpout only for single player\n"));
		return -1;
	}

	// find special warp ship reference
	valid_reference_ship = FALSE;
	ref_objnum = Ships[objp->instance].special_warpout_objnum;



	// Validate reference_objnum
	if ((ref_objnum >= 0) && (ref_objnum < MAX_OBJECTS)) {
		sp_objp = &Objects[ref_objnum];
		if (sp_objp->type == OBJ_SHIP) {
			if (Ship_info[Ships[sp_objp->instance].ship_info_index].flags & SIF_KNOSSOS_DEVICE) {
				valid_reference_ship = TRUE;
			}
		}
	}
	
	if (!valid_reference_ship) {
		Int3();
		mprintf(("special warpout reference ship not found\n"));
		return -1;
	}
	sip = &Ship_info[Ships[objp->instance].ship_info_index];

	
	//find the actual center of the model
	polymodel *pm = model_get(sip->model_num);
	vm_vec_add(&center_pos, &objp->pos, &pm->autocenter);

	// get facing normal from knossos
	vm_vec_sub(&vec_to_knossos, &sp_objp->pos, &center_pos);
	facing_normal = sp_objp->orient.vec.fvec;
	if (vm_vec_dotprod(&vec_to_knossos, &sp_objp->orient.vec.fvec) > 0) {
		vm_vec_negate(&facing_normal);
	}

	// find position to play the warp ani..
	dist_to_plane = fvi_ray_plane(warp_pos, &sp_objp->pos, &facing_normal, &center_pos, &objp->orient.vec.fvec, 0.0f);

	// calculate distance to warpout point.
	dist_to_plane += pm->mins.xyz.z;


	if (dist_to_plane < 0) {
		mprintf(("warpout started too late\n"));
		return -1;
	}

	// validate angle
	float max_warpout_angle = 0.707f;	// 45 degree half-angle cone for small ships
	if (Ship_info[Ships[objp->instance].ship_info_index].flags & (SIF_BIG_SHIP | SIF_HUGE_SHIP)) {
		max_warpout_angle = 0.866f;	// 30 degree half-angle cone for BIG or HUGE
	}

	if (-vm_vec_dotprod(&objp->orient.vec.fvec, &facing_normal) < max_warpout_angle) {	// within allowed angle
		Int3();
		mprintf(("special warpout angle exceeded\n"));
		return -1;
	}
	
	// Calculate how long to fly through the effect.  Not to get to the effect, just through it.
	*warp_time = shipfx_calculate_warp_time(objp, WD_WARP_OUT);

	// Calculate speed to fly through
	//*speed = shipfx_calculate_warp_dist(objp) / *warp_time;
	//WMC - use the function
	*speed = ship_get_warpout_speed(objp);
	
	// increase time, because we have extra distance to cover
	*warp_time += dist_to_plane / *speed;

	return 0;
}


void compute_warpout_stuff(object *objp, float *speed, float *warp_time, vec3d *warp_pos)
{
	Assert(objp);	// Goober5000
	ship *shipp = &Ships[objp->instance];
	ship_info *sip = &Ship_info[shipp->ship_info_index];

	// If we're warping through the knossos, do something different.
	if (shipp->special_warpout_objnum >= 0) {
		if (compute_special_warpout_stuff(objp, speed, warp_time, warp_pos) != -1) {
			return;
		} else {
			mprintf(("Invalid special warp\n"));
		}
	}

	float ship_move_dist, warp_dist;
	vec3d	center_pos;


	// Calculate how long to fly through the effect.  Not to get to the effect, just through it.
	*warp_time = shipfx_calculate_warp_time(objp, WD_WARP_OUT);

	// Pick some speed at which we want to go through the warp effect.  
	//*speed = shipfx_calculate_warp_dist(objp) / *warp_time;
	//WMC - Use the function
	*speed = ship_get_warpout_speed(objp);

	if ( objp == Player_obj )	{
		// Goober5000 - cap at 65
		*speed = MIN(0.8f*objp->phys_info.max_vel.xyz.z, 65.0f);
	}

	// center the effect properly
	if (object_is_docked(objp))
		dock_calc_docked_center(&center_pos, objp);
	else
	{
		polymodel *pm = model_get(sip->model_num);
		vm_vec_unrotate(&center_pos, &pm->autocenter, &objp->orient);
		vm_vec_add2(&center_pos,&objp->pos);
	}


	// If this is a huge ship, set the distance to the length of the ship
	if (sip->flags & SIF_HUGE_SHIP)
	{
		ship_move_dist = 0.5f * ship_class_get_length(sip);
	}
	else
	{
		float radius;

		if (object_is_docked(objp))
		{
			// we need to get the longitudinal radius of our ship, so find the semilatus rectum along the Z-axis
			radius = dock_calc_max_semilatus_rectum_parallel_to_axis(objp, Z_AXIS);
		}
		else
			radius = objp->radius;

		// Now we know our speed. Figure out how far the warp effect will be from here.  
		ship_move_dist = (*speed * SHIPFX_WARP_DELAY) + radius*1.5f;		// We want to get to 1.5R away from effect
		if ( ship_move_dist < radius*1.5f ) {
			ship_move_dist = radius*1.5f;
		}
	}


	// Acount for time to get to warp effect, before we actually go through it.
	*warp_time += ship_move_dist / *speed;

	warp_dist = ship_move_dist;

	// allow for off center
	if (sip->flags & SIF_HUGE_SHIP) {
		polymodel *pm = model_get(sip->model_num);
		warp_dist -= pm->mins.xyz.z;
	}

	vm_vec_scale_add( warp_pos, &center_pos, &objp->orient.vec.fvec, warp_dist );
}

// JAS - code to start the ship doing the warp out effect
// This puts the ship into a mode specified by SF_DEPARTING
// where it flies forward for a set time period at a set
// velocity, then disappears when that time is reached.  This
// also starts the animating 3d effect playing.
void shipfx_warpout_start( object *objp )
{
	//float warp_time;
	ship *shipp;
	shipp = &Ships[objp->instance];

	if ( 	shipp->flags & SF_DEPART_WARP )	{
		mprintf(( "Ship is already departing!\n" ));
		return;
	}

	Script_system.SetHookObject("Self", objp);
	if(Script_system.IsConditionOverride(CHA_WARPOUT, objp))
	{
		Script_system.RunCondition(CHA_WARPOUT, 0, NULL, objp);
		Script_system.RemHookVar("Self");
		return;
	}

	// if we're dying return
	if ( shipp->flags & SF_DYING ) {
		return;
	}

	//return if disabled
	if ( shipp->flags & SF_DISABLED ){
		return;
	}

	// if we're HUGE, keep alive - set guardian
	if (Ship_info[shipp->ship_info_index].flags & SIF_HUGE_SHIP) {
		shipp->ship_guardian_threshold = SHIP_GUARDIAN_THRESHOLD_DEFAULT;
	}

	// post a warpin event
	if(Game_mode & GM_DEMO_RECORD){
		demo_POST_warpout(objp->signature, shipp->flags);
	}

	// don't send ship depart packets for player ships
	if ( (MULTIPLAYER_MASTER) && !(objp->flags & OF_PLAYER_SHIP) ){
		send_ship_depart_packet( objp );
	}

	// don't do departure wormhole if ship flag is set which indicates no effect
	if ( shipp->flags & SF_NO_DEPARTURE_WARP ) {
		// DKA 5/25/99 If he's going to warpout, set it.  
		// Next line fixes assert in wing cleanup code when no warp effect.
		shipp->flags |= SF_DEPART_WARP;

		shipfx_actually_warpout(objp->instance);
		return;
	}

	shipp->warpout_effect->warpStart();
	/*
	if(sip->warpout_type == WT_IN_PLACE_ANIM || sip->warpout_type == WT_SWEEPER)
	{
		shipp->warp_radius = sip->warpout_radius;
		if(shipp->warp_radius <= 0.0f)
			shipp->warp_radius = objp->radius;
	}
	if(sip->warpout_type == WT_DEFAULT)
	{
		if ( objp == Player_obj )	{
	// changed by Goober5000 to be more accurate
	//		HUD_printf(XSTR( "Subspace node activated", 498) );
			HUD_printf(NOX("Subspace drive engaged"));
		}

		float	speed, effect_time, effect_radius;
		vec3d	warp_pos;
		// warp time from compute warpout stuff includes time to get up to warp_pos
		compute_warpout_stuff(objp, &speed, &warp_time, &warp_pos);
		shipp->warp_effect_pos = warp_pos;

		// The ending one means this is a warp-out effect
		int warp_objnum = -1;
		// Effect time is 'SHIPFX_WARP_DELAY' (1.5 secs) seconds to start, 'shipfx_calculate_warp_time' 
		// for ship to go thru, and 'SHIPFX_WARP_DELAY' (1.5 secs) to go away.
		// effect_time = shipfx_calculate_warp_time(objp) + SHIPFX_WARP_DELAY + SHIPFX_WARP_DELAY;
		effect_time = warp_time + SHIPFX_WARP_DELAY;
		effect_radius = shipfx_calculate_effect_radius(objp, WD_WARP_IN);

		// maybe special warpout
		if (shipp->special_warpout_objnum >= 0) {
			// cap radius to size of knossos
			effect_radius = MIN(effect_radius, 0.8f*Objects[shipp->special_warpout_objnum].radius);
			warp_objnum = fireball_create(&shipp->warp_effect_pos, FIREBALL_KNOSSOS, FIREBALL_WARP_EFFECT, shipp->special_warpout_objnum, effect_radius, 1, NULL, effect_time, shipp->ship_info_index);
		} else {

			// * For now, all the effects flag does is to use our orange effect when warping out    -Et1
			if( Cmdline_tbp )
			{
				warp_objnum = fireball_create(&shipp->warp_effect_pos, FIREBALL_KNOSSOS, FIREBALL_WARP_EFFECT, OBJ_INDEX(objp), effect_radius, 1, NULL, effect_time, shipp->ship_info_index);
			}
			else
			{
				warp_objnum = fireball_create(&shipp->warp_effect_pos, FIREBALL_WARP, FIREBALL_WARP_EFFECT, OBJ_INDEX(objp), effect_radius, 1, NULL, effect_time, shipp->ship_info_index);
			}
		}
		if (warp_objnum < 0 )	{	// JAS: This must always be created, if not, just warp the ship out
			shipfx_actually_warpout(objp->instance);
			return;
		}

		shipp->warp_effect_fvec = Objects[warp_objnum].orient.vec.fvec;
		// maybe negate if special warp effect
		if (shipp->special_warpout_objnum >= 0) {
			if (vm_vec_dotprod(&shipp->warp_effect_fvec, &objp->orient.vec.fvec) > 0) {
				vm_vec_negate(&shipp->warp_effect_fvec);
			}
		}

		// Make the warp effect stage 1 last SHIP_WARP_TIME1 seconds.
		if ( objp == Player_obj )	{
			warp_time = fireball_lifeleft(&Objects[warp_objnum]);
			shipp->final_warp_time = timestamp(fl2i(warp_time*1000.0f));
		} else {
			shipp->final_warp_time = timestamp(fl2i(warp_time*2.0f*1000.0f));
		}
		shipp->flags |= SF_DEPART_WARP;

	//	mprintf(( "Warp time = %.4f , effect time = %.4f ms\n", warp_time*1000.0f, effect_time ));

		// This is a hack to make the ship go at the right speed to go from it's current position to the warp_effect_pos;
		
		// Set ship's velocity to 'speed'
		// This should actually be an AI that flies from the current
		// position through 'shipp->warp_effect_pos' in 'warp_time'
		// and keeps going 
		if ( objp != Player_obj || Player_use_ai )	{
			vec3d vel;
			vel = objp->orient.vec.fvec;
			vm_vec_scale( &vel, speed );
			objp->phys_info.vel = vel;
			objp->phys_info.desired_vel = vel;
			objp->phys_info.prev_ramp_vel.xyz.x = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.y = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.z = speed;
			objp->phys_info.forward_thrust = 1.0f;		// How much the forward thruster is applied.  0-1.

			// special case for HUGE ships
			if (Ship_info[shipp->ship_info_index].flags & SIF_HUGE_SHIP) {
	//			objp->phys_info.flags |= PF_SPECIAL_WARP_OUT;
			}
		}
	}
	else if(sip->warpout_type == WT_IN_PLACE_ANIM)
	{
		shipp->warp_anim = bm_load_either(sip->warpout_anim, &shipp->warp_anim_nframes, &shipp->warp_anim_fps, 1);
		shipp->start_warp_time = timestamp();
		int total_time = fl2i(((float)shipp->warp_anim_nframes / (float)shipp->warp_anim_fps) * 1000.0f);
		shipp->final_warp_time = timestamp(total_time);
		shipp->flags |= SF_DEPART_WARP;
	}
	else if(sip->warpout_type == WT_SWEEPER)
	{
		shipp->warp_stage = 1;

		shipp->warp_height = 0.0f;
		shipp->warp_width = shipp->warp_radius;
		shipp->warp_anim = bm_load_either(sip->warpout_anim, &shipp->warp_anim_nframes, &shipp->warp_anim_fps, 1);

		vm_vec_scale_add(&shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, objp->radius);
		shipp->warp_effect_fvec = objp->orient.vec.fvec;
		vm_vec_negate(&shipp->warp_effect_fvec);

		shipp->start_warp_time = timestamp();
		shipp->final_warp_time = timestamp(500);
		shipp->flags |= SF_DEPART_WARP;
	}
	else
	{
		shipfx_actually_warpout(objp->instance);
	}
	*/
	Script_system.RunCondition(CHA_WARPOUT, 0, NULL, objp);
	Script_system.RemHookVar("Self");
}

void shipfx_warpout_frame( object *objp, float frametime )
{
	ship *shipp;
	shipp = &Ships[objp->instance];

	if ( shipp->flags & SF_DYING ) return;

	//disabled ships should stay on the battlefield if they were disabled during warpout
	//phreak 5/22/03
	if (shipp->flags & SF_DISABLED){
		shipp->flags &= ~(SF_DEPARTING);
		return;
	}

	shipp->warpout_effect->warpFrame(frametime);
	/*
	if(sip->warpout_type == WT_DEFAULT)
	{
		vec3d tempv;
		float warp_pos;	// position of warp effect in object's frame of reference

		vm_vec_sub( &tempv, &objp->pos, &shipp->warp_effect_pos );
		warp_pos = -vm_vec_dot( &tempv, &shipp->warp_effect_fvec );


		// Find the closest point on line from center of wormhole
		vec3d pos;
		float dist;

		fvi_ray_plane(&pos,&objp->pos,&shipp->warp_effect_fvec,&shipp->warp_effect_pos, &shipp->warp_effect_fvec, 0.0f );
		dist = vm_vec_dist( &pos, &objp->pos );

	//	mprintf(( "Warp pos = %.1f, rad=%.1f, center dist = %.1f\n", warp_pos, objp->radius, dist ));

		if ( objp == Player_obj )	{
			// Code for player warpout frame

			if ( (Player->control_mode==PCM_WARPOUT_STAGE2) && (warp_pos > objp->radius) )	{
				gameseq_post_event( GS_EVENT_PLAYER_WARPOUT_DONE_STAGE2 );
			}

			if ( timestamp_elapsed(shipp->final_warp_time) ) {

				// Something went wrong... oh well, warp him out anyway.
				if ( Player->control_mode != PCM_WARPOUT_STAGE3 )	{
					mprintf(( "Hmmm... player ship warpout time elapsed, but he wasn't in warp stage 3.\n" ));
				}

				shipfx_actually_warpout(objp->instance);
			}

		} else {
			// Code for all non-player ships warpout frame

			int timed_out = timestamp_elapsed(shipp->final_warp_time);
			if ( timed_out )	{
	//			mprintf(("Frame %i: Ship %s missed departue cue.\n", Framecount, shipp->ship_name ));
				int	delta_ms = timestamp_until(shipp->final_warp_time);
				if (delta_ms > 1000.0f * frametime ) {
					nprintf(("AI", "Frame %i: Ship %s missed departue cue by %7.3f seconds.\n", Framecount, shipp->ship_name, - (float) delta_ms/1000.0f));
				}
			}

			// MWA 10/21/97 -- added shipp->flags & SF_NO_DEPARTURE_WARP part of next if statement.  For ships
			// that don't get a wormhole effect, I wanted to drop into this code immediately.
			if ( (warp_pos > objp->radius)  || (shipp->flags & SF_NO_DEPARTURE_WARP) || timed_out )	{
				shipfx_actually_warpout(objp->instance);
			} 
		}
	}
	else if(sip->warpout_type == WT_IN_PLACE_ANIM)
	{
		//WMC - This is handled by code in ship_render

		//WMC - ship appears after warpout_time milliseconds
		if ( timestamp_elapsed(shipp->start_warp_time + sip->warpout_time )) {
			shipfx_actually_warpout(objp->instance);
		}
	}
	else if(sip->warpout_type == WT_SWEEPER)
	{
		if ( timestamp_elapsed(shipp->final_warp_time )) {
			shipp->warp_stage++;
			shipp->start_warp_time = timestamp();
			if(shipp->warp_stage == 2)
				shipp->final_warp_time = timestamp(sip->warpout_time);
			else if(shipp->warp_stage == 3)
				shipp->final_warp_time = timestamp(500);
		}
		float progress = ((float)timestamp() - (float)shipp->start_warp_time)/((float)shipp->final_warp_time - (float)shipp->start_warp_time);
		if(shipp->warp_stage == 1)
		{
			shipp->warp_height = shipp->warp_radius * progress;
		}
		else if(shipp->warp_stage == 2)
		{
			vm_vec_scale_add(&shipp->warp_effect_pos, &objp->pos, &objp->orient.vec.fvec, shipp->warp_radius*2.0f*(0.5f-progress));
		}
		else if(shipp->warp_stage == 3)
		{
			shipp->warp_height = shipp->warp_radius * (1.0f-progress);
		}
		else
		{
			shipfx_actually_warpout(objp->instance);
		}
	}
	*/
}


//==================================================
// Stuff for keeping track of which ships are in
// whose shadows.


// Given point p0, in object's frame of reference, find if 
// it can see the sun.
int shipfx_point_in_shadow( vec3d *p0, matrix *src_orient, vec3d *src_pos, float radius )
{
	mc_info mc;
	object *objp;
	ship_obj *so;
	int n_lights;
	int idx;

	vec3d rp0, rp1;

	vec3d light_dir;

	// Move rp0 into world coordinates	
	vm_vec_unrotate(&rp0, p0, src_orient);
	vm_vec_add2(&rp0, src_pos);

	// get the # of global lights
	n_lights = light_get_global_count();
	
	for(idx=0; idx<n_lights; idx++){
		// get the light dir for this light
		light_get_global_dir(&light_dir, idx);

		// Find rp1
		vm_vec_scale_add( &rp1, &rp0, &light_dir, 10000.0f );

		for ( so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so) )	{
			objp = &Objects[so->objnum];

			mc.model_num = Ship_info[Ships[objp->instance].ship_info_index].model_num;
			mc.orient = &objp->orient;
			mc.pos = &objp->pos;
			mc.p0 = &rp0;
			mc.p1 = &rp1;
			mc.flags = MC_CHECK_MODEL;	

			if ( model_collide(&mc) ){
				return 1;
			}
		}
	}

	// not in shadow
	return 0;
}


// Given an ship see if it is in a shadow.
int shipfx_in_shadow( object * src_obj )
{
	mc_info mc;
	object *objp;
	ship_obj *so;
	int n_lights;
	int idx;

	vec3d rp0, rp1;
	vec3d light_dir;

	rp0 = src_obj->pos;
	
	// get the # of global lights
	n_lights = light_get_global_count();

	for(idx=0; idx<n_lights; idx++){
		// get the direction for this light
		light_get_global_dir(&light_dir, idx);

		// Find rp1
		for ( so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so) )	{
			objp = &Objects[so->objnum];

			if ( src_obj != objp )	{
				vm_vec_scale_add( &rp1, &rp0, &light_dir, objp->radius*10.0f );

				mc.model_num = Ship_info[Ships[objp->instance].ship_info_index].model_num;
				mc.orient = &objp->orient;
				mc.pos = &objp->pos;
				mc.p0 = &rp0;
				mc.p1 = &rp1;
				mc.flags = MC_CHECK_MODEL;	

	//			mc.flags |= MC_CHECK_SPHERELINE;
	//			mc.radius = src_obj->radius;

				if ( model_collide(&mc) )	{
					return 1;
				}
			}
		}
	}

	// not in shadow
	return 0;
}

// Given world point see if it is in a shadow.
int shipfx_eye_in_shadow( vec3d *eye_pos, object * src_obj, int sun_n )
{
	mc_info mc;
	object *objp;
	ship_obj *so;

	vec3d rp0, rp1;
	vec3d light_dir;

	rp0 = *eye_pos;	
	
	// get the light dir
	if(!light_get_global_dir(&light_dir, sun_n)){
		return 0;
	}

	// Find rp1
	for ( so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so) )	{
		objp = &Objects[so->objnum];

		if ( src_obj != objp )	{
			vm_vec_scale_add( &rp1, &rp0, &light_dir, objp->radius*10.0f );

			ship_model_start(objp);

			mc.model_num = Ship_info[Ships[objp->instance].ship_info_index].model_num;
			mc.orient = &objp->orient;
			mc.pos = &objp->pos;
			mc.p0 = &rp0;
			mc.p1 = &rp1;
			mc.flags = MC_CHECK_MODEL;	

	//			mc.flags |= MC_CHECK_SPHERELINE;
	//			mc.radius = src_obj->radius;

			int hit = model_collide(&mc);

			ship_model_stop(objp);

			if (hit) {
				return 1;
			}
		}
	}

	// Check all the big hull debris pieces.
	debris	*db = Debris;

	int i;
	for ( i = 0; i < MAX_DEBRIS_PIECES; i++, db++ )	{
		if ( !(db->flags & DEBRIS_USED) || !db->is_hull ){
			continue;
		}

		objp = &Objects[db->objnum];

		vm_vec_scale_add( &rp1, &rp0, &light_dir, objp->radius*10.0f );

		mc.model_num = db->model_num;	// Fill in the model to check
		mc.submodel_num = db->submodel_num;
		model_clear_instance( mc.model_num );
		mc.orient = &objp->orient;					// The object's orient
		mc.pos = &objp->pos;							// The object's position
		mc.p0 = &rp0;				// Point 1 of ray to check
		mc.p1 = &rp1;					// Point 2 of ray to check
		mc.flags = (MC_CHECK_MODEL | MC_SUBMODEL);

		if (model_collide(&mc))	{
			return 1;
		}
	}

    // check asteroids
    asteroid *ast = Asteroids;

    if (Asteroid_field.num_initial_asteroids <= 0 )
    {
        return 0;
    }

    for (i = 0 ; i < MAX_ASTEROIDS; i++, ast++)
    {
        if (!(ast->flags & AF_USED))
        {
            continue;
        }

        objp = &Objects[ast->objnum];

        vm_vec_scale_add( &rp1, &rp0, &light_dir, objp->radius*10.0f );

		mc.model_num = Asteroid_info[ast->asteroid_type].model_num[ast->asteroid_subtype];	// Fill in the model to check
		mc.submodel_num = -1;
		model_clear_instance( mc.model_num );
		mc.orient = &objp->orient;					// The object's orient
		mc.pos = &objp->pos;							// The object's position
		mc.p0 = &rp0;				// Point 1 of ray to check
		mc.p1 = &rp1;					// Point 2 of ray to check
		mc.flags = MC_CHECK_MODEL;

		if (model_collide(&mc))	{
			return 1;
		}
    }

	// not in shadow
	return 0;
}

//=====================================================================================
// STUFF FOR DOING SHIP GUN FLASHES
//=====================================================================================

#define MAX_FLASHES	128			// How many flashes total
#define FLASH_LIFE_PRIMARY		0.25f			// How long flash lives
#define FLASH_LIFE_SECONDARY	0.50f			// How long flash lives


typedef struct ship_flash {
	int	objnum;					// object number of parent ship
	int	obj_signature;			// signature of that object
	int	light_num;				// which light in the model this uses
	float	life;						// how long this should be around
	float max_life;				// how long this has been around.
} ship_flash;

int Ship_flash_inited = 0;
int Ship_flash_highest = -1;
ship_flash Ship_flash[MAX_FLASHES];

// Resets the ship flash stuff. Call before each level.
void shipfx_flash_init()
{
	int i;
	
	for (i=0; i<MAX_FLASHES; i++ )	{
		Ship_flash[i].objnum = -1;			// mark as unused
	}
	Ship_flash_highest = -1;
	Ship_flash_inited = 1;	
}


// Given that a ship fired a weapon, light up the model
// accordingly.
void shipfx_flash_create(object *objp, int model_num, vec3d *gun_pos, vec3d *gun_dir, int is_primary, int weapon_info_index)
{
	int i;
	int objnum = OBJ_INDEX(objp);

	Assert(Ship_flash_inited);

	polymodel *pm = model_get(model_num);
	int closest_light = -1;
	float d, closest_dist = 0.0f;

	// ALWAYS do this - since this is called once per firing
	// if this is a cannon type weapon, create a muzzle flash
	// HACK - let the flak guns do this on their own since they fire so quickly
	if((Weapon_info[weapon_info_index].wi_flags & WIF_MFLASH) && !(Weapon_info[weapon_info_index].wi_flags & WIF_FLAK)){
		// spiffy new flash stuff

		vec3d real_dir;
		vm_vec_rotate(&real_dir, gun_dir,&objp->orient);
		//vm_vec_add2(&real_pos, &objp->pos);			
		//mflash_create(&real_pos, gun_dir, &objp->phys_info, Weapon_info[weapon_info_index].muzzle_flash);		
		mflash_create(gun_pos, &real_dir, &objp->phys_info, Weapon_info[weapon_info_index].muzzle_flash, objp);		
	}

	if ( pm->num_lights < 1 ) return;

	for (i=0; i<pm->num_lights; i++ )	{
		d = vm_vec_dist( &pm->lights[i].pos, gun_pos );
	
		if ( pm->lights[i].type == BSP_LIGHT_TYPE_WEAPON ) {
			if ( (closest_light==-1) || (d<closest_dist) )	{
				closest_light = i;
				closest_dist = d;
			}
		}
	}

	if ( closest_light == -1 ) return;

	int first_slot = -1;

	for (i=0; i<=Ship_flash_highest; i++ )	{
		if ( (first_slot==-1) && (Ship_flash[i].objnum < 0) )	{
			first_slot = i;
		}

		if ( (Ship_flash[i].objnum == objnum) && (Ship_flash[i].obj_signature==objp->signature) )	{
			if ( Ship_flash[i].light_num == closest_light )	{
				// This is already flashing!
				Ship_flash[i].life = 0.0f;
				if ( is_primary )	{
					Ship_flash[i].max_life = FLASH_LIFE_PRIMARY;
				} else {
					Ship_flash[i].max_life = FLASH_LIFE_SECONDARY;
				}
				return;
			}
		}
	}

	if ( first_slot == -1 )	{
		if ( Ship_flash_highest < MAX_FLASHES-1 )	{
			Ship_flash_highest++;
			first_slot = Ship_flash_highest;
		} else {
			//mprintf(( "SHIP_FLASH: Out of flash spots!\n" ));
			return;		// out of flash slots
		}
	}

	Assert( Ship_flash[first_slot].objnum == -1 );

	Ship_flash[first_slot].objnum = objnum;
	Ship_flash[first_slot].obj_signature = objp->signature;
	Ship_flash[first_slot].life = 0.0f;		// Start it up
	if ( is_primary )	{
		Ship_flash[first_slot].max_life = FLASH_LIFE_PRIMARY;
	} else {
		Ship_flash[first_slot].max_life = FLASH_LIFE_SECONDARY;
	}
	Ship_flash[first_slot].light_num = closest_light;		
}

// Sets the flash lights in the model used by this
// ship to the appropriate values.  There might not
// be any flashes linked to this ship in which
// case this function does nothing.
void shipfx_flash_light_model(object *objp, int model_num)
{
	int i, objnum = OBJ_INDEX(objp);
	polymodel *pm = model_get(model_num);

	for (i=0; i<=Ship_flash_highest; i++ )	{
		if ( (Ship_flash[i].objnum == objnum) && (Ship_flash[i].obj_signature==objp->signature) )	{
			float v = (Ship_flash[i].max_life - Ship_flash[i].life)/Ship_flash[i].max_life;

			pm->lights[Ship_flash[i].light_num].value += v / 255.0f;
		}
	}
}

// Does whatever processing needs to be done each frame.
void shipfx_flash_do_frame(float frametime)
{
	ship_flash *sf;
	int kill_it = 0;
	int i;

	for (i=0, sf = &Ship_flash[0]; i<=Ship_flash_highest; i++, sf++ )	{
		if ( sf->objnum > -1 )	{
			if ( Objects[sf->objnum].signature != sf->obj_signature )	{
				kill_it = 1;
			}
			sf->life += frametime;
			if ( sf->life >= sf->max_life )	kill_it = 1;

			if (kill_it) {
				sf->objnum = -1;
				if ( i == Ship_flash_highest )	{
					while( (Ship_flash_highest>0) && (Ship_flash[Ship_flash_highest].objnum == -1) )	{
						Ship_flash_highest--;
					}
				}
			}	
		}
	}	

}

float Particle_width = 1.2f;
DCF(particle_width, "Multiplier for angular width of the particle spew")
{
	if ( Dc_command ) {
		dc_get_arg(ARG_FLOAT);
		if ( (Dc_arg_float >= 0 ) && (Dc_arg_float <= 5) ) {
			Particle_width = Dc_arg_float;
		} else {
			dc_printf( "Illegal value for particle width. (Must be from 0-5) \n\n");
		}
	}
}

float Particle_number = 1.2f;
DCF(particle_num, "Multiplier for the number of particles created")
{
	if ( Dc_command ) {
		dc_get_arg(ARG_FLOAT);
		if ( (Dc_arg_float >= 0 ) && (Dc_arg_float <= 5) ) {
			Particle_number = Dc_arg_float;
		} else {
			dc_printf( "Illegal value for particle num. (Must be from 0-5) \n\n");
		}
	}
}

float Particle_life = 1.2f;
DCF(particle_life, "Multiplier for the lifetime of particles created")
{
	if ( Dc_command ) {
		dc_get_arg(ARG_FLOAT);
		if ( (Dc_arg_float >= 0 ) && (Dc_arg_float <= 5) ) {
			Particle_life = Dc_arg_float;
		} else {
			dc_printf( "Illegal value for particle life. (Must be from 0-5) \n\n");
		}
	}
}

// Make sparks fly off of ship n.
// sn = spark number to spark, corrosponding to element in
//      ship->hitpos array.  If this isn't -1, it is a just
//      got hit by weapon spark, otherwise pick one randomally.
void shipfx_emit_spark( int n, int sn )
{
	int create_spark = 1;
	object * obj;
	vec3d outpnt;
	ship *shipp = &Ships[n];
	float ship_radius, spark_scale_factor;

	ship_info *sip = &Ship_info[shipp->ship_info_index];
	if(sn > -1 && sip->ispew_max_particles == 0)
		return;

	if(sn < 0 && sip->dspew_max_particles == 0)
		return;
	
	if ( shipp->num_hits <= 0 )
		return;

	// get radius of ship
	ship_radius = model_get_radius(sip->model_num);

	// get spark_scale_factor -- how much to increase ship sparks, based on radius
	if (ship_radius > 40) {
		spark_scale_factor = 1.0f;
	} else if (ship_radius > 20) {
		spark_scale_factor = (ship_radius - 20.0f) / 20.0f;
	} else {
		spark_scale_factor = 0.0f;
	}

	float spark_time_scale  = 1.0f + spark_scale_factor * (Particle_life   - 1.0f);
	float spark_width_scale = 1.0f + spark_scale_factor * (Particle_width  - 1.0f);
	float spark_num_scale   = 1.0f + spark_scale_factor * (Particle_number - 1.0f);

	obj = &Objects[shipp->objnum];
	ship_info* si = &Ship_info[shipp->ship_info_index];

	float hull_percent = get_hull_pct(obj);
	if (hull_percent < 0.001) {
		hull_percent = 0.001f;
	}
	float fraction = 0.1f * obj->radius / hull_percent;
	if (fraction > 1.0f) {
		fraction = 1.0f;
	}

	int spark_num;
	if ( sn == -1 ) {
		spark_num = myrand() % shipp->num_hits;
	} else {
		spark_num = sn;
	}

	// don't display sparks that have expired
	if ( timestamp_elapsed(shipp->sparks[spark_num].end_time) ) {
		return;
	}

	// get spark position
	if (shipp->sparks[spark_num].submodel_num != -1) {
		ship_model_start(obj);
		model_find_world_point(&outpnt, &shipp->sparks[spark_num].pos, sip->model_num, shipp->sparks[spark_num].submodel_num, &obj->orient, &obj->pos);
		ship_model_stop(obj);
	} else {
		// rotate sparks correctly with current ship orient
		vm_vec_unrotate(&outpnt, &shipp->sparks[spark_num].pos, &obj->orient);
		vm_vec_add2(&outpnt,&obj->pos);
	}

	//WMC - I have two options here:
	//(A)	Break backwards compatibility and nobody will ever notice (but for this comment)
	//(B)	implement some hackish workaround with the new warpeffect system just for this
	//		utterly minor and trivial optimization/graphical thing
	//(C)	Contact me if (A) is REALLY necessary
    //
    // phreak: Mantis 1676 - Re-enable warpout clipping.
	
	if ((shipp->flags & (SF_ARRIVING|SF_DEPART_WARP)) && (shipp->warpout_effect))
    {
        vec3d warp_pnt, tmp;
        matrix warp_orient;

        shipp->warpout_effect->getWarpPosition(&warp_pnt);
        shipp->warpout_effect->getWarpOrientation(&warp_orient);

        vm_vec_sub( &tmp, &outpnt, &warp_pnt );
        
        if ( vm_vec_dot( &tmp, &warp_orient.vec.fvec ) < 0.0f )
        {
			if (shipp->flags & SF_ARRIVING)// if in front of warp plane, don't create.
				create_spark = 0;
		} else {
			if (shipp->flags & SF_DEPART_WARP)
				create_spark = 0;
		}
	}

	if ( create_spark )	{

		particle_emitter	pe;

		pe.pos = outpnt;				// Where the particles emit from

		if ( shipp->flags & (SF_ARRIVING|SF_DEPART_WARP) ) {
			// No velocity if going through warp.
			pe.vel = vmd_zero_vector;
		} else {
			// Initial velocity of all the particles.
			// 0.0f = 0% of parent's.
			// 1.0f = 100% of parent's.
			vm_vec_copy_scale( &pe.vel, &obj->phys_info.vel, 0.7f );
		}

		// TODO: add velocity from rotation if submodel is rotating
		// v_rot = w x r

		// r = outpnt - model_find_world_point(0)

		// w = model_find_world_dir(
		// model_find_world_dir(&out_dir, &in_dir, model_num, submodel_num, &objorient, &objpos);

		vec3d tmp_norm, tmp_vel;
		vm_vec_sub( &tmp_norm, &outpnt, &obj->pos );
		vm_vec_normalize_safe(&tmp_norm);

		tmp_vel = obj->phys_info.vel;
		if ( vm_vec_normalize_safe(&tmp_vel) > 1.0f )	{
			vm_vec_scale_add2(&tmp_norm,&tmp_vel, -2.0f);
			vm_vec_normalize_safe(&tmp_norm);
		}
				
		pe.normal = tmp_norm;			// What normal the particle emit around
		pe.normal_variance = 0.3f;		//	How close they stick to that normal 0=good, 1=360 degree
		pe.min_rad = 0.20f;				// Min radius
		pe.max_rad = 0.50f;				// Max radius

		// first time through - set up end time and make heavier initially
		if ( sn > -1 )	{
			// Sparks for first time at this spot
			if (si->flags & SIF_FIGHTER) {
				if (hull_percent > 0.6f) {
					// sparks only once when hull > 60%
					float spark_duration = (float)pow(2.0f, -5.0f*(hull_percent-1.3f)) * (1.0f + 0.6f*(frand()-0.5f));	// +- 30%
					shipp->sparks[spark_num].end_time = timestamp( (int) (1000.0f * spark_duration) );
				} else {
					// spark never ends when hull < 60% (~277 hr)
					shipp->sparks[spark_num].end_time = timestamp( 100000000 );
				}
			}

			pe.num_low  = 25;				// Lowest number of particles to create (hardware)
			if(sip->ispew_max_particles > 0) {
				pe.num_high = sip->ispew_max_particles;
			} else {
				pe.num_high = 30;				// Highest number of particles to create (hardware)
			}
			pe.normal_variance = 1.0f;	//	How close they stick to that normal 0=good, 1=360 degree
			pe.min_vel = 2.0f;				// How fast the slowest particle can move
			pe.max_vel = 12.0f;				// How fast the fastest particle can move
			pe.min_life = 0.05f;				// How long the particles live
			pe.max_life = 0.55f;				// How long the particles live

			particle_emit( &pe, PARTICLE_FIRE, 0 );
		} else {

			pe.min_rad = 0.7f;				// Min radius
			pe.max_rad = 1.3f;				// Max radius
			pe.num_low  = int (20 * spark_num_scale);		// Lowest number of particles to create (hardware)
			if(sip->dspew_max_particles > 0) {
				pe.num_high = sip->dspew_max_particles;
			} else {
				pe.num_high = int (50 * spark_num_scale);		// Highest number of particles to create (hardware)
			}
			pe.normal_variance = 0.2f * spark_width_scale;		//	How close they stick to that normal 0=good, 1=360 degree
			pe.min_vel = 3.0f;				// How fast the slowest particle can move
			pe.max_vel = 12.0f;				// How fast the fastest particle can move
			pe.min_life = 0.35f*2.0f * spark_time_scale;		// How long the particles live
			pe.max_life = 0.75f*2.0f * spark_time_scale;		// How long the particles live
			
			particle_emit( &pe, PARTICLE_SMOKE, 0 );
		}
	}

	// Select time to do next spark
//	Ships[n].next_hit_spark = timestamp_rand(100,500);
	shipp->next_hit_spark = timestamp_rand(50,100);
}



//=====================================================================================
// STUFF FOR DOING LARGE SHIP EXPLOSIONS
//=====================================================================================

int	Bs_exp_fire_low = 1;
float	Bs_exp_fire_time_mult = 1.0f;

DCF_BOOL(bs_exp_fire_low, Bs_exp_fire_low)
DCF(bs_exp_fire_time_mult, "Multiplier time between fireball in big ship explosion")
{
	if ( Dc_command ) {
		dc_get_arg(ARG_FLOAT);
		if ( (Dc_arg_float >= 0.1 ) && (Dc_arg_float <= 5) ) {
			Bs_exp_fire_time_mult = Dc_arg_float;
		} else {
			dc_printf( "Illegal value for bs_exp_fire_time_mult. (Must be from 0.1-5) \n\n");
		}
	}
}


#define DEBRIS_NONE			0
#define DEBRIS_DRAW			1
#define DEBRIS_FREE			2

typedef struct clip_ship {
	object*			parent_obj;
	float 			length_left;	// uncomsumed length
	matrix			orient;
	physics_info	phys_info;
	vec3d			local_pivot;								// world center of mass position of half ship
	vec3d			model_center_disp_to_orig_center;	// displacement from half ship center to original model center
	vec3d			clip_plane_norm;							// clip plane normal (local [0,0,1] or [0,0,-1])
	float				cur_clip_plane_pt;						// displacement from half ship clip plane to original model center
	float				explosion_vel;
	ubyte				draw_debris[MAX_DEBRIS_OBJECTS];
	int				next_fireball;
} clip_ship;

typedef struct split_ship {
	int				used;					// 0 if not used, 1 if used
	clip_ship		front_ship;
	clip_ship		back_ship;
	int				explosion_flash_timestamp;
	int				explosion_flash_started;
	int				sound_handle[NUM_SUB_EXPL_HANDLES];
} split_ship;


static SCP_vector<split_ship> Split_ships;

static int get_split_ship()
{
	int i;
	split_ship addition;

	// check for an existing free slot
	int max_size = (int)Split_ships.size();
	for (i = 0; i < max_size; i++) {
		if (!Split_ships[i].used)
			return i;
	}

	// nope.  we'll have to add a new one
	memset( &addition, 0, sizeof(split_ship) );

	Split_ships.push_back(addition);

	return (Split_ships.size() - 1);
}

static void maybe_fireball_wipe(clip_ship* half_ship, int* sound_handle);
static void split_ship_init( ship* shipp, split_ship* split_ship )
{
	object* parent_ship_obj = &Objects[shipp->objnum];
	matrix* orient = &parent_ship_obj->orient;
	for (int ii=0; ii<NUM_SUB_EXPL_HANDLES; ii++) {
		split_ship->sound_handle[ii] = shipp->sub_expl_sound_handle[ii];
	}

	// play 3d sound for shockwave explosion
	snd_play_3d( &Snds[SND_SHOCKWAVE_EXPLODE], &parent_ship_obj->pos, &View_position, 0.0f, NULL, 0, 1.0f, SND_PRIORITY_SINGLE_INSTANCE, NULL, 3.0f );

	// initialize both ships
	split_ship->front_ship.parent_obj = parent_ship_obj;
	split_ship->back_ship.parent_obj  = parent_ship_obj;
	split_ship->explosion_flash_timestamp = timestamp((int)(0.00075f*parent_ship_obj->radius));
	split_ship->explosion_flash_started = 0;
	split_ship->front_ship.orient = *orient;
	split_ship->back_ship.orient  = *orient;
	split_ship->front_ship.next_fireball = timestamp_rand(0, 100);
	split_ship->back_ship.next_fireball  = timestamp_rand(0, 100);

	split_ship->front_ship.clip_plane_norm = vmd_z_vector;
	vm_vec_copy_scale(&split_ship->back_ship.clip_plane_norm, &vmd_z_vector, -1.0f);

	// find the point at which the ship splits (relative to its pivot)
	polymodel* pm = model_get(Ship_info[shipp->ship_info_index].model_num);
	float init_clip_plane_dist;
	if (pm->num_split_plane > 0) {
		int index = rand()%pm->num_split_plane;
		init_clip_plane_dist = pm->split_plane[index];
	} else {
		init_clip_plane_dist = 0.5f * (0.5f - frand())*pm->core_radius;
	}

	split_ship->back_ship.cur_clip_plane_pt =  init_clip_plane_dist;
	split_ship->front_ship.cur_clip_plane_pt = init_clip_plane_dist;

	float dist;
	dist = (split_ship->front_ship.cur_clip_plane_pt+pm->maxs.xyz.z)/2.0f;
	vm_vec_copy_scale(&split_ship->front_ship.local_pivot, &orient->vec.fvec, dist);
	vm_vec_make(&split_ship->front_ship.model_center_disp_to_orig_center, 0.0f, 0.0f, -dist);
	dist = (split_ship->back_ship.cur_clip_plane_pt +pm->mins.xyz.z)/2.0f;
	vm_vec_copy_scale(&split_ship->back_ship.local_pivot, &orient->vec.fvec, dist);
	vm_vec_make(&split_ship->back_ship.model_center_disp_to_orig_center, 0.0f, 0.0f, -dist);
	vm_vec_add2(&split_ship->front_ship.local_pivot, &parent_ship_obj->pos );
	vm_vec_add2(&split_ship->back_ship.local_pivot,  &parent_ship_obj->pos );
	
	// find which debris pieces are in the front and back split ships
	for (int i=0; i<pm->num_debris_objects; i++ )	{
		vec3d temp_pos = ZERO_VECTOR;
		vec3d tmp = ZERO_VECTOR;		
		vec3d tmp1 = pm->submodel[pm->debris_objects[i]].offset;
		// tmp is world position,  temp_pos is world_pivot,  tmp1 is offset from world_pivot (in ship local coord)
		model_find_world_point(&tmp, &tmp1, pm->id, -1, &vmd_identity_matrix, &temp_pos );
		if (tmp.xyz.z > init_clip_plane_dist) {
			split_ship->front_ship.draw_debris[i] = DEBRIS_DRAW;
			split_ship->back_ship.draw_debris[i]  = DEBRIS_NONE;
		} else {
			split_ship->front_ship.draw_debris[i] = DEBRIS_NONE;
			split_ship->back_ship.draw_debris[i]  = DEBRIS_DRAW;
		}
	}

	/*
	// set the remaining debris slots to not draw
	for (i=pm->num_debris_objects; i<MAX_DEBRIS_OBJECTS; i++) {
		split_ship->front_ship.draw_debris[i] = DEBRIS_NONE;
		split_ship->back_ship.draw_debris[i]  = DEBRIS_NONE;
	} */

	// set up physics 
	physics_init( &split_ship->front_ship.phys_info );
	physics_init( &split_ship->back_ship.phys_info );
	split_ship->front_ship.phys_info.flags  |= (PF_ACCELERATES | PF_DEAD_DAMP);
	split_ship->back_ship.phys_info.flags |= (PF_ACCELERATES | PF_DEAD_DAMP);
	split_ship->front_ship.phys_info.side_slip_time_const = 10000.0f;
	split_ship->back_ship.phys_info.side_slip_time_const =  10000.0f;
	split_ship->front_ship.phys_info.rotdamp = 10000.0f;
	split_ship->back_ship.phys_info.rotdamp =  10000.0f;

	// set up explosion vel and relative velocities (assuming mass depends on length)
	float front_length = pm->maxs.xyz.z - split_ship->front_ship.cur_clip_plane_pt;
	float back_length  = split_ship->back_ship.cur_clip_plane_pt - pm->mins.xyz.z;
	float ship_length = front_length + back_length;
	split_ship->front_ship.length_left = front_length;
	split_ship->back_ship.length_left  = back_length;

	float expl_length_scale = (ship_length - 200.0f) / 2000.0f;
	// s_r_f effects speed of "wipe" and rotvel
	float speed_reduction_factor = (1.0f + 0.001f*parent_ship_obj->radius);
	float explosion_time = (3.0f + expl_length_scale + (frand()-0.5f)) * speed_reduction_factor;
	float long_length = MAX(front_length, back_length);
	float expl_vel = long_length / explosion_time;
	split_ship->front_ship.explosion_vel = expl_vel;
	split_ship->back_ship.explosion_vel  = -expl_vel;

	float rel_vel = (0.6f + 0.2f*frand()) * expl_vel * speed_reduction_factor;
	float front_vel = rel_vel * back_length / ship_length;
	float back_vel = -rel_vel * front_length / ship_length;
	// mprintf(("rel_vel %.1f, expl_vel %.1f\n", rel_vel, expl_vel));

	// set up rotational vel
	vec3d rotvel;
	vm_vec_rand_vec_quick(&rotvel);
	rotvel.xyz.z = 0.0f;
	vm_vec_normalize(&rotvel);
	vm_vec_scale(&rotvel, 0.15f / speed_reduction_factor);
	split_ship->front_ship.phys_info.rotvel = rotvel;
	vm_vec_copy_scale(&split_ship->back_ship.phys_info.rotvel, &rotvel, -(front_length*front_length)/(back_length*back_length));
	split_ship->front_ship.phys_info.rotvel.xyz.z = parent_ship_obj->phys_info.rotvel.xyz.z;
	split_ship->back_ship.phys_info.rotvel.xyz.z  = parent_ship_obj->phys_info.rotvel.xyz.z;


	// modify vel of each split ship based on rotvel of parent ship obj
	vec3d temp_rotvel = parent_ship_obj->phys_info.rotvel;
	temp_rotvel.xyz.z = 0.0f;
	vec3d vel_from_rotvel;
	vm_vec_crossprod(&vel_from_rotvel, &temp_rotvel, &split_ship->front_ship.local_pivot);
	//	vm_vec_scale_add2(&split_ship->front_ship.phys_info.vel, &vel_from_rotvel, 0.5f);
	vm_vec_crossprod(&vel_from_rotvel, &temp_rotvel, &split_ship->back_ship.local_pivot);
	//	vm_vec_scale_add2(&split_ship->back_ship.phys_info.vel, &vel_from_rotvel, 0.5f);

	// set up velocity and make initial fireballs and particles
	split_ship->front_ship.phys_info.vel = parent_ship_obj->phys_info.vel;
	split_ship->back_ship.phys_info.vel  = parent_ship_obj->phys_info.vel;
	maybe_fireball_wipe(&split_ship->front_ship, (int*)&split_ship->sound_handle);
	maybe_fireball_wipe(&split_ship->back_ship,  (int*)&split_ship->sound_handle);
	vm_vec_scale_add2(&split_ship->front_ship.phys_info.vel, &orient->vec.fvec, front_vel);
	vm_vec_scale_add2(&split_ship->back_ship.phys_info.vel,  &orient->vec.fvec, back_vel);

	// HANDLE LIVE DEBRIS - blow off if not already gone
	shipfx_maybe_create_live_debris_at_ship_death( parent_ship_obj );
}


static void half_ship_render_ship_and_debris(clip_ship* half_ship,ship *shipp)
{
	polymodel *pm = model_get(Ship_info[shipp->ship_info_index].model_num);

	// get rotated clip plane normal and world coord of original ship center
	vec3d orig_ship_world_center, clip_plane_norm, model_clip_plane_pt, debris_clip_plane_pt;
	vm_vec_unrotate(&clip_plane_norm, &half_ship->clip_plane_norm, &half_ship->orient);
	vm_vec_unrotate(&orig_ship_world_center, &half_ship->model_center_disp_to_orig_center, &half_ship->orient);
	vm_vec_add2(&orig_ship_world_center, &half_ship->local_pivot);

	// *out_pivot = orig_ship_world_center;

	// get debris clip plane pt and draw debris
	vm_vec_unrotate(&debris_clip_plane_pt, &half_ship->model_center_disp_to_orig_center, &half_ship->orient);
	vm_vec_add2(&debris_clip_plane_pt, &half_ship->local_pivot);
	g3_start_user_clip_plane( &debris_clip_plane_pt, &clip_plane_norm);

	// set up render flags
	uint render_flags = MR_NORMAL;

	for (int i=0; i<pm->num_debris_objects; i++ )	{
		// draw DEBRIS_FREE in test only
		if (half_ship->draw_debris[i] == DEBRIS_DRAW) {
			vec3d temp_pos = orig_ship_world_center;
			vec3d tmp = ZERO_VECTOR;
			vec3d tmp1 = pm->submodel[pm->debris_objects[i]].offset;

			// determine if explosion front has past debris piece
			// 67 ~ dist expl moves in 2 frames -- maybe fraction works better
			int is_live_debris = pm->submodel[pm->debris_objects[i]].is_live_debris;
			int create_debris = 0;
			// front ship
			if (half_ship->explosion_vel > 0) {
				if (half_ship->cur_clip_plane_pt > tmp1.xyz.z + pm->submodel[pm->debris_objects[i]].max.xyz.z - 0.1f*half_ship->explosion_vel) {
					create_debris = 1;
				}
				// is the debris visible
//				if (half_ship->cur_clip_plane_pt > tmp1.z + pm->submodel[pm->debris_objects[i]].min.z - 0.5f*half_ship->explosion_vel) {
//					render_debris = 1;
//				}
			// back ship
			} else {
				if (half_ship->cur_clip_plane_pt < tmp1.xyz.z + pm->submodel[pm->debris_objects[i]].min.xyz.z - 0.1f*half_ship->explosion_vel) {
					create_debris = 1;
				}
				// is the debris visible
//				if (half_ship->cur_clip_plane_pt < tmp1.z + pm->submodel[pm->debris_objects[i]].max.z - 0.5f*half_ship->explosion_vel) {
//					render_debris = 1;
//				}
			}

			// Draw debris, but not live debris
			if ( !is_live_debris ) {
				model_find_world_point(&tmp, &tmp1, pm->id, -1, &half_ship->orient, &temp_pos);
				submodel_render(pm->id, pm->debris_objects[i], &half_ship->orient, &tmp, render_flags, -1, shipp->ship_replacement_textures);
			}

			// make free piece of debris
			if ( create_debris ) {
				half_ship->draw_debris[i] = DEBRIS_FREE;		// mark debris to not render with model
				vec3d center_to_debris, debris_vel, radial_vel;
				// check if last debris piece, ie, debris_count == 0
				int debris_count = 0;
				for (int j=0; j<pm->num_debris_objects; j++ ) {
					if (half_ship->draw_debris[j] == DEBRIS_DRAW) {
						debris_count++;
					}
				} 
				// do debris create here, but not for live debris
				// debris vel (1) split ship vel (2) split ship rotvel (3) random
				if ( !is_live_debris ) {
					object* debris_obj;
					debris_obj = debris_create(half_ship->parent_obj, pm->id, pm->debris_objects[i], &tmp, &half_ship->local_pivot, 1, 1.0f);
					// AL: make sure debris_obj isn't NULL!
					if ( debris_obj ) {
						vm_vec_scale(&debris_obj->phys_info.rotvel, 4.0f);
						debris_obj->orient = half_ship->orient;
						// if (debris_count > 0) {
							//mprintf(( "base rotvel %.1f, debris rotvel mag %.2f\n", vm_vec_mag(&half_ship->phys_info.rotvel), vm_vec_mag(&debris_obj->phys_info.rotvel) ));
							vm_vec_sub(&center_to_debris, &tmp, &half_ship->local_pivot);
							vm_vec_crossprod(&debris_vel, &center_to_debris, &half_ship->phys_info.rotvel);
							vm_vec_add2(&debris_vel, &half_ship->phys_info.vel);
							vm_vec_copy_normalize(&radial_vel, &center_to_debris);
							float radial_mag = 10.0f + 30.0f*frand();
							vm_vec_scale_add2(&debris_vel, &radial_vel, radial_mag);
							debris_obj->phys_info.vel = debris_vel;
							shipfx_debris_limit_speed(&Debris[debris_obj->instance], shipp);
						/* } else {
							debris_obj->phys_info.vel = half_ship->phys_info.vel;
							debris_obj->phys_info.rotvel = half_ship->phys_info.rotvel;
						} */
					}
				}
			}
		}
	}

	// get model clip plane pt and draw model
	vec3d temp;
	vm_vec_make(&temp, 0.0f, 0.0f, half_ship->cur_clip_plane_pt);
	vm_vec_unrotate(&model_clip_plane_pt, &temp, &half_ship->orient);
	vm_vec_add2(&model_clip_plane_pt, &orig_ship_world_center);
	g3_start_user_clip_plane( &model_clip_plane_pt, &clip_plane_norm );
	model_render(pm->id, &half_ship->orient, &orig_ship_world_center, render_flags, -1, -1, shipp->ship_replacement_textures);
}

void shipfx_large_blowup_level_init()
{
	Split_ships.clear();

	if(Ship_cannon_bitmap != -1){
		bm_release(Ship_cannon_bitmap);
		Ship_cannon_bitmap = bm_load(SHIP_CANNON_BITMAP);
	}
}

// Returns 0 if couldn't init
int shipfx_large_blowup_init(ship *shipp)
{
	int i;

	i = get_split_ship();

	Split_ships[i].used = 1;
	shipp->large_ship_blowup_index = i;

	split_ship_init(shipp, &Split_ships[i] );
	
	return 1;
}

void shipfx_debris_limit_speed(debris *db, ship *shipp)
{
	if(db == NULL || shipp == NULL)
		return;

	object *ship_obj = &Objects[shipp->objnum];
	physics_info *pi = &Objects[db->objnum].phys_info;
	ship_info *sip = &Ship_info[shipp->ship_info_index];

	float curspeed = vm_vec_mag(&pi->vel);
	if(sip->debris_min_speed >= 0.0f && sip->debris_max_speed >= 0.0f)
	{
		float debris_speed = (( sip->debris_max_speed - sip->debris_min_speed ) * frand()) + sip->debris_min_speed;
		if(fabs(curspeed) >= 0.001f)
		{
			float scale = debris_speed / curspeed;
			vm_vec_scale(&pi->vel, scale);
		}
		else
		{
			vm_vec_copy_scale(&pi->vel, &ship_obj->orient.vec.fvec, debris_speed);
		}
	}
	else if(sip->debris_min_speed >= 0.0f)
	{
		if(curspeed < sip->debris_min_speed)
		{
			if(fabs(curspeed) >= 0.001f)
			{
				float scale = sip->debris_min_speed / curspeed;
				vm_vec_scale(&pi->vel, scale);
			}
			else
			{
				vm_vec_copy_scale(&pi->vel, &ship_obj->orient.vec.fvec, sip->debris_min_speed);
			}
		}
	}
	else if(sip->debris_max_speed >= 0.0f)
	{
		if(curspeed > sip->debris_max_speed)
		{
			if(fabs(curspeed) >= 0.001f)
			{
				float scale = sip->debris_max_speed / curspeed;
				vm_vec_scale(&pi->vel, scale);
			}
			else
			{
				vm_vec_copy_scale(&pi->vel, &ship_obj->orient.vec.fvec, sip->debris_max_speed);
			}
		}
	}

	//WMC - Rotational velocity user cap
	float currotvel = vm_vec_mag(&pi->rotvel);
	if(sip->debris_min_rotspeed >= 0.0f && sip->debris_max_rotspeed >= 0.0f)
	{
		float debris_rotspeed = (( sip->debris_max_rotspeed - sip->debris_min_rotspeed ) * frand()) + sip->debris_min_rotspeed;
		if(fabs(currotvel) >= 0.001f)
		{
			float scale = debris_rotspeed / currotvel;
			vm_vec_scale(&pi->rotvel, scale);
		}
		else
		{
			vm_vec_copy_scale(&pi->rotvel, &ship_obj->orient.vec.uvec, debris_rotspeed);
		}
	}
	else if(sip->debris_min_rotspeed >= 0.0f)
	{
		if(curspeed < sip->debris_min_rotspeed)
		{
			if(fabs(currotvel) >= 0.001f)
			{
				float scale = sip->debris_min_rotspeed / currotvel;
				vm_vec_scale(&pi->rotvel, scale);
			}
			else
			{
				vm_vec_copy_scale(&pi->rotvel, &ship_obj->orient.vec.uvec, sip->debris_min_rotspeed);
			}
		}
	}
	else if(sip->debris_max_rotspeed >= 0.0f)
	{
		float curspeed = vm_vec_mag(&pi->rotvel);
		if(curspeed > sip->debris_max_rotspeed)
		{
			if(fabs(currotvel) >= 0.001f)
			{
				float scale = sip->debris_max_rotspeed / currotvel;
				vm_vec_scale(&pi->rotvel, scale);
			}
			else
			{
				vm_vec_copy_scale(&pi->rotvel, &ship_obj->orient.vec.uvec, sip->debris_max_rotspeed);
			}
		}
	}

	int ship_type = sip->class_type;
	if(ship_type > -1)
	{
		if(vm_vec_mag(&pi->vel) > Ship_types[ship_type].debris_max_speed) {
			float scale = Ship_types[ship_type].debris_max_speed / vm_vec_mag(&pi->vel);
			vm_vec_scale(&pi->vel, scale);
		}
	}

	Assert(is_valid_vec(&pi->vel));
	Assert(is_valid_vec(&pi->rotvel));
}

// ----------------------------------------------------------------------------
// uses list of model z values with constant increment to find the radius of the 
// cross section at the current model z value
float get_model_cross_section_at_z(float z, polymodel* pm)
{
	if (pm->num_xc < 2) {
		return 0.0f;
	}

	float index, increment;
	increment = (pm->xc[pm->num_xc-1].z - pm->xc[0].z) / (float)(pm->num_xc - 1);
	index = (z - pm->xc[0].z) / increment;

	if (index < 0.5f) {
		return pm->xc[0].radius;
	} else if (index > (pm->num_xc - 1.0f - 0.5f)) {
		return pm->xc[pm->num_xc-1].radius;
	} else {
		int floor_index = (int)floor(index);
		int ceil_index  = (int)ceil(index);
		return MAX(pm->xc[ceil_index].radius, pm->xc[floor_index].radius);
	}
}

// returns how long sound has been playing
int get_sound_time_played(int snd_id, int handle)
{
	if (handle == -1) {
		return 100000;
	}

	int time_left = snd_time_remaining(handle);
	int duration = snd_get_duration(snd_id);
	
	return (duration - time_left);
}

// sound manager for big ship sub explosions sounds.
// forces playing of sub-explosion sounds.  keeps track of active sounds, plays them for >= 750 ms
// when sound has played >= 750, sound is stopped and new instance is started 
void do_sub_expl_sound(float radius, vec3d* sound_pos, int* sound_handle)
{
	int sound_index, handle;
	// multiplier for range (near and far distances) to apply attenuation
	float sound_range = 1.0f + 0.0043f*radius;

	int handle_index = rand()%NUM_SUB_EXPL_HANDLES;
	//mprintf(("handle_index %d\n", *handle_index));

	// sound_index = get_sub_explosion_sound_index(handle_index);
	sound_index = SND_SHIP_EXPLODE_1;
	handle = sound_handle[handle_index];


	// mprintf(("dist to sound %.1f snd_indx: %d, h1: %d, h2: %d\n", vm_vec_dist(&Player_obj->pos, sound_pos), next_sound_index, sound_handle[0], sound_handle[1]));

	if (handle == -1) {
		// if no handle, get one
		sound_handle[handle_index] = snd_play_3d( &Snds[sound_index], sound_pos, &View_position, 0.0f, NULL, 0, 0.6f, SND_PRIORITY_MUST_PLAY, NULL, sound_range );
	} else if (!snd_is_playing(handle)) {
		// if sound not playing and old, get new one
		// I don't think will happen with SND_PRIORITY_MUST_PLAY
		if (get_sound_time_played(Snds[sound_index].id, handle) > 400) {
			//mprintf(("sound not playing %d, time_played %d, stopped\n", handle, get_sound_time_played(Snds[sound_index].id, handle)));
			snd_stop(sound_handle[handle_index]);
			sound_handle[handle_index] = snd_play_3d( &Snds[sound_index], sound_pos, &View_position, 0.0f, NULL, 0, 0.6f, SND_PRIORITY_MUST_PLAY, NULL, sound_range );
		}
	} else if (get_sound_time_played(Snds[sound_index].id, handle) > 750) {
		//mprintf(("time %f, cur sound %d time_played %d num sounds %d\n", f2fl(Missiontime), handle_index, get_sound_time_played(Snds[sound_index].id, handle), snd_num_playing() ));
		sound_handle[handle_index] = snd_play_3d( &Snds[sound_index], sound_pos, &View_position, 0.0f, NULL, 0, 0.6f, SND_PRIORITY_MUST_PLAY, NULL, sound_range );
	}
}

// maybe create a fireball along model clip plane
// also maybe plays explosion sound
static void maybe_fireball_wipe(clip_ship* half_ship, int* sound_handle)
{
	// maybe make fireball to cover wipe.
	if ( timestamp_elapsed(half_ship->next_fireball) ) {
		if ( half_ship->length_left > 0.2f*fl_abs(half_ship->explosion_vel) )	{
			ship_info *sip = &Ship_info[Ships[half_ship->parent_obj->instance].ship_info_index];
			
			polymodel* pm = model_get(sip->model_num);

			vec3d model_clip_plane_pt, orig_ship_world_center, temp;

			vm_vec_unrotate(&orig_ship_world_center, &half_ship->model_center_disp_to_orig_center, &half_ship->orient);
			vm_vec_add2(&orig_ship_world_center, &half_ship->local_pivot);

			vm_vec_make(&temp, 0.0f, 0.0f, half_ship->cur_clip_plane_pt);
			vm_vec_unrotate(&model_clip_plane_pt, &temp, &half_ship->orient);
			vm_vec_add2(&model_clip_plane_pt, &orig_ship_world_center);
			vm_vec_rand_vec_quick(&temp);
			vm_vec_scale(&temp, 0.1f*frand());
			vm_vec_add2(&model_clip_plane_pt, &temp);

			float rad = get_model_cross_section_at_z(half_ship->cur_clip_plane_pt, pm);
			if (rad < 1) {
				rad = half_ship->parent_obj->radius * frand_range(0.4f, 0.6f);
			} else {
				// make fireball radius (1.5 +/- .1) * model_cross_section value
				rad *= frand_range(1.4f, 1.6f);
			}

			rad *= 1.5f;
			rad = MIN(rad, half_ship->parent_obj->radius);

			// mprintf(("xc %.1f model %.1f\n", rad, half_ship->parent_obj->radius*0.25));

			int fireball_type = fireball_ship_explosion_type(sip);
			if(fireball_type < 0) {
				fireball_type = FIREBALL_EXPLOSION_LARGE1 + rand()%FIREBALL_NUM_LARGE_EXPLOSIONS;
			}
			int low_res_fireballs = Bs_exp_fire_low;
			fireball_create(&model_clip_plane_pt, fireball_type, FIREBALL_LARGE_EXPLOSION, OBJ_INDEX(half_ship->parent_obj), rad, 0, &half_ship->parent_obj->phys_info.vel, 0.0f, -1, NULL, low_res_fireballs);

			// start the next fireball up (3-4 per frame) + 30%
			int time_low, time_high;
			time_low = int(650 * Bs_exp_fire_time_mult);
			time_high = int(900 * Bs_exp_fire_time_mult);
			half_ship->next_fireball = timestamp_rand(time_low, time_high);

			// do sound
			do_sub_expl_sound(half_ship->parent_obj->radius, &model_clip_plane_pt, sound_handle);

			// do particles
			particle_emitter	pe;

			pe.num_low = 40;					// Lowest number of particles to create
			pe.num_high = 80;				// Highest number of particles to create
			pe.pos = model_clip_plane_pt;	// Where the particles emit from
			pe.vel = half_ship->phys_info.vel;		// Initial velocity of all the particles

#ifdef FS2_DEMO
			float range = 1.0f + 0.002f*half_ship->parent_obj->radius * 5.0f;
#else 
			float range = 1.0f + 0.002f*half_ship->parent_obj->radius;
#endif

#ifdef FS2_DEMO
			pe.min_life = 2.0f*range;				// How long the particles live
			pe.max_life = 10.0f*range;				// How long the particles live
#else
			pe.min_life = 0.5f*range;				// How long the particles live
			pe.max_life = 6.0f*range;				// How long the particles live
#endif
			pe.normal = vmd_x_vector;		// What normal the particle emit around
			pe.normal_variance = 2.0f;		//	How close they stick to that normal 0=on normal, 1=180, 2=360 degree
			pe.min_vel = 0.0f;				// How fast the slowest particle can move
			pe.max_vel = half_ship->explosion_vel;				// How fast the fastest particle can move

#ifdef FS2_DEMO
			float scale = half_ship->parent_obj->radius * 0.02f;
#else
			float scale = half_ship->parent_obj->radius * 0.01f;
#endif
			pe.min_rad = 0.5f*scale;				// Min radius
			pe.max_rad = 1.5f*scale;				// Max radius

			particle_emit( &pe, PARTICLE_SMOKE2, 0, range );

		} else {
			// time out forever
			half_ship->next_fireball = timestamp(-1);
		}
	}
}

void big_explosion_flash(float);
// Returns 1 when explosion is done
int shipfx_large_blowup_do_frame(ship *shipp, float frametime)
{
	// DAVE:  I made this not do any movement just to try to get things working...
	// return 0;

	Assert( shipp->large_ship_blowup_index > -1 );
	Assert( shipp->large_ship_blowup_index < (int)Split_ships.size() );

	split_ship *the_split_ship = &Split_ships[shipp->large_ship_blowup_index];
	Assert( the_split_ship->used );		// Get John

	// Do fireballs, particles, shockwave here
	// Note parent ship is still valid, vel and pos updated in obj_move_all

	if ( timestamp_elapsed(the_split_ship->explosion_flash_timestamp) ) {
		if ( !the_split_ship->explosion_flash_started ) {
			object* objp = &Objects[shipp->objnum];
			if (objp->flags & OF_WAS_RENDERED) {
				float excess_dist = vm_vec_dist(&Player_obj->pos, &objp->pos) - 2.0f*objp->radius - Player_obj->radius;
				float intensity = 1.0f - 0.1f*excess_dist / objp->radius;

				if (intensity > 1) {
					intensity = 1.0f;
				}

				if (intensity > 0.1f && Ship_info[shipp->ship_info_index].flags2 & SIF2_FLASH) {
					big_explosion_flash(intensity);
				}
			}
			the_split_ship->explosion_flash_started = 1;
		}
	}

	physics_sim(&the_split_ship->front_ship.local_pivot, &the_split_ship->front_ship.orient, &the_split_ship->front_ship.phys_info, frametime);
	physics_sim(&the_split_ship->back_ship.local_pivot,  &the_split_ship->back_ship.orient,  &the_split_ship->back_ship.phys_info,  frametime);
	the_split_ship->front_ship.length_left -= the_split_ship->front_ship.explosion_vel*frametime;
	the_split_ship->back_ship.length_left  += the_split_ship->back_ship.explosion_vel *frametime;
	the_split_ship->front_ship.cur_clip_plane_pt += the_split_ship->front_ship.explosion_vel*frametime;
	the_split_ship->back_ship.cur_clip_plane_pt  += the_split_ship->back_ship.explosion_vel *frametime;

	float length_left = MAX( the_split_ship->front_ship.length_left, the_split_ship->back_ship.length_left );

	//	mprintf(( "Blowup frame, dist = %.1f \n", length_left ));

	if ( length_left < 0 )	{
		the_split_ship->used = 0;
		return 1;
	}

	maybe_fireball_wipe(&the_split_ship->front_ship, (int*)&the_split_ship->sound_handle);
	maybe_fireball_wipe(&the_split_ship->back_ship,  (int*)&the_split_ship->sound_handle);
	return 0;
}

void shipfx_large_blowup_render(ship* shipp)
{
// This actually renders the original model like it should render.
//	object *objp = &Objects[shipp->objnum];
//	model_render( shipp->modelnum, &objp->orient, &objp->pos, MR_NORMAL, -1, -1, shipp->replacement_textures );
//	return;

	Assert( shipp->large_ship_blowup_index > -1 );
	Assert( shipp->large_ship_blowup_index < (int)Split_ships.size() );

	split_ship *the_split_ship = &Split_ships[shipp->large_ship_blowup_index];
	Assert( the_split_ship->used );		// Get John

	// vec3d front_global_pivot, back_global_pivot;

	if (the_split_ship->front_ship.length_left > 0) {
		half_ship_render_ship_and_debris(&the_split_ship->front_ship,shipp);
	}

	if (the_split_ship->back_ship.length_left > 0) {
		half_ship_render_ship_and_debris(&the_split_ship->back_ship,shipp);
	}

	g3_stop_user_clip_plane();			
}


// ================== DO THE ELECTRIC ARCING STUFF =====================
// Creates any new ones, moves old ones.

#define MAX_ARC_LENGTH_PERCENTAGE 0.25f

#define MAX_EMP_ARC_TIMESTAMP		 (150.0f)

void shipfx_do_damaged_arcs_frame( ship *shipp )
{
	int i;
	int should_arc;
	object *obj = &Objects[shipp->objnum];
	int model_num = Ship_info[shipp->ship_info_index].model_num;

	should_arc = 1;
	int disrupted_arc=0;

	float damage = get_hull_pct(obj);	

	if (damage < 0) {
		damage = 0.0f;
	}

	// don't draw an arc based on damage
	if ( damage > 0.30f )	{
		// Don't do spark.
		should_arc = 0;
	}

	// SUSHI: If the lightning type is NONE, we can skip this
	if (Ship_info[shipp->ship_info_index].damage_lightning_type == SLT_NONE)
		should_arc = 0;

	// we should draw an arc
	if( shipp->emp_intensity > 0.0f){
		should_arc = 1;
	}

	if ((ship_subsys_disrupted(shipp,SUBSYSTEM_ENGINE)) ||
		(ship_subsys_disrupted(shipp,SUBSYSTEM_WEAPONS)) || 
		(ship_subsys_disrupted(shipp,SUBSYSTEM_SENSORS)) )
	{
		disrupted_arc=1;
		should_arc=1;
	}

	// Kill off old sparks
	for(i=0; i<MAX_SHIP_ARCS; i++){
		if(timestamp_valid(shipp->arc_timestamp[i]) && timestamp_elapsed(shipp->arc_timestamp[i])){			
			shipp->arc_timestamp[i] = timestamp(-1);
		}
	}

	// if we shouldn't draw an arc, return
	if(!should_arc){
		return;
	}

	if (!timestamp_valid(shipp->arc_next_time))	{
		// start the next fireball up in the next 10 seconds or so... 
		int freq;
		
		// if the emp effect is active or its disrupted
		if((shipp->emp_intensity > 0.0f) || (disrupted_arc)){
			freq = fl2i(MAX_EMP_ARC_TIMESTAMP);
		}
		// otherwise if we're arcing based upon damage
		else {
			freq = fl2i((damage+0.1f)*5000.0f);
		}

		// set the next arc time
		shipp->arc_next_time = timestamp_rand(freq*2,freq*4);
	}

	if ( timestamp_elapsed(shipp->arc_next_time) )	{

		shipp->arc_next_time = timestamp(-1);		// invalid, so it gets restarted next frame

		//mprintf(( "Creating new ship arc!\n" ));

		int n, n_arcs = ((rand()>>5) % 3)+1;		// Create 1-3 sparks

		vec3d v1, v2, v3, v4;
		submodel_get_two_random_points(model_num, -1, &v1, &v2);
		submodel_get_two_random_points(model_num, -1, &v3, &v4);

		// For large ships, cap the length to be 25% of max radius
		if ( obj->radius > 200.0f )	{
			float max_dist = obj->radius * MAX_ARC_LENGTH_PERCENTAGE;
			
			vec3d tmp;
			float d;

			// Cap arc 2->1
			vm_vec_sub( &tmp, &v1, &v2 );
			d = vm_vec_mag_quick( &tmp );
			if ( d > max_dist )	{
				vm_vec_scale_add( &v1, &v2, &tmp, max_dist / d );
			}

			// Cap arc 2->3
			vm_vec_sub( &tmp, &v3, &v2 );
			d = vm_vec_mag_quick( &tmp );
			if ( d > max_dist )	{
				vm_vec_scale_add( &v3, &v2, &tmp, max_dist / d );
			}


			// Cap arc 2->4
			vm_vec_sub( &tmp, &v4, &v2 );
			d = vm_vec_mag_quick( &tmp );
			if ( d > max_dist )	{
				vm_vec_scale_add( &v4, &v2, &tmp, max_dist / d );
			}
			
		}
		
		n = 0;

//		int a = 100, b = 1000;
		float factor = 1.0f + 0.0025f*obj->radius;
		int a = (int) (factor*100.0f);
		int b = (int) (factor*1000.0f);
		int lifetime = (myrand()%((b)-(a)+1))+(a);

		// Create the arc effects
		for (i=0; i<MAX_SHIP_ARCS; i++ )	{
			if ( !timestamp_valid( shipp->arc_timestamp[i] ) )	{
				//shipp->arc_timestamp[i] = timestamp_rand(400,1000);	// live up to a second
				shipp->arc_timestamp[i] = timestamp(lifetime);	// live up to a second

				switch( n )	{
				case 0:
					shipp->arc_pts[i][0] = v1;
					shipp->arc_pts[i][1] = v2;
					break;
				case 1:
					shipp->arc_pts[i][0] = v2;
					shipp->arc_pts[i][1] = v3;
					break;

				case 2:
					shipp->arc_pts[i][0] = v2;
					shipp->arc_pts[i][1] = v4;
					break;

				default:
					Int3();
				}

				// determine what kind of arc to create
				if((shipp->emp_intensity > 0.0f) || (disrupted_arc)){
					shipp->arc_type[i] = MARC_TYPE_EMP;
				} else {
					shipp->arc_type[i] = MARC_TYPE_NORMAL;
				}
					
				n++;
				if ( n == n_arcs )
					break;	// Don't need to create anymore
			}
	
			// rotate v2 out of local coordinates into world.
			// Use v2 since it is used in every bolt.  See above switch().
			vec3d snd_pos;
			vm_vec_unrotate(&snd_pos, &v2, &obj->orient);
			vm_vec_add2(&snd_pos, &obj->pos );

			//Play a sound effect
			if ( lifetime > 750 )	{
				// 1.00 second effect
				snd_play_3d( &Snds[SND_DEBRIS_ARC_05], &snd_pos, &View_position, obj->radius );
			} else if ( lifetime >  500 )	{
				// 0.75 second effect
				snd_play_3d( &Snds[SND_DEBRIS_ARC_04], &snd_pos, &View_position, obj->radius );
			} else if ( lifetime >  250 )	{
				// 0.50 second effect
				snd_play_3d( &Snds[SND_DEBRIS_ARC_03], &snd_pos, &View_position, obj->radius );
			} else if ( lifetime >  100 )	{
				// 0.25 second effect
				snd_play_3d( &Snds[SND_DEBRIS_ARC_02], &snd_pos, &View_position, obj->radius );
			} else {
				// 0.10 second effect
				snd_play_3d( &Snds[SND_DEBRIS_ARC_01], &snd_pos, &View_position, obj->radius );
			}
		}
	}

	// maybe move arc points around
	for (i=0; i<MAX_SHIP_ARCS; i++ )	{
		if ( timestamp_valid( shipp->arc_timestamp[i] ) )	{
			if ( !timestamp_elapsed( shipp->arc_timestamp[i] ) )	{							
				// Maybe move a vertex....  20% of the time maybe?
				int mr = myrand();
				if ( mr < RAND_MAX/5 )	{
					vec3d v1, v2;
					submodel_get_two_random_points(model_num, -1, &v1, &v2);

					vec3d static_one;

					if ( mr % 2 )	{
						static_one = shipp->arc_pts[i][0];
					} else {
						static_one = shipp->arc_pts[i][1];
					}

					// For large ships, cap the length to be 25% of max radius
					if ( obj->radius > 200.0f )	{
						float max_dist = obj->radius * MAX_ARC_LENGTH_PERCENTAGE;
						
						vec3d tmp;
						float d;

						// Cap arc 2->1
						vm_vec_sub( &tmp, &v1, &static_one );
						d = vm_vec_mag_quick( &tmp );
						if ( d > max_dist )	{
							vm_vec_scale_add( &v1, &static_one, &tmp, max_dist / d );
						}
					}

					shipp->arc_pts[i][mr % 2] = v1;
				}
			}
		}
	}
}

int l_cruiser_count = 1;
int l_big_count = 2;
int l_huge_count = 3;
float l_max_radius = 3000.0f;
void shipfx_do_lightning_frame( ship *shipp )
{
	/*
	ship_info *sip;
	object *objp;
	int stamp, count;
	vec3d v1, v2, n1, n2, temp, temp2;
	bolt_info binfo;

	// sanity checks
	Assert(shipp != NULL);
	if(shipp == NULL){
		return;
	} 
	Assert(shipp->ship_info_index >= 0);
	if(shipp->ship_info_index < 0){
		return;
	}	
	Assert(shipp->objnum >= 0);
	if(shipp->objnum < 0){
		return;
	}	

	// get some pointers
	sip = &Ship_info[shipp->ship_info_index];
	objp = &Objects[shipp->objnum];	

	// if this is not a nebula mission, don't do anything
	if(!(The_mission.flags & MISSION_FLAG_FULLNEB)){
		shipp->lightning_stamp = -1;
		return;
	}
	
	// if this not a cruiser or big ship
	if(!((sip->flags & SIF_CRUISER) || (sip->flags & SIF_BIG_SHIP) || (sip->flags & SIF_HUGE_SHIP))){
		shipp->lightning_stamp = -1;
		return;
	}

	// determine stamp and count values
	if(sip->flags & SIF_CRUISER){
		stamp = (int)((float)(Nebl_cruiser_min + ((Nebl_cruiser_max - Nebl_cruiser_min) * Nebl_intensity)) * frand_range(0.8f, 1.1f));
		count = l_cruiser_count;
	} 
	else {
		if(sip->flags & SIF_HUGE_SHIP){
			stamp = (int)((float)(Nebl_supercap_min + ((Nebl_supercap_max - Nebl_supercap_min) * Nebl_intensity)) * frand_range(0.8f, 1.1f));
			count = l_huge_count;
		} else {
			stamp = (int)((float)(Nebl_cap_min + ((Nebl_cap_max - Nebl_cap_min) * Nebl_intensity)) * frand_range(0.8f, 1.1f));
			count = l_big_count;
		}
	}

	// if his timestamp is unset
	if(shipp->lightning_stamp == -1){
		shipp->lightning_stamp = timestamp(stamp);
		return;
	}
	// if his timestamp is currently unelapsed
	if(!timestamp_elapsed(shipp->lightning_stamp)){
		return;
	}

	mprintf(("SHIP BOLT\n"));

	// restamp him first
	shipp->lightning_stamp = timestamp(stamp);

	// ah, now we can create some lightning bolts
	count = (int)frand_range(0.0f, (float)count);
	while(count > 0){
		// get 2 points on the hull of the ship
		submodel_get_two_random_points(shipp->modelnum, 0, &v1, &v2, &n1, &n2);		

		// make up to 2 bolts
		if(objp->radius > l_max_radius){
			vm_vec_scale_add(&temp2, &v1, &n1, l_max_radius);
		} else {
			vm_vec_scale_add(&temp2, &v1, &n1, objp->radius);
		}
		vm_vec_unrotate(&temp, &temp2, &objp->orient);
		vm_vec_add2(&temp, &objp->pos);
		vm_vec_unrotate(&temp2, &v1, &objp->orient);
		vm_vec_add2(&temp2, &objp->pos);

		// create the bolt
		binfo.start = temp;
		binfo.strike = temp2;
		binfo.num_strikes = 3;
		binfo.noise = 0.045f;
		binfo.life = 375;
		binfo.delay = (int)frand_range(0.0f, 1600.0f);
		nebl_bolt(&binfo);
		count--;
	
		// done
		if(count <= 0){
			break;
		}

		// one more		
		if(objp->radius > l_max_radius){
			vm_vec_scale_add(&temp2, &v2, &n2, l_max_radius);
		} else {
			vm_vec_scale_add(&temp2, &v2, &n2, objp->radius);
		}
		vm_vec_unrotate(&temp, &temp2, &objp->orient);
		vm_vec_add2(&temp, &objp->pos);
		vm_vec_unrotate(&temp2, &v2, &objp->orient);
		vm_vec_add2(&temp2, &objp->pos);

		// create the bolt
		binfo.start = temp;
		binfo.strike = temp2;
		binfo.num_strikes = 3;
		binfo.noise = 0.045f;
		binfo.life = 375;
		binfo.delay = (int)frand_range(0.0f, 1600.0f);
		nebl_bolt(&binfo);		
		count--;
	}
	*/
}

// do all shockwaves for a ship blowing up
void shipfx_do_shockwave_stuff(ship *shipp, shockwave_create_info *sci)
{
	ship_info *sip;
	object *objp;
	polymodel *pm;
	vec3d temp, dir, shockwave_pos;
	vec3d head = vmd_zero_vector;
	vec3d tail = vmd_zero_vector;	
	float len, step, cur;
	int idx;

	// sanity checks
	Assert(shipp != NULL);
	if(shipp == NULL){
		return;
	} 
	Assert(shipp->ship_info_index >= 0);
	if(shipp->ship_info_index < 0){
		return;
	}	
	Assert(shipp->objnum >= 0);
	if(shipp->objnum < 0){
		return;
	}
	Assert(sci != NULL);
	if (sci == NULL) {
		return;
	}

	// get some pointers
	sip = &Ship_info[shipp->ship_info_index];
	objp = &Objects[shipp->objnum];	

	if(sip->shockwave_count <= 0){
		return;
	}

	// get vectors at the head and tail of the object, dead center		
	pm = model_get(sip->model_num);
	if(pm == NULL){
		return;
	}
	head.xyz.x = pm->submodel[0].offset.xyz.x;
	head.xyz.y = pm->submodel[0].offset.xyz.y;
	head.xyz.z = pm->maxs.xyz.z;

	tail.xyz.x = pm->submodel[0].offset.xyz.x;
	tail.xyz.y = pm->submodel[0].offset.xyz.y;
	tail.xyz.z = pm->mins.xyz.z;

	// transform the vectors into world coords
	vm_vec_unrotate(&temp, &head, &objp->orient);
	vm_vec_add(&head, &temp, &objp->pos);
	vm_vec_unrotate(&temp, &tail, &objp->orient);
	vm_vec_add(&tail, &temp, &objp->pos);

	// now create as many shockwaves as needed
	vm_vec_sub(&dir, &head, &tail);
	len = vm_vec_mag(&dir);
	step = 1.0f / ((float)sip->shockwave_count + 1.0f);
	cur = step;
	for(idx=0; idx<sip->shockwave_count; idx++){
		// get the shockwave position		
		temp = dir;
		vm_vec_scale(&temp, cur);
		vm_vec_add(&shockwave_pos, &tail, &temp);

		// if knossos device, make shockwave in center
		if (Ship_info[shipp->ship_info_index].flags & SIF_KNOSSOS_DEVICE) {
			shockwave_pos = Objects[shipp->objnum].pos;
		}

		// create the shockwave
		shockwave_create_info sci2 = *sci;
		sci2.blast = (sci->blast / (float)sip->shockwave_count) * frand_range(0.75f, 1.25f);
		sci2.damage = (sci->damage / (float)sip->shockwave_count) * frand_range(0.75f, 1.25f);
		sci2.speed = sci->speed * frand_range(0.75f, 1.25f);
		sci2.rot_angles.p = frand_range(0.0f, 1.99f*PI);
		sci2.rot_angles.b = frand_range(0.0f, 1.99f*PI);
		sci2.rot_angles.h = frand_range(0.0f, 1.99f*PI);

		shockwave_create(shipp->objnum, &shockwave_pos, &sci2, SW_SHIP_DEATH, (int)frand_range(0.0f, 350.0f));
		// shockwave_create(shipp->objnum, &objp->pos, sip->shockwave_speed, sip->inner_rad, sip->outer_rad, sip->damage, sip->blast, SW_SHIP_DEATH);

		// next shockwave
		cur += step;
	}
}

extern int model_should_render_engine_glow(int objnum, int bank_obj);
int Wash_on = 1;
DCF_BOOL(engine_wash, Wash_on)
#define ENGINE_WASH_CHECK_INTERVAL		250	// (4x sec)
// Do engine wash effect for ship
// Assumes length of engine wash is greater than radius of engine wash hemisphere
void engine_wash_ship_process(ship *shipp)
{
	int idx, j;		
	object *objp, *max_ship_intensity_objp;
	int started_with_no_wash = shipp->wash_intensity <= 0 ? 1 : 0;

	if (!Wash_on) {
		return;
	}

	Assert(shipp != NULL);
	Assert(shipp->objnum >= 0);
	objp = &Objects[shipp->objnum];
	ship_obj *so;

	vec3d world_thruster_pos, world_thruster_norm, apex, thruster_to_ship, apex_to_ship, temp;
	float dist_sqr, inset_depth, dot_to_ship, max_ship_intensity;
	polymodel *pm;

	float max_wash_dist, half_angle, radius_mult;

	// if this is not a fighter or bomber, we don't care
	if ((objp->type != OBJ_SHIP) || !(Ship_info[shipp->ship_info_index].flags & (SIF_FIGHTER|SIF_BOMBER)) ) {
		return;
	}

	// is it time to check for engine wash 
	int time_to_next_hit = timestamp_until(shipp->wash_timestamp);
	if (time_to_next_hit < 0) {
		if (time_to_next_hit < -ENGINE_WASH_CHECK_INTERVAL) {
			time_to_next_hit = 0;
		}

		// keep interval constant independent of framerate
		shipp->wash_timestamp = timestamp(ENGINE_WASH_CHECK_INTERVAL + time_to_next_hit);

		// initialize wash params
		shipp->wash_intensity = 0.0f;
		vm_vec_zero(&shipp->wash_rot_axis);
		max_ship_intensity_objp = NULL;
		max_ship_intensity = 0;
	} else {
		return;
	}

	// only do damage if we're within half of the max wash distance
	int do_damage = 0;

	// go thru Ship_used_list and check if we're in wash from CAP or SUPERCAP (SIF_HUGE)
	for (so = GET_FIRST(&Ship_obj_list); so != END_OF_LIST(&Ship_obj_list); so = GET_NEXT(so)) {
		if (so->objnum < 0) {
			continue;
		}

		object *wash_objp = &Objects[so->objnum];

		if (!wash_objp || (wash_objp->instance < 0) || (wash_objp->type != OBJ_SHIP)) {
			continue;
		}

		ship *wash_shipp = &Ships[wash_objp->instance];
		ship_info *wash_sip = &Ship_info[wash_shipp->ship_info_index];

		// don't do small ships
		if ( (wash_sip->flags & SIF_SMALL_SHIP) ) {
			continue;
		}

		pm = model_get(wash_sip->model_num);
		float ship_intensity = 0;

		// if engines disabled, no engine wash
		if (ship_get_subsystem_strength(wash_shipp, SUBSYSTEM_ENGINE) < 0.01) {
			continue;
		}

		float	speed_scale;
		if (wash_objp->phys_info.speed > 20.0f)
			speed_scale = 1.0f;
		else
			speed_scale = wash_objp->phys_info.speed/20.0f;

		for (idx = 0; idx < pm->n_thrusters; idx++) {
			thruster_bank *bank = &pm->thrusters[idx];

			// make sure this engine is functional before we try to process a wash from it
			if ( !model_should_render_engine_glow(OBJ_INDEX(wash_objp), bank->obj_num) ) {
				continue;
			}

			// check if thruster bank has engine wash
			if (bank->wash_info_pointer == NULL) {
				// if huge, give default engine wash
				if ((wash_sip->flags & SIF_HUGE_SHIP) && Engine_wash_info.size()) {
					bank->wash_info_pointer = &Engine_wash_info[0];
					nprintf(("wash", "Adding default engine wash to ship %s", wash_sip->name));
				} else {
					continue;
				}
			}

			engine_wash_info *ewp = bank->wash_info_pointer;
			half_angle = ewp->angle;
			radius_mult = ewp->radius_mult;

			for (j=0; j<bank->num_points; j++) {
				// get world pos of thruster
				vm_vec_unrotate(&world_thruster_pos, &bank->points[j].pnt, &wash_objp->orient);
				vm_vec_add2(&world_thruster_pos, &wash_objp->pos);
				
				// get world norm of thruster;
				vm_vec_unrotate(&world_thruster_norm, &bank->points[j].norm, &wash_objp->orient);

				// get vector from thruster to ship
				vm_vec_sub(&thruster_to_ship, &objp->pos, &world_thruster_pos);

				// check if on back side of thruster
				dot_to_ship = vm_vec_dotprod(&thruster_to_ship, &world_thruster_norm);
				if (dot_to_ship > 0) {

					// get max wash distance
					max_wash_dist = MAX(ewp->length, bank->points[j].radius * ewp->radius_mult);

					// check if within dist range
					dist_sqr = vm_vec_mag_squared(&thruster_to_ship);
					if (dist_sqr < max_wash_dist*max_wash_dist) {

						// check if inside the sphere
						if ( dist_sqr < ((radius_mult * radius_mult) * (bank->points[j].radius * bank->points[j].radius)) ) {
							vm_vec_crossprod(&temp, &world_thruster_norm, &thruster_to_ship);
							vm_vec_scale_add2(&shipp->wash_rot_axis, &temp, dot_to_ship / dist_sqr);
//							shipp->wash_intensity += (1.0f - dist_sqr / (max_wash_dist*max_wash_dist));
							ship_intensity += (1.0f - dist_sqr / (max_wash_dist*max_wash_dist));
							if (!do_damage) {
								if (dist_sqr < 0.25 * max_wash_dist * max_wash_dist) {
									do_damage = 1;
								}
							}
						} else {
							// check if inside cone - first fine apex of cone
							inset_depth = (bank->points[j].radius / fl_tan(half_angle));
							vm_vec_scale_add(&apex, &world_thruster_pos, &world_thruster_norm, -inset_depth);
							vm_vec_sub(&apex_to_ship, &objp->pos, &apex);
							vm_vec_normalize(&apex_to_ship);

							// check if inside cone angle
							if (vm_vec_dotprod(&apex_to_ship, &world_thruster_norm) > cos(half_angle)) {
								vm_vec_crossprod(&temp, &world_thruster_norm, &thruster_to_ship);
								vm_vec_scale_add2(&shipp->wash_rot_axis, &temp, dot_to_ship / dist_sqr);
//								shipp->wash_intensity += (1.0f - dist_sqr / (max_wash_dist*max_wash_dist));
								ship_intensity += (1.0f - dist_sqr / (max_wash_dist*max_wash_dist));
								if (!do_damage) {
									if (dist_sqr < 0.25 * max_wash_dist * max_wash_dist) {
										do_damage = 1;
									}
								}
							}
						}
					}
				}
			}
		}
		shipp->wash_intensity += ship_intensity * speed_scale;
		if (ship_intensity > max_ship_intensity) {
			max_ship_intensity = ship_intensity;
			max_ship_intensity_objp = wash_objp;
		}
	}

	// apply damage at rate of 1%/sec
	if (shipp->wash_intensity > 0) {
		Assert(max_ship_intensity_objp != NULL);

		nprintf(("wash", "Wash intensity %.2f\n", shipp->wash_intensity));

		float damage;
		if (!do_damage) {
			damage = 0;
		} else {
			damage = (0.001f * 0.003f * ENGINE_WASH_CHECK_INTERVAL * shipp->ship_max_hull_strength * shipp->wash_intensity);
		}

		ship_apply_wash_damage(objp, max_ship_intensity_objp, damage);

		// if we had no wash before now, add the wash object sound
		if(started_with_no_wash){
			if(shipp != Player_ship){
				obj_snd_assign(shipp->objnum, SND_ENGINE_WASH, &vmd_zero_vector, 1);
			} else {				
				Player_engine_wash_loop = snd_play_looping( &Snds[SND_ENGINE_WASH], 0.0f , -1, -1, 1.0f);
			}
		}
	} 
	// if we've got no wash, kill any wash object sounds from this guy
	else {
		if(shipp != Player_ship){
			obj_snd_delete_type(shipp->objnum, SND_ENGINE_WASH);
		} else {
			snd_stop(Player_engine_wash_loop);
			Player_engine_wash_loop = -1;
		}
	}
}

// engine wash level init
void shipfx_engine_wash_level_init()
{
	Player_engine_wash_loop = -1;
}

// pause engine wash sounds
void shipfx_stop_engine_wash_sound()
{
	if(Player_engine_wash_loop != -1){
		snd_stop(Player_engine_wash_loop);
		Player_engine_wash_loop = -1;
	}
}

void shipfx_start_cloak(ship *shipp, int warmup, int recalc_matrix, int device)
{

	//get a random vector to translate the texture matrix
	//since we don't need a z-value, zero it and renormalize
	if (recalc_matrix)
	{	
		vm_vec_rand_vec_quick(&shipp->texture_translation_key);
		shipp->texture_translation_key.xyz.z=0;
		vm_vec_normalize_quick(&shipp->texture_translation_key);
		vm_vec_zero(&shipp->current_translation);
	}

	shipp->time_until_full_cloak=timestamp(warmup);
	shipp->cloak_stage=1;

	//this will be true if we aren't cheating ;)
	if (device)
	{
	}
	else
	{
		shipp->time_until_uncloak=0;
	}

}

float shipfx_calc_visibility(object *obj, vec3d *view_pt)
{
	if (obj->type != OBJ_SHIP)
		return 1.0f;

	float dist = vm_vec_dist(&obj->pos,view_pt);
			
	//half bright if less than 3x radius
	if (dist <= (obj->radius *3.0f) )
	{
		return 0.5f;
	}
	
	//almost invis if greater than 20x radius
	else if (dist >= (obj->radius * 20.0f) )
	{
		return 0.03125;
	}
	
	//otherwise do linear attenuation
	//(final radius * max_alpha / dx ) - (slope * (dist / radius))
	else
	{
		float factor = dist / obj->radius;
		float alpha = (10.0f/17.0f) - ((.46875f/17.0f) * factor);
		return alpha;	
	}
}

void shipfx_cloak_frame(ship *shipp, float frametime)
{
	switch (shipp->cloak_stage)
	{
		case 1:
			if (timestamp_elapsed(shipp->time_until_full_cloak))
			{
				shipp->cloak_stage=2;
				shipp->flags2 |= SF2_STEALTH;
			}
			break;
		
		case 2:
		{

			if (timestamp_elapsed(shipp->time_until_uncloak) && (shipp->time_until_uncloak != 0))
			{
				break;
			}
			//uncloak if departing or arriving
			if (shipp->flags & (SF_DEPART_WARP | SF_ARRIVING))
			{
				shipfx_stop_cloak(shipp,1000);
				break;
			}

			//find an approiate alpha level for the ship
			object *obj=&Objects[shipp->objnum];
			//stop cloaking if <20% hits left. make it look cool tho
			if (get_hull_pct(obj) < .2f)
			{
				shipfx_stop_cloak(shipp,25000);
				break;			//don't need to calc alpha
			}

			shipp->cloak_alpha = fl2i(shipfx_calc_visibility(obj, &Eye_position) * 255.0f);

		}
			break;
		case 3:
			if (timestamp_elapsed(shipp->time_until_full_cloak))
			{
				shipp->cloak_stage=0;
			}
			break;
		
		default:
			return;
	}

	//do something to the texture matrix
	vm_vec_scale_add2(&shipp->current_translation,&shipp->texture_translation_key,frametime);

}

void shipfx_stop_cloak(ship *shipp, int warmdown)
{
	shipp->cloak_stage=3;
	shipp->time_until_full_cloak=timestamp(warmdown);
	if (!(Ship_info[shipp->ship_info_index].flags & SIF_STEALTH))
		shipp->flags2 &= ~(SF2_STEALTH);
}

class CombinedVariable
{
public:
	static const int TYPE_NONE;
	static const int TYPE_FLOAT;
	static const int TYPE_IMAGE;
	static const int TYPE_INT;
	static const int TYPE_SOUND;
	static const int TYPE_STRING;
private:
	int Type;
	union StorageUnion
	{
		float	su_Float;
		int		su_Image;
		int		su_Int;
		int		su_Sound;
		char	*su_String;
	} StorageUnion;
public:
	//TYPE_NONE
	CombinedVariable();
	//TYPE_FLOAT
	CombinedVariable(float n_Float);
	//TYPE_INT
	CombinedVariable(int n_Int);
	//TYPE_IMAGE, TYPE_SOUND
	CombinedVariable(int n_Int, ubyte type_override);
	//TYPE_STRING
	CombinedVariable(char *n_String);
	//All types
	~CombinedVariable();

	//Returns 1 if buffer was successfully written to
	int getFloat(float *output);
	//Returns handle or < 0 on failure/wrong type
	int getHandle();
	//Returns handle, or < 0 on failure/wrong type
	int getImage();
	//Returns 1 if buffer was successfully written to
	int getInt(int *output);
	//Returns handle, or < 0 on failure/wrong type
	int getSound();
	//Returns 1 if buffer was successfully written to
	int getString(char *output, size_t output_max);

	//Returns true if TYPE_NONE
	bool isEmpty();
};

//Workaround for MSVC6
const int CombinedVariable::TYPE_NONE=0;
const int CombinedVariable::TYPE_FLOAT = 1;
const int CombinedVariable::TYPE_IMAGE = 2;
const int CombinedVariable::TYPE_INT = 3;
const int CombinedVariable::TYPE_SOUND = 4;
const int CombinedVariable::TYPE_STRING  = 5;

//Member functions
CombinedVariable::CombinedVariable()
{
	Type = TYPE_NONE;
}

CombinedVariable::CombinedVariable(float n_Float)
{
	Type = TYPE_FLOAT;
	StorageUnion.su_Float = n_Float;
}

CombinedVariable::CombinedVariable(int n_Int)
{
	Type = TYPE_INT;
	StorageUnion.su_Int = n_Int;
}

CombinedVariable::CombinedVariable(int n_Int, ubyte type_override)
{
	if(type_override == TYPE_IMAGE)
	{
		Type = TYPE_IMAGE;
		StorageUnion.su_Image = n_Int;
	}
	else if(type_override == TYPE_SOUND)
	{
		Type = TYPE_SOUND;
		StorageUnion.su_Sound = n_Int;
	}
	else
	{
		Type = TYPE_INT;
		StorageUnion.su_Int = n_Int;
	}
}

CombinedVariable::CombinedVariable(char *n_String)
{
	Type = TYPE_STRING;
	StorageUnion.su_String = (char *)malloc(strlen(n_String)+1);
	strcpy(StorageUnion.su_String, n_String);
}

CombinedVariable::~CombinedVariable()
{
	if(Type == TYPE_STRING)
	{
		free(StorageUnion.su_String);
	}
}

int CombinedVariable::getFloat(float *output)
{
	if(Type == TYPE_FLOAT)
	{
		*output  = StorageUnion.su_Float;
		return 1;
	}
	if(Type == TYPE_IMAGE)
	{
		*output = i2fl(StorageUnion.su_Image);
		return 1;
	}
	if(Type == TYPE_INT)
	{
		*output = i2fl(StorageUnion.su_Int);
		return 1;
	}
	if(Type == TYPE_SOUND)
	{
		*output = i2fl(StorageUnion.su_Sound);
		return 1;
	}
	if(Type == TYPE_STRING)
	{
		*output = (float)atof(StorageUnion.su_String);
		return 1;
	}
	return 0;
}
int CombinedVariable::getHandle()
{
	int i = 0;
	if(this->getInt(&i))
		return i;
	else
		return -1;
}
int CombinedVariable::getImage()
{
	if(Type == TYPE_IMAGE)
		return this->getHandle();
	else
		return -1;
}
int CombinedVariable::getInt(int *output)
{
	if(output == NULL)
		return 0;

	if(Type == TYPE_FLOAT)
	{
		*output  = fl2i(StorageUnion.su_Float);
		return 1;
	}
	if(Type == TYPE_IMAGE)
	{
		*output = StorageUnion.su_Image;
		return 1;
	}
	if(Type == TYPE_INT)
	{
		*output = StorageUnion.su_Int;
		return 1;
	}
	if(Type == TYPE_SOUND)
	{
		*output = StorageUnion.su_Sound;
		return 1;
	}
	if(Type == TYPE_STRING)
	{
		*output = atoi(StorageUnion.su_String);
		return 1;
	}

	return 0;
}
int CombinedVariable::getSound()
{
	if(Type == TYPE_SOUND)
		return this->getHandle();
	else
		return -1;
}
int CombinedVariable::getString(char *output, size_t output_max)
{
	if(output == NULL || output_max == 0)
		return 0;

	if(Type == TYPE_FLOAT)
	{
		snprintf(output, output_max, "%f", StorageUnion.su_Float);
		return 1;
	}
	if(Type == TYPE_IMAGE)
	{
		if(bm_is_valid(StorageUnion.su_Image))
			snprintf(output, output_max, "%s", bm_get_filename(StorageUnion.su_Image));
		return 1;
	}
	if(Type == TYPE_INT)
	{
		snprintf(output, output_max, "%i", StorageUnion.su_Int);
		return 1;
	}
	if(Type == TYPE_SOUND)
	{
		Error(LOCATION, "Sound CombinedVariables are not supported yet.");
		/*if(snd_is_valid(StorageUnion.su_Sound))
			snprintf(output, output_max, "%s", snd_get_filename(StorageUnion.su_Sound));*/
		return 1;
	}
	if(Type == TYPE_STRING)
	{
		strncpy(output, StorageUnion.su_String, output_max);
		return 1;
	}
	return 0;
}
bool CombinedVariable::isEmpty()
{
	return (Type != TYPE_NONE);
}

void parse_combined_variable_list(CombinedVariable *dest, flag_def_list *src, size_t num)
{
	if(dest == NULL || src == NULL || num == 0)
		return;

	char buf[NAME_LENGTH*2];
	flag_def_list *sp = NULL;
	CombinedVariable *dp = NULL;
	for(size_t i = 0; i < num; i++)
	{
		sp = &src[i];
		dp = &dest[i];

		snprintf(buf, sizeof(buf)-1, "+%s:", sp->name);
		if(optional_string(buf))
		{
			switch(sp->var)
			{
				case CombinedVariable::TYPE_FLOAT:
				{
					float f = 0.0f;
					stuff_float(&f);
					*dp = CombinedVariable(f);
					break;
				}
				case CombinedVariable::TYPE_INT:
				{
					int myInt = 0;
					stuff_int(&myInt);
					*dp = CombinedVariable(myInt);
					break;
				}
				case CombinedVariable::TYPE_IMAGE:
				{
					char buf2[MAX_FILENAME_LEN];
					stuff_string(buf2, F_NAME, MAX_FILENAME_LEN);
					int idx = bm_load(buf2);
					*dp = CombinedVariable(idx, CombinedVariable::TYPE_IMAGE);
					break;
				}
				case CombinedVariable::TYPE_SOUND:
				{
					char buf2[MAX_FILENAME_LEN];
					stuff_string(buf2, F_NAME, MAX_FILENAME_LEN);
					int idx = gamesnd_get_by_name(buf);
					*dp = CombinedVariable(idx, CombinedVariable::TYPE_SOUND);
					break;
				}
				case CombinedVariable::TYPE_STRING:
				{
					char buf2[MAX_NAME_LEN + MAX_FILENAME_LEN];
					stuff_string(buf2, F_NAME, MAX_FILENAME_LEN+MAX_NAME_LEN);
					*dp = CombinedVariable(buf2);
					break;
				}
			}
		}
	}
}

#define WV_ANIMATION		0
#define WV_RADIUS			1
#define WV_SPEED			2
#define WV_TIME				3

flag_def_list Warp_variables[] = {
	{"Animation",		WV_ANIMATION,		CombinedVariable::TYPE_STRING},
	{"Radius",			WV_RADIUS,			CombinedVariable::TYPE_FLOAT},
	{"Speed",			WV_SPEED,			CombinedVariable::TYPE_FLOAT},
	{"Time",			WV_TIME,			CombinedVariable::TYPE_FLOAT},
};
static const size_t Warp_variables_num = sizeof(Warp_variables)/sizeof(flag_def_list);

//********************-----CLASS: WarpEffect-----********************//
WarpEffect::WarpEffect()
{
	this->clear();
}

WarpEffect::WarpEffect(object *n_objp, int n_direction)
{
	this->clear();
	if(n_objp != NULL && n_objp->type == OBJ_SHIP && n_objp->instance > -1 && Ships[n_objp->instance].ship_info_index > -1)
	{
		objp = n_objp;
		direction = n_direction;

		//Setup courtesy variables
		shipp = &Ships[objp->instance];
		sip = &Ship_info[shipp->ship_info_index];
	}
}

void WarpEffect::clear()
{
	objp = NULL;
	direction = WD_NONE;
	shipp = NULL;
	sip = NULL;
}

bool WarpEffect::isValid()
{
	if(objp == NULL)
		return false;

	return true;
}

void WarpEffect::pageIn()
{
}
void WarpEffect::pageOut()
{
}

int WarpEffect::warpStart()
{
	if(!this->isValid())
		return 0;

	return 1;
}

int WarpEffect::warpFrame(float frametime)
{
	return 0;
}

int WarpEffect::warpShipClip()
{
	return 0;
}

int WarpEffect::warpShipRender()
{
	return 0;
}

int WarpEffect::warpEnd()
{
	if(!this->isValid())
		return 0;

	shipp->flags &= (~SF_ARRIVING_STAGE_1);
	shipp->flags &= (~SF_ARRIVING_STAGE_2);
	shipp->flags &= (~SF_DEPART_WARP);

	// let physics in on it too.
	objp->phys_info.flags &= (~PF_WARP_IN);
	objp->phys_info.flags &= (~PF_WARP_OUT);

	if(direction == WD_WARP_OUT)
		ship_actually_depart(objp->instance);

	return 1;
}

int WarpEffect::getWarpPosition(vec3d *output)
{
	if(!this->isValid())
		return 0;

	*output = objp->pos;
	return 1;
}

int WarpEffect::getWarpOrientation(matrix* output)
{
	if(!this->isValid())
		return 0;

	*output = objp->orient;
	return 1;
}

//********************-----CLASS: WE_Default-----********************//
WE_Default::WE_Default(object *n_objp, int n_direction)
	:WarpEffect(n_objp, n_direction)
{
	if(!this->isValid())
		return;

	portal_objp = NULL;
	stage_duration[0] = 0;

	pos = vmd_zero_vector;
	fvec = vmd_zero_vector;
}

int WE_Default::warpStart()
{
	if(!this->isValid())
		return 0;

	if(direction == WD_WARP_OUT && objp == Player_obj)
	{
		HUD_printf(NOX("Subspace drive engaged"));
	}

	int portal_objnum;
	if (direction == WD_WARP_IN)
		portal_objnum = shipp->special_warpin_objnum;
	else if (direction == WD_WARP_OUT)
		portal_objnum = shipp->special_warpout_objnum;
	else
		portal_objnum = -1;

	portal_objp = NULL;
	if(portal_objnum >= 0 && shipfx_special_warp_objnum_valid(portal_objnum))
	{
		portal_objp = &Objects[portal_objnum];
	}

	float warpout_speed = 0.0f;
	float warp_time = 0.0f;
	if(direction == WD_WARP_IN)
	{
		polymodel *pm = model_get(sip->model_num);
		vm_vec_scale_add( &pos, &objp->pos, &objp->orient.vec.fvec, (pm != NULL) ? -pm->mins.xyz.z : objp->radius );

		// Effect time is 'SHIPFX_WARP_DELAY' (1.5 secs) seconds to start, 'shipfx_calculate_warp_time' 
		// for ship to go thru, and 'SHIPFX_WARP_DELAY' (1.5 secs) to go away.
		warp_time = shipfx_calculate_warp_time(objp, WD_WARP_IN) + SHIPFX_WARP_DELAY + SHIPFX_WARP_DELAY;
	}
	else
	{
		compute_warpout_stuff(objp, &warpout_speed, &warp_time, &pos);
		warp_time += SHIPFX_WARP_DELAY;
	}

	radius = shipfx_calculate_effect_radius(objp, direction);
	// cap radius to size of knossos
	if(portal_objp != NULL)
	{
		// cap radius to size of knossos
		radius = MIN(radius, 0.8f*portal_objp->radius);
	}

	int warp_objnum = -1;
	if (direction == WD_WARP_OUT)
	{
		// maybe special warpout
		int fireball_type = ((portal_objp != NULL) || (sip->warpout_type == WT_USE_KNOSSOS)) ? FIREBALL_KNOSSOS : FIREBALL_WARP;

		// create fireball
		warp_objnum = fireball_create(&pos, fireball_type, FIREBALL_WARP_EFFECT, OBJ_INDEX(objp), radius, 1, NULL, warp_time, shipp->ship_info_index, NULL, 0, 0, sip->warpout_snd_start, sip->warpout_snd_end);
	}
	else if (direction == WD_WARP_IN)
	{
		// maybe special warpin
		int fireball_type = (portal_objp != NULL) ? FIREBALL_KNOSSOS : FIREBALL_WARP;

		// create fireball
		warp_objnum = fireball_create(&pos, fireball_type, FIREBALL_WARP_EFFECT, OBJ_INDEX(objp), radius, 0, NULL, warp_time, shipp->ship_info_index, NULL, 0, 0, sip->warpin_snd_start, sip->warpin_snd_end);
	}
	else
	{
		Int3();
	}

	//WMC - bail
	// JAS: This must always be created, if not, just warp the ship in/out
	if (warp_objnum < 0)
	{
		this->warpEnd();
		return 0;
	}

	// maybe negate if special warp effect
	fvec = Objects[warp_objnum].orient.vec.fvec;
	if (portal_objp != NULL)
	{
        float dot = vm_vec_dotprod(&fvec, &objp->orient.vec.fvec);

		if ( ((dot < 0) && (direction == WD_WARP_IN)) ||
            ((dot > 0) && (direction == WD_WARP_OUT)) )
		{
			vm_vec_negate(&fvec);
		}
	}

	stage_time_start = total_time_start = timestamp();
	if(direction == WD_WARP_IN)
	{
		stage_duration[1] = fl2i(SHIPFX_WARP_DELAY*1000.0f);
		stage_duration[2] = fl2i(shipfx_calculate_warp_time(objp, WD_WARP_IN)*1000.0f);
		stage_time_end = timestamp(stage_duration[1]);
		total_time_end = stage_duration[1] + stage_duration[2];
		shipp->flags |= SF_ARRIVING_STAGE_1;
	}
	else if(direction == WD_WARP_OUT)
	{
		shipp->flags |= SF_DEPART_WARP;

		// Make the warp effect stage 1 last SHIP_WARP_TIME1 seconds.
		if ( objp == Player_obj )	{
			stage_duration[1] = fl2i(fireball_lifeleft(&Objects[warp_objnum])*1000.0f);
			total_time_end = timestamp(fl2i(warp_time*1000.0f));
		} else {
			total_time_end = timestamp(fl2i(warp_time*2.0f*1000.0f));
		}

	//	mprintf(( "Warp time = %.4f , effect time = %.4f ms\n", warp_time*1000.0f, effect_time ));

		// This is a hack to make the ship go at the right speed to go from it's current position to the warp_effect_pos;
		
		// Set ship's velocity to 'speed'
		// This should actually be an AI that flies from the current
		// position through 'shipp->warp_effect_pos' in 'warp_time'
		// and keeps going 
		if ( objp != Player_obj || Player_use_ai )	{
			vec3d vel;
			vel = objp->orient.vec.fvec;
			vm_vec_scale( &vel, warpout_speed );
			objp->phys_info.vel = vel;
			objp->phys_info.desired_vel = vel;
			objp->phys_info.prev_ramp_vel.xyz.x = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.y = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.z = warpout_speed;
			objp->phys_info.forward_thrust = 1.0f;		// How much the forward thruster is applied.  0-1.

			// special case for HUGE ships
			if (sip->flags & SIF_HUGE_SHIP) {
	//			objp->phys_info.flags |= PF_SPECIAL_WARP_OUT;
			}
		}
	}

	return 1;
}

int WE_Default::warpFrame(float frametime)
{
	if(direction == WD_WARP_IN)
	{
		if ((shipp->flags & SF_ARRIVING_STAGE_1) && timestamp_elapsed(stage_time_end))
		{
			// let physics know the ship is going to warp in.
			objp->phys_info.flags |= PF_WARP_IN;

			// done doing stage 1 of warp, so go on to stage 2
			shipp->flags &= (~SF_ARRIVING_STAGE_1);
			shipp->flags |= SF_ARRIVING_STAGE_2;

			float warp_time = shipfx_calculate_warp_time(objp, WD_WARP_IN);
			float speed = shipfx_calculate_warp_dist(objp) / warp_time;		// How long it takes to move through warp effect

			// Make ship move at velocity so that it moves two radii in warp_time seconds.
			vec3d vel;
			vel = objp->orient.vec.fvec;
			vm_vec_scale( &vel, speed );
			objp->phys_info.vel = vel;
			objp->phys_info.desired_vel = vel;
			objp->phys_info.prev_ramp_vel.xyz.x = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.y = 0.0f;
			objp->phys_info.prev_ramp_vel.xyz.z = speed;
			objp->phys_info.forward_thrust = 0.0f;		// How much the forward thruster is applied.  0-1.

			stage_time_end = timestamp(fl2i(warp_time*1000.0f));
		}
		else if ( (shipp->flags & SF_ARRIVING_STAGE_2) && timestamp_elapsed(stage_time_end) )
		{
			// done doing stage 2 of warp, so turn off arriving flag
			this->warpEnd();

			// notify physics to slow down
			if (sip->flags & SIF_SUPERCAP) {
				// let physics know this is a special warp in
				objp->phys_info.flags |= PF_SPECIAL_WARP_IN;
			}
		}
	}
	else if(direction == WD_WARP_OUT)
	{
		vec3d tempv;
		float warp_pos;	// position of warp effect in object's frame of reference

		vm_vec_sub( &tempv, &objp->pos, &pos );
		warp_pos = -vm_vec_dot( &tempv, &fvec );


		// Find the closest point on line from center of wormhole
		vec3d cpos;
		float dist;

		fvi_ray_plane(&cpos, &objp->pos, &fvec, &pos, &fvec, 0.0f );
		dist = vm_vec_dist( &cpos, &objp->pos );

	//	mprintf(( "Warp pos = %.1f, rad=%.1f, center dist = %.1f\n", warp_pos, objp->radius, dist ));

		if ( objp == Player_obj )	{
			// Code for player warpout frame

			if ( (Player->control_mode==PCM_WARPOUT_STAGE2) && (warp_pos > objp->radius) )	{
				gameseq_post_event( GS_EVENT_PLAYER_WARPOUT_DONE_STAGE2 );
			}

			if ( timestamp_elapsed(total_time_end) ) {

				// Something went wrong... oh well, warp him out anyway.
				if ( Player->control_mode != PCM_WARPOUT_STAGE3 )	{
					mprintf(( "Hmmm... player ship warpout time elapsed, but he wasn't in warp stage 3.\n" ));
				}

				this->warpEnd();
			}

		} else {
			// Code for all non-player ships warpout frame

			int timed_out = timestamp_elapsed(total_time_end);
			if ( timed_out )	{
	//			mprintf(("Frame %i: Ship %s missed departue cue.\n", Framecount, shipp->ship_name ));
				int	delta_ms = timestamp_until(total_time_end);
				if (delta_ms > 1000.0f * frametime ) {
					nprintf(("AI", "Frame %i: Ship %s missed departue cue by %7.3f seconds.\n", Framecount, shipp->ship_name, - (float) delta_ms/1000.0f));
				}
			}

			// MWA 10/21/97 -- added shipp->flags & SF_NO_DEPARTURE_WARP part of next if statement.  For ships
			// that don't get a wormhole effect, I wanted to drop into this code immediately.
			if ( (warp_pos > objp->radius)  || (shipp->flags & SF_NO_DEPARTURE_WARP) || timed_out )	{
				this->warpEnd();
			} 
		}
	}

	return 1;
}

int WE_Default::warpShipClip()
{
	if(!this->isValid())
		return 0;

	g3_start_user_clip_plane( &pos, &fvec );
	return 1;
}

int WE_Default::warpShipRender()
{
	return 1;
}

int WE_Default::getWarpPosition(vec3d *output)
{
	if(!this->isValid())
		return 0;

	*output = pos;
	return 1;
}

int WE_Default::getWarpOrientation(matrix* output)
{
    if (!this->isValid())
    {
        return 0;
    }

    vm_vector_2_matrix(output, &objp->orient.vec.fvec, NULL, NULL);
    return 1;
}

//********************-----CLASS: WE_BSG-----********************//
WE_BSG::WE_BSG(object *n_objp, int n_direction)
	:WarpEffect(n_objp, n_direction)
{
	//Zero animation and such
	anim = shockwave = -1;
	anim_fps = shockwave_fps = 0;
	anim_nframes = shockwave_nframes = 0;
	anim_total_time = shockwave_total_time = 0;

	//Setup anim name
	char tmp_name[MAX_FILENAME_LEN];
	memset(tmp_name, 0, MAX_FILENAME_LEN);
	if(direction == WD_WARP_IN)
		strcpy_s( tmp_name, sip->warpin_anim );
	else if(direction == WD_WARP_OUT)
		strcpy_s( tmp_name, sip->warpout_anim );
	strlwr(tmp_name);

	if(strlen(tmp_name))
	{
		//Load anim
		anim = bm_load_either(tmp_name, &anim_nframes, &anim_fps, NULL, 1);
		if(anim > -1)
		{
			anim_total_time = fl2i(((float)anim_nframes / (float)anim_fps) * 1000.0f);
		}

		//Setup shockwave name
		strncat(tmp_name, "-shockwave", MAX_FILENAME_LEN-1);

		//Load shockwave
		shockwave = bm_load_either(tmp_name, &shockwave_nframes, &shockwave_fps, NULL, 1);
		if(shockwave > -1)
		{
			shockwave_total_time = fl2i(((float)shockwave_nframes / (float)shockwave_fps) * 1000.0f);
		}
	}
	//Set radius
	tube_radius = 0.0f;
	if(direction == WD_WARP_IN)
		tube_radius = sip->warpin_radius;
	else
		tube_radius = sip->warpout_radius;

	polymodel *pm = model_get(sip->model_num);
	if(pm == NULL)
	{
		autocenter = vmd_zero_vector;
		z_offset_max = objp->radius;
		z_offset_min = -objp->radius;

		if(tube_radius <= 0.0f)
			tube_radius = objp->radius;
	}
	else
	{
		//Autogenerate everything from ship dimensions
		if(tube_radius <= 0.0f)
			tube_radius = MAX((pm->maxs.xyz.y - pm->mins.xyz.y), (pm->maxs.xyz.x - pm->mins.xyz.x))/2.0f;
		autocenter = pm->autocenter;
		z_offset_max = pm->maxs.xyz.z - pm->autocenter.xyz.z;
		z_offset_min = pm->mins.xyz.z - pm->autocenter.xyz.z;
	}

	//*****Timing
	stage = -1;
	if(direction == WD_WARP_IN)
	{
		stage_duration[0] = sip->warpin_time;
		stage_duration[1] = MAX(anim_total_time - sip->warpin_time, shockwave_total_time);
	}
	else
	{
		stage_duration[0] = sip->warpout_time;
		stage_duration[1] = MAX(anim_total_time - sip->warpout_time, shockwave_total_time);
	}
	stage_time_start = stage_time_end = total_time_start = total_time_end = timestamp();

	//*****Graphics
	batcher.allocate(1);

	//*****Sound
	snd_range_factor = 1.0f;
	snd_start = snd_end = -1;
	snd_start_gs = snd_end_gs = NULL;

	//*****Instance
	pos = vmd_zero_vector;
}

WE_BSG::~WE_BSG()
{
	if(anim > -1)
		bm_unload(anim);
	if(shockwave > -1)
		bm_unload(shockwave);
}

void WE_BSG::pageIn()
{
	if(anim > -1)
		bm_page_in_texture(anim);
	if(shockwave > -1)
		bm_page_in_texture(shockwave);
}

int WE_BSG::warpStart()
{
	if(!WarpEffect::warpStart())
		return 0;

	//WMC - bail
	if (anim < 0 && shockwave < 0)
	{
		this->warpEnd();
		return 0;
	}

	//WMC - If object is docked now, update data:
	if(object_is_docked(objp))
	{
		z_offset_max = dock_calc_max_semilatus_rectum_parallel_to_axis(objp, Z_AXIS);
		z_offset_min = -z_offset_max;
		if(tube_radius <= 0.0f)
		{
			float x_radius = dock_calc_max_semilatus_rectum_parallel_to_axis(objp, X_AXIS);
			float y_radius = dock_calc_max_semilatus_rectum_parallel_to_axis(objp, Y_AXIS);
			tube_radius = MAX(x_radius, y_radius);
		}

		vec3d dock_center;
		dock_calc_docked_center(&dock_center, objp);
		vm_vec_sub(&autocenter, &dock_center, &objp->pos);
	}

	if(direction == WD_WARP_IN)
		shipp->flags |= SF_ARRIVING_STAGE_1;
	else
		shipp->flags |= SF_DEPART_WARP;

	//*****Sound
	int gs_start_index = -1;
	int gs_end_index = -1;
	if(direction == WD_WARP_IN)
	{
		shipp->flags |= SF_ARRIVING_STAGE_1;
		gs_start_index = sip->warpin_snd_start;
		gs_end_index = sip->warpin_snd_end;
	}
	else if(direction == WD_WARP_OUT)
	{
		shipp->flags |= SF_DEPART_WARP;
		gs_start_index = sip->warpout_snd_start;
		gs_end_index = sip->warpout_snd_end;
	}
	else
	{
		this->warpEnd();
		return 0;
	}

	//Base is 100m long/diameter ship
	//WMC - Leave this as 1.0f
	//snd_range_factor = objp->radius / 50.0f;
	if(gs_start_index > -1)
	{
		snd_start_gs = &Snds[gs_start_index];
		snd_start = snd_play_3d(snd_start_gs, &objp->pos, &View_position, 0.0f, NULL, 0, 1, SND_PRIORITY_SINGLE_INSTANCE, NULL, snd_range_factor);
	}
	if(gs_end_index > -1)
	{
		snd_end_gs = &Snds[gs_end_index];
		snd_end = -1;
	}

	stage = 0;
	int total_duration = 0;
	for(int i = 0; i < WE_BSG_NUM_STAGES; i++)
		total_duration += stage_duration[i];

	total_time_start = timestamp();
	total_time_end = timestamp(total_duration);

	stage_time_start = total_time_start;
	stage_time_end = timestamp(stage_duration[stage]);

	return 1;
}

int WE_BSG::warpFrame(float frametime)
{
	if(!this->isValid())
		return 0;

	while( timestamp_elapsed(stage_time_end ))
	{
		stage++;
		if(stage < WE_BSG_NUM_STAGES)
		{
			stage_time_start = timestamp();
			stage_time_end = timestamp(stage_duration[stage]);
		}
		switch(stage)
		{
			case 1:
				if(direction == WD_WARP_IN)
				{
					//objp->phys_info.flags |= PF_WARP_IN;
					shipp->flags &= (~SF_ARRIVING_STAGE_1);
					shipp->flags |= SF_ARRIVING_STAGE_2;
				}
				break;
			default:
				this->warpEnd();
				return 0;
		}
	}

	switch(stage)
	{
		case 0:
		case 1:
			vm_vec_unrotate(&pos, &autocenter, &objp->orient);
			vm_vec_add2(&pos, &objp->pos);
			break;
		default:
			this->warpEnd();
			return 0;
	}

	if(snd_start > -1)
		snd_update_3d_pos(snd_start, snd_start_gs, &objp->pos, 0.0f, snd_range_factor);

	return 1;
}

int WE_BSG::warpShipClip()
{
	if(!this->isValid())
		return 0;

	if(direction == WD_WARP_OUT && stage > 0)
	{
		vec3d pos;
		vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, objp->radius);
		g3_start_user_clip_plane( &pos, &objp->orient.vec.fvec );
	}
	return 1;
}

int WE_BSG::warpShipRender()
{
	if(!this->isValid())
		return 0;

	if(anim < 0 && shockwave < 0)
		return 0;

	// turn off zbuffering	
	int saved_zbuffer_mode = gr_zbuffer_get();
	gr_zbuffer_set(GR_ZBUFF_NONE);

	if(anim > -1)
	{
		//Figure out which frame we're on
		//int anim_frame = fl2i( (((float)timestamp() - (float)total_time_start) / ((float) anim_total_time)) * (float)(anim_nframes-1) + 0.5f);
		int anim_frame = fl2i( ((float)(timestamp() - total_time_start)/1000.0f) * (float)anim_fps);

		if ( anim_frame < anim_nframes )
		{
			//Set the correct frame
			gr_set_bitmap(anim + anim_frame, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, 1.0f);		

			//Do warpout geometry
			vec3d start, end;
			vm_vec_scale_add(&start, &pos, &objp->orient.vec.fvec, z_offset_min);
			vm_vec_scale_add(&end, &pos, &objp->orient.vec.fvec, z_offset_max);

			//vm_vec_scale_add(&start, &objp->pos, &objp->orient.vec.fvec, z_offset_min);
			//vm_vec_scale_add(&end, &objp->pos, &objp->orient.vec.fvec, z_offset_max);

			batcher.draw_beam(&start, &end, tube_radius*2.0f, 1.0f);	

			//Render the warpout effect
			batcher.render(TMAP_FLAG_GOURAUD | TMAP_FLAG_RGB | TMAP_FLAG_TEXTURED | TMAP_FLAG_CORRECT | TMAP_HTL_3D_UNLIT);
		}
	}

	if(stage == 1 && shockwave > -1)
	{
		//int shockwave_frame = fl2i( (((float)timestamp() - (float)stage_time_start) / ((float) shockwave_total_time)) * (float)(shockwave_nframes-1) + 0.5f);
		int shockwave_frame = fl2i( ((float)(timestamp() - stage_time_start)/1000.0f) * (float)shockwave_fps);

		if(shockwave_frame < shockwave_nframes)
		{
			vertex p;
			//vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, (z_offset_max - z_offset_min)/2.0f);
			extern int Cmdline_nohtl;
			if(Cmdline_nohtl) {
				g3_rotate_vertex(&p, &pos );
			}else{
				g3_transfer_vertex(&p, &pos);
			}
			gr_set_bitmap(shockwave + shockwave_frame, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, 1.0f );
			g3_draw_bitmap(&p, 0, tube_radius, TMAP_FLAG_TEXTURED | TMAP_HTL_3D_UNLIT );
		}
	}

	// restore zbuffer mode
	gr_zbuffer_set(saved_zbuffer_mode);
	return 1;
}

int WE_BSG::warpEnd()
{
	if(snd_start > -1)
		snd_stop(snd_start);
	if(snd_end_gs != NULL)
		snd_end = snd_play_3d(snd_end_gs, &objp->pos, &View_position, 0.0f, NULL, 0, 1.0f, SND_PRIORITY_SINGLE_INSTANCE, NULL, snd_range_factor);

	return WarpEffect::warpEnd();
}

//WMC - These two functions are used to fool collision detection code
//And do player warpout
int WE_BSG::getWarpPosition(vec3d *output)
{
	if(!this->isValid())
		return 0;

	vec3d pos;
	if(direction == WD_WARP_OUT && stage > 0)
	{
		vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, objp->radius);
	}
	else
	{
		vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, objp->radius);
	}

	*output = pos;
	return 1;
}

int WE_BSG::getWarpOrientation(matrix* output)
{
    if (!this->isValid())
    {
        return 0;
    }

    vm_vector_2_matrix(output, &objp->orient.vec.fvec, NULL, NULL);
    return 1;
}

//********************-----CLASS: WE_Homeworld-----********************//
WE_Homeworld::WE_Homeworld(object *n_objp, int n_direction)
	:WarpEffect(n_objp, n_direction)
{
	if(!this->isValid())
		return;

	//Stage and time
	stage = 0;
	stage_time_start = stage_time_end = timestamp();

	//Stage duration presets
	stage_duration[0] = 0;
	stage_duration[1] = 1000;
	stage_duration[2] = 0;
	stage_duration[3] = -1;
	stage_duration[4] = 0;
	stage_duration[5] = 1000;

	//Configure stage duration 3
	if(direction == WD_WARP_IN)
		stage_duration[3] = sip->warpin_time - (stage_duration[1] + stage_duration[2] + stage_duration[4] + stage_duration[5]);
	else if(direction == WD_WARP_OUT)
		stage_duration[3] = sip->warpout_time - (stage_duration[1] + stage_duration[2] + stage_duration[4] + stage_duration[5]);
	if(stage_duration[3] <= 0)
		stage_duration[3] = 3000;

	//Anim
	if(direction == WD_WARP_IN)
		anim = bm_load_either(sip->warpin_anim, &anim_nframes, &anim_fps, NULL, 1);
	else if(direction == WD_WARP_OUT)
		anim = bm_load_either(sip->warpout_anim, &anim_nframes, &anim_fps, NULL, 1);
	else
		anim = -1;

	pos = vmd_zero_vector;
	fvec = vmd_zero_vector;

	polymodel *pm = model_get(sip->model_num);
	width_full = sip->warpin_radius;
	if(pm != NULL)
	{
		if(width_full <= 0.0f)
		{
			width_full = pm->maxs.xyz.x - pm->mins.xyz.x;
			height_full = pm->maxs.xyz.y - pm->mins.xyz.y;
			z_offset_max = pm->maxs.xyz.z;
			z_offset_min = pm->mins.xyz.z;
		}
	}
	else
	{
		if(width_full <= 0.0f)
		{
			width_full = 2.0f*objp->radius;
		}
		height_full = width_full;
		z_offset_max = objp->radius;
		z_offset_min = -objp->radius;
	}
	//WMC - This scales up or down the sound depending on ship size, with ~100m diameter ship as base
	//REMEMBER: Radius != diameter
	snd_range_factor = sqrt(width_full*width_full+height_full*height_full)/141.421356f;

	if(width_full <= 0.0f)
		width_full = 1.0f;
	if(height_full <= 0.0f)
		height_full = 1.0f;
	width = width_full;
	height = 0.0f;

	//Sound
	snd = -1;
	snd_gs = NULL;
}

WE_Homeworld::~WE_Homeworld()
{
	if(anim > -1)
		bm_unload(anim);
}

int WE_Homeworld::warpStart()
{
	if(!this->isValid())
		return 0;

	if(anim < 0)
	{
		this->warpEnd();
		return 0;
	}
	
	stage = 1;
	total_time_start = timestamp();
	total_time_end = 0;
	for(int i = 0; i < WE_HOMEWORLD_NUM_STAGES; i++)
	{
		total_time_end += stage_duration[i];
	}
	stage_time_start = total_time_start;
	stage_time_end = timestamp(stage_duration[stage]);

	//Position
	vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, z_offset_max);
	fvec = objp->orient.vec.fvec;
	if(direction == WD_WARP_OUT)
		vm_vec_negate(&fvec);

	width = width_full;
	height = 0.0f;

	int gs_index = -1;
	if(direction == WD_WARP_IN)
	{
		shipp->flags |= SF_ARRIVING_STAGE_1;
		gs_index = sip->warpin_snd_start;
	}
	else if(direction == WD_WARP_OUT)
	{
		shipp->flags |= SF_DEPART_WARP;
		gs_index = sip->warpout_snd_start;
	}
	else
	{
		this->warpEnd();
		return 0;
	}

	if(gs_index > -1)
	{
		snd_gs = &Snds[gs_index];
		snd = snd_play_3d(snd_gs, &pos, &View_position, 0.0f, NULL, 0, 1, SND_PRIORITY_SINGLE_INSTANCE, NULL, snd_range_factor);
	}

	return 1;
}

int WE_Homeworld::warpFrame(float frametime)
{
	if(!this->isValid())
		return 0;

	//Setup stage
	while( timestamp_elapsed(stage_time_end ))
	{
		stage++;
		if(stage < WE_HOMEWORLD_NUM_STAGES)
		{
			stage_time_start = timestamp();
			stage_time_end = timestamp(stage_duration[stage]);
		}
		switch(stage)
		{
			case 2:
				break;
			case 3:
				if(direction == WD_WARP_IN)
				{
					objp->phys_info.flags |= PF_WARP_IN;
					shipp->flags &= (~SF_ARRIVING_STAGE_1);
					shipp->flags |= SF_ARRIVING_STAGE_2;
				}

				//obj_snd_assign(OBJ_INDEX(objp), SND_BEAM_LOOP, &vmd_zero_vector, 1);
				break;
			case 4:
				break;
			case 5:
				//snd_play_3d( &Snds[SND_CAPITAL_WARP_IN], &objp->pos, &View_position, shipp->warp_radius);
				break;
			default:
				this->warpEnd();
				return 0;
		}
	}

	//Process stage
	float progress = ((float)timestamp() - (float)stage_time_start)/((float)stage_time_end - (float)stage_time_start);
	switch(stage)
	{
		case 1:
			height = height_full * progress;
			break;
		case 2:
			break;
		case 3:
			vm_vec_scale_add(&pos, &objp->pos, &objp->orient.vec.fvec, z_offset_max - progress*(z_offset_max-z_offset_min));
			break;
		case 4:
			break;
		case 5:
			height = height_full * (1.0f-progress);
			break;
		default:
			this->warpEnd();
			return 0;
	}

	//Update sound
	if(snd > -1)
		snd_update_3d_pos(snd, snd_gs, &pos, 0.0f, snd_range_factor);
		
	return 1;
}

int WE_Homeworld::warpShipClip()
{
	if(!this->isValid())
		return 0;

	g3_start_user_clip_plane( &pos, &fvec );
	return 1;
}

int WE_Homeworld::warpShipRender()
{
	if(!this->isValid())
		return 0;

	int frame = 0;
	if(anim_fps > 0)
		frame = fl2i( (int)(((float)(timestamp() - (float)total_time_start)/1000.0f) * (float)anim_fps) % anim_nframes);

	//Set the correct frame
	gr_set_bitmap(anim + frame, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, 1.0f);	
	g3_draw_polygon(&pos, &objp->orient, width, height, TMAP_FLAG_TEXTURED | TMAP_HTL_3D_UNLIT);
	return 1;
}

int WE_Homeworld::warpEnd()
{
	//obj_snd_delete(OBJ_INDEX(objp), obj_snd);
	if(snd > -1)
		snd_stop(snd);
	return WarpEffect::warpEnd();
}

int WE_Homeworld::getWarpPosition(vec3d *output)
{
	if(!this->isValid())
		return 0;

	*output = pos;
	return 1;
}

int WE_Homeworld::getWarpOrientation(matrix* output)
{
    if (!this->isValid())
    {
        return 0;
    }

    vm_vector_2_matrix(output, &fvec, NULL, NULL);
    return 1;
}

//********************-----CLASS: WE_Hyperspace----********************//
WE_Hyperspace::WE_Hyperspace(object *n_objp, int n_direction)
	:WarpEffect(n_objp, n_direction)
{
	total_duration = 0;
	if(direction == WD_WARP_IN)
		total_duration = sip->warpin_time;
	else if(direction == WD_WARP_OUT)
		total_duration = sip->warpout_time;
	if(total_duration <= 0)
		total_duration = 1000;
	
	total_time_start = total_time_end = timestamp();
	pos_final = vmd_zero_vector;
	scale_factor = 750.0f * objp->radius;
}

int WE_Hyperspace::warpStart()
{
	if(!this->isValid())
		return 0;

	total_time_start = timestamp();
	total_time_end = timestamp(total_duration);

	if(direction == WD_WARP_IN)
	{
		shipp->flags |= SF_ARRIVING_STAGE_2;
		objp->phys_info.flags |= PF_WARP_IN;
		objp->phys_info.vel.xyz.z = (scale_factor / sip->warpin_time)*1000.0f;
		objp->flags &= ~OF_PHYSICS;
	}
	else if(direction == WD_WARP_OUT)
	{
		shipp->flags |= SF_DEPART_WARP;
	}
	else
	{
		this->warpEnd();
	}

	pos_final = objp->pos;

	return 1;
}

int WE_Hyperspace::warpFrame(float frametime)
{
	if(!this->isValid())
		return 0;

	if(timestamp_elapsed(total_time_end))
	{
		objp->pos = pos_final;
		objp->phys_info.vel.xyz.z = 0.0f;
		objp->flags |= OF_PHYSICS;
		this->warpEnd();
	}
	else
	{
		float progress = ((float)timestamp() - (float)total_time_start)/(float)total_duration;
		float scale = 0.0f;
		if(direction == WD_WARP_IN)
			scale = -scale_factor*cos(progress*PI_2);
		else
			scale = scale_factor*sin(progress*PI_2);
		vm_vec_scale_add(&objp->pos, &pos_final, &objp->orient.vec.fvec, scale);
	}
	return 1;
}

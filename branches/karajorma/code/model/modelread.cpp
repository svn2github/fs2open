/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 



#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#endif

#define MODEL_LIB

#include "model/model.h"
#include "model/modelsinc.h"
#include "math/vecmat.h"
#include "object/object.h"
#include "bmpman/bmpman.h"
#include "io/key.h"
#include "render/3dinternal.h"
#include "globalincs/linklist.h"
#include "io/timer.h"
#include "freespace2/freespace.h"		// For flFrameTime
#include "math/fvi.h"
#include "ship/ship.h"
#include "cfile/cfile.h"
#include "parse/parselo.h"
#include "cmdline/cmdline.h"


#include "gamesnd/gamesnd.h"

flag_def_list model_render_flags[] =
{
	{"no lighting",		MR_NO_LIGHTING},
	{"transparent",		MR_ALL_XPARENT},
	{"no Zbuffer",		MR_NO_ZBUFFER},
	{"no cull",			MR_NO_CULL},
	{"no glowmaps",		MR_NO_GLOWMAPS},
	{"force clamp",		MR_FORCE_CLAMP},
};
  	 
int model_render_flags_size = sizeof(model_render_flags)/sizeof(flag_def_list);

#define MAX_SUBMODEL_COLLISION_ROT_ANGLE (PI / 6.0f)	// max 30 degrees per frame

// info for special polygon lists

polymodel *Polygon_models[MAX_POLYGON_MODELS];

static int model_initted = 0;
extern int Cmdline_nohtl;

#ifndef NDEBUG
CFILE *ss_fp = NULL;			// file pointer used to dump subsystem information
char  model_filename[_MAX_PATH];		// temp used to store filename
char	debug_name[_MAX_PATH];
int ss_warning_shown = 0;		// have we shown the warning dialog concerning the subsystems?
char	Global_filename[256];
int Model_ram = 0;			// How much RAM the models use total
#endif

// Anything less than this is considered incompatible.
#define PM_COMPATIBLE_VERSION 1900

// Anything greater than or equal to PM_COMPATIBLE_VERSION and 
// whose major version is less than or equal to this is considered
// compatible.  
#define PM_OBJFILE_MAJOR_VERSION 30

static int Model_signature = 0;

void generate_vertex_buffers(bsp_info*, polymodel*);
void model_set_subsys_path_nums(polymodel *pm, int n_subsystems, model_subsystem *subsystems);
void model_set_bay_path_nums(polymodel *pm);


// Goober5000 - see SUBSYSTEM_X in model.h
// NOTE: Each subsystem must match up with its #define, or there will be problems
char *Subsystem_types[SUBSYSTEM_MAX] =
{
	"None",
	"Engines",
	"Turrets",
	"Radar",
	"Navigation",
	"Communications",
	"Weapons",
	"Sensors",
	"Solar panels",
	"Gas collection",
	"Activation",
	"Unknown"
};


//WMC - For general compatibility stuff.
//Note that the order of the items in this list
//determine the order that they are tried in ai_goal_fixup_dockpoints
flag_def_list Dock_type_names[] =
{
	{ "cargo",		DOCK_TYPE_CARGO,	0 },
	{ "rearm",		DOCK_TYPE_REARM,	0 },
	{ "generic",	DOCK_TYPE_GENERIC,	0 }
};

int Num_dock_type_names = sizeof(Dock_type_names) / sizeof(flag_def_list);

// Free up a model, getting rid of all its memory
// With the basic page in system this can be called from outside of modelread.cpp
void model_unload(int modelnum, int force)
{
	int i, j, num;

	if ( modelnum >= MAX_POLYGON_MODELS ) {
		num = modelnum % MAX_POLYGON_MODELS;
	} else {
		num = modelnum;
	}

	if ( (num < 0) || (num >= MAX_POLYGON_MODELS))	{
		return;
	}

	polymodel *pm = Polygon_models[num];

	if ( !pm )	{
		return;
	}

	Assert( pm->used_this_mission >= 0 );

	if (!force && (--pm->used_this_mission > 0))
		return;


	// so that the textures can be released
	pm->used_this_mission = 0;

	// we want to call bm_release() from here rather than just bm_unload() in order
	// to get the slots back so we set "release" to true.
	model_page_out_textures(pm->id, true);

#ifndef NDEBUG
	Model_ram -= pm->ram_used;
#endif

	safe_kill(pm->ship_bay);
	
	if (pm->paths)	{
		for (i=0; i<pm->n_paths; i++ )	{
			for (j=0; j<pm->paths[i].nverts; j++ )	{
				if ( pm->paths[i].verts[j].turret_ids )	{
					vm_free(pm->paths[i].verts[j].turret_ids);
				}
			}
			if (pm->paths[i].verts)	{
				vm_free(pm->paths[i].verts);
			}
		}
		vm_free(pm->paths);
	}

	if ( pm->shield.verts )	{
		vm_free( pm->shield.verts );
	}

	if ( pm->shield.tris )	{
		vm_free(pm->shield.tris);
	}

	if ( pm->missile_banks )	{
		vm_free(pm->missile_banks);
	}

	if ( pm->docking_bays )	{
		for (i=0; i<pm->n_docks; i++ )	{
			if ( pm->docking_bays[i].splines )	{
				vm_free( pm->docking_bays[i].splines );
			}
		}
		vm_free(pm->docking_bays);
	}


	if ( pm->thrusters ) {
		for (i = 0; i < pm->n_thrusters; i++) {
			if (pm->thrusters[i].points)
				vm_free(pm->thrusters[i].points);
		}

		vm_free(pm->thrusters);
	}

	if ( pm->glow_point_banks )	{ // free the glows!!! -Bobboau
		for (i = 0; i < pm->n_glow_point_banks; i++) {
			if (pm->glow_point_banks[i].points)
				vm_free(pm->glow_point_banks[i].points);
		}

		vm_free(pm->glow_point_banks);
	}

#ifndef NDEBUG
	if ( pm->debug_info )	{
		vm_free(pm->debug_info);
	}
#endif

	model_octant_free( pm );

	if (pm->submodel) {
		for (i = 0; i < pm->n_models; i++) {
			if ( !Cmdline_nohtl ) {
				for (j = 0; j < (int)pm->submodel[i].buffer.size(); j++) {
					pm->submodel[i].buffer[j].index_buffer.release();
				}

				pm->submodel[i].buffer.clear();
				gr_destroy_buffer(pm->submodel[i].indexed_vertex_buffer);
			}
			
			if ( pm->submodel[i].bsp_data )	{
				vm_free(pm->submodel[i].bsp_data);
			}
		}

		vm_free(pm->submodel);
	}

	if ( pm->xc ) {
		vm_free(pm->xc);
	}

	if ( pm->lights )	{
		vm_free(pm->lights);
	}

	if ( pm->gun_banks )	{
		vm_free(pm->gun_banks);
	}

	if ( pm->shield_collision_tree ) {
		vm_free(pm->shield_collision_tree);
	}

	// run through Ship_info[] and if the model has been loaded we'll need to reset the modelnum to -1.
	for (i = 0; i < Num_ship_classes; i++) {
		if ( pm->id == Ship_info[i].model_num ) {
			Ship_info[i].model_num = -1;
		}

		if ( pm->id == Ship_info[i].model_num_hud ) {
			Ship_info[i].model_num_hud = -1;
		}
	}

	pm->id = 0;
	memset( pm, 0, sizeof(polymodel));
	vm_free( pm );

	Polygon_models[num] = NULL;	
}

void model_free_all()
{
	int i;

	if ( !model_initted)	{
		model_init();
		return;
	}

	mprintf(( "Freeing all existing models...\n" ));

	for (i=0;i<MAX_POLYGON_MODELS;i++) {
		// forcefully unload all loaded models (be careful with this)
		model_unload(i, 1);		
	}

}

void model_page_in_start()
{
	int i;

	if ( !model_initted ) {
		model_init();
		return;
	}

	mprintf(( "Starting model page in...\n" ));

	for (i=0; i<MAX_POLYGON_MODELS; i++) {
		if (Polygon_models[i] != NULL)
			Polygon_models[i]->used_this_mission = 0;
	}
}

void model_page_in_stop()
{
	int i;

	Assert( model_initted );

	mprintf(( "Stopping model page in...\n" ));

	for (i=0; i<MAX_POLYGON_MODELS; i++) {
		if (Polygon_models[i] == NULL)
			continue;

		if (Polygon_models[i]->used_this_mission)
			continue;
	
		model_unload(i);
	}
}

void model_init()
{
	int i;

	if ( model_initted )		{
		Int3();		// Model_init shouldn't be called twice!
		return;
	}

#ifndef NDEBUG
	Model_ram = 0;
#endif

	for (i=0;i<MAX_POLYGON_MODELS;i++) {
		Polygon_models[i] = NULL;
	}

	// Init the model caching system
//	model_cache_init();

	atexit( model_free_all );
	model_initted = 1;
}

// routine to parse out values from a user property field of an object
void get_user_prop_value(char *buf, char *value)
{
	char *p, *p1, c;

	p = buf;
	while ( isspace(*p) || (*p == '=') )		// skip white space and equal sign
		p++;
	p1 = p;
	while ( !iscntrl(*p1) )
		p1++;
	c = *p1;
	*p1 = '\0';
	strcpy(value, p);
	*p1 = c;
}

// funciton to copy model data from one subsystem set to another subsystem set.  This function
// is called when two ships use the same model data, but since the model only gets read in one time,
// the subsystem data is only present in one location.  The ship code will call this routine to fix
// this situation by copying stuff from the source subsystem set to the dest subsystem set.
void model_copy_subsystems( int n_subsystems, model_subsystem *d_sp, model_subsystem *s_sp )
{
	int i, j;
	model_subsystem *source, *dest;

	for (i = 0; i < n_subsystems; i++ ) {
		source = &s_sp[i];
		for ( j = 0; j < n_subsystems; j++ ) {
			dest = &d_sp[j];
			if ( !subsystem_stricmp( source->subobj_name, dest->subobj_name) ) {
				dest->flags = source->flags;
				dest->subobj_num = source->subobj_num;
				dest->model_num = source->model_num;
				dest->pnt = source->pnt;
				dest->radius = source->radius;
				dest->type = source->type;
				dest->turn_rate = source->turn_rate;
				dest->turret_gun_sobj = source->turret_gun_sobj;

				strcpy_s( dest->name, source->name );

				if ( dest->type == SUBSYSTEM_TURRET ) {
					int nfp;

					dest->turret_fov = source->turret_fov;
					dest->turret_num_firing_points = source->turret_num_firing_points;
					dest->turret_norm = source->turret_norm;
					dest->turret_matrix = source->turret_matrix;

					for (nfp = 0; nfp < dest->turret_num_firing_points; nfp++ )
						dest->turret_firing_point[nfp] = source->turret_firing_point[nfp];

					if ( dest->flags & MSS_FLAG_CREWPOINT )
						strcpy_s(dest->crewspot, source->crewspot);
				}
				break;
			}
		}
		if ( j == n_subsystems )
			Int3();							// get allender -- something is amiss with models

	}
}

// routine to get/set subsystem information
static void set_subsystem_info( model_subsystem *subsystemp, char *props, char *dname )
{
	char *p;
	char buf[64];
	char	lcdname[256];

	if ( (p = strstr(props, "$name")) != NULL)
		get_user_prop_value(p+5, subsystemp->name);
	else
		strcpy_s( subsystemp->name, dname );

	strcpy_s(lcdname, dname);
	strlwr(lcdname);

	// check the name for its specific type
	if ( strstr(lcdname, "engine") ) {
		subsystemp->type = SUBSYSTEM_ENGINE;
	} else if ( strstr(lcdname, "radar") ) {
		subsystemp->type = SUBSYSTEM_RADAR;
	} else if ( strstr(lcdname, "turret") ) {
		float angle;

		subsystemp->type = SUBSYSTEM_TURRET;
		if ( (p = strstr(props, "$fov")) != NULL )
			get_user_prop_value(p+4, buf);			// get the value of the fov
		else
			strcpy_s(buf,"180");
		angle = ANG_TO_RAD(atoi(buf))/2.0f;
		subsystemp->turret_fov = (float)cos(angle);
		subsystemp->turret_num_firing_points = 0;

		if ( (p = strstr(props, "$crewspot")) != NULL) {
			subsystemp->flags |= MSS_FLAG_CREWPOINT;
			get_user_prop_value(p+9, subsystemp->crewspot);
		}

	} else if ( strstr(lcdname, "navigation") ) {
		subsystemp->type = SUBSYSTEM_NAVIGATION;
	} else if ( strstr(lcdname, "communication") )  {
		subsystemp->type = SUBSYSTEM_COMMUNICATION;
	} else if ( strstr(lcdname, "weapon") ) {
		subsystemp->type = SUBSYSTEM_WEAPONS;
	} else if ( strstr(lcdname, "sensor") ) {
		subsystemp->type = SUBSYSTEM_SENSORS;
	} else if ( strstr(lcdname, "solar") ) {
		subsystemp->type = SUBSYSTEM_SOLAR;
	} else if ( strstr(lcdname, "gas") ) {
		subsystemp->type = SUBSYSTEM_GAS_COLLECT;
	} else if ( strstr(lcdname, "activator") ) {
		subsystemp->type = SUBSYSTEM_ACTIVATION;
	}  else { // If unrecognized type, set to unknown so artist can continue working...
		subsystemp->type = SUBSYSTEM_UNKNOWN;
		mprintf(("Potential problem found: Unrecognized subsystem type '%s', believed to be in ship %s\n", dname, Global_filename));
	}

	if ( (p = strstr(props, "$triggered:")) != NULL ) {
		subsystemp->flags |= MSS_FLAG_ROTATES;
		subsystemp->flags |= MSS_FLAG_TRIGGERED;
	}

	// Rotating subsystem
	if ( (p = strstr(props, "$rotate")) != NULL)	{
		subsystemp->flags |= MSS_FLAG_ROTATES;

		// get time for (a) complete rotation (b) step (c) activation
		float turn_time;
		get_user_prop_value(p+7, buf);
		turn_time = (float)atof(buf);

		// CASE OF WEAPON ROTATION (primary only)
		if ( (p = strstr(props, "$pbank")) != NULL)	{
			subsystemp->flags |= MSS_FLAG_ARTILLERY;

			// get which pbank should trigger rotation
			get_user_prop_value(p+6, buf);
			subsystemp->weapon_rotation_pbank = (int)atoi(buf);
		} // end of weapon rotation stuff

		
		// *** determine how the subsys rotates ***

		// CASE OF STEPPED ROTATION
		if ( (p = strstr(props, "$stepped")) != NULL) {

			subsystemp->stepped_rotation = new(stepped_rotation);
			subsystemp->flags |= MSS_FLAG_STEPPED_ROTATE;

			// get number of steps
			if ( (p = strstr(props, "$steps")) != NULL) {
				get_user_prop_value(p+6, buf);
			   subsystemp->stepped_rotation->num_steps = atoi(buf);
			 } else {
			    subsystemp->stepped_rotation->num_steps = 8;
			 }

			// get pause time
			if ( (p = strstr(props, "$t_paused")) != NULL) {
				get_user_prop_value(p+9, buf);
			   subsystemp->stepped_rotation->t_pause = (float)atof(buf);
			 } else {
			    subsystemp->stepped_rotation->t_pause = 2.0f;
			 }

			// get transition time - time to go between steps
			if ( (p = strstr(props, "$t_transit")) != NULL) {
				get_user_prop_value(p+10, buf);
			    subsystemp->stepped_rotation->t_transit = (float)atof(buf);
			} else {
			    subsystemp->stepped_rotation->t_transit = 2.0f;
			}

			// get fraction of time spent in accel
			if ( (p = strstr(props, "$fraction_accel")) != NULL) {
				get_user_prop_value(p+15, buf);
			    subsystemp->stepped_rotation->fraction = (float)atof(buf);
			   Assert(subsystemp->stepped_rotation->fraction > 0 && subsystemp->stepped_rotation->fraction < 0.5);
			} else {
			    subsystemp->stepped_rotation->fraction = 0.3f;
			}

			int num_steps = subsystemp->stepped_rotation->num_steps;
			float t_trans = subsystemp->stepped_rotation->t_transit;
			float fraction = subsystemp->stepped_rotation->fraction;

			subsystemp->stepped_rotation->max_turn_accel = PI2 / (fraction*(1.0f - fraction) * num_steps * t_trans*t_trans);
			subsystemp->stepped_rotation->max_turn_rate =  PI2 / ((1.0f - fraction) * num_steps *t_trans);
		}

		// CASE OF NORMAL CONTINUOUS ROTATION
		else {
			// commented by Goober5000 to allow faster than 1sec rotation
/*			if ( fabs(turn_time) < 1 )
			{
				// Warning(LOCATION, "%s has subsystem %s with rotation time less than 1 sec", dname, Global_filename );
				subsystemp->flags &= ~MSS_FLAG_ROTATES;
				subsystemp->turn_rate = 0.0f;
			}
			else */
			{
				subsystemp->turn_rate = PI2 / turn_time;
			}
		}
	}
}

// used in collision code to check if submode rotates too far
float get_submodel_delta_angle(submodel_instance_info *sii)
{
	vec3d diff;
	vm_vec_sub(&diff, (vec3d*)&sii->angs, (vec3d*)&sii->prev_angs);

	// find the angle
	float delta_angle = vm_vec_mag(&diff);

	// make sure we get the short way around
	if (delta_angle > PI) {
		delta_angle = (PI2 - delta_angle);
	}

	return delta_angle;
}

void do_new_subsystem( int n_subsystems, model_subsystem *slist, int subobj_num, float rad, vec3d *pnt, char *props, char *subobj_name, int model_num )
{
	int i;
	model_subsystem *subsystemp;

	if ( slist==NULL ) {
#ifndef NDEBUG
		if (!ss_warning_shown) {
			mprintf(("No subsystems found for model \"%s\".\n", model_get(model_num)->filename));
			ss_warning_shown = 1;
		}
#endif
		return;			// For TestCode, POFView, etc don't bother
	}
	
	// try to find the name of the subsystem passed here on the list of subsystems currently on the
	// ship.  Assign the values only when the right subsystem is found

	for (i = 0; i < n_subsystems; i++ ) {
		subsystemp = &slist[i];

#ifndef NDEBUG
		// Goober5000 - notify if there's a mismatch
		if ( stricmp(subobj_name, subsystemp->subobj_name) && !subsystem_stricmp(subobj_name, subsystemp->subobj_name) )
		{
			Warning(LOCATION, "Subsystem \"%s\" in model \"%s\" is represented as \"%s\" in ships.tbl.  "
				"Although FS2_OPEN 3.6 and later will catch and correct this error, earlier "
				"versions (as well as retail FS2) will not.  You are advised to fix this if "
				"you plan to support earlier versions of FreeSpace.\n", subobj_name, model_get(model_num)->filename, subsystemp->subobj_name);

		}
#endif

		if (!subsystem_stricmp(subobj_name, subsystemp->subobj_name))
		{
			//commented by Goober5000 because this is also set when the table is parsed
			//subsystemp->flags = 0;

			subsystemp->subobj_num = subobj_num;
			subsystemp->turret_gun_sobj = -1;
			subsystemp->model_num = model_num;
			subsystemp->pnt = *pnt;				// use the offset to get the center point of the subsystem
			subsystemp->radius = rad;
			set_subsystem_info( subsystemp, props, subobj_name);
			strcpy_s(subsystemp->subobj_name, subobj_name);						// copy the object name
			return;
		}
	}
#ifndef NDEBUG
	char bname[_MAX_FNAME];

	if ( !ss_warning_shown) {
		_splitpath(model_filename, NULL, NULL, bname, NULL);
		// Lets still give a comment about it and not just erase it
		Warning(LOCATION,"Not all subsystems in model \"%s\" have a record in ships.tbl.\nThis can cause game to crash.\n\nList of subsystems not found from table is in log file.\n", model_get(model_num)->filename );
		mprintf(("Subsystem %s in model was not found in ships.tbl!\n", subobj_name));
//		Warning(LOCATION, "A subsystem was found in model %s that does not have a record in ships.tbl.\nA list of subsystems for this ship will be dumped to:\n\ndata%stables%s%s.subsystems for inclusion\ninto ships.tbl.", model_filename, DIR_SEPARATOR_STR, DIR_SEPARATOR_STR, bname);
		ss_warning_shown = 1;
	} else
#endif
		mprintf(("Subsystem %s in model was not found in ships.tbl!\n", subobj_name));

#ifndef NDEBUG
	if ( ss_fp )	{
		_splitpath(model_filename, NULL, NULL, bname, NULL);
		mprintf(("A subsystem was found in model %s that does not have a record in ships.tbl.\nA list of subsystems for this ship will be dumped to:\n\ndata%stables%s%s.subsystems for inclusion\ninto ships.tbl.\n", model_filename, DIR_SEPARATOR_STR, DIR_SEPARATOR_STR, bname));
		char tmp_buffer[128];
		sprintf(tmp_buffer, "$Subsystem:\t\t\t%s,1,0.0\n", subobj_name);
		cfputs(tmp_buffer, ss_fp);
	}
#endif

}

void print_family_tree( polymodel *obj, int modelnum, char * ident, int islast )	
{
	char temp[50];

	if ( modelnum < 0 ) return;
	if (obj==NULL) return;

	if (strlen(ident)==0 )	{
		mprintf(( " %s", obj->submodel[modelnum].name ));
		sprintf( temp, " " );
	} else if ( islast ) 	{
		mprintf(( "%s��%s", ident, obj->submodel[modelnum].name ));
		sprintf( temp, "%s  ", ident );
	} else {
		mprintf(( "%s��%s", ident, obj->submodel[modelnum].name ));
		sprintf( temp, "%s� ", ident );
	}

	mprintf(( "\n" ));

	int child = obj->submodel[modelnum].first_child;
	while( child > -1 )	{
		if ( obj->submodel[child].next_sibling < 0 )
			print_family_tree( obj, child, temp,1 );
		else
			print_family_tree( obj, child, temp,0 );
		child = obj->submodel[child].next_sibling;
	}
}

void dump_object_tree(polymodel *obj)
{
	print_family_tree( obj, 0, "", 0 );
	key_getch();
}

void create_family_tree(polymodel *obj)
{
	int i;
	for (i=0; i<obj->n_models; i++ )	{
		obj->submodel[i].num_children = 0;
		obj->submodel[i].first_child = -1;
		obj->submodel[i].next_sibling = -1;
	}

	for (i=0; i<obj->n_models; i++ )	{
		int pn;
		pn = obj->submodel[i].parent;
		if ( pn > -1 )	{
			obj->submodel[pn].num_children++;
			int tmp = obj->submodel[pn].first_child;
			obj->submodel[pn].first_child = i;
			obj->submodel[i].next_sibling = tmp;
		}
	}
}

// Goober5000
bool maybe_swap_mins_maxs(vec3d *mins, vec3d *maxs)
{
	float temp;
	bool swap_was_necessary = false;

	if (mins->xyz.x > maxs->xyz.x)
	{
		temp = mins->xyz.x;
		mins->xyz.x = maxs->xyz.x;
		maxs->xyz.x = temp;
		swap_was_necessary = true;
	}
	if (mins->xyz.y > maxs->xyz.y)
	{
		temp = mins->xyz.y;
		mins->xyz.y = maxs->xyz.y;
		maxs->xyz.y = temp;
		swap_was_necessary = true;
	}
	if (mins->xyz.z > maxs->xyz.z)
	{
		temp = mins->xyz.z;
		mins->xyz.z = maxs->xyz.z;
		maxs->xyz.z = temp;
		swap_was_necessary = true;
	}

// This is a mini utility that prints out the proper hex string for the
// mins and maxs so that the POF file can be modified in a hex editor.
// Currently none of the major POF editors allow editing of bounding boxes.
#if 0
	if (swap_was_necessary)
	{
		// use C hackery to convert float values to raw bytes
		const int NUM_BYTES = 24;
		typedef struct converter
		{
			union
			{
				struct
				{
					float min_x, min_y, min_z, max_x, max_y, max_z;
				} _float;
				ubyte _byte[NUM_BYTES];
			};
		} converter;

		// fill in the values
		converter z;
		z._float.min_x = mins->xyz.x;
		z._float.min_y = mins->xyz.y;
		z._float.min_z = mins->xyz.z;
		z._float.max_x = maxs->xyz.x;
		z._float.max_y = maxs->xyz.y;
		z._float.max_z = maxs->xyz.z;

		// prep string
		char hex_str[5];
		char text[100 + (5 * NUM_BYTES)];
		strcpy_s(text, "The following is the correct hex string for the minima and maxima:\n");

		// append hex values to the string
		for (int i = 0; i < NUM_BYTES; i++)
		{
			sprintf(hex_str, "%02X ", z._byte[i]);
			strcat_s(text, hex_str);
		}

		// notify the user
		Warning(LOCATION, text);
	}
#endif

	return swap_was_necessary;
}

void model_calc_bound_box( vec3d *box, vec3d *big_mn, vec3d *big_mx)
{
	box[0].xyz.x = big_mn->xyz.x; box[0].xyz.y = big_mn->xyz.y; box[0].xyz.z = big_mn->xyz.z;
	box[1].xyz.x = big_mx->xyz.x; box[1].xyz.y = big_mn->xyz.y; box[1].xyz.z = big_mn->xyz.z;
	box[2].xyz.x = big_mx->xyz.x; box[2].xyz.y = big_mx->xyz.y; box[2].xyz.z = big_mn->xyz.z;
	box[3].xyz.x = big_mn->xyz.x; box[3].xyz.y = big_mx->xyz.y; box[3].xyz.z = big_mn->xyz.z;


	box[4].xyz.x = big_mn->xyz.x; box[4].xyz.y = big_mn->xyz.y; box[4].xyz.z = big_mx->xyz.z;
	box[5].xyz.x = big_mx->xyz.x; box[5].xyz.y = big_mn->xyz.y; box[5].xyz.z = big_mx->xyz.z;
	box[6].xyz.x = big_mx->xyz.x; box[6].xyz.y = big_mx->xyz.y; box[6].xyz.z = big_mx->xyz.z;
	box[7].xyz.x = big_mn->xyz.x; box[7].xyz.y = big_mx->xyz.y; box[7].xyz.z = big_mx->xyz.z;
}


//	Debug thing so we don't repeatedly show warning messages.
#ifndef NDEBUG
int Bogus_warning_flag_1903 = 0;
#endif
void parse_triggers(int &n_trig, queued_animation **triggers, char *props);


IBX ibuffer_info;

//reads a binary file containing a 3d model
int read_model_file(polymodel * pm, char *filename, int n_subsystems, model_subsystem *subsystems, int ferror)
{
	CFILE *fp;
	int version;
	int id, len, next_chunk;
	int i,j;
	vec3d temp_vec;

#ifndef NDEBUG
	strcpy_s(Global_filename, filename);
#endif

	// little test code i used in fred2
	//char pwd[128];
	//getcwd(pwd, 128);

	fp = cfopen(filename,"rb");

	if (!fp) {
		if (ferror == 1) {
			Error( LOCATION, "Can't open model file <%s>", filename );
		} else {
			Warning( LOCATION, "Can't open model file <%s>", filename );
		}

		return -1;
	}		

	// Begin IBX code - taylor
	if ( !Cmdline_nohtl && !Cmdline_noibx ) {
		// generate checksum for the POF
		uint pof_checksum = 0;
		cfseek(fp, 0, SEEK_SET);	
		cf_chksum_long(fp, &pof_checksum);
		cfseek(fp, 0, SEEK_SET);

		// clear struct and prepare for IBX usage
		memset( &ibuffer_info, 0, sizeof(IBX) );

		// get name for tangent space file
		strcpy_s( ibuffer_info.tsb_name, filename );
		char *pb = strchr( ibuffer_info.tsb_name, '.' );
		if ( pb ) *pb = 0;
		strcat_s( ibuffer_info.tsb_name, NOX(".tsb") );

		// use the same filename as the POF but with an .ibx extension
		strcpy_s( ibuffer_info.name, filename );
		pb = strchr( ibuffer_info.name, '.' );
		if ( pb ) *pb = 0;
		strcat_s( ibuffer_info.name, NOX(".ibx") );

		ibuffer_info.read = cfopen( ibuffer_info.name, "rb", CFILE_NORMAL, CF_TYPE_CACHE );

		// check if it's a zero size file and if so bail out to create a new one
		if ( (ibuffer_info.read != NULL) && !cfilelength(ibuffer_info.read) ) {
			cfclose( ibuffer_info.read );
			ibuffer_info.read = NULL;
		}

		if ( Cmdline_normal && (ibuffer_info.read != NULL) ) {
			ibuffer_info.tsb_read = cfopen( ibuffer_info.tsb_name, "rb", CFILE_NORMAL, CF_TYPE_CACHE );

			if ( (ibuffer_info.tsb_read == NULL) || !cfilelength(ibuffer_info.tsb_read) ) {
				if (ibuffer_info.tsb_read != NULL)
					cfclose( ibuffer_info.tsb_read);

				ibuffer_info.tsb_read = NULL;

				cfclose( ibuffer_info.read );
				ibuffer_info.read = NULL;
			}
		}

		if (ibuffer_info.read != NULL) {
			bool ibx_valid = false;
			bool tsb_valid = true;	// the TSB is assumed valid by default

			// grab a checksum of the IBX, for debugging purposes
			uint ibx_checksum = 0;
			cfseek(ibuffer_info.read, 0, SEEK_SET);
			cf_chksum_long(ibuffer_info.read, &ibx_checksum);
			cfseek(ibuffer_info.read, 0, SEEK_SET);

			// get the file size that we use to safety check with.
			// be sure to subtract from this when we read something out
			ibuffer_info.size = cfilelength( ibuffer_info.read );

			// file id
			int ibx = cfread_int( ibuffer_info.read );
			ibuffer_info.size -= sizeof(int); // subtract

			// make sure the file is valid
			switch (ibx)
			{
				// "XBI " - (" IBX" in file)
				case 0x58424920:
				// "XBI2" - ("2IBX" in file)
				case 0x58424932:
					ibx_valid = true;
					break;
			}

			if (ibx_valid) {
				// file is valid so grab the checksum out of the .ibx and verify it matches the POF
				uint ibx_sum = cfread_uint( ibuffer_info.read );
				ibuffer_info.size -= sizeof(uint); // subtract

				if (ibx_sum != pof_checksum) {
					// bah, it's invalid for this POF
					ibx_valid = false;

					mprintf(("IBX:  Warning!  Found invalid IBX file: '%s'\n", ibuffer_info.name));
				}

				if (ibuffer_info.tsb_read != NULL) {
					// get the file size that we use to safety check with.
					// be sure to subtract from this when we read something out
					ibuffer_info.tsb_size = cfilelength( ibuffer_info.tsb_read );

					// check file id
					int tsb = cfread_int( ibuffer_info.tsb_read );
					ibuffer_info.tsb_size -= sizeof(int);

					// "BST2" - ("2TSB" in file)
					if (tsb != 0x42535432)
						tsb_valid = false;

					// if it's valid then check for the correct IBX checksum
					if (tsb_valid) {
						uint tsb_sum = cfread_uint( ibuffer_info.tsb_read );
						ibuffer_info.tsb_size -= sizeof(uint);

						if (tsb_sum != ibx_checksum)
							tsb_valid = false;
					}

					if ( !tsb_valid )
						mprintf(("IBX:  Warning!  Found invalid TSB file: '%s'\n", ibuffer_info.tsb_name));
				}
			}


			if ( !ibx_valid || !tsb_valid ) {
				cfclose( ibuffer_info.read );
				ibuffer_info.read = NULL;
				ibuffer_info.size = 0;

				if (ibuffer_info.tsb_read != NULL) {
					cfclose( ibuffer_info.tsb_read);
					ibuffer_info.tsb_read = NULL;
					ibuffer_info.tsb_size = 0;
				}
			} else {
				mprintf(("IBX: Found a good %s to read for '%s'.\n", (ibuffer_info.tsb_read != NULL) ? "IBX/TSB" : "IBX", filename));
				mprintf(("IBX-DEBUG => POF checksum: 0x%08x, IBX checksum: 0x%08x -- \"%s\"\n", pof_checksum, ibx_checksum, filename));
			}
		}

		// if the read file is absent or invalid then write out the new info
		if (ibuffer_info.read == NULL) {
			ibuffer_info.write = cfopen( ibuffer_info.name, "wb", CFILE_NORMAL, CF_TYPE_CACHE );

			if (ibuffer_info.write != NULL) {
				mprintf(("IBX: Starting a new IBX for '%s'.\n", filename));

				// file id, default to version 1
				cfwrite_int( 0x58424920, ibuffer_info.write ); // "XBI " - (" IBX" in file)

				// POF checksum
				cfwrite_uint( pof_checksum, ibuffer_info.write );


				// tangent space data
				if (Cmdline_normal) {
					ibuffer_info.tsb_write = cfopen( ibuffer_info.tsb_name, "wb", CFILE_NORMAL, CF_TYPE_CACHE );

					if (ibuffer_info.tsb_write != NULL) {
						mprintf(("IBX: Starting a new TSB for '%s'.\n", filename));

						// file id
						cfwrite_int( 0x42535432, ibuffer_info.tsb_write );	// "BST2" - ("2TSB" in file)

						// POF checksum (NOTE: This gets replaced by the IBX checksum after it's been created!)
						cfwrite_uint( pof_checksum, ibuffer_info.tsb_write );
					}
				}
			}
		}
	} // End IBX code

	// code to get a filename to write out subsystem information for each model that
	// is read.  This info is essentially debug stuff that is used to help get models
	// into the game quicker
#if 0
	{
		char bname[_MAX_FNAME];

		_splitpath(filename, NULL, NULL, bname, NULL);
		sprintf(debug_name, "%s.subsystems", bname);
		ss_fp = cfopen(debug_name, "wb", CFILE_NORMAL, CF_TYPE_TABLES );
		if ( !ss_fp )	{
			mprintf(( "Can't open debug file for writing subsystems for %s\n", filename));
		} else {
			strcpy_s(model_filename, filename);
			ss_warning_shown = 0;
		}
	}
#endif

	id = cfread_int(fp);

	if (id != POF_HEADER_ID)
		Error( LOCATION, "Bad ID in model file <%s>",filename);

	// Version is major*100+minor
	// So, major = version / 100;
	//     minor = version % 100;
	version = cfread_int(fp);

	//Warning( LOCATION, "POF Version = %d", version );
	
	if (version < PM_COMPATIBLE_VERSION || (version/100) > PM_OBJFILE_MAJOR_VERSION)	{
		Warning(LOCATION,"Bad version (%d) in model file <%s>",version,filename);
		return 0;
	}

	pm->version = version;
	Assert( strlen(filename) < FILESPEC_LENGTH );
	strcpy_s(pm->filename, filename);

	memset( &pm->view_positions, 0, sizeof(pm->view_positions) );

	// reset insignia counts
	pm->num_ins = 0;

	// reset glow points!! - Goober5000
	pm->n_glow_point_banks = 0;

	// reset SLDC
	pm->shield_collision_tree = NULL;
	pm->sldc_size = 0;

	id = cfread_int(fp);
	len = cfread_int(fp);
	next_chunk = cftell(fp) + len;

	while (!cfeof(fp)) {

//		mprintf(("Processing chunk <%c%c%c%c>, len = %d\n",id,id>>8,id>>16,id>>24,len));
//		key_getch();

		switch (id) {

			case ID_OHDR: {		//Object header
				//vector v;

				//mprintf(0,"Got chunk OHDR, len=%d\n",len);

#if defined( FREESPACE1_FORMAT )
				pm->n_models = cfread_int(fp);
//				mprintf(( "Num models = %d\n", pm->n_models ));
				pm->rad = cfread_float(fp);
				pm->flags = cfread_int(fp);	// 1=Allow tiling
#elif defined( FREESPACE2_FORMAT )
				pm->rad = cfread_float(fp);
				pm->flags = cfread_int(fp);	// 1=Allow tiling
				pm->n_models = cfread_int(fp);
//				mprintf(( "Num models = %d\n", pm->n_models ));
#endif
				
				pm->submodel = (bsp_info *)vm_malloc( sizeof(bsp_info)*pm->n_models );
				Assert(pm->submodel != NULL );
				for ( i = 0; i < pm->n_models; i++ )
				{
					/* HACK: This is an almighty hack because it is late at night and I don't want to screw up a vm_free */
					new ( &( pm->submodel[ i ].buffer ) ) SCP_vector<buffer_data>( );
					pm->submodel[ i ].Reset( );
				}

				//Assert(pm->n_models <= MAX_SUBMODELS);

				cfread_vector(&pm->mins,fp);
				cfread_vector(&pm->maxs,fp);

				// sanity first!
				if (maybe_swap_mins_maxs(&pm->mins, &pm->maxs)) {
					Warning(LOCATION, "Inverted bounding box on model '%s'!  Swapping values to compensate.", pm->filename);
				}
				model_calc_bound_box(pm->bounding_box, &pm->mins, &pm->maxs);
				
				pm->n_detail_levels = cfread_int(fp);
			//	mprintf(( "There are %d detail levels\n", pm->n_detail_levels ));
				for (i=0; i<pm->n_detail_levels;i++ )	{
					pm->detail[i] = cfread_int(fp);
					pm->detail_depth[i] = 0.0f;
			///		mprintf(( "Detail level %d is model %d.\n", i, pm->detail[i] ));
				}

				pm->num_debris_objects = cfread_int(fp);
				Assert( pm->num_debris_objects <= MAX_DEBRIS_OBJECTS );
				// mprintf(( "There are %d debris objects\n", pm->num_debris_objects ));
				for (i=0; i<pm->num_debris_objects;i++ )	{
					pm->debris_objects[i] = cfread_int(fp);
					// mprintf(( "Debris object %d is model %d.\n", i, pm->debris_objects[i] ));
				}

				if ( pm->version >= 1903 )	{
	
					if ( pm->version >= 2009 )	{
																	
						pm->mass = cfread_float(fp);
						cfread_vector( &pm->center_of_mass, fp );
						cfread_vector( &pm->moment_of_inertia.vec.rvec, fp );
						cfread_vector( &pm->moment_of_inertia.vec.uvec, fp );
						cfread_vector( &pm->moment_of_inertia.vec.fvec, fp );
					} else {
						// old code where mass wasn't based on area, so do the calculation manually

						float vol_mass = cfread_float(fp);
						//	Attn: John Slagel:  The following is better done in bspgen.
						// Convert volume (cubic) to surface area (quadratic) and scale so 100 -> 100
						float area_mass = (float) pow(vol_mass, 0.6667f) * 4.65f;

						pm->mass = area_mass;
						float mass_ratio = vol_mass / area_mass; 
							
						cfread_vector( &pm->center_of_mass, fp );
						cfread_vector( &pm->moment_of_inertia.vec.rvec, fp );
						cfread_vector( &pm->moment_of_inertia.vec.uvec, fp );
						cfread_vector( &pm->moment_of_inertia.vec.fvec, fp );

						// John remove this with change to bspgen
						vm_vec_scale( &pm->moment_of_inertia.vec.rvec, mass_ratio );
						vm_vec_scale( &pm->moment_of_inertia.vec.uvec, mass_ratio );
						vm_vec_scale( &pm->moment_of_inertia.vec.fvec, mass_ratio );
					}

					// a custom MOI is only used for ships, but we should probably log it anyway
					if ( IS_VEC_NULL(&pm->moment_of_inertia.vec.rvec)
						&& IS_VEC_NULL(&pm->moment_of_inertia.vec.uvec)
						&& IS_VEC_NULL(&pm->moment_of_inertia.vec.fvec) )
					{
						mprintf(("Model %s has a null moment of inertia!  (This is only a problem if the model is a ship.)\n", filename));
					}

				} else {
#ifndef NDEBUG
					if (stricmp("fighter04.pof", filename)) {
						if (Bogus_warning_flag_1903 == 0) {
							Warning(LOCATION, "Ship %s is old.  Cannot compute mass.\nSetting to 50.0f.  Talk to John.", filename);
							Bogus_warning_flag_1903 = 1;
						}
					}
#endif
					pm->mass = 50.0f;
					vm_vec_zero( &pm->center_of_mass );
					vm_set_identity( &pm->moment_of_inertia );
					vm_vec_scale(&pm->moment_of_inertia.vec.rvec, 0.001f);
					vm_vec_scale(&pm->moment_of_inertia.vec.uvec, 0.001f);
					vm_vec_scale(&pm->moment_of_inertia.vec.fvec, 0.001f);
				}

				// read in cross section info
				pm->xc = NULL;
				if ( pm->version >= 2014 ) {
					pm->num_xc = cfread_int(fp);
					if (pm->num_xc > 0) {
						pm->xc = (cross_section*) vm_malloc(pm->num_xc*sizeof(cross_section));
						for (i=0; i<pm->num_xc; i++) {
							pm->xc[i].z = cfread_float(fp);
							pm->xc[i].radius = cfread_float(fp);
						}
					}
				} else {
					pm->num_xc = 0;
				}

				if ( pm->version >= 2007 )	{
					pm->num_lights = cfread_int(fp);
					//mprintf(( "Found %d lights!\n", pm->num_lights ));

					if (pm->num_lights > 0) {
						pm->lights = (bsp_light *)vm_malloc( sizeof(bsp_light)*pm->num_lights );
						for (i=0; i<pm->num_lights; i++ )	{			
							cfread_vector(&pm->lights[i].pos,fp);
							pm->lights[i].type = cfread_int(fp);
							pm->lights[i].value = 0.0f;
						}
					}
				} else {
					pm->num_lights = 0;
					pm->lights = NULL;
				}

				break;
			}
			
			case ID_SOBJ: {		//Subobject header
				int n;
				char *p, props[MAX_PROP_LEN];
//				float d;

				//mprintf(0,"Got chunk SOBJ, len=%d\n",len);

				n = cfread_int(fp);
				//mprintf(("SOBJ IDed itself as %d", n));

				Assert(n < pm->n_models );

#if defined( FREESPACE2_FORMAT )	
				pm->submodel[n].rad = cfread_float(fp);		//radius
#endif

				pm->submodel[n].parent = cfread_int(fp);

//				cfread_vector(&pm->submodel[n].norm,fp);
//				d = cfread_float(fp);				
//				cfread_vector(&pm->submodel[n].pnt,fp);
				cfread_vector(&pm->submodel[n].offset,fp);

//			mprintf(( "Subobj %d, offs = %.1f, %.1f, %.1f\n", n, pm->submodel[n].offset.xyz.x, pm->submodel[n].offset.xyz.y, pm->submodel[n].offset.xyz.z ));
	
#if defined ( FREESPACE1_FORMAT )
				pm->submodel[n].rad = cfread_float(fp);		//radius
#endif

//				pm->submodel[n].tree_offset = cfread_int(fp);	//offset
//				pm->submodel[n].data_offset = cfread_int(fp);	//offset

				cfread_vector(&pm->submodel[n].geometric_center,fp);

				cfread_vector(&pm->submodel[n].min,fp);
				cfread_vector(&pm->submodel[n].max,fp);

				pm->submodel[n].name[0] = '\0';

				cfread_string_len(pm->submodel[n].name, MAX_NAME_LEN, fp);		// get the name
				cfread_string_len(props, MAX_PROP_LEN, fp);			// and the user properties

				// sanity first!
				if (maybe_swap_mins_maxs(&pm->submodel[n].min, &pm->submodel[n].max)) {
					Warning(LOCATION, "Inverted bounding box on submodel '%s' of model '%s'!  Swapping values to compensate.", pm->submodel[n].name, pm->filename);
				}
				model_calc_bound_box(pm->submodel[n].bounding_box, &pm->submodel[n].min, &pm->submodel[n].max);

				pm->submodel[n].movement_type = cfread_int(fp);
				pm->submodel[n].movement_axis = cfread_int(fp);

				// change turret movement type to MOVEMENT_TYPE_ROT_SPECIAL
				if (pm->submodel[n].movement_type == MOVEMENT_TYPE_ROT) {
					if ( strstr(pm->submodel[n].name, "turret") || strstr(pm->submodel[n].name, "gun") || strstr(pm->submodel[n].name, "cannon")) {
						pm->submodel[n].movement_type = MOVEMENT_TYPE_ROT_SPECIAL;
					} else if (strstr(pm->submodel[n].name, "thruster")) {
						// Int3();
						pm->submodel[n].movement_type = MOVEMENT_TYPE_NONE;
						pm->submodel[n].movement_axis = MOVEMENT_AXIS_NONE;
					}else if(strstr(props, "$triggered:")){
						pm->submodel[n].movement_type = MOVEMENT_TYPE_TRIGGERED;
					}
				}

				if(( p = strstr(props, "$dumb_rotate:"))!= NULL ){ //Iyojj skybox 4
					pm->submodel[n].movement_type = MSS_FLAG_DUM_ROTATES;
					pm->submodel[n].dumb_turn_rate = (float)atof(p+13);
				}else{
					pm->submodel[n].dumb_turn_rate = 0.0f;
				}

				if ( pm->submodel[n].name[0] == '\0' ) {
					strcpy_s(pm->submodel[n].name, "unknown object name");
				}

				bool rotating_submodel_has_subsystem = !(pm->submodel[n].movement_type == MOVEMENT_TYPE_ROT);
				if ( ( p = strstr(props, "$special"))!= NULL ) {
					char type[64];

					get_user_prop_value(p+9, type);
					if ( !stricmp(type, "subsystem") ) {	// if we have a subsystem, put it into the list!
						do_new_subsystem( n_subsystems, subsystems, n, pm->submodel[n].rad, &pm->submodel[n].offset, props, pm->submodel[n].name, pm->id );
						rotating_submodel_has_subsystem = true;
					} else if ( !stricmp(type, "no_rotate") ) {
						// mark those submodels which should not rotate - ie, those with no subsystem
						pm->submodel[n].movement_type = MOVEMENT_TYPE_NONE;
						pm->submodel[n].movement_axis = MOVEMENT_AXIS_NONE;
					} else {
						// if submodel rotates (via bspgen), then there is either a subsys or special=no_rotate
						Assert( pm->submodel[n].movement_type != MOVEMENT_TYPE_ROT );
					}
				}

				// adding a warning if rotation is specified without movement axis.
				if ((pm->submodel[n].movement_type == MOVEMENT_TYPE_ROT) && (pm->submodel[n].movement_axis == MOVEMENT_AXIS_NONE)){
					Warning(LOCATION, "Rotation without rotation axis defined on submodel '%s' of model '%s'!", pm->submodel[n].name, pm->filename);
				}

/*				if ( strstr(props, "$nontargetable")!= NULL ) {
					pm->submodel[n].targetable = 0;
				}else{
					pm->submodel[n].targetable = 1;
				}
*/
//				pm->submodel[n].n_triggers = 0;
//				pm->submodel[n].triggers = NULL;

				//parse_triggers(pm->submodel[n].n_triggers, &pm->submodel[n].triggers, &props[0]);

				if (strstr(props, "$no_collisions") != NULL )
					pm->submodel[n].no_collisions = true;
				else
					pm->submodel[n].no_collisions = false;

				if (strstr(props, "$nocollide_this_only") != NULL )
					pm->submodel[n].nocollide_this_only = true;
				else
					pm->submodel[n].nocollide_this_only = false;

				if (strstr(props, "$collide_invisible") != NULL )
					pm->submodel[n].collide_invisible = true;
				else
					pm->submodel[n].collide_invisible = false;

				if ( (p = strstr(props, "$gun_rotation:")) == NULL )
					pm->submodel[n].gun_rotation = true;
				else
					pm->submodel[n].gun_rotation = false;

				if ( (p = strstr(props, "$lod0_name")) != NULL)
					get_user_prop_value(p+10, pm->submodel[n].lod_name);

				if ( (p = strstr(props, "$detail_box:")) != NULL ) {
					p += 12;
					while (*p == ' ') p++;
					pm->submodel[n].use_render_box = atoi(p);

					if ( (p = strstr(props, "$box_min:")) != NULL ) {
						p += 9;
						while (*p == ' ') p++;
						pm->submodel[n].render_box_min.xyz.x = (float)strtod(p, (char **)NULL);
						while (*p != ',') p++;
						pm->submodel[n].render_box_min.xyz.y = (float)strtod(++p, (char **)NULL);
						while (*p != ',') p++;
						pm->submodel[n].render_box_min.xyz.z = (float)strtod(++p, (char **)NULL);
					} else {
						pm->submodel[n].render_box_min = pm->submodel[n].min;
					}

					if ( (p = strstr(props, "$box_max:")) != NULL ) {
						p += 9;
						while (*p == ' ') p++;
						pm->submodel[n].render_box_max.xyz.x = (float)strtod(p, (char **)NULL);
						while (*p != ',') p++;
						pm->submodel[n].render_box_max.xyz.y = (float)strtod(++p, (char **)NULL);
						while (*p != ',') p++;
						pm->submodel[n].render_box_max.xyz.z = (float)strtod(++p, (char **)NULL);
					} else {
						pm->submodel[n].render_box_max = pm->submodel[n].max;
					}
				}

				// Added for new handling of turret orientation - KeldorKatarn
				matrix	*orient = &pm->submodel[n].orientation;

				if ( (p = strstr(props, "$uvec:")) != NULL ) {
					p += 6;

					char *parsed_string = p;

					while (*parsed_string == ' ') {
						parsed_string++; // Skip spaces
					}

					orient->vec.uvec.xyz.x = (float)(strtod(parsed_string, (char **)NULL));

					// Find end of number
					parsed_string = strchr(parsed_string, ',');
					Assert(parsed_string != NULL);
					parsed_string++;

					while (*parsed_string == ' ') {
						parsed_string++; // Skip spaces
					}

					orient->vec.uvec.xyz.y = (float)(strtod(parsed_string, (char **)NULL));

					// Find end of number
					parsed_string = strchr(parsed_string, ',');
					Assert(parsed_string != NULL);
					parsed_string++;

					while (*parsed_string == ' ') {
						parsed_string++; // Skip spaces
					}

					orient->vec.uvec.xyz.z = (float)(strtod(parsed_string, (char **)NULL));

					if ( (p = strstr(props, "$fvec:")) != NULL ) {
						parsed_string = p + 6;

						while (*parsed_string == ' ') {
							parsed_string++; // Skip spaces
						}

						orient->vec.fvec.xyz.x = (float)(strtod(parsed_string, (char **)NULL));

						// Find end of number
						parsed_string = strchr(parsed_string, ',');
						Assert(parsed_string != NULL);
						parsed_string++;

						while (*parsed_string == ' ') {
							parsed_string++; // Skip spaces
						}

						orient->vec.fvec.xyz.y = (float)(strtod(parsed_string, (char **)NULL));

						// Find end of number
						parsed_string = strchr(parsed_string, ',');
						Assert(parsed_string != NULL);
						parsed_string++;

						while (*parsed_string == ' ') {
							parsed_string++; // Skip spaces
						}

						orient->vec.fvec.xyz.z = (float)(strtod(parsed_string, (char **)NULL));

						pm->submodel[n].force_turret_normal = true;

						vm_vec_normalize(&orient->vec.uvec);
						vm_vec_normalize(&orient->vec.fvec);

						vm_vec_crossprod(&orient->vec.rvec, &orient->vec.uvec, &orient->vec.fvec);
						vm_vec_crossprod(&orient->vec.fvec, &orient->vec.rvec, &orient->vec.uvec);

						vm_vec_normalize(&orient->vec.fvec);
						vm_vec_normalize(&orient->vec.rvec);

						vm_orthogonalize_matrix(orient);
					} else {
						int parent_num = pm->submodel[n].parent;

						if (parent_num > -1) {
							*orient = pm->submodel[parent_num].orientation;
						} else {
							*orient = vmd_identity_matrix;
						}

						Warning( LOCATION, "Improper custom orientation matrix, you must define a up vector, then a forward vector");
					}
				} else {
					int parent_num = pm->submodel[n].parent;

					if (parent_num > -1) {
						*orient = pm->submodel[parent_num].orientation;
					} else {
						*orient = vmd_identity_matrix;
					}

					if (strstr(props, "$fvec:") != NULL) {
						Warning( LOCATION, "Improper custom orientation matrix, you must define a up vector, then a forward vector");
					}
				}

				if ( !rotating_submodel_has_subsystem ) {
					nprintf(("Model", "Model %s: Rotating Submodel without subsystem: %s\n", pm->filename, pm->submodel[n].name));

					// mark those submodels which should not rotate - ie, those with no subsystem
					pm->submodel[n].movement_type = MOVEMENT_TYPE_NONE;
					pm->submodel[n].movement_axis = MOVEMENT_AXIS_NONE;
				}


				pm->submodel[n].angs.p = 0.0f;
				pm->submodel[n].angs.b = 0.0f;
				pm->submodel[n].angs.h = 0.0f;

				{
					int nchunks = cfread_int( fp );		// Throw away nchunks
					if ( nchunks > 0 )	{
						Error( LOCATION, "Model '%s' is chunked.  See John or Adam!\n", pm->filename );
					}
				}
				pm->submodel[n].bsp_data_size = cfread_int(fp);
				if ( pm->submodel[n].bsp_data_size > 0 )	{
					pm->submodel[n].bsp_data = (ubyte *)vm_malloc(pm->submodel[n].bsp_data_size);
					cfread(pm->submodel[n].bsp_data,1,pm->submodel[n].bsp_data_size,fp);
					swap_bsp_data( pm, pm->submodel[n].bsp_data );
				} else {
					pm->submodel[n].bsp_data = NULL;
				}

				if ( strstr( pm->submodel[n].name, "thruster") )	
					pm->submodel[n].is_thruster=1;
				else
					pm->submodel[n].is_thruster=0;

				// Genghis: if we have a thruster and none of the collision 
				// properties were provided, then set "nocollide_this_only".
				if (pm->submodel[n].is_thruster && !(pm->submodel[n].no_collisions) && !(pm->submodel[n].nocollide_this_only) && !(pm->submodel[n].collide_invisible) )
				{
					pm->submodel[n].nocollide_this_only = true;
				}

				if ( strstr( pm->submodel[n].name, "-destroyed") )	
					pm->submodel[n].is_damaged=1;
				else
					pm->submodel[n].is_damaged=0;

				//mprintf(( "Submodel %d, name '%s', parent = %d\n", n, pm->submodel[n].name, pm->submodel[n].parent ));
				//key_getch();

		//mprintf(( "Submodel %d, tree offset %d\n", n, pm->submodel[n].tree_offset ));
		//mprintf(( "Submodel %d, data offset %d\n", n, pm->submodel[n].data_offset ));
		//key_getch();

				if(!Cmdline_nohtl) {
					generate_vertex_buffers(&pm->submodel[n], pm);
				}
				break;

			}

			case ID_SLDC: // kazan - Shield Collision tree
				{
					pm->sldc_size = cfread_int(fp);
					pm->shield_collision_tree = (ubyte *)vm_malloc(pm->sldc_size);
					cfread(pm->shield_collision_tree,1,pm->sldc_size,fp);
					swap_sldc_data(pm->shield_collision_tree);
					//mprintf(( "Shield Collision Tree, %d bytes in size\n", pm->sldc_size));
				}
				break;

			case ID_SHLD:
				{
					pm->shield.nverts = cfread_int( fp );		// get the number of vertices in the list

					if (pm->shield.nverts > 0) {
						pm->shield.verts = (shield_vertex *)vm_malloc(pm->shield.nverts * sizeof(shield_vertex) );
						Assert( pm->shield.verts );
						for ( i = 0; i < pm->shield.nverts; i++ ) {						// read in the vertex list
							cfread_vector( &(pm->shield.verts[i].pos), fp );
						}
					}

					pm->shield.ntris = cfread_int( fp );		// get the number of triangles that compose the shield

					if (pm->shield.ntris > 0) {
						pm->shield.tris = (shield_tri *)vm_malloc(pm->shield.ntris * sizeof(shield_tri) );
						Assert( pm->shield.tris );
						for ( i = 0; i < pm->shield.ntris; i++ ) {
							cfread_vector( &temp_vec, fp );
							vm_vec_normalize_safe(&temp_vec);
							pm->shield.tris[i].norm = temp_vec;
							for ( j = 0; j < 3; j++ ) {
								pm->shield.tris[i].verts[j] = cfread_int( fp );		// read in the indices into the shield_vertex list
#ifndef NDEBUG
								if (pm->shield.tris[i].verts[j] >= pm->shield.nverts) {
									Error(LOCATION, "Ship %s has a bogus shield mesh.\nOnly %i vertices, index %i found.\n", filename, pm->shield.nverts, pm->shield.tris[i].verts[j]);
								}
#endif
							}
							
							for ( j = 0; j < 3; j++ ) {
								pm->shield.tris[i].neighbors[j] = cfread_int( fp );	// read in the neighbor indices -- indexes into tri list
#ifndef NDEBUG
								if (pm->shield.tris[i].neighbors[j] >= pm->shield.ntris) {
									Error(LOCATION, "Ship %s has a bogus shield mesh.\nOnly %i triangles, index %i found.\n", filename, pm->shield.ntris, pm->shield.tris[i].neighbors[j]);
								}
#endif
							}
						}
					}
				}
				break;

			case ID_GPNT:
				pm->n_guns = cfread_int(fp);

				if (pm->n_guns > 0) {
					pm->gun_banks = (w_bank *)vm_malloc(sizeof(w_bank) * pm->n_guns);
					Assert( pm->gun_banks != NULL );

					for (i = 0; i < pm->n_guns; i++ ) {
						w_bank *bank = &pm->gun_banks[i];

						bank->num_slots = cfread_int(fp);
						Assert ( bank->num_slots < MAX_SLOTS );
						for (j = 0; j < bank->num_slots; j++) {
							cfread_vector( &(bank->pnt[j]), fp );
							cfread_vector( &temp_vec, fp );
							vm_vec_normalize_safe(&temp_vec);
							bank->norm[j] = temp_vec;
						}
					}
				}
				break;
			
			case ID_MPNT:
				pm->n_missiles = cfread_int(fp);

				if (pm->n_missiles > 0) {
					pm->missile_banks = (w_bank *)vm_malloc(sizeof(w_bank) * pm->n_missiles);
					Assert( pm->missile_banks != NULL );

					for (i = 0; i < pm->n_missiles; i++ ) {
						w_bank *bank = &pm->missile_banks[i];

						bank->num_slots = cfread_int(fp);
						Assert ( bank->num_slots < MAX_SLOTS );
						for (j = 0; j < bank->num_slots; j++) {
							cfread_vector( &(bank->pnt[j]), fp );
							cfread_vector( &temp_vec, fp );
							vm_vec_normalize_safe(&temp_vec);
							bank->norm[j] = temp_vec;
						}
					}
				}
				break;

			case ID_DOCK: {
				char props[MAX_PROP_LEN];

				pm->n_docks = cfread_int(fp);

				if (pm->n_docks > 0) {
					pm->docking_bays = (dock_bay *)vm_malloc(sizeof(dock_bay) * pm->n_docks);
					Assert( pm->docking_bays != NULL );

					for (i = 0; i < pm->n_docks; i++ ) {
						char *p;
						dock_bay *bay = &pm->docking_bays[i];

						cfread_string_len( props, MAX_PROP_LEN, fp );
						if ( (p = strstr(props, "$name"))!= NULL )
							get_user_prop_value(p+5, bay->name);
						else
							sprintf(bay->name, "<unnamed bay %c>", 'A' + i);
						bay->num_spline_paths = cfread_int( fp );
						if ( bay->num_spline_paths > 0 ) {
							bay->splines = (int *)vm_malloc(sizeof(int) * bay->num_spline_paths);
							for ( j = 0; j < bay->num_spline_paths; j++ )
								bay->splines[j] = cfread_int(fp);
						} else {
							bay->splines = NULL;
						}

						// determine what this docking bay can be used for
						if ( !strnicmp(bay->name, "cargo", 5) )
							bay->type_flags = DOCK_TYPE_CARGO;
						else
							bay->type_flags = (DOCK_TYPE_REARM | DOCK_TYPE_GENERIC);

						bay->num_slots = cfread_int(fp);

						if(bay->num_slots != 2) {
							Warning(LOCATION, "Model '%s' has %d slots in dock point '%s'; models must have exactly %d slots per dock point.", filename, bay->num_slots, bay->name, 2);
						}

						for (j = 0; j < bay->num_slots; j++) {
							cfread_vector( &(bay->pnt[j]), fp );
							cfread_vector( &(bay->norm[j]), fp );
						}
					}
				}
				break;
			}

			case ID_GLOW:					//start glow point reading -Bobboau
			{
				char props[MAX_PROP_LEN];

				int gpb_num = cfread_int(fp);

				pm->n_glow_point_banks = gpb_num;
				pm->glow_point_banks = NULL;

				if (gpb_num > 0)
				{
					pm->glow_point_banks = (glow_point_bank *) vm_malloc(sizeof(glow_point_bank) * gpb_num);
					Assert(pm->glow_point_banks != NULL);
				}

				for (int gpb = 0; gpb < gpb_num; gpb++)
				{
					glow_point_bank *bank = &pm->glow_point_banks[gpb];

					bank->is_on = 1;
					bank->glow_timestamp = 0;
					bank->disp_time = cfread_int(fp);
					bank->on_time = cfread_int(fp);
					bank->off_time = cfread_int(fp);
					bank->submodel_parent = cfread_int(fp);
					bank->LOD = cfread_int(fp);
					bank->type = cfread_int(fp);
					bank->num_points = cfread_int(fp);
					bank->points = NULL;

					if (bank->num_points > 0)
						bank->points = (glow_point *) vm_malloc(sizeof(glow_point) * bank->num_points);

					if((bank->off_time > 0) && (bank->disp_time > 0))
						bank->is_on = 0;
	
					cfread_string_len(props, MAX_PROP_LEN, fp);
					// look for $glow_texture=xxx
					int length = strlen(props);

					if (length > 0)
					{
						int base_length = strlen("$glow_texture=");
						Assert(strstr( (const char *)&props, "$glow_texture=") != NULL);
						Assert(length > base_length);
						char *glow_texture_name = props + base_length;
						
						if (glow_texture_name[0] == '$')
							glow_texture_name++;

						bank->glow_bitmap = bm_load(glow_texture_name);

						if (bank->glow_bitmap < 0)
						{
							Warning( LOCATION, "Couldn't open texture '%s'\nreferenced by model '%s'\n", glow_texture_name, pm->filename);
						}
						else
						{
							nprintf(( "Model", "Glow point bank %i texture num is %d for '%s'\n", gpb, bank->glow_bitmap, pm->filename));
						}

						strcat(glow_texture_name, "-neb");
						bank->glow_neb_bitmap = bm_load(glow_texture_name);

						if (bank->glow_neb_bitmap < 0)
						{
							bank->glow_neb_bitmap = bank->glow_bitmap;
							nprintf(( "Model", "Glow point bank texture not found for '%s', setting as the normal one num\n", pm->filename));
						//	Error( LOCATION, "Couldn't open texture '%s'\nreferenced by model '%s'\n", glow_texture_name, pm->filename );
						}
						else
						{
							nprintf(( "Model", "Glow point bank %i nebula texture num is %d for '%s'\n", gpb, bank->glow_neb_bitmap, pm->filename));
						}
					}

					for (j = 0; j < bank->num_points; j++)
					{
						glow_point *p = &bank->points[j];

						cfread_vector(&(p->pnt), fp);
						cfread_vector( &temp_vec, fp );
						if (!IS_VEC_NULL_SQ_SAFE(&temp_vec))
							vm_vec_normalize(&temp_vec);
						else
							vm_vec_zero(&temp_vec);
						p->norm = temp_vec;
						p->radius = cfread_float( fp);
					}
				}
				break;					
			 }

			case ID_FUEL:
				char props[MAX_PROP_LEN];
				pm->n_thrusters = cfread_int(fp);

				if (pm->n_thrusters > 0) {
					pm->thrusters = (thruster_bank *)vm_malloc(sizeof(thruster_bank) * pm->n_thrusters);
					Assert( pm->thrusters != NULL );

					for (i = 0; i < pm->n_thrusters; i++ ) {
						thruster_bank *bank = &pm->thrusters[i];

						bank->num_points = cfread_int(fp);
						bank->points = NULL;

						if (bank->num_points > 0)
							bank->points = (glow_point *) vm_malloc(sizeof(glow_point) * bank->num_points);

						bank ->obj_num = -1;

						if (pm->version < 2117) {
							bank->wash_info_pointer = NULL;
						} else {
							cfread_string_len( props, MAX_PROP_LEN, fp );
							// look for $engine_subsystem=xxx
							int length = strlen(props);
							if (length > 0) {
								int base_length = strlen("$engine_subsystem=");
								Assert( strstr( (const char *)&props, "$engine_subsystem=") != NULL );
								Assert( length > base_length );
								char *engine_subsys_name = props + base_length;
								if (engine_subsys_name[0] == '$') {
									engine_subsys_name++;
								}

								nprintf(("wash", "Ship %s with engine wash associated with subsys %s\n", filename, engine_subsys_name));

								// set wash_info_index to invalid
								int table_error = 1;
								bank->wash_info_pointer = NULL;
								for (int k=0; k<n_subsystems; k++) {
									if ( !subsystem_stricmp(subsystems[k].subobj_name, engine_subsys_name) ) {
										bank->wash_info_pointer = subsystems[k].engine_wash_pointer;
										if (bank->wash_info_pointer != NULL) {
											table_error = 0;
										}
										// also set what subsystem this is attached to but not if we only have one thruster bank
										// do this so that original :V: models still work like they used to
										if (pm->n_thrusters > 1) {
											bank->obj_num = k;
										}
										break;
									}
								}

								if ( (bank->wash_info_pointer == NULL) && (n_subsystems > 0) ) {
									if (table_error) {
									//	Warning(LOCATION, "No engine wash table entry in ships.tbl for ship model %s", filename);
									} else {
										Warning(LOCATION, "Inconsistent model: Engine wash engine subsystem does not match any ship subsytem names for ship model %s", filename);
									}
								}
							} else {
								bank->wash_info_pointer = NULL;
							}
						}

						for (j = 0; j < bank->num_points; j++) {
							glow_point *p = &bank->points[j];

							cfread_vector( &(p->pnt), fp );
							cfread_vector( &temp_vec, fp );
							vm_vec_normalize_safe(&temp_vec);
							p->norm = temp_vec;

							if ( pm->version > 2004 )	{
								p->radius = cfread_float( fp );
								//mprintf(( "Rad = %.2f\n", rad ));
							} else {
								p->radius = 1.0f;
							}
						}
						//mprintf(( "Num slots = %d\n", bank->num_slots ));
					}
				}
				break;

			case ID_TGUN:
			case ID_TMIS: {
				int n_banks, n_slots, parent;
				model_subsystem *subsystemp;
				int snum=-1;
	
				n_banks = cfread_int(fp);				// number of turret points
				for ( i = 0; i < n_banks; i++ ) {
					int physical_parent;			// who are we attached to?
					parent = cfread_int( fp );			// get the turret parent of the object

					physical_parent = cfread_int(fp);	// The parent subobj that this is physically attached to

					if ( subsystems ) {
						for ( snum = 0; snum < n_subsystems; snum++ ) {
							subsystemp = &subsystems[snum];

							if ( parent == subsystemp->subobj_num ) {
								cfread_vector( &temp_vec, fp );
								vm_vec_normalize_safe(&temp_vec);
								subsystemp->turret_norm = temp_vec;
								vm_vector_2_matrix(&subsystemp->turret_matrix,&subsystemp->turret_norm,NULL,NULL);

								n_slots = cfread_int( fp );
								subsystemp->turret_gun_sobj = physical_parent;
								if(n_slots > MAX_TFP) {
									Warning(LOCATION, "Model %s has too many turret firing points on subsystem %s", subsystemp->name);
								}

								for (j = 0; j < n_slots; j++ )	{
									if(j < MAX_TFP)
										cfread_vector( &subsystemp->turret_firing_point[j], fp );
									else
									{
										vec3d bogus;
										cfread_vector(&bogus, fp);
									}
								}
								Assert( n_slots > 0 );

								subsystemp->turret_num_firing_points = n_slots;

								break;
							}
						}
					}

//turret_gun_sobj

					if ( (n_subsystems == 0) || (snum == n_subsystems) ) {
						vec3d bogus;

						nprintf(("Warning", "Turret object not found for turret firing point in model %s\n", model_filename));
						cfread_vector( &bogus, fp );
						n_slots = cfread_int( fp );
						for (j = 0; j < n_slots; j++ )
							cfread_vector( &bogus, fp );
					}
				}
				break;
			}

			case ID_SPCL: {
				char name[MAX_NAME_LEN], props[MAX_PROP_LEN], *p;
				int n_specials;
				float radius;
				vec3d pnt;

				n_specials = cfread_int(fp);		// get the number of special subobjects we have
				for (i = 0; i < n_specials; i++) {

					// get the next free object of the subobject list.  Flag error if no more room

					cfread_string_len(name, MAX_NAME_LEN, fp);			// get the name of this special polygon

					cfread_string_len(props, MAX_PROP_LEN, fp);		// will definately have properties as well!
					cfread_vector( &pnt, fp );
					radius = cfread_float( fp );

					// check if $Split
					p = strstr(name, "$split");
					if (p != NULL) {
						pm->split_plane[pm->num_split_plane] = pnt.xyz.z;
						pm->num_split_plane++;
						Assert(pm->num_split_plane <= MAX_SPLIT_PLANE);
					} else if ( ( p = strstr(props, "$special"))!= NULL ) {
						char type[64];

						get_user_prop_value(p+9, type);
						if ( !stricmp(type, "subsystem") )						// if we have a subsystem, put it into the list!
							do_new_subsystem( n_subsystems, subsystems, -1, radius, &pnt, props, &name[1], pm->id );		// skip the first '$' character of the name
					} else if ( strstr(name, "$enginelarge") || strstr(name, "$enginehuge") ){
						do_new_subsystem( n_subsystems, subsystems, -1, radius, &pnt, props, &name[1], pm->id );		// skip the first '$' character of the name
					} else {
						nprintf(("Warning", "Unknown special object type %s while reading model %s\n", name, pm->filename));
					}					
				}
				break;
			}
			
			case ID_TXTR: {		//Texture filename list
				int n;
//				char name_buf[128];

				//mprintf(0,"Got chunk TXTR, len=%d\n",len);


				n = cfread_int(fp);
				pm->n_textures = n;
				// Don't overwrite memory!!
				Assert(n <= MAX_MODEL_TEXTURES);
				//mprintf(0,"  num textures = %d\n",n);
				for (i=0; i<n; i++ )
				{
					char tmp_name[256];
					cfread_string_len(tmp_name,127,fp);
					model_load_texture(pm, i, tmp_name);
					//mprintf(0,"<%s>\n",name_buf);
				}


				break;
			}
			
/*			case ID_IDTA:		//Interpreter data
				//mprintf(0,"Got chunk IDTA, len=%d\n",len);

				pm->model_data = (ubyte *)vm_malloc(len);
				pm->model_data_size = len;
				Assert(pm->model_data != NULL );
			
				cfread(pm->model_data,1,len,fp);
			
				break;
*/

			case ID_INFO:		// don't need to do anything with info stuff

				#ifndef NDEBUG
					pm->debug_info_size = len;
					pm->debug_info = (char *)vm_malloc(pm->debug_info_size+1);
					Assert(pm->debug_info!=NULL);
					memset(pm->debug_info,0,len+1);
					cfread( pm->debug_info, 1, len, fp );
				#endif
				break;

			case ID_GRID:
				break;

			case ID_PATH:
				pm->n_paths = cfread_int( fp );

				if (pm->n_paths <= 0) {
					break;
				}

				pm->paths = (model_path *)vm_malloc(sizeof(model_path)*pm->n_paths);
				Assert( pm->paths != NULL );

				memset( pm->paths, 0, sizeof(model_path) * pm->n_paths );
					
				for (i=0; i<pm->n_paths; i++ )	{
					cfread_string_len(pm->paths[i].name , MAX_NAME_LEN-1, fp);
					if ( pm->version >= 2002 ) {
						// store the sub_model name number of the parent
						cfread_string_len(pm->paths[i].parent_name , MAX_NAME_LEN-1, fp);
						// get rid of leading '$' char in name
						if ( pm->paths[i].parent_name[0] == '$' ) {
							char tmpbuf[MAX_NAME_LEN];
							strcpy_s(tmpbuf, pm->paths[i].parent_name+1);
							strcpy_s(pm->paths[i].parent_name, tmpbuf);
						}
						// store the sub_model index (ie index into pm->submodel) of the parent
						pm->paths[i].parent_submodel = -1;
						for ( j = 0; j < pm->n_models; j++ ) {
							if ( !stricmp( pm->submodel[j].name, pm->paths[i].parent_name) ) {
								pm->paths[i].parent_submodel = j;
							}
						}
					} else {
						pm->paths[i].parent_name[0] = 0;
						pm->paths[i].parent_submodel = -1;
					}

					pm->paths[i].nverts = cfread_int( fp );
					pm->paths[i].verts = (mp_vert *)vm_malloc( sizeof(mp_vert) * pm->paths[i].nverts );
					pm->paths[i].goal = pm->paths[i].nverts - 1;
					pm->paths[i].type = MP_TYPE_UNUSED;
					pm->paths[i].value = 0;
					Assert(pm->paths[i].verts!=NULL);
					memset( pm->paths[i].verts, 0, sizeof(mp_vert) * pm->paths[i].nverts );

					for (j=0; j<pm->paths[i].nverts; j++ )	{
						cfread_vector(&pm->paths[i].verts[j].pos,fp );
						pm->paths[i].verts[j].radius = cfread_float( fp );
						
						{					// version 1802 added turret stuff
							int nturrets, k;

							nturrets = cfread_int( fp );
							pm->paths[i].verts[j].nturrets = nturrets;

							if (nturrets > 0) {
								pm->paths[i].verts[j].turret_ids = (int *)vm_malloc( sizeof(int) * nturrets );
								for ( k = 0; k < nturrets; k++ )
									pm->paths[i].verts[j].turret_ids[k] = cfread_int( fp );
							}
						} 
						
					}
				}
				break;

			case ID_EYE:					// an eye position(s)
				{
					int num_eyes;

					// all eyes points are stored simply as vectors and their normals.
					// 0th element is used as usual player view position.

					num_eyes = cfread_int( fp );
					pm->n_view_positions = num_eyes;
					Assert ( num_eyes < MAX_EYES );
					for (i = 0; i < num_eyes; i++ ) {
						pm->view_positions[i].parent = cfread_int( fp );
						cfread_vector( &pm->view_positions[i].pnt, fp );
						cfread_vector( &pm->view_positions[i].norm, fp );
					}
				}
				break;			

			case ID_INSG:				
				int num_ins, num_verts, num_faces, idx, idx2, idx3;			
				
				// get the # of insignias
				num_ins = cfread_int(fp);
				pm->num_ins = num_ins;
				
				// read in the insignias
				for(idx=0; idx<num_ins; idx++){
					// get the detail level
					pm->ins[idx].detail_level = cfread_int(fp);

					// # of faces
					num_faces = cfread_int(fp);
					pm->ins[idx].num_faces = num_faces;
					Assert(num_faces <= MAX_INS_FACES);

					// # of vertices
					num_verts = cfread_int(fp);
					Assert(num_verts <= MAX_INS_VECS);

					// read in all the vertices
					for(idx2=0; idx2<num_verts; idx2++){
						cfread_vector(&pm->ins[idx].vecs[idx2], fp);
					}

					// read in world offset
					cfread_vector(&pm->ins[idx].offset, fp);

					// read in all the faces
					for(idx2=0; idx2<pm->ins[idx].num_faces; idx2++){						
						// read in 3 vertices
						for(idx3=0; idx3<3; idx3++){
							pm->ins[idx].faces[idx2][idx3] = cfread_int(fp);
							pm->ins[idx].u[idx2][idx3] = cfread_float(fp);
							pm->ins[idx].v[idx2][idx3] = cfread_float(fp);
						}
						vec3d tempv;

						//get three points (rotated) and compute normal

						vm_vec_perp(&tempv, 
							&pm->ins[idx].vecs[pm->ins[idx].faces[idx2][0]], 
							&pm->ins[idx].vecs[pm->ins[idx].faces[idx2][1]], 
							&pm->ins[idx].vecs[pm->ins[idx].faces[idx2][2]]);

						vm_vec_normalize_safe(&tempv);

						pm->ins[idx].norm[idx2] = tempv;
//						mprintf(("insignorm %.2f %.2f %.2f\n",pm->ins[idx].norm[idx2].xyz.x, pm->ins[idx].norm[idx2].xyz.y, pm->ins[idx].norm[idx2].xyz.z));

					}
				}					
				break;

			// autocentering info
			case ID_ACEN:
				cfread_vector(&pm->autocenter, fp);
				pm->flags |= PM_FLAG_AUTOCEN;
				break;

			default:
				mprintf(("Unknown chunk <%c%c%c%c>, len = %d\n",id,id>>8,id>>16,id>>24,len));
				cfseek(fp,len,SEEK_CUR);
				break;

		}
		cfseek(fp,next_chunk,SEEK_SET);

		id = cfread_int(fp);
		len = cfread_int(fp);
		next_chunk = cftell(fp) + len;

	}

#ifndef NDEBUG
	if ( ss_fp) {
		int size;
		
		cfclose(ss_fp);
		ss_fp = cfopen(debug_name, "rb");
		if ( ss_fp )	{
			size = cfilelength(ss_fp);
			cfclose(ss_fp);
			if ( size <= 0 )	{
				_unlink(debug_name);
			}
		}
	}
#endif

	cfclose(fp);

	// these must be reset to NULL for the tests to work correctly later
	if (ibuffer_info.read != NULL)
		cfclose( ibuffer_info.read );

	if (ibuffer_info.tsb_read != NULL)
		cfclose( ibuffer_info.tsb_read );

	if (ibuffer_info.write != NULL) {
		// if we switched to v2 at any point during IBX creation, make sure to update the header
		if (ibuffer_info.version == 2) {
			cfseek( ibuffer_info.write, 0, CF_SEEK_SET );
			cfwrite_int( 0x58424932, ibuffer_info.write ); // "XBI2" - ("2IBX" in file)
		}

		cfclose( ibuffer_info.write );
	}

	if (ibuffer_info.tsb_write != NULL) {
		// update TSB with IBX checksum, for data sanity reasons
		CFILE *cfp = cfopen( ibuffer_info.name, "rb", CFILE_NORMAL, CF_TYPE_CACHE );
		Assert( cfp );

		uint ibx_checksum = 0;
		cfseek(cfp, 0, SEEK_SET);
		cf_chksum_long(cfp, &ibx_checksum);
		cfseek(cfp, 0, SEEK_SET);

		cfseek( ibuffer_info.tsb_write, sizeof(int), CF_SEEK_SET );
		// write new checksum
		cfwrite_int( ibx_checksum, ibuffer_info.tsb_write );

		cfclose( cfp );
		cfclose( ibuffer_info.tsb_write );
	}

	memset( &ibuffer_info, 0, sizeof(IBX) );

	// mprintf(("Done processing chunks\n"));
	return 1;
}

void model_init_texture_map(texture_map *tmap)
{
	if (tmap == NULL)
		return;

	memset(tmap, 0, sizeof(texture_map));

	for(int i = 0; i < TM_NUM_TYPES; i++)
	{
		tmap->textures[i].clear();
	}
}

//Goober
void model_load_texture(polymodel *pm, int i, char *file)
{
	// NOTE: it doesn't help to use more than MAX_FILENAME_LEN here as bmpman will use that restriction
	//       we also have to make sure there is always a trailing NUL since overflow doesn't add it
	char tmp_name[MAX_FILENAME_LEN];
	strcpy_s(tmp_name, file);
	strlwr(tmp_name);

	texture_map *tmap = &pm->maps[i];
	model_init_texture_map(tmap);

	//WMC - IMPORTANT!!
	//The Fred_running checks are there so that FRED will see those textures and put them in the
	//texture replacement box.

	// base maps ---------------------------------------------------------------
	texture_info *tbase = &tmap->textures[TM_BASE_TYPE];
	if (strstr(tmp_name, "thruster") || strstr(tmp_name, "invisible") || strstr(tmp_name, "warpmap"))
	{
		// Don't load textures for thruster animations or invisible textures
		// or warp models!-Bobboau
		tbase->clear();
	}
	else
	{
		// check if we should be transparent, include "-trans" but make sure to skip anything that might be "-transport"
		if ( (strstr(tmp_name, "-trans") && !strstr(tmp_name, "-transpo")) || strstr(tmp_name, "shockwave") ) {
			tmap->is_transparent = true;
		}

		if (strstr(tmp_name, "-amb")) {
			tmap->is_ambient = true;
		}

		tbase->LoadTexture(tmp_name, pm->filename);
		if(tbase->GetTexture() < 0)
			Warning(LOCATION, "Couldn't open texture '%s'\nreferenced by model '%s'\n", tmp_name, pm->filename);
	}
	// -------------------------------------------------------------------------

	// glow maps ---------------------------------------------------------------
	texture_info *tglow = &tmap->textures[TM_GLOW_TYPE];
	if ( (!Cmdline_glow && !Fred_running) || (tbase->GetTexture() < 0))
	{
		tglow->clear();
	}
	else
	{
		strcpy_s(tmp_name, file);
		strcat_s(tmp_name, "-glow" );
		strlwr(tmp_name);

		tglow->LoadTexture(tmp_name, pm->filename);
	}
	// -------------------------------------------------------------------------

	// specular maps -----------------------------------------------------------
	texture_info *tspec = &tmap->textures[TM_SPECULAR_TYPE];
	if ( (!Cmdline_spec && !Fred_running) || (tbase->GetTexture() < 0))
	{
		tspec->clear();
	}
	else
	{
		strcpy_s(tmp_name, file);
		strcat_s(tmp_name, "-shine");
		strlwr(tmp_name);

		tspec->LoadTexture(tmp_name, pm->filename);
	}
	//tmap->spec_map.original_texture = tmap->spec_map.texture;
	// -------------------------------------------------------------------------

	// bump maps ---------------------------------------------------------------
	texture_info *tnorm = &tmap->textures[TM_NORMAL_TYPE];
	texture_info *theight = &tmap->textures[TM_HEIGHT_TYPE];
	if ( (!Cmdline_normal && !Fred_running) || (tbase->GetTexture() < 0) ) {
		tnorm->clear();
		theight->clear();
	} else {
		strcpy_s(tmp_name, file);
		strcat_s(tmp_name, "-normal");
		strlwr(tmp_name);

		tnorm->LoadTexture(tmp_name, pm->filename);

		// try to get a height map too
		if ( Cmdline_height && (tnorm->GetTexture() > 0) ) {
			strcpy_s(tmp_name, file);
			strcat_s(tmp_name, "-height");
			strlwr(tmp_name);

			theight->LoadTexture(tmp_name, pm->filename);
		}
	}
	// -------------------------------------------------------------------------
}

//returns the number of this model
int model_load(char *filename, int n_subsystems, model_subsystem *subsystems, int ferror, int duplicate)
{
	int i, num, arc_idx;
	polymodel *pm = NULL;

	if ( !model_initted )
		model_init();

//	int Model_ram = 0;

#ifndef NDEBUG
	int ram_before = TotalRam;
#endif

	//Assert(strlen(filename) <= 12);

	num = -1;

	for (i=0; i< MAX_POLYGON_MODELS; i++)	{
		if ( Polygon_models[i] )	{
			if (!stricmp(filename, Polygon_models[i]->filename) && !duplicate)		{
				// Model already loaded; just return.
				Polygon_models[i]->used_this_mission++;
				return Polygon_models[i]->id;
			}
		} else if ( num == -1 )	{
			// This is the first empty slot
			num = i;
		}
	}

	// No empty slot
	if ( num == -1 )	{
		Error( LOCATION, "Too many models" );
		return -1;
	}	

	mprintf(( "Loading model '%s'\n", filename ));

	pm = (polymodel *)vm_malloc( sizeof(polymodel) );
	Assert( pm != NULL );
	
	Polygon_models[num] = pm;
	
	memset(pm, 0, sizeof(polymodel));

	pm->n_paths = 0;
	pm->paths = NULL;

	int org_sig = Model_signature;
	Model_signature+=MAX_POLYGON_MODELS;
	if ( Model_signature < org_sig )	{
		Model_signature = 0;
	}
	Assert( (Model_signature % MAX_POLYGON_MODELS) == 0 );
	pm->id = Model_signature + num;
	Assert( (pm->id % MAX_POLYGON_MODELS) == num );

	extern int Parse_normal_problem_count;
	Parse_normal_problem_count = 0;

	pm->used_this_mission = 0;

#ifndef NDEBUG
	char busy_text[60] = { '\0' };

	strcat_s( busy_text, "** ModelLoad: " );
	strcat_s( busy_text, filename );
	strcat_s( busy_text, " **" );

	game_busy(busy_text);
#endif

	if (read_model_file(pm, filename, n_subsystems, subsystems, ferror) < 0)	{
		if (pm != NULL) {
			vm_free(pm);
			pm = NULL;
		}

		Polygon_models[num] = NULL;
		return -1;
	}

	pm->used_this_mission++;

#ifdef _DEBUG
	if(Fred_running && Parse_normal_problem_count > 0)
	{
		char buffer[100];
		sprintf(buffer,"Serious problem loading model %s, %d normals capped to zero",
			filename, Parse_normal_problem_count);
		MessageBox(NULL,buffer,"Error", MB_OK);
	}
#endif


//mprintf(( "Loading model '%s'\n", filename ));
//key_getch();

//=============================
// Find the destroyed replacement models

	// Set up the default values
	for (i=0; i<pm->n_models; i++ )	{
		pm->submodel[i].my_replacement = -1;	// assume nothing replaces this
		pm->submodel[i].i_replace = -1;		// assume this doesn't replaces anything
	}

	// Search for models that have destroyed versions
	for (i=0; i<pm->n_models; i++ )	{
		int j;
		char destroyed_name[128];

		strcpy_s( destroyed_name, pm->submodel[i].name );
		strcat_s( destroyed_name, "-destroyed" );
		for (j=0; j<pm->n_models; j++ )	{
			if ( !stricmp( pm->submodel[j].name, destroyed_name ))	{
				// mprintf(( "Found destroyed model for '%s'\n", pm->submodel[i].name ));
				pm->submodel[i].my_replacement = j;
				pm->submodel[j].i_replace = i;
			}
		}

		// Search for models with live debris
		// This debris comes from a destroyed subsystem when ship is still alive
		char live_debris_name[128];

		strcpy_s( live_debris_name, "debris-" );
		strcat_s( live_debris_name, pm->submodel[i].name );


		pm->submodel[i].num_live_debris = 0;
		for (j=0; j<pm->n_models; j++ ) {
			// check if current model name is substring of destroyed
			if ( strstr( pm->submodel[j].name, live_debris_name ))	{
				mprintf(( "Found live debris model for '%s'\n", pm->submodel[i].name ));
				Assert(pm->submodel[i].num_live_debris < MAX_LIVE_DEBRIS);
				pm->submodel[i].live_debris[pm->submodel[i].num_live_debris++] = j;
				pm->submodel[j].is_live_debris = 1;
			}
		}

	}

	create_family_tree(pm);
//	dump_object_tree(pm);

//==============================
// Find all the lower detail versions of the hires model
	for (i=0; i<pm->n_models; i++ )	{
		int j, l1;
		bsp_info * sm1 = &pm->submodel[i];

		// set all arc types to be default 		
		for(arc_idx=0; arc_idx < MAX_ARC_EFFECTS; arc_idx++){
			sm1->arc_type[arc_idx] = MARC_TYPE_NORMAL;
		}

		sm1->num_details = 0;
		// If a backward compatibility LOD name is declared use it
		if (strlen(sm1->lod_name) != 0) {
			l1=strlen(sm1->lod_name);
		}
		// otherwise use the name for LOD comparision
		else {
			l1 = strlen(sm1->name);
		}

		for (j=0; j<pm->num_debris_objects;j++ )	{
			if ( i == pm->debris_objects[j] )	{
				sm1->is_damaged = 1;
			} 
		}


		for (j=0; j<MAX_MODEL_DETAIL_LEVELS; j++ )	{
			sm1->details[j] = -1;
		}

		for (j=0; j<pm->n_models; j++ )	{
			int k;
			bsp_info * sm2 = &pm->submodel[j];

			if ( i==j ) continue;
			
			// set all arc types to be default 		
			for(arc_idx=0; arc_idx < MAX_ARC_EFFECTS; arc_idx++){
				sm2->arc_type[arc_idx] = MARC_TYPE_NORMAL;
			}

			// if sm2 is a detail of sm1 and sm1 is a high detail, then add it to sm1's list
			if ((int)strlen(sm2->name)!=l1) continue; 
	
			int ndiff = 0;
			int first_diff = 0;
			for ( k=0; k<l1; k++)	{
				// If a backward compatibility LOD name is declared use it
				if (strlen(sm1->lod_name) != 0) {
					if (sm1->lod_name[k] != sm2->name[k] )	{
						if (ndiff==0) first_diff = k;
						ndiff++;
					}
				}
				// otherwise do the standard LOD comparision
				else {
					if (sm1->name[k] != sm2->name[k] )	{
						if (ndiff==0) first_diff = k;
						ndiff++;
					}
				}
			}
			if (ndiff==1)	{		// They only differ by one character!
				int dl1, dl2;
				// If a backward compatibility LOD name is declared use it
				if (strlen(sm1->lod_name) != 0) {
					dl1 = tolower(sm1->lod_name[first_diff]) - 'a';
				}
				// otherwise do the standard LOD comparision
				else {
					dl1 = tolower(sm1->name[first_diff]) - 'a';
				}
				dl2 = tolower(sm2->name[first_diff]) - 'a';

				if ( (dl1<0) || (dl2<0) || (dl1>=MAX_MODEL_DETAIL_LEVELS) || (dl2>=MAX_MODEL_DETAIL_LEVELS) ) continue;	// invalid detail levels

				if ( dl1 == 0 )	{
					dl2--;	// Start from 1 up...
					if (dl2 >= sm1->num_details ) sm1->num_details = dl2+1;
					sm1->details[dl2] = j;
  				    mprintf(( "Submodel '%s' is detail level %d of '%s'\n", sm2->name, dl2 + 1, sm1->name ));
				}
			}
		}

		for (j=0; j<sm1->num_details; j++ )	{
			if ( sm1->details[j] == -1 )	{
			//	Warning( LOCATION, "Model '%s' could find all detail levels for submodel '%s'", pm->filename, sm1->name );
				sm1->num_details = 0;
			}
		}

	}


	model_octant_create( pm );

	// Find the core_radius... the minimum of 
	float rx, ry, rz;
	rx = fl_abs( pm->submodel[pm->detail[0]].max.xyz.x - pm->submodel[pm->detail[0]].min.xyz.x );
	ry = fl_abs( pm->submodel[pm->detail[0]].max.xyz.y - pm->submodel[pm->detail[0]].min.xyz.y );
	rz = fl_abs( pm->submodel[pm->detail[0]].max.xyz.z - pm->submodel[pm->detail[0]].min.xyz.z );

	pm->core_radius = MIN( rx, MIN(ry, rz) ) / 2.0f;

	for (i=0; i<pm->n_view_positions; i++ )	{
		if ( pm->view_positions[i].parent == pm->detail[0] )	{
			float d = vm_vec_mag( &pm->view_positions[i].pnt );

			d += 0.1f;		// Make the eye 1/10th of a meter inside the sphere.

			if ( d > pm->core_radius )	{
				//mprintf(( "Model %s core radius increased from %.1f to %.1f to fit eye\n", pm->filename, pm->core_radius, d ));
				pm->core_radius = d;
			}		
		}
	}

#ifndef NDEBUG
	int ram_after = TotalRam;

	pm->ram_used = ram_after - ram_before;
	Model_ram += pm->ram_used;
	//mprintf(( "Model RAM = %d KB\n", Model_ram ));
#endif

	// Goober5000 - originally done in ship_create for no apparent reason
	model_set_subsys_path_nums(pm, n_subsystems, subsystems);
	model_set_bay_path_nums(pm);

	return pm->id;
}

// ensure that the subsys path is at least SUBSYS_PATH_DIST from the 
// second last to last point.
void model_maybe_fixup_subsys_path(polymodel *pm, int path_num)
{
	vec3d	*v1, *v2, dir;
	float	dist;
	int		index_1, index_2;

	Assert( (path_num >= 0) && (path_num < pm->n_paths) );

	model_path *mp;
	mp = &pm->paths[path_num];

	Assert(mp != NULL);
	Assert(mp->nverts > 1);
	
	index_1 = 1;
	index_2 = 0;

	v1 = &mp->verts[index_1].pos;
	v2 = &mp->verts[index_2].pos;
	
	dist = vm_vec_dist(v1, v2);
	if (dist < (SUBSYS_PATH_DIST - 10))
	{
		vm_vec_normalized_dir(&dir, v2, v1);
		vm_vec_scale_add(v2, v1, &dir, SUBSYS_PATH_DIST);
	}
}

// fill in the path_num field inside the model_subsystem struct.  This is an index into
// the pm->paths[] array, which is a path that provides a frontal approach to a subsystem
// (used for attacking purposes)
//
// NOTE: path_num in model_subsystem has the follows the following convention:
//			> 0	=> index into pm->paths[] for model that subsystem sits on
//			-1		=> path is not yet determined (may or may not exist)
//			-2		=> path doesn't yet exist for this subsystem
void model_set_subsys_path_nums(polymodel *pm, int n_subsystems, model_subsystem *subsystems)
{
	int i, j;

	for (i = 0; i < n_subsystems; i++)
		subsystems[i].path_num = -1;

	for (i = 0; i < n_subsystems; i++)
	{
		for (j = 0; j < pm->n_paths; j++)
		{
			if ( ((subsystems[i].subobj_num != -1) && (subsystems[i].subobj_num == pm->paths[j].parent_submodel)) ||
				(!subsystem_stricmp(subsystems[i].subobj_name, pm->paths[j].parent_name)) )
			{
				if (pm->n_paths > j)
				{
					subsystems[i].path_num = j;
					model_maybe_fixup_subsys_path(pm, j);

					break;
				}
			}
		}

		// If a path num wasn't located, then set value to -2
		if (subsystems[i].path_num == -1)
			subsystems[i].path_num = -2;
	}
}

// Determine the path indices (indicies into pm->paths[]) for the paths used for approaching/departing
// a fighter bay on a capital ship.
void model_set_bay_path_nums(polymodel *pm)
{
	int i;

	if (pm->ship_bay != NULL)
	{
		vm_free(pm->ship_bay);
		pm->ship_bay = NULL;
	}

	/*
	// currently only capital ships have fighter bays
	if ( !(sip->flags & (SIF_BIG_SHIP | SIF_HUGE_SHIP)) ) {
		return;
	}
	*/

	// malloc out storage for the path information
	pm->ship_bay = (ship_bay *) vm_malloc(sizeof(ship_bay));
	Assert(pm->ship_bay != NULL);

	pm->ship_bay->num_paths = 0;
	// TODO: determine if zeroing out here is affecting any earlier initializations
	pm->ship_bay->arrive_flags = 0;	// bitfield, set to 1 when that path number is reserved for an arrival
	pm->ship_bay->depart_flags = 0;	// bitfield, set to 1 when that path number is reserved for a departure


	// iterate through the paths that exist in the polymodel, searching for $bayN pathnames
	bool too_many_paths = false;
	for (i = 0; i < pm->n_paths; i++)
	{
		if (!strnicmp(pm->paths[i].name, NOX("$bay"), 4))
		{
			int bay_num;
			char temp[3];

			strncpy(temp, pm->paths[i].name + 4, 2);
			temp[2] = 0;
			bay_num = atoi(temp);

			if (bay_num < 1 || bay_num > MAX_SHIP_BAY_PATHS)
			{
				if(bay_num > MAX_SHIP_BAY_PATHS)
				{
					too_many_paths = true;
				}
				if(bay_num < 1)
				{
					Warning(LOCATION, "Model '%s' bay path '%s' index '%d' has an invalid bay number of %d", pm->filename, pm->paths[i].name, i, bay_num);
				}
				continue;
			}

			pm->ship_bay->path_indexes[bay_num - 1] = i;
			pm->ship_bay->num_paths++;
		}
	}
	if(too_many_paths)
	{
		Warning(LOCATION, "Model '%s' has too many bay paths - max is %d", pm->filename, MAX_SHIP_BAY_PATHS);
	}
}

// Get "parent" submodel for live debris submodel
int model_get_parent_submodel_for_live_debris( int model_num, int live_debris_model_num )
{
	polymodel *pm = model_get(model_num);

	Assert(pm->submodel[live_debris_model_num].is_live_debris == 1);

	int mn;
	bsp_info *child;

	// Start with the high level of detail hull 
	// Check all its children until we find the submodel to which the live debris belongs
	child = &pm->submodel[pm->detail[0]];
	mn = child->first_child;

	while (mn > 0) {
		child = &pm->submodel[mn];

		if (child->num_live_debris > 0) {
			// check all live debris submodels for the current child
			for (int idx=0; idx<child->num_live_debris; idx++) {
				if (child->live_debris[idx] == live_debris_model_num) {
					return mn;
				}
			}
			// DKA 5/26/99: can multiple live debris subsystems with each ship
			// NO LONGER TRUE Can only be 1 submodel with live debris
			// Error( LOCATION, "Could not find parent submodel for live debris.  Possible model error");
		}

		// get next child
		mn = child->next_sibling;
	}
	Error( LOCATION, "Could not find parent submodel for live debris");
	return -1;
}


float model_get_radius( int modelnum )
{
	polymodel *pm;

	pm = model_get(modelnum);

	return pm->rad;
}

float model_get_core_radius( int modelnum )
{
	polymodel *pm;

	pm = model_get(modelnum);

	return pm->core_radius;
}

float submodel_get_radius( int modelnum, int submodelnum )
{
	polymodel *pm;

	pm = model_get(modelnum);

	return pm->submodel[submodelnum].rad;
}



polymodel * model_get(int model_num)
{
	Assert( model_num >= 0 );
	if ( model_num < 0 )
		return NULL;

	int num = model_num % MAX_POLYGON_MODELS;
	
	Assert( num >= 0 );
	Assert( num < MAX_POLYGON_MODELS );
	Assert( Polygon_models[num] );
	Assert( Polygon_models[num]->id == model_num );

	return Polygon_models[num];
}


/*
// Finds the 3d bounding box of a model.  If submodel_num is -1,
// then it starts from the root object.   If inc_children is non-zero, 
// then this will recurse and find the bounding box for all children
// also.
void model_find_bound_box_3d(int model_num,int submodel_num, int inc_children, matrix *orient, vec3d * pos, vec3d * box )
{
	polymodel * pm;
	vec3d to_root_xlat;
	matrix to_root_rotate;
	int n_steps, steps[16];
	int tmp_sobj;
	
	if ( (model_num < 0) || (model_num >= N_polygon_models) ) return;

	pm = &Polygon_models[model_num];

	if ( submodel_num < 0 ) submodel_num = pm->detail[0];

	// traverse up the model tree to a root object.
	// Store this path in n_steps,
	n_steps = 0;
	tmp_sobj = submodel_num;
	while( tmp_sobj > -1 )	{
		steps[n_steps++] = tmp_sobj;
		tmp_sobj = pm->submodel[tmp_sobj].parent;
	}
	
	

//	vm_copy_transpose_matrix(&to_world_rotate, orient );
//	to_world_xlat = *pos;

}
*/



// Returns zero is x1,y1,x2,y2 are valid
// returns 1 for invalid model, 2 for point offscreen.
// note that x1,y1,x2,y2 aren't clipped to 2d screen coordinates!
int model_find_2d_bound_min(int model_num,matrix *orient, vec3d * pos,int *x1, int *y1, int *x2, int *y2 )
{
	polymodel * po;
	int n_valid_pts;
	int i, x,y,min_x, min_y, max_x, max_y;
	int rval = 0;

	po = model_get(model_num);

	g3_start_instance_matrix(pos,orient,false);
	
	n_valid_pts = 0;

	int hull = po->detail[0];

	min_x = min_y = max_x = max_y = 0;

	for (i=0; i<8; i++ )	{
		vertex pt;
		ubyte flags;

		flags = g3_rotate_vertex(&pt,&po->submodel[hull].bounding_box[i]);
		if ( !(flags&CC_BEHIND) ) {
			g3_project_vertex(&pt);

			if (!(pt.flags & PF_OVERFLOW)) {
				x = fl2i(pt.sx);
				y = fl2i(pt.sy);
				if ( n_valid_pts == 0 )	{
					min_x = x;
					min_y = y;
					max_x = x;
					max_y = y;
				} else {
					if ( x < min_x ) min_x = x;
					if ( y < min_y ) min_y = y;

					if ( x > max_x ) max_x = x;
					if ( y > max_y ) max_y = y;
				}
				n_valid_pts++;
			}
		}
	}

	if ( n_valid_pts < 8 )	{
		rval = 2;
	}

	if (x1) *x1 = min_x;
	if (y1) *y1 = min_y;

	if (x2) *x2 = max_x;
	if (y2) *y2 = max_y;

	g3_done_instance(false);

	return rval;
}


// Returns zero is x1,y1,x2,y2 are valid
// returns 1 for invalid model, 2 for point offscreen.
// note that x1,y1,x2,y2 aren't clipped to 2d screen coordinates!
int submodel_find_2d_bound_min(int model_num,int submodel, matrix *orient, vec3d * pos,int *x1, int *y1, int *x2, int *y2 )
{
	polymodel * po;
	int n_valid_pts;
	int i, x,y,min_x, min_y, max_x, max_y;
	bsp_info * sm;

	po = model_get(model_num);
	if ( (submodel < 0) || (submodel >= po->n_models ) ) return 1;
	sm = &po->submodel[submodel];
	
	g3_start_instance_matrix(pos,orient,false);
	
	n_valid_pts = 0;

	min_x = min_y = max_x = max_y = 0;

	for (i=0; i<8; i++ )	{
		vertex pt;
		ubyte flags;

		flags = g3_rotate_vertex(&pt,&sm->bounding_box[i]);
		if ( !(flags&CC_BEHIND) ) {
			g3_project_vertex(&pt);

			if (!(pt.flags & PF_OVERFLOW)) {
				x = fl2i(pt.sx);
				y = fl2i(pt.sy);
				if ( n_valid_pts == 0 )	{
					min_x = x;
					min_y = y;
					max_x = x;
					max_y = y;
				} else {
					if ( x < min_x ) min_x = x;
					if ( y < min_y ) min_y = y;

					if ( x > max_x ) max_x = x;
					if ( y > max_y ) max_y = y;
				}
				n_valid_pts++;
			}
		}
	}

	if ( n_valid_pts == 0 )	{
		return 2;
	}

	if (x1) *x1 = min_x;
	if (y1) *y1 = min_y;

	if (x2) *x2 = max_x;
	if (y2) *y2 = max_y;

	g3_done_instance(false);

	return 0;
}


// Returns zero is x1,y1,x2,y2 are valid
// returns 1 for invalid model, 2 for point offscreen.
// note that x1,y1,x2,y2 aren't clipped to 2d screen coordinates!
int model_find_2d_bound(int model_num,matrix *orient, vec3d * pos,int *x1, int *y1, int *x2, int *y2 )
{
	float t,w,h;
	vertex pnt;
	ubyte flags;
	polymodel * po;

	po = model_get(model_num);
	float width = po->rad;
	float height = po->rad;

	flags = g3_rotate_vertex(&pnt,pos);

	if ( pnt.flags & CC_BEHIND ) 
		return 2;

	if (!(pnt.flags&PF_PROJECTED))
		g3_project_vertex(&pnt);

	if (pnt.flags & PF_OVERFLOW)
		return 2;

	t = (width * Canv_w2)/pnt.z;
	w = t*Matrix_scale.xyz.x;

	t = (height*Canv_h2)/pnt.z;
	h = t*Matrix_scale.xyz.y;

	if (x1) *x1 = fl2i(pnt.sx - w);
	if (y1) *y1 = fl2i(pnt.sy - h);

	if (x2) *x2 = fl2i(pnt.sx + w);
	if (y2) *y2 = fl2i(pnt.sy + h);

	return 0;
}

// Returns zero is x1,y1,x2,y2 are valid
// returns 2 for point offscreen.
// note that x1,y1,x2,y2 aren't clipped to 2d screen coordinates!
int subobj_find_2d_bound(float radius ,matrix *orient, vec3d * pos,int *x1, int *y1, int *x2, int *y2 )
{
	float t,w,h;
	vertex pnt;
	ubyte flags;

	float width = radius;
	float height = radius;

	flags = g3_rotate_vertex(&pnt,pos);

	if ( pnt.flags & CC_BEHIND ) 
		return 2;

	if (!(pnt.flags&PF_PROJECTED))
		g3_project_vertex(&pnt);

	if (pnt.flags & PF_OVERFLOW)
		return 2;

	t = (width * Canv_w2)/pnt.z;
	w = t*Matrix_scale.xyz.x;

	t = (height*Canv_h2)/pnt.z;
	h = t*Matrix_scale.xyz.y;

	if (x1) *x1 = fl2i(pnt.sx - w);
	if (y1) *y1 = fl2i(pnt.sy - h);

	if (x2) *x2 = fl2i(pnt.sx + w);
	if (y2) *y2 = fl2i(pnt.sy + h);

	return 0;
}


// Given a vector that is in sub_model_num's frame of
// reference, and given the object's orient and position,
// return the vector in the model's frame of reference.
void model_find_obj_dir(vec3d *w_vec, vec3d *m_vec, object *ship_obj, int sub_model_num)
{
	vec3d tvec, vec;
	matrix m;
	int mn;

	Assert(ship_obj->type == OBJ_SHIP);

	polymodel *pm = model_get(Ship_info[Ships[ship_obj->instance].ship_info_index].model_num);
	vec = *m_vec;
	mn = sub_model_num;

	// instance up the tree for this point
	while ( (mn >= 0) && (pm->submodel[mn].parent >= 0) ) {
		// By using this kind of computation, the rotational angles can always
		// be computed relative to the submodel itself, instead of relative
		// to the parent - KeldorKatarn
		matrix rotation_matrix = pm->submodel[mn].orientation;
		vm_rotate_matrix_by_angles(&rotation_matrix, &pm->submodel[mn].angs);

		matrix inv_orientation;
		vm_copy_transpose_matrix(&inv_orientation, &pm->submodel[mn].orientation);

		vm_matrix_x_matrix(&m, &rotation_matrix, &inv_orientation);

		vm_vec_unrotate(&tvec, &vec, &m);
		vec = tvec;

		mn = pm->submodel[mn].parent;
	}

	// now instance for the entire object
	vm_vec_unrotate(w_vec, &vec, &ship_obj->orient);
}


// Given a point (pnt) that is in sub_model_num's frame of
// reference, return the point in in the object's frame of reference
void model_rot_sub_into_obj(vec3d * outpnt, vec3d *mpnt,polymodel *pm, int sub_model_num)
{
	vec3d pnt;
	vec3d tpnt;
	matrix m;
	int mn;

	pnt = *mpnt;
	mn = sub_model_num;

	//instance up the tree for this point
	while ( (mn >= 0) && (pm->submodel[mn].parent >= 0) ) {
		// By using this kind of computation, the rotational angles can always
		// be computed relative to the submodel itself, instead of relative
		// to the parent - KeldorKatarn
		matrix rotation_matrix = pm->submodel[mn].orientation;
		vm_rotate_matrix_by_angles(&rotation_matrix, &pm->submodel[mn].angs);
 
		matrix inv_orientation;
		vm_copy_transpose_matrix(&inv_orientation, &pm->submodel[mn].orientation);

		vm_matrix_x_matrix(&m, &rotation_matrix, &inv_orientation);

		vm_vec_unrotate(&tpnt, &pnt, &m);
		vm_vec_add(&pnt, &tpnt, &pm->submodel[mn].offset);

		mn = pm->submodel[mn].parent;
	}

	//now instance for the entire object
	*outpnt = pnt;
}


// Given a rotating submodel, find the ship and world axes or rotatation.
void model_get_rotating_submodel_axis(vec3d *model_axis, vec3d *world_axis, int modelnum, int submodel_num, object *obj)
{
	polymodel *pm = model_get(modelnum);

	bsp_info *sm = &pm->submodel[submodel_num];
	Assert(sm->movement_type == MOVEMENT_TYPE_ROT);

	if (sm->movement_axis == MOVEMENT_AXIS_X) {
		vm_vec_make(model_axis, 1.0f, 0.0f, 0.0f);
	} else if (sm->movement_axis == MOVEMENT_AXIS_Y) {
		vm_vec_make(model_axis, 0.0f, 1.0f, 0.0f);
	} else {
		Assert(sm->movement_axis == MOVEMENT_AXIS_Z);
		vm_vec_make(model_axis, 0.0f, 0.0f, 1.0f);
	}

	model_find_obj_dir(world_axis, model_axis, obj, submodel_num);
}


// Does stepped rotation of a submodel
void submodel_stepped_rotate(model_subsystem *psub, submodel_instance_info *sii)
{
	Assert(psub->flags & MSS_FLAG_STEPPED_ROTATE);

	if ( psub->subobj_num < 0 ) return;

	polymodel *pm = model_get(psub->model_num);
	bsp_info *sm = &pm->submodel[psub->subobj_num];

	if ( sm->movement_type != MOVEMENT_TYPE_ROT ) return;

	// get active rotation time this frame
	int end_stamp = timestamp();
	// just to make sure this issue wont pop up again... might cause odd jerking in some extremely odd situations
	// but given that those issues would require the timer to be reseted in any case it probably wont hurt
	float rotation_time;
	if ((end_stamp - sii->step_zero_timestamp) < 0) {
		sii->step_zero_timestamp = end_stamp;
		rotation_time = 0.0f;
	} else {
		rotation_time = 0.001f * (end_stamp - sii->step_zero_timestamp);
	}
	//Assert(rotation_time >= 0);

	// save last angles
	sii->prev_angs = sii->angs;

	// float pointer into struct to get angle (either p,b,h)
	float *ang_prev = NULL, *ang_next = NULL;
	switch( sm->movement_axis ) {
	case MOVEMENT_AXIS_X:
		ang_prev = &sii->prev_angs.p;
		ang_next = &sii->angs.p;
		break;

	case MOVEMENT_AXIS_Y:	
		ang_prev = &sii->prev_angs.h;
		ang_next = &sii->angs.h;
		break;

	case MOVEMENT_AXIS_Z:	
		ang_prev = &sii->prev_angs.b;
		ang_next = &sii->angs.b;
		break;
	}

	// just in case we got through that switch statement in error
	if ( (ang_prev == NULL) && (ang_next == NULL) )
		return;

	// angular displacement of one step
	float step_size = (PI2 / psub->stepped_rotation->num_steps);

	// get time to complete one step, including pause
	float step_time = psub->stepped_rotation->t_transit + psub->stepped_rotation->t_pause;

	// cur_step is step number relative to zero (0 - num_steps)
	// step_offset_time is TIME into current step
	float step_offset_time = (float)fmod(rotation_time, step_time);
	// subtract off fractional step part, round up  (ie, 1.999999 -> 2)
	int cur_step = int( ((rotation_time - step_offset_time) / step_time) + 0.5f);
	// mprintf(("cur step %d\n", cur_step));
	// Assert(step_offset_time >= 0);

	if (cur_step >= psub->stepped_rotation->num_steps) {
		// I don;t know why, but removing this line makes it all good.
		// sii->step_zero_timestamp += int(1000.0f * (psub->stepped_rotation->num_steps * step_time) + 0.5f);

		// reset cur_step (use mod to handle physics/ai pause)
		cur_step = cur_step % psub->stepped_rotation->num_steps;
	}

	// get base angle
	*ang_next = cur_step * step_size;

	// determine which phase of rotation we're in
	float coast_start_time = psub->stepped_rotation->fraction * psub->stepped_rotation->t_transit;
	float decel_start_time = psub->stepped_rotation->t_transit * (1.0f - psub->stepped_rotation->fraction);
	float pause_start_time = psub->stepped_rotation->t_transit;

	float start_coast_angle = 0.5f * psub->stepped_rotation->max_turn_accel * coast_start_time * coast_start_time;

	if (step_offset_time < coast_start_time) {
		// do accel
		float accel_time = step_offset_time;
		*ang_next += 0.5f * psub->stepped_rotation->max_turn_accel * accel_time * accel_time;
		sii->cur_turn_rate = psub->stepped_rotation->max_turn_accel * accel_time;
	} else if (step_offset_time < decel_start_time) {
		// do coast
		float coast_time = step_offset_time - coast_start_time;
		*ang_next += start_coast_angle + psub->stepped_rotation->max_turn_rate * coast_time;
		sii->cur_turn_rate = psub->stepped_rotation->max_turn_rate;
	} else if (step_offset_time < pause_start_time) {
		// do decel
		float time_to_pause = psub->stepped_rotation->t_transit - step_offset_time;
		*ang_next += (step_size - 0.5f * psub->stepped_rotation->max_turn_accel * time_to_pause * time_to_pause);
		sii->cur_turn_rate = psub->stepped_rotation->max_turn_rate * time_to_pause;
	} else {
		// do pause
		*ang_next += step_size;
		sii->cur_turn_rate = 0.0f;
	}
}

// Rotates the angle of a submodel.  Use this so the right unlocked axis
// gets stuffed.
void submodel_rotate(model_subsystem *psub, submodel_instance_info *sii)
{
	bsp_info * sm;

	if ( psub->subobj_num < 0 ) return;

	polymodel *pm = model_get(psub->model_num);
	sm = &pm->submodel[psub->subobj_num];

	if ( sm->movement_type != MOVEMENT_TYPE_ROT ) return;

	// save last angles
	sii->prev_angs = sii->angs;

	// probably send in a calculated desired turn rate
	float diff = sii->desired_turn_rate - sii->cur_turn_rate;

	float final_turn_rate;
	if (diff > 0) {
		final_turn_rate = sii->cur_turn_rate + sii->turn_accel * flFrametime;
		if (final_turn_rate > sii->desired_turn_rate) {
			final_turn_rate = sii->desired_turn_rate;
		}
	} else if (diff < 0) {
		final_turn_rate = sii->cur_turn_rate - sii->turn_accel * flFrametime;
		if (final_turn_rate < sii->desired_turn_rate) {
			final_turn_rate = sii->desired_turn_rate;
		}
	} else {
		final_turn_rate = sii->desired_turn_rate;
	}

	float delta = (sii->cur_turn_rate + final_turn_rate) * 0.5f * flFrametime;
	sii->cur_turn_rate = final_turn_rate;


	//float delta = psub->turn_rate * flFrametime;

	switch( sm->movement_axis )	{
	case MOVEMENT_AXIS_X:	
		sii->angs.p += delta;
		if (sii->angs.p > PI2 )
			sii->angs.p -= PI2;
		else if (sii->angs.p < 0.0f )
			sii->angs.p += PI2;
		break;
	case MOVEMENT_AXIS_Y:	
		sii->angs.h += delta;
		if (sii->angs.h > PI2 )
			sii->angs.h -= PI2;
		else if (sii->angs.h < 0.0f )
			sii->angs.h += PI2;
		break;
	case MOVEMENT_AXIS_Z:	
		sii->angs.b += delta;
		if (sii->angs.b > PI2 )
			sii->angs.b -= PI2;
		else if (sii->angs.b < 0.0f )
			sii->angs.b += PI2;
		break;
	}
}
/*
void submodel_ai_rotate(model_subsystem *psub, submodel_instance_info *sii)
{
	bsp_info * sm;

	if ( psub->subobj_num < 0 ) return;
	if(psub->ai_rotation.type = 0) return;

	polymodel *pm = model_get(psub->model_num);
	sm = &pm->submodel[psub->subobj_num];

	if ( sm->movement_type != MOVEMENT_TYPE_ROT ) return;

	
	// save last angles
	sii->prev_angs = sii->angs;

	// probably send in a calculated desired turn rate
	float diff = sii->desired_turn_rate - sii->cur_turn_rate;

	float final_turn_rate;
	if (diff > 0) {
		final_turn_rate = sii->cur_turn_rate + sii->turn_accel * flFrametime;
		if (final_turn_rate > sii->desired_turn_rate) {
			final_turn_rate = sii->desired_turn_rate;
		}
	} else if (diff < 0) {
		final_turn_rate = sii->cur_turn_rate - sii->turn_accel * flFrametime;
		if (final_turn_rate < sii->desired_turn_rate) {
			final_turn_rate = sii->desired_turn_rate;
		}
	} else {
		final_turn_rate = sii->desired_turn_rate;
	}

	float delta = (sii->cur_turn_rate + final_turn_rate) * 0.5f * flFrametime;
	sii->cur_turn_rate = final_turn_rate;

	
	//float delta = psub->turn_rate * flFrametime;

	switch( sm->movement_axis )	{
	case MOVEMENT_AXIS_X:
		if (sii->angs.p + delta > psub->ai_rotation.max ){//if it will or has gone past it's max then set it to the max/min
			sii->angs.p = psub->ai_rotation.max;
			return;
		} else if(sii->angs.p + delta < psub->ai_rotation.min){
			sii->angs.p = psub->ai_rotation.min;
			return;
		}
		sii->angs.p += delta;
		if (sii->angs.p > PI2 )
			sii->angs.p -= PI2;
		else if (sii->angs.p < 0.0f )
			sii->angs.p += PI2;
		break;
	case MOVEMENT_AXIS_Y:	
		sii->angs.h += delta;
		if (sii->angs.h > PI2 )
			sii->angs.h -= PI2;
		else if (sii->angs.h < 0.0f )
			sii->angs.h += PI2;
		break;
	case MOVEMENT_AXIS_Z:	
		sii->angs.b += delta;
		if (sii->angs.b > PI2 )
			sii->angs.b -= PI2;
		else if (sii->angs.b < 0.0f )
			sii->angs.b += PI2;
		break;
	}
}
*/


//=========================================================================
// Make a turret's correct orientation matrix.   This should be done when 
// the model is read, but I wasn't sure at what point all the data that I
// needed was read, so I just check a flag and call this routine when
// I determine I need the correct matrix.   In this code, you can't use
// vm_vec_2_matrix or anything, since these turrets could be either 
// right handed or left handed.
void model_make_turret_matrix(int model_num, model_subsystem * turret )
{
	polymodel * pm;
	vec3d fvec, uvec, rvec;

	pm = model_get(model_num);
	bsp_info * gun = &pm->submodel[turret->turret_gun_sobj];
	bsp_info * base = &pm->submodel[turret->subobj_num];
	float offset_base_h = 0.0f;
	float offset_barrel_h = 0.0f;
#ifdef WMC_SIDE_TURRETS
	offset_base_h = -PI_2;
	offset_barrel_h = -PI_2;
#endif

	if (base->force_turret_normal == true)
		turret->turret_norm = base->orientation.vec.uvec;

	model_clear_instance(model_num);
	base->angs.h = offset_base_h;
	gun->angs.h = offset_barrel_h;
	model_find_world_dir(&fvec, &turret->turret_norm, model_num, turret->turret_gun_sobj, &vmd_identity_matrix, NULL );

	base->angs.h = -PI_2 + offset_base_h;
	gun->angs.p = -PI_2;
	gun->angs.h = offset_barrel_h;
	model_find_world_dir(&rvec, &turret->turret_norm, model_num, turret->turret_gun_sobj, &vmd_identity_matrix, NULL );

	base->angs.h = 0.0f + offset_base_h;
	gun->angs.p = -PI_2;
	gun->angs.h = offset_barrel_h;
	model_find_world_dir(&uvec, &turret->turret_norm, model_num, turret->turret_gun_sobj, &vmd_identity_matrix, NULL );
									
	vm_vec_normalize(&fvec);
	vm_vec_normalize(&rvec);
	vm_vec_normalize(&uvec);

	turret->turret_matrix.vec.fvec = fvec;
	turret->turret_matrix.vec.rvec = rvec;
	turret->turret_matrix.vec.uvec = uvec;

//	vm_vector_2_matrix(&turret->turret_matrix,&turret->turret_norm,NULL,NULL);

	// HACK!! WARNING!!!
	// I'm doing nothing to verify that this matrix is orthogonal!!
	// In other words, there's no guarantee that the vectors are 90 degrees
	// from each other.
	// I'm not doing this because I don't know how to do it without ruining
	// the handedness of the matrix... however, I'm not too worried about	
	// this because I am creating these 3 vectors by making them 90 degrees
	// apart, so this should be close enough.  I think this will start 
	// causing weird errors when we view from turrets. -John
	turret->flags |= MSS_FLAG_TURRET_MATRIX;
}

// Tries to move joints so that the turret points to the point dst.
// turret1 is the angles of the turret, turret2 is the angles of the gun from turret
//	Returns 1 if rotated gun, 0 if no gun to rotate (rotation handled by AI)
int model_rotate_gun(int model_num, model_subsystem *turret, matrix *orient, angles *base_angles, angles *gun_angles, vec3d *pos, vec3d *dst, int obj_idx, bool reset)
{
	polymodel * pm;
	object *objp = &Objects[obj_idx];
	ship *shipp = &Ships[objp->instance];
	ship_subsys *ss = ship_get_subsys(shipp, turret->subobj_name);

	pm = model_get(model_num);
	bsp_info * gun = &pm->submodel[turret->turret_gun_sobj];
	bsp_info * base = &pm->submodel[turret->subobj_num];

	// Check for a valid turret
	Assert( turret->turret_num_firing_points > 0 );

	//This should not happen
	if ( base == gun ) {
		return 0;
	}

	// Build the correct turret matrix if there isn't already one
	if ( !(turret->flags & MSS_FLAG_TURRET_MATRIX) )
		model_make_turret_matrix(model_num, turret );

	Assert( turret->flags & MSS_FLAG_TURRET_MATRIX);
//	Assert( gun->movement_axis == MOVEMENT_AXIS_X );				// Gun must be able to change pitch
//	Assert( base->movement_axis == MOVEMENT_AXIS_Z );	// Parent must be able to change heading

	//------------	
	// rotate the dest point into the turret gun normal's frame of
	// reference, but not using the turret's angles.
	// Call this vector of_dst
	vec3d of_dst;							
	matrix world_to_turret_matrix;		// converts world coordinates to turret's FOR
	vec3d world_to_turret_translate;	// converts world coordinates to turret's FOR
	vec3d tempv;

	vm_vec_unrotate( &tempv, &base->offset, orient);
	vm_vec_add( &world_to_turret_translate, pos, &tempv );

	if (turret->flags & MSS_FLAG_TURRET_ALT_MATH)
		world_to_turret_matrix = ss->world_to_turret_matrix;
	else
		vm_matrix_x_matrix( &world_to_turret_matrix, orient, &turret->turret_matrix );

	vm_vec_sub( &tempv, dst, &world_to_turret_translate );
	vm_vec_rotate( &of_dst, &tempv, &world_to_turret_matrix );

	vm_vec_normalize(&of_dst);

	//------------	
	// Find the heading and pitch that the gun needs to turn to
	// by extracting them from the of_dst vector.
	// Call this the desired_angles
	angles desired_angles;
//	vm_extract_angles_vector(&desired_angles, &of_dst);

	if (reset == false) {
		desired_angles.p = (float)acos(of_dst.xyz.z);
		desired_angles.h = PI - atan2_safe(of_dst.xyz.x, of_dst.xyz.y);
		desired_angles.b = 0.0f;
	} else {
		desired_angles.p = 0.0f;
		desired_angles.h = 0.0f;
		desired_angles.b = 0.0f;
		if (turret->n_triggers > 0) {
			int i;
			for (i = 0; i<turret->n_triggers; i++) {
				if (turret->triggers[i].type == TRIGGER_TYPE_INITIAL) {
					desired_angles.p = turret->triggers[i].angle.xyz.x;
					desired_angles.h = turret->triggers[i].angle.xyz.y;
					i = turret->n_triggers;
				}
			}
		}
	}

	//	mprintf(( "Z = %.1f, atan= %.1f\n", of_dst.xyz.z, desired_angles.p ));

	//------------	
	// Gradually turn the turret towards the desired angles
	float step_size = turret->turret_turning_rate * flFrametime;
	float base_delta, gun_delta;

	if (turret->flags & MSS_FLAG_TURRET_ALT_MATH) {
		vec3d turret_base_to_enemy = of_dst;
		if ( (turret_base_to_enemy.xyz.x) != 0 || (turret_base_to_enemy.xyz.y != 0) )  {
			turret_base_to_enemy.xyz.z = 0;
			vm_vec_normalize(&turret_base_to_enemy);
			// if these two do not point roughly to the same direction...
			// swing the gun to the forward position before continuing to chase the target
			if ((turret_base_to_enemy.xyz.x * sin(base_angles->h)) < 0)
				desired_angles.h = 0;
		}
	}

	if (reset == true)
		step_size /= 3.0f;
	else
		ss->rotation_timestamp = timestamp(turret->turret_reset_delay);

	// reset these two
	ss->base_rotation_rate_pct = 0.0f;
	ss->gun_rotation_rate_pct = 0.0f;

	base_delta = vm_interp_angle(&base_angles->h, desired_angles.h, step_size);
	gun_delta = vm_interp_angle(&gun_angles->p, desired_angles.p, step_size);

	if (turret->turret_base_rotation_snd != -1)	
	{
		if (step_size > 0)
		{
			base_delta = (float) (fabs(base_delta)) / step_size;
			if (base_delta > 1.0f)
				base_delta = 1.0f;
			ss->base_rotation_rate_pct = base_delta;
		}
	}

	if (turret->turret_gun_rotation_snd != -1)
	{
		if (step_size > 0)
		{
			gun_delta = (float) (fabs(gun_delta)) / step_size;
			if (gun_delta > 1.0f)
				gun_delta = 1.0f;
			ss->gun_rotation_rate_pct = gun_delta;
		}
	}

//	base_angles->h -= step_size*(key_down_timef(KEY_1)-key_down_timef(KEY_2) );
//	gun_angles->p += step_size*(key_down_timef(KEY_3)-key_down_timef(KEY_4) );

	if (turret->flags & MSS_FLAG_FIRE_ON_TARGET)
	{
		base_delta = vm_delta_from_interp_angle( base_angles->h, desired_angles.h );
		gun_delta = vm_delta_from_interp_angle( gun_angles->p, desired_angles.p );
		ss->points_to_target = sqrt( pow(base_delta,2) + pow(gun_delta,2));
	}

	return 1;

}


// Goober5000
// For a submodel, return its overall offset from the main model.
void model_find_submodel_offset(vec3d *outpnt, int model_num, int sub_model_num)
{
	int mn;
	polymodel *pm = model_get(model_num);

	vm_vec_zero(outpnt);
	mn = sub_model_num;

	//instance up the tree for this point
	while ( (mn >= 0) && (pm->submodel[mn].parent >= 0) ) {
		vm_vec_add2(outpnt, &pm->submodel[mn].offset);

		mn = pm->submodel[mn].parent;
	}
}

// Given a point (pnt) that is in sub_model_num's frame of
// reference, and given the object's orient and position, 
// return the point in 3-space in outpnt.
void model_find_world_point(vec3d * outpnt, vec3d *mpnt,int model_num,int sub_model_num, matrix * objorient, vec3d * objpos )
{
	vec3d pnt;
	vec3d tpnt;
	matrix m;
	int mn;
	polymodel *pm = model_get(model_num);

	pnt = *mpnt;
	mn = sub_model_num;

	//instance up the tree for this point
	while ( (mn >= 0) && (pm->submodel[mn].parent >= 0) ) {
		// By using this kind of computation, the rotational angles can always
		// be computed relative to the submodel itself, instead of relative
		// to the parent - KeldorKatarn
		matrix rotation_matrix = pm->submodel[mn].orientation;
		vm_rotate_matrix_by_angles(&rotation_matrix, &pm->submodel[mn].angs);

		matrix inv_orientation;
		vm_copy_transpose_matrix(&inv_orientation, &pm->submodel[mn].orientation);

		vm_matrix_x_matrix(&m, &rotation_matrix, &inv_orientation);

		vm_vec_unrotate(&tpnt, &pnt, &m);

		vm_vec_add(&pnt, &tpnt, &pm->submodel[mn].offset);

		mn = pm->submodel[mn].parent;
	}

	//now instance for the entire object
	vm_vec_unrotate(outpnt,&pnt,objorient);
	vm_vec_add2(outpnt,objpos);
}

// Given a point in the world RF, find the corresponding point in the model RF.
// This is special purpose code, specific for model collision.
// NOTE - this code ASSUMES submodel is 1 level down from hull (detail[0])
//
// out - point in model RF
// world_pt - point in world RF
// pm - polygon model
// submodel_num - submodel in whose RF we're trying to find the corresponding world point
// orient - orient matrix of ship
// pos - pos vector of ship
void world_find_model_point(vec3d *out, vec3d *world_pt, polymodel *pm, int submodel_num, matrix *orient, vec3d *pos)
{
	Assert( (pm->submodel[submodel_num].parent == pm->detail[0]) || (pm->submodel[submodel_num].parent == -1) );

	vec3d tempv1, tempv2;
	matrix m;

	// get into ship RF
	vm_vec_sub(&tempv1, world_pt, pos);
	vm_vec_rotate(&tempv2, &tempv1, orient);

	if (pm->submodel[submodel_num].parent == -1) {
		*out  = tempv2;
		return;
	}

	// put into submodel RF
	vm_vec_sub2(&tempv2, &pm->submodel[submodel_num].offset);

	// By using this kind of computation, the rotational angles can always
	// be computed relative to the submodel itself, instead of relative
	// to the parent - KeldorKatarn
	matrix rotation_matrix = pm->submodel[submodel_num].orientation;
	vm_rotate_matrix_by_angles(&rotation_matrix, &pm->submodel[submodel_num].angs);

	matrix inv_orientation;
	vm_copy_transpose_matrix(&inv_orientation, &pm->submodel[submodel_num].orientation);

	vm_matrix_x_matrix(&m, &rotation_matrix, &inv_orientation);

	vm_vec_rotate(out, &tempv2, &m);
}

// Verify rotating submodel has corresponding ship subsystem -- info in which to store rotation angle
int rotating_submodel_has_ship_subsys(int submodel, ship *shipp)
{
	model_subsystem	*psub;
	ship_subsys			*pss;

	int found = 0;

	// Go through all subsystems and look for submodel
	// the subsystems that need it.
	for ( pss = GET_FIRST(&shipp->subsys_list); pss != END_OF_LIST(&shipp->subsys_list); pss = GET_NEXT(pss) ) {
		psub = pss->system_info;
		if (psub->subobj_num == submodel) {
			found = 1;
			break;
		}
	}
	
	return found;
}

void model_get_rotating_submodel_list(int *submodel_list, int *num_rotating_submodels, object *objp)
{
	Assert(objp->type == OBJ_SHIP);

	// Check if not currently rotating - then treat as part of superstructure.
	int modelnum = Ship_info[Ships[objp->instance].ship_info_index].model_num;
	polymodel *pm = model_get(modelnum);
	bsp_info *child_submodel;

	*num_rotating_submodels = 0;
	child_submodel = &pm->submodel[pm->detail[0]];

	int i = child_submodel->first_child;
	while ( i >= 0 )	{
		child_submodel = &pm->submodel[i];

		// Don't check it or its children if it is destroyed or it is a replacement (non-moving)
		if ( !child_submodel->blown_off && (child_submodel->i_replace == -1) )	{

			// Only look for submodels that rotate
			if (child_submodel->movement_type == MOVEMENT_TYPE_ROT) {

				// find ship subsys and check submodel rotation is less than max allowed.
				ship *pship = &Ships[objp->instance];
				ship_subsys *subsys;

				for ( subsys = GET_FIRST(&pship->subsys_list); subsys !=END_OF_LIST(&pship->subsys_list); subsys = GET_NEXT(subsys) ) {
					Assert(subsys->system_info->model_num == modelnum);
					if (i == subsys->system_info->subobj_num) {
						// found the correct subsystem - now check delta rotation angle not too large
						float delta_angle = get_submodel_delta_angle(&subsys->submodel_info_1);
						if (delta_angle < MAX_SUBMODEL_COLLISION_ROT_ANGLE) {
							Assert(*num_rotating_submodels < MAX_ROTATING_SUBMODELS-1);
							submodel_list[(*num_rotating_submodels)++] = i;
						}
						break;
					}
				}
			}
		}
		i = child_submodel->next_sibling;
	}

	// error checking
//#define MODEL_CHECK
#ifdef MODEL_CHECK
	ship *pship = &Ships[objp->instance];
	for (int idx=0; idx<*num_rotating_submodels; idx++) {
		int valid = rotating_submodel_has_ship_subsys(submodel_list[idx], pship);
//		Assert( valid );
		if ( !valid ) {

			Warning( LOCATION, "Ship %s has rotating submodel [%s] without ship subsystem\n", pship->ship_name, pm->submodel[submodel_list[idx]].name );
			pm->submodel[submodel_list[idx]].movement_type &= ~MOVEMENT_TYPE_ROT;
			*num_rotating_submodels = 0;
		}
	}
#endif

}


// Given a direction (pnt) that is in sub_model_num's frame of
// reference, and given the object's orient and position, 
// return the point in 3-space in outpnt.
void model_find_world_dir(vec3d * out_dir, vec3d *in_dir,int model_num, int sub_model_num, matrix * objorient, vec3d * objpos )
{
	vec3d pnt;
	vec3d tpnt;
	matrix m;
	int mn;
	polymodel *pm = model_get(model_num);

	pnt = *in_dir;
	mn = sub_model_num;

	//instance up the tree for this point
	while ( (mn >= 0) && (pm->submodel[mn].parent >= 0) ) {
		// By using this kind of computation, the rotational angles can always
		// be computed relative to the submodel itself, instead of relative
		// to the parent - KeldorKatarn
		matrix rotation_matrix = pm->submodel[mn].orientation;
		vm_rotate_matrix_by_angles(&rotation_matrix, &pm->submodel[mn].angs);

		matrix inv_orientation;
		vm_copy_transpose_matrix(&inv_orientation, &pm->submodel[mn].orientation);

		vm_matrix_x_matrix(&m, &rotation_matrix, &inv_orientation);

		vm_vec_unrotate(&tpnt, &pnt, &m);
		pnt = tpnt;

		mn = pm->submodel[mn].parent;
	}

	//now instance for the entire object
	vm_vec_unrotate(out_dir,&pnt,objorient);
}


// Clears all the submodel instances stored in a model to their defaults.
void model_clear_instance(int model_num)
{
	polymodel * pm;
	int i;

	pm = model_get(model_num);

	pm->gun_submodel_rotation = 0.0f;
	// reset textures to original ones
	for (i=0; i<pm->n_textures; i++ )	{
		pm->maps[i].Reset();
	}
	
	for (i=0; i<pm->n_models; i++ )	{
		bsp_info *sm = &pm->submodel[i];
		
		if ( pm->submodel[i].is_damaged )	{
			sm->blown_off = 1;
		} else {
			sm->blown_off = 0;
		}
		sm->angs.p = 0.0f;
		sm->angs.b = 0.0f;
		sm->angs.h = 0.0f;

		// set pointer to other ship subsystem info [turn rate, accel, moment, axis, ...]
		sm->sii = NULL;

		sm->num_arcs = 0;		// Turn off any electric arcing effects
	}

	for (i=0; i<pm->num_lights; i++ )	{
		pm->lights[i].value = 0.0f;
	}

	interp_clear_instance();

//	if ( keyd_pressed[KEY_1] ) pm->lights[0].value = 1.0f/255.0f;
//	if ( keyd_pressed[KEY_2] ) pm->lights[1].value = 1.0f/255.0f;
//	if ( keyd_pressed[KEY_3] ) pm->lights[2].value = 1.0f/255.0f;
//	if ( keyd_pressed[KEY_4] ) pm->lights[3].value = 1.0f/255.0f;
//	if ( keyd_pressed[KEY_5] ) pm->lights[4].value = 1.0f/255.0f;
//	if ( keyd_pressed[KEY_6] ) pm->lights[5].value = 1.0f/255.0f;


}

// initialization during ship set
void model_clear_instance_info( submodel_instance_info * sii )
{
	sii->blown_off = 0;
	sii->angs.p = 0.0f;
	sii->angs.b = 0.0f;
	sii->angs.h = 0.0f;
	sii->prev_angs.p = 0.0f;
	sii->prev_angs.b = 0.0f;
	sii->prev_angs.h = 0.0f;

	sii->cur_turn_rate = 0.0f;
	sii->desired_turn_rate = 0.0f;
	sii->turn_accel = 0.0f;
}

// initialization during ship set
void model_set_instance_info(submodel_instance_info *sii, float turn_rate, float turn_accel)
{
	sii->blown_off = 0;
	sii->angs.p = 0.0f;
	sii->angs.b = 0.0f;
	sii->angs.h = 0.0f;
	sii->prev_angs.p = 0.0f;
	sii->prev_angs.b = 0.0f;
	sii->prev_angs.h = 0.0f;

	sii->cur_turn_rate = turn_rate * 0.0f;
	sii->desired_turn_rate = turn_rate;
	sii->turn_accel = turn_accel;
	sii->axis_set = 0;
	sii->step_zero_timestamp = timestamp();
}



// Sets the submodel instance data in a submodel (for all detail levels)
void model_set_instance(int model_num, int sub_model_num, submodel_instance_info * sii)
{
	int i;
	polymodel * pm;

	pm = model_get(model_num);

	Assert( sub_model_num >= 0 );
	Assert( sub_model_num < pm->n_models );

	if ( sub_model_num < 0 ) return;
	if ( sub_model_num >= pm->n_models ) return;
	bsp_info *sm = &pm->submodel[sub_model_num];

	// Set the "blown out" flags	
	sm->blown_off = sii->blown_off;

	if ( sm->blown_off )	{
		if ( sm->my_replacement > -1 )	{
			pm->submodel[sm->my_replacement].blown_off = 0;
			pm->submodel[sm->my_replacement].angs = sii->angs;
			pm->submodel[sm->my_replacement].sii = sii;
		}
	} else {
		if ( sm->my_replacement > -1 )	{
			pm->submodel[sm->my_replacement].blown_off = 1;
		}
	}

	// Set the angles
	sm->angs = sii->angs;
	sm->sii = sii;

	// For all the detail levels of this submodel, set them also.
	for (i=0; i<sm->num_details; i++ )	{
		model_set_instance(model_num, sm->details[i], sii );
	}
}



void model_do_childeren_dumb_rotation(polymodel * pm, int mn){
	while ( mn >= 0 )	{

		bsp_info * sm = &pm->submodel[mn];

		if ( sm->movement_type == MSS_FLAG_DUM_ROTATES ){
			float *ang;
			int axis = sm->movement_axis;
			switch(axis){
			case MOVEMENT_AXIS_X:
				ang = &sm->angs.p;
					break;
			case MOVEMENT_AXIS_Z:
				ang = &sm->angs.b;
					break;
			default:
			case MOVEMENT_AXIS_Y:
				ang = &sm->angs.h;
					break;
			}
			*ang = sm->dumb_turn_rate * float(timestamp())/1000.0f;
			*ang = ((*ang/(PI*2.0f))-float(int(*ang/(PI*2.0f))))*(PI*2.0f);
			//this keeps ang from getting bigger than 2PI
		}

		if(pm->submodel[mn].first_child >-1)model_do_childeren_dumb_rotation(pm, pm->submodel[mn].first_child);

		mn = pm->submodel[mn].next_sibling;
	}
}
void model_do_dumb_rotation(int pn){
	polymodel * pm;

	pm = model_get(pn);
	int mn = pm->detail[0];

	model_do_childeren_dumb_rotation(pm,mn);
}


// Finds a point on the rotation axis of a submodel, used in collision, generally find rotational velocity
void model_init_submodel_axis_pt(submodel_instance_info *sii, int model_num, int submodel_num)
{
	vec3d axis;
	vec3d *mpoint1, *mpoint2;
	vec3d p1, v1, p2, v2, int1;

	polymodel *pm = model_get(model_num);
	Assert(pm->submodel[submodel_num].movement_type == MOVEMENT_TYPE_ROT);
	Assert(sii);

	mpoint1 = NULL;
	mpoint2 = NULL;

	// find 2 fixed points in submodel RF
	// these will be rotated to about the axis an angle of 0 and PI and we'll find the intersection of the
	// two lines to find a point on the axis
	if (pm->submodel[submodel_num].movement_axis == MOVEMENT_AXIS_X) {
		axis = vmd_x_vector;
		mpoint1 = &vmd_y_vector;
		mpoint2 = &vmd_z_vector;
	} else if (pm->submodel[submodel_num].movement_axis == MOVEMENT_AXIS_Y) {
		mpoint1 = &vmd_x_vector;
		axis = vmd_z_vector;		// rotation about y is a change in heading (p,b,h), so we need z
		mpoint2 = &vmd_z_vector;
	} else if (pm->submodel[submodel_num].movement_axis == MOVEMENT_AXIS_Z) {
		mpoint1 = &vmd_x_vector;
		mpoint2 = &vmd_y_vector;
		axis = vmd_y_vector;		// rotation about z is a change in bank (p,b,h), so we need y
	} else {
		// must be one of these axes or submodel_rot_hit is incorrectly set
		Int3();
	}

	// copy submodel angs
	angles copy_angs = pm->submodel[submodel_num].angs;

	// find two points rotated into model RF when angs set to 0
	vm_vec_copy_scale((vec3d*)&pm->submodel[submodel_num].angs, &axis, 0.0f);
	model_find_world_point(&p1, mpoint1, model_num, submodel_num, &vmd_identity_matrix, &vmd_zero_vector);
	model_find_world_point(&p2, mpoint2, model_num, submodel_num, &vmd_identity_matrix, &vmd_zero_vector);

	// find two points rotated into model RF when angs set to PI
	vm_vec_copy_scale((vec3d*)&pm->submodel[submodel_num].angs, &axis, PI);
	model_find_world_point(&v1, mpoint1, model_num, submodel_num, &vmd_identity_matrix, &vmd_zero_vector);
	model_find_world_point(&v2, mpoint2, model_num, submodel_num, &vmd_identity_matrix, &vmd_zero_vector);

	// reset submodel angs
	pm->submodel[submodel_num].angs = copy_angs;

	// find direction vectors of the two lines
	vm_vec_sub2(&v1, &p1);
	vm_vec_sub2(&v2, &p2);

	// find the intersection of the two lines
	float s, t;
	fvi_two_lines_in_3space(&p1, &v1, &p2, &v2, &s, &t);

	// find the actual intersection points
	vm_vec_scale_add(&int1, &p1, &v1, s);

	// set flag to init
	sii->pt_on_axis = int1;
	sii->axis_set = 1;
}


// Adds an electrical arcing effect to a submodel
void model_add_arc(int model_num, int sub_model_num, vec3d *v1, vec3d *v2, int arc_type )
{
	polymodel * pm;

	pm = model_get(model_num);

	if ( sub_model_num == -1 )	{
		sub_model_num = pm->detail[0];
	}

	Assert( sub_model_num >= 0 );
	Assert( sub_model_num < pm->n_models );

	if ( sub_model_num < 0 ) return;
	if ( sub_model_num >= pm->n_models ) return;
	bsp_info *sm = &pm->submodel[sub_model_num];

	if ( sm->num_arcs < MAX_ARC_EFFECTS )	{
		sm->arc_type[sm->num_arcs] = (ubyte)arc_type;
		sm->arc_pts[sm->num_arcs][0] = *v1;
		sm->arc_pts[sm->num_arcs][1] = *v2;
		sm->num_arcs++;
	}
}

// function to return an index into the docking_bays array which matches the criteria passed
// to this function.  dock_type is one of the DOCK_TYPE_XXX defines in model.h
// Goober5000 - now finds more than one dockpoint of this type
int model_find_dock_index(int modelnum, int dock_type, int index_to_start_at)
{
	int i;
	polymodel *pm;

	// get model and make sure it has dockpoints
	pm = model_get(modelnum);
	if ( pm->n_docks <= 0 )
		return -1;

	// look for a dockpoint of this type
	for (i = index_to_start_at; i < pm->n_docks; i++ )
	{
		if ( dock_type & pm->docking_bays[i].type_flags )
			return i;
	}

	// if we get here, type wasn't found -- return -1 and hope for the best
	return -1;
}

// function to return an index into the docking_bays array which matches the string passed
// Fred uses strings to identify docking positions.  This function also accepts generic strings
// so that a desginer doesn't have to know exact names if building a mission from hand.
int model_find_dock_name_index( int modelnum, char *name )
{
	int i;
	polymodel *pm;

	// get model and make sure it has dockpoints
	pm = model_get(modelnum);
	if ( pm->n_docks <= 0 )
		return -1;

	// check the generic names and call previous function to find first dock point of
	// the specified type
	for(i = 0; i < Num_dock_type_names; i++)
	{
		if(!stricmp(name, Dock_type_names[i].name)) {
			return model_find_dock_index(modelnum, Dock_type_names[i].def);
		}
	}
	/*
	if ( !stricmp(name, "cargo") )
		return model_find_dock_index( modelnum, DOCK_TYPE_CARGO );
	else if (!stricmp( name, "rearm") )
		return model_find_dock_index( modelnum, DOCK_TYPE_REARM );
	else if (!stricmp( name, "generic") )
		return model_find_dock_index( modelnum, DOCK_TYPE_GENERIC );
	*/

	// look for a dockpoint with this name
	for (i = 0; i < pm->n_docks; i++ )
	{
		if ( !stricmp(pm->docking_bays[i].name, name) )
			return i;
	}

	// if the bay does not have a name in the model, the model loading code
	// will assign it a default name... check for that here
	if (!strnicmp(name, "<unnamed bay ", 13))
	{
		int index = (name[13] - 'A');
		if (index >= 0 && index < pm->n_docks)
			return index;
	}

	// if we get here, name wasn't found -- return -1 and hope for the best
	return -1;
}

// returns the actual name of a docking point on a model, needed by Fred.
char *model_get_dock_name(int modelnum, int index)
{
	polymodel *pm;

	pm = model_get(modelnum);
	Assert((index >= 0) && (index < pm->n_docks));
	return pm->docking_bays[index].name;
}

int model_get_num_dock_points(int modelnum)
{
	polymodel *pm;

	pm = model_get(modelnum);
	return pm->n_docks;
}

int model_get_dock_index_type(int modelnum, int index)
{
	polymodel *pm = model_get(modelnum);				

	return pm->docking_bays[index].type_flags;
}

// get all the different docking point types on a model
int model_get_dock_types(int modelnum)
{
	int i, type = 0;
	polymodel *pm;

	pm = model_get(modelnum);
	for (i=0; i<pm->n_docks; i++)
		type |= pm->docking_bays[i].type_flags;

	return type;
}

// Goober5000
// returns index in [0, MAX_SHIP_BAY_PATHS)
int model_find_bay_path(int modelnum, char *bay_path_name)
{
	int i;
	polymodel *pm = model_get(modelnum);

	if (pm->ship_bay == NULL)
		return -1;

	if (pm->ship_bay->num_paths <= 0)
		return -1;

	for (i = 0; i < pm->ship_bay->num_paths; i++)
	{
		if (!stricmp(pm->paths[pm->ship_bay->path_indexes[i]].name, bay_path_name))
			return i;
	}

	return -1;
}

#if BYTE_ORDER == BIG_ENDIAN
extern void model_allocate_interp_data(int, int, int);

// tigital -
void swap_bsp_defpoints(ubyte * p)
{
	int n, i;
	int nverts = INTEL_INT( w(p+8) );		//tigital
	int offset = INTEL_INT( w(p+16) );
	int n_norms = INTEL_INT( w(p+12) );

	w(p+8) = nverts;
	w(p+16) = offset;
	w(p+12) = n_norms;

	ubyte * normcount = p+20;
	vec3d *src = vp(p+offset);

	model_allocate_interp_data(nverts, n_norms, 0);

	for (n=0; n<nverts; n++ )	{
		src->xyz.x = INTEL_FLOAT( &src->xyz.x );		//tigital
		src->xyz.y = INTEL_FLOAT( &src->xyz.y );
		src->xyz.z = INTEL_FLOAT( &src->xyz.z );

		Interp_verts[n] = src;
		src++;	//tigital

		for (i=0; i<normcount[n]; i++){
			src->xyz.x = INTEL_FLOAT( &src->xyz.x );		//tigital
			src->xyz.y = INTEL_FLOAT( &src->xyz.y );
			src->xyz.z = INTEL_FLOAT( &src->xyz.z );
			src++;
		}
	}
}

void swap_bsp_tmappoly( polymodel * pm, ubyte * p )
{
	int i, nv;
	model_tmap_vert *verts;
	vec3d * normal = vp(p+8);	//tigital
	vec3d * center = vp(p+20);
	float radius = INTEL_FLOAT( &fl(p+32) );

	fl(p+32) = radius;

	normal->xyz.x = INTEL_FLOAT( &normal->xyz.x );
	normal->xyz.y = INTEL_FLOAT( &normal->xyz.y );
	normal->xyz.z = INTEL_FLOAT( &normal->xyz.z );
	center->xyz.x = INTEL_FLOAT( &center->xyz.x );
	center->xyz.y = INTEL_FLOAT( &center->xyz.y );
	center->xyz.z = INTEL_FLOAT( &center->xyz.z );

	nv = INTEL_INT( w(p+36));		//tigital
		w(p+36) = nv;

	int tmap_num = INTEL_INT( w(p+40) );	//tigital
		w(p+40) = tmap_num;

	if ( nv < 0 ) return;

	verts = (model_tmap_vert *)(p+44);
	for (i=0;i<nv;i++){
		verts[i].vertnum = INTEL_SHORT( verts[i].vertnum );
		verts[i].normnum = INTEL_SHORT( verts[i].normnum );
		verts[i].u = INTEL_FLOAT( &verts[i].u );
		verts[i].v = INTEL_FLOAT( &verts[i].v );
	}

	if ( pm->version < 2003 )	{
		// Set the "normal_point" part of field to be the center of the polygon
		vec3d center_point;
		vm_vec_zero( &center_point );

		for (i=0;i<nv;i++)	{
			vm_vec_add2( &center_point, Interp_verts[verts[i].vertnum] );
		}

		center_point.xyz.x /= nv;
		center_point.xyz.y /= nv;
		center_point.xyz.z /= nv;

		*vp(p+20) = center_point;

		float rad = 0.0f;

		for (i=0;i<nv;i++)	{
			float dist = vm_vec_dist( &center_point, Interp_verts[verts[i].vertnum] );
			if ( dist > rad )	{
				rad = dist;
			}
		}
		fl(p+32) = rad;
	}
}

void swap_bsp_flatpoly( polymodel * pm, ubyte * p )
{
	int i, nv;
	short *verts;
	vec3d * normal = vp(p+8);	//tigital
	vec3d * center = vp(p+20);

	float radius = INTEL_FLOAT( &fl(p+32) );

	fl(p+32) = radius; 

	normal->xyz.x = INTEL_FLOAT( &normal->xyz.x );
	normal->xyz.y = INTEL_FLOAT( &normal->xyz.y );
	normal->xyz.z = INTEL_FLOAT( &normal->xyz.z );
	center->xyz.x = INTEL_FLOAT( &center->xyz.x );
	center->xyz.y = INTEL_FLOAT( &center->xyz.y );
	center->xyz.z = INTEL_FLOAT( &center->xyz.z );

	nv = INTEL_INT( w(p+36));		//tigital
		w(p+36) = nv;
        
	if ( nv < 0 ) return;

	verts = (short *)(p+44);
	for (i=0; i<nv*2; i++){
		verts[i] = INTEL_SHORT( verts[i] );
	}

	if ( pm->version < 2003 )	{
		// Set the "normal_point" part of field to be the center of the polygon
		vec3d center_point;
		vm_vec_zero( &center_point );

		for (i=0;i<nv;i++)	{
			vm_vec_add2( &center_point, Interp_verts[verts[i*2]] );
		}

		center_point.xyz.x /= nv;
		center_point.xyz.y /= nv;
		center_point.xyz.z /= nv;

		*vp(p+20) = center_point;

		float rad = 0.0f;

		for (i=0;i<nv;i++)	{
			float dist = vm_vec_dist( &center_point, Interp_verts[verts[i*2]] );
			if ( dist > rad )	{
				rad = dist;
			}
		}
		fl(p+32) = rad;
	}
}

void swap_bsp_sortnorms( polymodel * pm, ubyte * p )
{
	int frontlist = INTEL_INT( w(p+36) );	//tigital
	int backlist = INTEL_INT( w(p+40) );
	int prelist = INTEL_INT( w(p+44) );
	int postlist = INTEL_INT( w(p+48) );
	int onlist = INTEL_INT( w(p+52) );

	w(p+36) = frontlist;
	w(p+40) = backlist;
	w(p+44) = prelist;
	w(p+48) = postlist;
	w(p+52) = onlist;

	vec3d * normal = vp(p+8);	//tigital
	vec3d * center = vp(p+20);
	int  tmp = INTEL_INT( w(p+32) );
	
	w(p+32) = tmp;

	normal->xyz.x = INTEL_FLOAT( &normal->xyz.x );
	normal->xyz.y = INTEL_FLOAT( &normal->xyz.y );
	normal->xyz.z = INTEL_FLOAT( &normal->xyz.z );
	center->xyz.x = INTEL_FLOAT( &center->xyz.x );
	center->xyz.y = INTEL_FLOAT( &center->xyz.y );
	center->xyz.z = INTEL_FLOAT( &center->xyz.z );

	vec3d * bmin = vp(p+56);	//tigital
	vec3d * bmax = vp(p+68);

	bmin->xyz.x = INTEL_FLOAT( &bmin->xyz.x );
	bmin->xyz.y = INTEL_FLOAT( &bmin->xyz.y );
	bmin->xyz.z = INTEL_FLOAT( &bmin->xyz.z );
	bmax->xyz.x = INTEL_FLOAT( &bmax->xyz.x );
	bmax->xyz.y = INTEL_FLOAT( &bmax->xyz.y );
	bmax->xyz.z = INTEL_FLOAT( &bmax->xyz.z );

	if (prelist) swap_bsp_data(pm,p+prelist);
	if (backlist) swap_bsp_data(pm,p+backlist);
	if (onlist) swap_bsp_data(pm,p+onlist);
	if (frontlist) swap_bsp_data(pm,p+frontlist);
	if (postlist) swap_bsp_data(pm,p+postlist);
}
#endif // BIG_ENDIAN

void swap_bsp_data( polymodel * pm, void *model_ptr )
{
#if BYTE_ORDER == BIG_ENDIAN
	ubyte *p = (ubyte *)model_ptr;
	int chunk_type, chunk_size;
	vec3d * min;
	vec3d * max;

	chunk_type = INTEL_INT( w(p) );	//tigital
	chunk_size = INTEL_INT( w(p+4) );
	w(p) = chunk_type;
	w(p+4) = chunk_size;

	while (chunk_type != OP_EOF) {
		switch (chunk_type) {
			case OP_EOF:
				return;
			case OP_DEFPOINTS:
				swap_bsp_defpoints(p); 
				break;
			case OP_FLATPOLY:
				swap_bsp_flatpoly(pm, p);
				break;
			case OP_TMAPPOLY:
				swap_bsp_tmappoly(pm, p);
				break;
			case OP_SORTNORM:	
				swap_bsp_sortnorms(pm, p);
				break;
			case OP_BOUNDBOX:
				min = vp(p+8);
				max = vp(p+20);
				min->xyz.x = INTEL_FLOAT( &min->xyz.x );
				min->xyz.y = INTEL_FLOAT( &min->xyz.y );
				min->xyz.z = INTEL_FLOAT( &min->xyz.z );
				max->xyz.x = INTEL_FLOAT( &max->xyz.x );
				max->xyz.y = INTEL_FLOAT( &max->xyz.y );
				max->xyz.z = INTEL_FLOAT( &max->xyz.z );
				break;
			default:
				mprintf(( "Bad chunk type %d, len=%d in modelread:swap_bsp_data\n", chunk_type, chunk_size ));
				Int3();		// Bad chunk type!
			return;
		}

		p += chunk_size;
		chunk_type = INTEL_INT( w(p));	//tigital
		chunk_size = INTEL_INT( w(p+4) );
		w(p) = chunk_type;
		w(p+4) = chunk_size;
	}

	return;
#endif
}

void swap_sldc_data(ubyte *buffer)
{
#if BYTE_ORDER == BIG_ENDIAN
	char *type_p = (char *)(buffer);
	int *size_p = (int *)(buffer+1);
	*size_p = INTEL_INT(*size_p);

	// split and polygons
	vec3d *minbox_p = (vec3d*)(buffer+5);
	vec3d *maxbox_p = (vec3d*)(buffer+17);

	minbox_p->xyz.x = INTEL_FLOAT(&minbox_p->xyz.x);
	minbox_p->xyz.y = INTEL_FLOAT(&minbox_p->xyz.y);
	minbox_p->xyz.z = INTEL_FLOAT(&minbox_p->xyz.z);

	maxbox_p->xyz.x = INTEL_FLOAT(&maxbox_p->xyz.x);
	maxbox_p->xyz.y = INTEL_FLOAT(&maxbox_p->xyz.y);
	maxbox_p->xyz.z = INTEL_FLOAT(&maxbox_p->xyz.z);


	// split
	unsigned int *front_offset_p = (unsigned int*)(buffer+29);
	unsigned int *back_offset_p = (unsigned int*)(buffer+33);

	// polygons
	unsigned int *num_polygons_p = (unsigned int*)(buffer+29);

	unsigned int *shld_polys = (unsigned int*)(buffer+33);

	if (*type_p == 0) // SPLIT
	{
			*front_offset_p = INTEL_INT(*front_offset_p);
			*back_offset_p = INTEL_INT(*back_offset_p);
	}
	else
	{
		*num_polygons_p = INTEL_INT(*num_polygons_p);
		for (unsigned int i = 0; i < *num_polygons_p; i++)
		{
			shld_polys[i] = INTEL_INT(shld_polys[i]);
		}			
	}
#endif
}

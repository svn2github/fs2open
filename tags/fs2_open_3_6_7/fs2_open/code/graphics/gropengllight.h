/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/


/*
 * $Logfile: /Freespace2/code/Graphics/GrOpenGLLight.h $
 * $Revision: 1.7 $
 * $Date: 2005-09-05 09:36:41 $
 * $Author: taylor $
 *
 * header file containing definitions for HT&L lighting in OpenGL
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2005/07/13 03:15:51  Goober5000
 * remove PreProcDefine #includes in FS2
 * --Goober5000
 *
 * Revision 1.5  2005/04/12 02:04:56  phreak
 * gr_set_ambient_light() function for the ambient light sliders in FRED
 *
 * Revision 1.4  2005/02/04 10:12:29  taylor
 * merge with Linux/OSX tree - p0204
 *
 * Revision 1.3  2005/01/03 18:45:22  taylor
 * dynamic allocation of num supported OpenGL lights
 * add config option for more realistic light settings
 * don't render spec maps in nebula to address lighting issue
 *
 * Revision 1.2  2004/08/11 05:06:24  Kazan
 * added preprocdefines.h to prevent what happened with fred -- make sure to make all fred2 headers include this file as the _first_ include -- i have already modified fs2 files to do this
 *
 * Revision 1.1  2004/05/24 07:25:32  taylor
 * filename case change
 *
 * Revision 2.4  2004/04/26 13:02:27  taylor
 * light setup changes, support cmdline ambient value
 *
 * Revision 2.3  2004/04/13 01:55:41  phreak
 * put in the correct fields for the CVS comments to register
 * fixed a glowmap problem that occured when rendering glowmapped and non-glowmapped ships
 *
 *
 * $NoKeywords: $
 */

#ifndef _GROPENGLLIGHT_H
#define _GROPENGLLIGHT_H

#include "graphics/gropengl.h"


#define MAX_LIGHTS 256

enum
{
	LT_DIRECTIONAL,		// A light like a sun
	LT_POINT,			// A point light, like an explosion
	LT_TUBE,			// A tube light, like a fluorescent light
};

//Variables
extern struct opengl_light opengl_lights[MAX_LIGHTS];
extern bool active_light_list[MAX_LIGHTS];
extern int *currently_enabled_lights;
extern bool lighting_is_enabled;
extern GLint GL_max_lights;
extern int active_gl_lights;
extern int n_active_gl_lights;
extern int GL_center_alpha;

struct ogl_light_struct_col
{
		float r,g,b,a;
};

struct ogl_light_struct_pos 
{
		float x,y,z,w;
};

// Structures
struct opengl_light{
	opengl_light():occupied(false), priority(1){};
	ogl_light_struct_col Diffuse, Specular, Ambient;
	ogl_light_struct_pos Position;
	float ConstantAtten, LinearAtten, QuadraticAtten;
	bool occupied;
	int priority;
};

struct light_data;


//Functions
void FSLight2GLLight(opengl_light *GLLight, light_data *FSLight);
int	gr_opengl_make_light(light_data* light, int idx, int priority);		//unused -- stub function
void gr_opengl_modify_light(light_data* light, int idx, int priority);	//unused -- stub function
void gr_opengl_destroy_light(int idx);									//unused -- stub function
void gr_opengl_set_light(light_data *light);
void gr_opengl_reset_lighting();
void gr_opengl_set_lighting(bool set, bool state);
void opengl_pre_render_init_lights();
void opengl_change_active_lights(int);
void opengl_init_light();
void gr_opengl_center_alpha(int type);
void gr_opengl_set_center_alpha(int type);
void gr_opengl_set_ambient_light(int red, int green, int blue);

#endif //_GROPENGLLIGHT_H

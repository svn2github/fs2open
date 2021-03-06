/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/


/*
 * $Logfile: /Freespace2/code/Graphics/GrOpenGLTNL.cpp $
 * $Revision: 1.10 $
 * $Date: 2004-10-31 21:45:13 $
 * $Author: taylor $
 *
 * source for doing the fun TNL stuff
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.9  2004/07/29 09:35:29  taylor
 * fix NULL pointer and try to prevent in future, remove excess commands in opengl_cleanup()
 *
 * Revision 1.8  2004/07/26 20:47:32  Kazan
 * remove MCD complete
 *
 * Revision 1.7  2004/07/17 18:49:57  taylor
 * oops, I can't spell
 *
 * Revision 1.6  2004/07/17 18:40:40  taylor
 * Bob say we fix OGL, so me fix OGL <grunt>
 *
 * Revision 1.5  2004/07/12 16:32:48  Kazan
 * MCD - define _MCD_CHECK to use memory tracking
 *
 * Revision 1.4  2004/07/11 03:22:49  bobboau
 * added the working decal code
 *
 * Revision 1.3  2004/07/05 05:09:19  bobboau
 * FVF code, only the data that is needed is sent off to the card,,
 * OGL can take advantage of this if they want but it won't break
 * anything if they don't. also state block code has been implemented,
 * that's totaly internal to D3D no high level code is involved.
 *
 * Revision 1.2  2004/06/28 02:13:07  bobboau
 * high level index buffer suport and d3d implementation,
 * OGL people need to get this working on your end as it's broke now
 *
 * Revision 1.1  2004/05/24 07:25:32  taylor
 * filename case change
 *
 * Revision 2.4  2004/04/26 13:05:19  taylor
 * respect -glow and -spec
 *
 * Revision 2.3  2004/04/13 01:55:41  phreak
 * put in the correct fields for the CVS comments to register
 * fixed a glowmap problem that occured when rendering glowmapped and non-glowmapped ships
 *
 *
 * $NoKeywords: $
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include "globalincs/pstypes.h"

#include "model/model.h"

#include "graphics/2d.h"
#include "graphics/grinternal.h"
#include "graphics/gropengl.h"
#include "graphics/gropenglextension.h"
#include "graphics/gropengltexture.h"
#include "graphics/gropengllight.h"
#include "graphics/gropengltnl.h"

#include "math/vecmat.h"
#include "render/3d.h"

#include "debugconsole/timerbar.h"



extern int VBO_ENABLED;
extern int GLOWMAP;
extern int CLOAKMAP;
extern int SPECMAP;
extern vector G3_user_clip_normal;
extern vector G3_user_clip_point;
extern int Interp_multitex_cloakmap;
extern int Interp_cloakmap_alpha;

int GL_modelview_matrix_depth = 1;
int GL_htl_projection_matrix_set = 0;
int GL_htl_view_matrix_set = 0;

struct opengl_vertex_buffer
{
	int used;
	int stride;			// the current stride
	GLenum format;		// the format passed to glInterleavedArrays()
	int n_prim;
	int n_verts;
	float *array_list;	// interleaved array
	uint vbo;			// buffer for VBO
	uint flags;			// FVF
};

#define MAX_SUBOBJECTS 64

#ifdef INF_BUILD
#define MAX_BUFFERS_PER_SUBMODEL 24
#else
#define MAX_BUFFERS_PER_SUBMODEL 16
#endif

#define MAX_BUFFERS MAX_POLYGON_MODELS*MAX_SUBOBJECTS*MAX_BUFFERS_PER_SUBMODEL

static opengl_vertex_buffer vertex_buffers[MAX_BUFFERS];
static opengl_vertex_buffer *g_vbp;

//zeros everything out
void opengl_init_vertex_buffers()
{
	memset(vertex_buffers,0,sizeof(opengl_vertex_buffer)*MAX_BUFFERS);
}

int opengl_find_first_free_buffer()
{
	for (int i=0; i < MAX_BUFFERS; i++)
	{
		if (!vertex_buffers[i].used)
			return i;
	}
	
	return -1;
}

int opengl_check_for_errors()
{
	int error = 0;

	if ((error=glGetError()) != GL_NO_ERROR) {
		mprintf(("!!ERROR!!: %s\n", gluErrorString(error)));
		return 1;
	}

	return 0;
}

int opengl_mod_depth()
{
	int mv;
	glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &mv);
	return mv;
}

void gr_opengl_set_buffer(int idx)
{
	if (Cmdline_nohtl)
		return;

	g_vbp = &vertex_buffers[idx];
}

uint opengl_create_vbo(uint size, GLfloat *data)
{
	if (!data)
		return 0;

	if (!*data)
		return 0;

	if (size == 0)
		return 0;


	// Kazan: A) This makes that if (buffer_name) work correctly (false = 0, true = anything not 0)
	//				if glGenBuffersARB() doesn't initialized it for some reason
	//        B) It shuts up MSVC about may be used without been initalized
	uint buffer_name=0;

#ifndef GL_NO_HTL

	glGenBuffersARB(1, &buffer_name);
	
	//make sure we have one
	if (buffer_name)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer_name);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, size, data, GL_STATIC_DRAW_ARB );
				
		// just in case
		if (opengl_check_for_errors()) {
			return 0;
		}
	}

#endif // GL_NO_HTL

	return buffer_name;
}

int gr_opengl_make_buffer(poly_list *list, uint flags)
{
	if (Cmdline_nohtl)
		return -1;

	int buffer_num = -1;

#ifndef GL_NO_HTL

	buffer_num = opengl_find_first_free_buffer();

	//we have a valid buffer
	if (buffer_num > -1) {
		int arsize = 0;
		int list_size, i;

		gr_opengl_set_buffer( buffer_num );

		opengl_vertex_buffer *vbp = g_vbp;

		// defaults
		vbp->format = 0;
		vbp->vbo = 0;

		// setup using flags
		if ( (flags & VERTEX_FLAG_UV1) && (flags & VERTEX_FLAG_NORMAL) && (flags & VERTEX_FLAG_POSITION) ) {
			vbp->stride = (8 * sizeof(float));
			vbp->format = GL_T2F_N3F_V3F;
		} else if ( (flags & VERTEX_FLAG_UV1) && (flags & VERTEX_FLAG_POSITION) ) {
			vbp->stride = (5 * sizeof(float));
			vbp->format = GL_T2F_V3F;
		} else if ( (flags & VERTEX_FLAG_POSITION) ) {
			vbp->stride = (3 * sizeof(float));
			vbp->format = GL_V3F;
		} else {
			Assert( 0 );
		}

		// total size of data
		list_size = vbp->stride * list->n_verts;

		// allocate the storage list
		vbp->array_list = (float*)malloc(list_size);

		// return invalid if we don't have the memory
		if (vbp->array_list == NULL)
			return -1;
		
		memset(vbp->array_list, 0, list_size);

		// generate the array
		for (i=0; i<list->n_verts; i++) {
			vertex *vl = &list->vert[i];
			vector *nl = &list->norm[i];

			// don't try to generate more data than what's available
			Assert( int((arsize * sizeof(float)) + vbp->stride) <= list_size );

			// NOTE: TEX->NORM->VERT, This array order *must* be preserved!!

			// tex coords
			if (flags & VERTEX_FLAG_UV1) {
				vbp->array_list[arsize++] = vl->u;
				vbp->array_list[arsize++] = vl->v;
			}

			// normals
			if (flags & VERTEX_FLAG_NORMAL) {
				vbp->array_list[arsize++] = nl->xyz.x;
				vbp->array_list[arsize++] = nl->xyz.y;
				vbp->array_list[arsize++] = nl->xyz.z;
			}

			// verts
			if (flags & VERTEX_FLAG_POSITION) {
				vbp->array_list[arsize++] = vl->x;
				vbp->array_list[arsize++] = vl->y;
				vbp->array_list[arsize++] = vl->z;
			}
		}

		vbp->used = 1;
		vbp->flags = flags;

		vbp->n_prim = list->n_verts;
		vbp->n_verts = list->n_verts;

		// maybe load it into a vertex buffer object
		if (VBO_ENABLED) {
			vbp->vbo = opengl_create_vbo( list_size, vbp->array_list );

			if (vbp->vbo) {
				free(vbp->array_list);
				vbp->array_list = NULL;
			}
		}
	}
	
#endif // GL_NO_HTL

	return buffer_num;
}
	
void gr_opengl_destroy_buffer(int idx)
{
#ifndef GL_NO_HTL

	if (Cmdline_nohtl)
		return;

	if (idx > -1)
		gr_opengl_set_buffer(idx);

	opengl_vertex_buffer *vbp = g_vbp;

	if (vbp->array_list)
		free(vbp->array_list);

	if (vbp->vbo)
		glDeleteBuffersARB(1, &vbp->vbo);

	memset(vbp, 0, sizeof(opengl_vertex_buffer));

#endif // GL_NO_HTL
}


//#define DRAW_DEBUG_LINES
extern float Model_Interp_scale_x,Model_Interp_scale_y,Model_Interp_scale_z;

//start is the first part of the buffer to render, n_prim is the number of primitives, index_list is an index buffer, if index_list == NULL render non-indexed
void gr_opengl_render_buffer(int start, int n_prim, short* index_list)
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	TIMERBAR_PUSH(2);

	float u_scale, v_scale;
	int r, g, b, a, tmap_type;

	opengl_vertex_buffer *vbp = g_vbp;

	if (glIsEnabled(GL_CULL_FACE))
		glFrontFace(GL_CW);
	
	glColor3ub(255,255,255);

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	if (vbp->vbo) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbp->vbo);
		glInterleavedArrays(vbp->format, 0, (void*)NULL);
	} else {
		glInterleavedArrays(vbp->format, 0, vbp->array_list);
	}

	if ( (Interp_multitex_cloakmap > 0) && (vbp->flags & VERTEX_FLAG_UV1) ) {
		SPECMAP = -1;	// don't add a spec map if we are cloaked
		GLOWMAP = -1;	// don't use a glowmap either, shouldn't see them

		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (vbp->vbo) {
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbp->vbo);
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, (void*)0 );
		} else {
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, vbp->array_list );
		}
	}

	if ( (GLOWMAP > -1) && !Cmdline_noglow && (vbp->flags & VERTEX_FLAG_UV1) ) {
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (vbp->vbo) {
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbp->vbo);
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, (void*)0 );
		} else {
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, vbp->array_list );
		}
	}

	if ( (SPECMAP > -1) && !Cmdline_nospec && (vbp->flags & VERTEX_FLAG_UV1) && (GL_supported_texture_units > 2) ) {
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (vbp->vbo) {
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbp->vbo);
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, (void*)NULL );
		} else {
			glTexCoordPointer( 2, GL_FLOAT, vbp->stride, vbp->array_list );
		}
	}

	opengl_setup_render_states(r,g,b,a,tmap_type,TMAP_FLAG_TEXTURED,0);

	if (gr_screen.current_bitmap==CLOAKMAP)
	{
		glBlendFunc(GL_ONE,GL_ONE);
		r=g=b=Interp_cloakmap_alpha;
		a=255;
	}

	gr_tcache_set(gr_screen.current_bitmap, tmap_type, &u_scale, &v_scale, 0, gr_screen.current_bitmap_sx, gr_screen.current_bitmap_sy, 0);

	opengl_pre_render_init_lights();
	opengl_change_active_lights(0);

	glLockArraysEXT( 0, vbp->n_verts);

	if (index_list != NULL) {
		glDrawRangeElements(GL_TRIANGLES, start, (n_prim * 3), (n_prim * 3), GL_UNSIGNED_SHORT, (ushort*)index_list);
	} else {
		glDrawArrays(GL_TRIANGLES, 0, vbp->n_verts);
	}

	glUnlockArraysEXT();

	if((lighting_is_enabled)&&((n_active_gl_lights-1)/max_gl_lights > 0)) {
		gr_opengl_set_state( TEXTURE_SOURCE_DECAL, ALPHA_BLEND_ALPHA_ADDITIVE, ZBUFFER_TYPE_READ );
		opengl_switch_arb(1,0);
		opengl_switch_arb(2,0);
		for(int i=1; i< (n_active_gl_lights-1)/max_gl_lights; i++)
		{
			opengl_change_active_lights(i);

			if (index_list != NULL) {
				glDrawRangeElements(GL_TRIANGLES, start, (n_prim * 3), (n_prim * 3), GL_UNSIGNED_SHORT, (ushort*)index_list);
			} else {
				glDrawArrays(GL_TRIANGLES, 0, vbp->n_verts);
			}
		}
	}

	TIMERBAR_POP();

	if (VBO_ENABLED) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	}

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

#if defined(DRAW_DEBUG_LINES) && defined(_DEBUG)
	glBegin(GL_LINES);
		glColor3ub(255,0,0);
		glVertex3d(0,0,0);
		glVertex3d(20,0,0);

		glColor3ub(0,255,0);
		glVertex3d(0,0,0);
		glVertex3d(0,20,0);

		glColor3ub(0,0,255);
		glVertex3d(0,0,0);
		glVertex3d(0,0,20);
	glEnd();
#endif

	
}

void gr_opengl_start_instance_matrix(vector *offset, matrix* rotation)
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	if (!offset)
		offset = &vmd_zero_vector;
	if (!rotation)
		rotation = &vmd_identity_matrix;	

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	vector axis;
	float ang;
	vm_matrix_to_rot_axis_and_angle(rotation,&ang,&axis);
	glTranslatef(offset->xyz.x,offset->xyz.y,offset->xyz.z);
	glRotatef(fl_degrees(ang),axis.xyz.x,axis.xyz.y,axis.xyz.z);
	GL_modelview_matrix_depth++;
}

void gr_opengl_start_instance_angles(vector *pos, angles* rotation)
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	matrix m;
	vm_angles_2_matrix(&m,rotation);
	gr_opengl_start_instance_matrix(pos,&m);
}

void gr_opengl_end_instance_matrix()
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	GL_modelview_matrix_depth--;
}

//the projection matrix; fov, aspect ratio, near, far
void gr_opengl_set_projection_matrix(float fov, float aspect, float z_near, float z_far)
{
	if (Cmdline_nohtl)
		return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(fl_degrees(fov),aspect,z_near,z_far);
	glMatrixMode(GL_MODELVIEW);
	GL_htl_projection_matrix_set = 1;
}

void gr_opengl_end_projection_matrix()
{
	if (Cmdline_nohtl)
		return;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	GL_htl_projection_matrix_set = 0;
}

void gr_opengl_set_view_matrix(vector *pos, matrix* orient)
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_modelview_matrix_depth == 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix();
	
	vector fwd;
	vector *uvec=&orient->vec.uvec;

	vm_vec_add(&fwd, pos, &orient->vec.fvec);

	gluLookAt(pos->xyz.x,pos->xyz.y,-pos->xyz.z,
	fwd.xyz.x,fwd.xyz.y,-fwd.xyz.z,
	uvec->xyz.x, uvec->xyz.y,-uvec->xyz.z);

	glScalef(1,1,-1);
	glViewport(gr_screen.offset_x,gr_screen.max_h-gr_screen.offset_y-gr_screen.clip_height,gr_screen.clip_width,gr_screen.clip_height);

	GL_modelview_matrix_depth = 2;
	GL_htl_view_matrix_set = 1;	
}

void gr_opengl_end_view_matrix()
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_modelview_matrix_depth == 2);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glLoadIdentity();
	glViewport(0,0,gr_screen.max_w, gr_screen.max_h);

	GL_modelview_matrix_depth = 1;
	GL_htl_view_matrix_set = 0;
}

void gr_opengl_push_scale_matrix(vector *scale_factor)
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	GL_modelview_matrix_depth++;
	glScalef(scale_factor->xyz.x,scale_factor->xyz.y,scale_factor->xyz.z);
}

void gr_opengl_pop_scale_matrix()
{
	if (Cmdline_nohtl)
		return;

	Assert(GL_htl_projection_matrix_set);
	Assert(GL_htl_view_matrix_set);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	GL_modelview_matrix_depth--;
}

void gr_opengl_end_clip_plane()
{
	if (Cmdline_nohtl)
		return;

	glDisable(GL_CLIP_PLANE0);
}

void gr_opengl_start_clip_plane()
{
	if (Cmdline_nohtl)
		return;

	double clip_equation[4];
	vector n;
	vector p;

	n=G3_user_clip_normal;	
	p=G3_user_clip_point;

	clip_equation[0]=n.xyz.x;
	clip_equation[1]=n.xyz.y;
	clip_equation[2]=n.xyz.z;
	clip_equation[3]=-vm_vec_dot(&p, &n);
	glClipPlane(GL_CLIP_PLANE0, clip_equation);
	glEnable(GL_CLIP_PLANE0);
}

//face -1 is to turn it off, the rest are right, left, up, down , front, and back
void gr_opengl_render_to_env(int FACE)
{
	if (Cmdline_nohtl)
		return;
}

/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/




#ifndef _GROPENGLTNL_H
#define _GROPENGLTNL_H


#include "globalincs/pstypes.h"


extern GLint GL_max_elements_vertices;
extern GLint GL_max_elements_indices;

struct poly_list;

void gr_opengl_start_instance_matrix(vec3d *offset, matrix *rotation);
void gr_opengl_start_instance_angles(vec3d *pos, angles *rotation);
void gr_opengl_end_instance_matrix();
void gr_opengl_set_projection_matrix(float fov, float aspect, float z_near, float z_far);
void gr_opengl_end_projection_matrix();
void gr_opengl_set_view_matrix(vec3d *pos, matrix *orient);
void gr_opengl_end_view_matrix();
void gr_opengl_set_2d_matrix(/*int x, int y, int w, int h*/);
void gr_opengl_end_2d_matrix();
void gr_opengl_push_scale_matrix(vec3d *scale_factor);
void gr_opengl_pop_scale_matrix();

void gr_opengl_start_clip_plane();
void gr_opengl_end_clip_plane();

int gr_opengl_make_buffer(poly_list *list, uint flags);
void gr_opengl_destroy_buffer(int idx);
void gr_opengl_set_buffer(int idx);
void gr_opengl_render_buffer(int start, int n_prim, ushort *sbuffer, uint *ibuffer, int flags);
void gr_opengl_render_to_env(int FACE);

void gr_opengl_start_state_block();
int gr_opengl_end_state_block();
void gr_opengl_set_state_block(int);

void opengl_tnl_shutdown();

#endif //_GROPENGLTNL_H

/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Graphics/gropenglbmpman.cpp $
 * $Revision: 1.11 $
 * $Date: 2005-08-20 20:34:51 $
 * $Author: taylor $
 *
 * OpenGL specific bmpman routines
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2005/03/24 23:42:21  taylor
 * s/gr_ogl_/gr_opengl_/g
 * add empty gr_opengl_draw_line_list() so that it's not a NULL pointer
 * make gr_opengl_draw_htl_sphere() just use GLU so we don't need yet another friggin API
 *
 * Revision 1.9  2005/03/19 20:35:45  wmcoolmon
 * small bool fix
 *
 * Revision 1.8  2005/03/11 14:11:53  taylor
 * apparently this change never got in
 *
 * Revision 1.7  2005/03/07 13:10:21  bobboau
 * commit of render target code, d3d should be totaly functional,
 * OGL still needs implementation.
 *
 * Revision 1.6  2005/02/04 20:06:04  taylor
 * merge with Linux/OSX tree - p0204-2
 *
 * Revision 1.5  2005/02/04 10:12:29  taylor
 * merge with Linux/OSX tree - p0204
 *
 * Revision 1.4  2005/01/21 08:54:53  taylor
 * slightly better memory management
 *
 * Revision 1.3  2004/12/05 01:28:39  taylor
 * support uncompressed DDS images
 * use TexSubImage2D for video anis
 *
 * Revision 1.2  2004/11/23 00:10:06  taylor
 * try and protect the bitmap_entry stuff a bit better
 * fix the transparent support ship, again, but correctly this time
 *
 * Revision 1.1  2004/10/31 21:21:11  taylor
 * initial import from bmpman merge
 *
 *
 * $NoKeywords: $
 */

#include "globalincs/pstypes.h"
#include "bmpman/bmpman.h"
#include "ddsutils/ddsutils.h"
#include "tgautils/tgautils.h"
#include "jpgutils/jpgutils.h"
#include "pcxutils/pcxutils.h"
#include "graphics/gropengltexture.h"
#include "globalincs/systemvars.h"

#define BMPMAN_INTERNAL
#include "bmpman/bm_internal.h"

extern int Cmdline_jpgtga;


// anything API specific to freeing bm data
void gr_opengl_bm_free_data(int n)
{
	Assert( (n >= 0) && (n < MAX_BITMAPS) );

	// might as well free up the on card texture data too in order to get rid
	// of old interface stuff but don't free USER types since we can reuse
	// ANI slots for faster and less resource intensive rendering - taylor
	if (bm_bitmaps[n].type != BM_TYPE_USER)
		opengl_free_texture_slot( n );
}

// API specifics for creating a user bitmap
void gr_opengl_bm_create(int n)
{
	Assert( (n >= 0) && (n < MAX_BITMAPS) );
}

// Load an image and validate it while retrieving information for later use
// Input:	type		= current BM_TYPE_*
//			n			= location in bm_bitmaps[]
//			filename	= name of the current file
//			img_cfp		= already open CFILE handle, if available
//
// Output:	w			= bmp width
//			h			= bmp height
//			bpp			= bmp bits per pixel
//			c_type		= output for an updated BM_TYPE_*
//			mm_lvl		= number of mipmap levels for the image
//			size		= size of the data contained in the image
int gr_opengl_bm_load(ubyte type, int n, char *filename, CFILE *img_cfp, int *w, int *h, int *bpp, ubyte *c_type, int *mm_lvl, int *size)
{
	int dds_ct;

	if (type == BM_TYPE_DDS) {
		int dds_error = dds_read_header( filename, img_cfp, w, h, bpp, &dds_ct, mm_lvl, size );
		if (dds_error != DDS_ERROR_NONE) {
			mprintf(("dds: Couldn't open '%s' -- error description %s\n", filename, dds_error_string(dds_error)));
			return -1;
		}

		switch (dds_ct) {
			case DDS_DXT1:
				*c_type = BM_TYPE_DXT1;
				break;

			case DDS_DXT3:
				*c_type = BM_TYPE_DXT3;
				break;

			case DDS_DXT5:
				*c_type = BM_TYPE_DXT5;
				break;

			case DDS_UNCOMPRESSED:
				*c_type = BM_TYPE_DDS;
				break;

			default:
				Error(LOCATION, "bad DDS file compression.  Not using DXT1,3,5 %s", filename);
				return -1;
		}
	}
	// if its a tga file
	else if (type == BM_TYPE_TGA) {
		int tga_error = targa_read_header( filename, img_cfp, w, h, bpp, NULL );
		if ( tga_error != TARGA_ERROR_NONE )	{
			mprintf(( "tga: Couldn't open '%s'\n", filename ));
			return -1;
		}
	}
	// if its a jpg file
	else if (type == BM_TYPE_JPG) {
		int jpg_error=jpeg_read_header( filename, img_cfp, w, h, bpp, NULL );
		if ( jpg_error != JPEG_ERROR_NONE ) {
			mprintf(( "jpg: Couldn't open '%s'\n", filename ));
			return -1;
		}
	}
	// if its a pcx file
	else if (type == BM_TYPE_PCX) {
		int pcx_error = pcx_read_header( filename, img_cfp, w, h, bpp, NULL );
		if ( pcx_error != PCX_ERROR_NONE )	{
			mprintf(( "pcx: Couldn't open '%s'\n", filename ));
			return -1;
		}
	} else {
		Assert( 0 );

		return -1;
	}

	return 0;
}

// API specific init instructions
void gr_opengl_bm_init(int n)
{
	Assert( (n >= 0) && (n < MAX_BITMAPS) );
}

// specific instructions for setting up the start of a page-in session
void gr_opengl_bm_page_in_start()
{
	gr_opengl_preload_init();
}

// Lock an image files data into memory
int gr_opengl_bm_lock( char *filename, int handle, int bitmapnum, ubyte bpp, ubyte flags )
{
	ubyte c_type = BM_TYPE_NONE;
	ubyte true_bpp;

	bitmap_entry *be = &bm_bitmaps[bitmapnum];
	bitmap *bmp = &be->bm;

	if (Is_standalone) {
		true_bpp = 8;
	}
	// not really sure how well this is going to work out in every case but...
	else if ( Cmdline_jpgtga && (bmp->true_bpp > bpp) ) {
		true_bpp = bmp->true_bpp;
	} else {
		true_bpp = bpp;
	}

	// don't do a bpp check here since it could be different in OGL - taylor
	if ( (bmp->data == 0) ) {
		Assert(be->ref_count == 1);

		if ( be->type != BM_TYPE_USER ) {
			if ( bmp->data == 0 ) {
				nprintf (("BmpMan","Loading %s for the first time.\n", be->filename));
			}
		}

		if ( !Bm_paging )	{
			if ( be->type != BM_TYPE_USER ) {
				char flag_text[64];
				strcpy( flag_text, "--" );							
				nprintf(( "Paging", "Loading %s (%dx%dx%dx%s)\n", be->filename, bmp->w, bmp->h, true_bpp, flag_text ));
			}
		}

		// select proper format
		if(flags & BMP_AABITMAP){
			BM_SELECT_ALPHA_TEX_FORMAT();
		} else if(flags & BMP_TEX_ANY){
			BM_SELECT_TEX_FORMAT();					
		} else {
			BM_SELECT_SCREEN_FORMAT();
		}

		// make sure we use the real graphic type for EFFs
		if ( be->type == BM_TYPE_EFF ) {
			c_type = be->info.eff.type;
		} else {
			c_type = be->type;
		}

		switch ( c_type ) {
			case BM_TYPE_PCX:
				bm_lock_pcx( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			case BM_TYPE_ANI:
				bm_lock_ani( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			case BM_TYPE_USER:	
				bm_lock_user( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			case BM_TYPE_TGA:
				bm_lock_tga( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			case BM_TYPE_JPG:
				bm_lock_jpg( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			case BM_TYPE_DDS:
			case BM_TYPE_DXT1:
			case BM_TYPE_DXT3:
			case BM_TYPE_DXT5:
				bm_lock_dds( handle, bitmapnum, be, bmp, true_bpp, flags );
				break;

			default:
				Warning(LOCATION, "Unsupported type in bm_lock -- %d\n", c_type );
				return -1;
		}		

		// always go back to screen format
		BM_SELECT_SCREEN_FORMAT();
	}

	// make sure we actually did something
	if ( !(bmp->data) ) {
		// crap, bail...
		return -1;
	}

	return 0;
}

//gr_ogl_make_render_target: function makes a texture sutable for rendering to as close 
//to the desiered resolution as posable in the specified texture slot, if the desiered 
//resolution and the final resolution are diferent the function should change the input 
//values (hence passing by reference). if for some reason a texture cannot be made in 
//the specified slot it returns false, if everything goes ok, it returns true
//
//the three flags I have done so far that you will have to take care of
//#define BMP_TEX_STATIC_RENDER_TARGET		(1<<7)				// a texture made for being rendered to infreqently
//#define BMP_TEX_DYNAMIC_RENDER_TARGET		(1<<8)				// a texture made for being rendered to freqently
//#define BMP_TEX_CUBEMAP						(1<<8)				// a texture made for cubic environment map
//*****static render targets must be able to survive anything, includeing application minimiseation*****//
bool gr_opengl_bm_make_render_target(int n, int &x, int &y, int flags)
{
	return false;//the error code, this function doesn't do anything yet so return acordingly, this should return the texture index of the created render target
}

//sets rendering to the specified texture handle (note this is diferent that texture index)
//returns true if it's able to do so, false if it is unable to do so
//only textures created by gr_ogl_make_render_target may be used.
bool gr_opengl_bm_set_render_target(int handle, int face)
{
	return false;	//becase this is a stub, the render target doesn't change, this should be true on suceses once this function is implemented
}

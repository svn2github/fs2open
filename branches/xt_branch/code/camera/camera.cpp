#include "camera/camera.h"
#include "math/vecmat.h"
#include "graphics/2d.h"
#include "globalincs/alphacolors.h"
#include "parse/parselo.h"
#include "physics/physics.h" //apply_physics

//Some global vars
std::vector<subtitle> Subtitles;
std::vector<camera> Cameras;
//Preset cameras
camera* Free_camera = NULL;
camera* Current_camera = NULL;

//This is where the camera class beings! :D
camera::camera(char *in_name)
{
	set_name(in_name);

	//These are our two pointers, new'd and delete'd only by this class
	desired_position = NULL;
	desired_orientation = NULL;

	reset();
}

camera::~camera()
{
	//Cleanup
	if(desired_position != NULL)
		delete desired_position;

	if(desired_orientation != NULL)
		delete desired_orientation;

	//Check if this is in use
	if(Current_camera == this)
	{
		if(Cameras.size() > 0)
			Current_camera = &Cameras[0];
		else
			Current_camera = NULL;
	}
}

void camera::reset()
{
	position = vmd_zero_vector;
	
	if(desired_position != NULL)
	{
		delete desired_position;
		desired_position = NULL;
	}
	
	translation_velocity = translation_vel_limit = translation_acc_limit = vmd_zero_vector;

	vm_set_identity(&orientation);
	
	if(desired_orientation != NULL)
	{
		delete desired_orientation;
		desired_orientation = NULL;
	}
	rotation_rate = rotation_vel_limit = rotation_acc_limit = vmd_zero_vector;
}

void camera::set_name(char *in_name)
{
	if(name != NULL)
		strncpy(name, in_name, NAME_LENGTH-1);
}

#define square(a) (a*a)
void camera::set_position(vec3d *in_position, float in_translation_time, float in_translation_acceleration_time)
{
	//*****Instantly change to new position (or stop if no position given)
	if(in_translation_time <= 0.0f || in_position == NULL)
	{
		if(in_position != NULL)
			position = *in_position;

		if(desired_position != NULL)
		{
			delete desired_position;
			desired_position = NULL;
		}

		translation_velocity = vmd_zero_vector;
		translation_acc_limit = vmd_zero_vector;
		translation_vel_limit = vmd_zero_vector;

		return;
	}

	//*****Perform intermediate calculations

	//Create desired orientation if necessary
	if(desired_position == NULL)
		desired_position = new vec3d;

	//Set desired orientation
	*desired_position = *in_position;

	//Find the distance
	vec3d d;
	vm_vec_sub(&d, in_position, &position);

	//*****Instantly accelerate to velocity
	if(in_translation_acceleration_time <= 0.0f)
	{
		vm_vec_copy_scale(&translation_velocity, &d, 1.0f/in_translation_time);

		translation_vel_limit = rotation_rate;
		translation_acc_limit = vmd_zero_vector;

		return;
	}

	//*****Set variables for graduated movement
	//Safety
	if(in_translation_acceleration_time*2.0f > in_translation_time)
		in_translation_acceleration_time = in_translation_time/2.0f;

	//Calculate the maximum acceleration speed
	translation_acc_limit.xyz.x = fabsf(d.xyz.x / ((in_translation_time * in_translation_acceleration_time) - square(in_translation_acceleration_time)));
	translation_acc_limit.xyz.y = fabsf(d.xyz.y / ((in_translation_time * in_translation_acceleration_time) - square(in_translation_acceleration_time)));
	translation_acc_limit.xyz.z = fabsf(d.xyz.z / ((in_translation_time * in_translation_acceleration_time) - square(in_translation_acceleration_time)));

	//Calculate the maximum velocity
	translation_vel_limit.xyz.x = fabsf(translation_acc_limit.xyz.x * in_translation_acceleration_time);
	translation_vel_limit.xyz.y = fabsf(translation_acc_limit.xyz.y * in_translation_acceleration_time);
	translation_vel_limit.xyz.z = fabsf(translation_acc_limit.xyz.z * in_translation_acceleration_time);
}

void camera::set_translation_velocity(vec3d *in_velocity)
{
	if(in_velocity == NULL)
		translation_velocity = *in_velocity;
	else
		vm_vec_zero(&translation_velocity);
}

void camera::set_rotation(matrix *in_orientation, float in_rotation_time, float in_rotation_acceleration_time)
{
	//*****Instantly rotate to new position (or stop if no position given)
	if(in_rotation_time <= 0.0f || in_orientation == NULL)
	{
		if(in_orientation != NULL)
			orientation = *in_orientation;

		if(desired_orientation != NULL)
		{
			delete desired_orientation;
			desired_orientation = NULL;
		}

		rotation_rate = vmd_zero_vector;
		rotation_acc_limit = vmd_zero_vector;
		rotation_vel_limit = vmd_zero_vector;

		return;
	}

	//*****Perform intermediate calculations

	//Create desired orientation if necessary
	if(desired_orientation == NULL)
		desired_orientation = new matrix;

	//Set desired orientation
	*desired_orientation = *in_orientation;

	//Get the angles in radians
	angles curr_orientation, new_orientation;
	vm_extract_angles_matrix(&curr_orientation, &orientation);
	vm_extract_angles_matrix(&new_orientation, in_orientation);

	//Find the distance
	angles d;
	d.p = new_orientation.p - curr_orientation.p;
	d.b = new_orientation.b - curr_orientation.b;
	d.h = new_orientation.h - curr_orientation.h;

	//*****Instantly accelerate to velocity
	if(in_rotation_acceleration_time <= 0.0f)
	{
		rotation_rate.xyz.z = d.p / in_rotation_time;
		rotation_rate.xyz.x = d.b / in_rotation_time;
		rotation_rate.xyz.y = d.h / in_rotation_time;

		rotation_vel_limit = rotation_rate;
		rotation_acc_limit = vmd_zero_vector;

		return;
	}
	//*****Set variables for graduated movement
	//Safety
	if(in_rotation_acceleration_time*2.0f > in_rotation_time)
		in_rotation_acceleration_time = in_rotation_time/2.0f;
	//Calculate the maximum acceleration speed
	rotation_acc_limit.xyz.z = fabsf(d.p / ((in_rotation_time * in_rotation_acceleration_time) - square(in_rotation_acceleration_time)));
	rotation_acc_limit.xyz.x = fabsf(d.b / ((in_rotation_time * in_rotation_acceleration_time) - square(in_rotation_acceleration_time)));
	rotation_acc_limit.xyz.y = fabsf(d.h / ((in_rotation_time * in_rotation_acceleration_time) - square(in_rotation_acceleration_time)));

	//Calculate the maximum velocity
	rotation_vel_limit.xyz.x = fabsf(rotation_acc_limit.xyz.x * in_rotation_acceleration_time);
	rotation_vel_limit.xyz.y = fabsf(rotation_acc_limit.xyz.y * in_rotation_acceleration_time);
	rotation_vel_limit.xyz.z = fabsf(rotation_acc_limit.xyz.z * in_rotation_acceleration_time);
}

void camera::set_rotation(angles *in_angles, float in_rotation_time, float in_rotation_acceleration_time)
{
	matrix temp_matrix = IDENTITY_MATRIX;

	if(in_angles != NULL)
		vm_angles_2_matrix(&temp_matrix, in_angles);

	set_rotation(&temp_matrix, in_rotation_time, in_rotation_acceleration_time);
}

void camera::set_rotation_facing(vec3d *in_target, float in_rotation_time, float in_rotation_acceleration_time)
{
	matrix temp_matrix = IDENTITY_MATRIX;

	if(in_target != NULL)
	{
		vec3d targetvec;
		vm_vec_normalized_dir(&targetvec, in_target, &position);
		vm_vector_2_matrix(&temp_matrix, &targetvec, NULL, NULL);
	}

	set_rotation(&temp_matrix, in_rotation_time, in_rotation_acceleration_time);
}

void camera::set_rotation_velocity(vec3d *in_rotation_rate)
{
	if(in_rotation_rate != NULL)
		rotation_rate = *in_rotation_rate;
	else
		vm_vec_zero(&rotation_rate);

	/*if(in_rotation_rate == NULL)
	{
		rotation_rate.p = rotation_rate.b = rotation_rate.h = 0.0f;

		rotation_rate_delta_time = rotation_rate_delta_time_left = in_rotation_acceleration_time = 0.0f;
		rotation_rate_decel_time_til = -1.0f;
		return;
	}

	rotation_rate_delta_time = rotation_rate_delta_time_left = in_rotation_acceleration_time;
	rotation_rate_decel_time_til = -1.0f;

	rotation_rate_delta.p = in_rotation_rate->p / in_rotation_acceleration_time;
	rotation_rate_delta.b = in_rotation_rate->b / in_rotation_acceleration_time;
	rotation_rate_delta.h = in_rotation_rate->h / in_rotation_acceleration_time;*/
}

void camera::do_frame(float in_frametime)
{
	//float frametime, rot_frametime, trans_frametime;
	//DO ROTATION STUFF
	if(desired_orientation != NULL)
	{
		matrix ori_out;
		vec3d vel_out;

		vm_matrix_interpolate(desired_orientation, &orientation, &rotation_rate, in_frametime, &ori_out, &vel_out, &rotation_vel_limit, &rotation_acc_limit, 1);
		orientation = ori_out;
		rotation_rate = vel_out;

		//TODO: Make this "if(*desired_orienation == orientation)"
		//Note that this means we are finished rotating
		if(vel_out.xyz.x == 0.0f && vel_out.xyz.y == 0.0f && vel_out.xyz.z == 0.0f)
		{
			delete desired_orientation;
			desired_orientation = NULL;
			rotation_rate = rotation_vel_limit = rotation_acc_limit = vmd_zero_vector;
		}
	}
	else if(rotation_rate.xyz.x || rotation_rate.xyz.y || rotation_rate.xyz.z)
	{
		//TODO: Make this right.
		angles new_orientation;

		vm_extract_angles_matrix(&new_orientation, &orientation);

		new_orientation.p += rotation_rate.xyz.z;
		new_orientation.b += rotation_rate.xyz.x;
		new_orientation.h += rotation_rate.xyz.y;

		vm_angles_2_matrix(&orientation, &new_orientation);
	}
	
	if(desired_position != NULL)
	{
		vec3d d_p;

		apply_physics(translation_vel_limit.xyz.x/translation_acc_limit.xyz.x, translation_vel_limit.xyz.x, translation_velocity.xyz.x, in_frametime, &translation_velocity.xyz.x, &d_p.xyz.x);
		apply_physics(translation_vel_limit.xyz.y/translation_acc_limit.xyz.y, translation_vel_limit.xyz.y, translation_velocity.xyz.y, in_frametime, &translation_velocity.xyz.y, &d_p.xyz.y);
		apply_physics(translation_vel_limit.xyz.z/translation_acc_limit.xyz.z, translation_vel_limit.xyz.z, translation_velocity.xyz.z, in_frametime, &translation_velocity.xyz.z, &d_p.xyz.z);

		vm_vec_add2(&position, &d_p);
		//WMC - This is your past self talking. You were attempting to use vm_matrix_interpolate
		//to move the camera, but never got around to actually getting it completed + tested
		/*
		matrix ori_out;
		vec3d vel_out;
		matrix vec_ori;
		matrix temp_ori;

		//vm_vector_2_matrix(&vec_ori, &desired_position);
		vm_matrix_interpolate(&vec_ori, &temp_ori, &translation_velocity, in_frametime, &ori_out, &vel_out, &translation_vel_limit, &translation_acc_limit, 1);
		//vm_vec_rotate(&position, &vmd_identity_matrix, &ori_out);
		translation_velocity = vel_out;

		//TODO: Make this "if(*desired_orienation == orientation)"
		//Note that this means we are finished rotating
		if(vel_out.xyz.x == 0.0f && vel_out.xyz.y == 0.0f && vel_out.xyz.z == 0.0f)
		{
			delete desired_position;
			desired_position = NULL;
			translation_velocity = translation_vel_limit = translation_acc_limit = vmd_zero_vector;
		}*/
	}
	else
	{
		vm_vec_scale_add2(&position, &translation_velocity, in_frametime);
	}
}

#define MAX_SUBTITLE_LINES		64
subtitle::subtitle(int in_x_pos, int in_y_pos, char* in_text, float in_display_time, char* in_imageanim, float in_fade_time, color *in_text_color, bool center_x, bool center_y, int in_width)
{
	// basic init, this always has to be done
	memset( imageanim, 0, sizeof(imageanim) );
	image_id = -1;

	if ( ((in_text != NULL) && (strlen(in_text) <= 0)) && ((in_imageanim != NULL) && (strlen(in_imageanim) <= 0)) )
		return;


	int num_text_lines = 0;
	char *text_line_ptrs[MAX_SUBTITLE_LINES];
	int text_line_lens[MAX_SUBTITLE_LINES];

	//Setup text
	if ( (in_text != NULL) && (strlen(in_text) > 0) ) {
		num_text_lines = split_str(in_text, in_width, text_line_lens, text_line_ptrs, MAX_SUBTITLE_LINES);
		std::string temp_str;
		for(int i = 0; i < num_text_lines; i++)
		{
			temp_str.assign(text_line_ptrs[i], text_line_lens[i]);
			text_lines.push_back(temp_str);
		}
	}

	//Setup text color
	if(in_text_color)
		text_color = *in_text_color;
	else
		gr_init_alphacolor(&text_color, 255, 255, 255, 255);

	//Setup display and fade time
	display_time = fl_abs(in_display_time);
	fade_time = fl_abs(in_fade_time);

	//Setup image
	if ( (in_imageanim != NULL) && (strlen(in_imageanim) > 0) )
	{
		image_id = bm_load(in_imageanim);

		if (image_id >= 0)
			strncpy(imageanim, in_imageanim, sizeof(imageanim) - 1);
	}

	//Setup pos
	int w=0, h=0, tw=0, th=0;
	float deltax, deltay;
	image_pos[0] = 0;
	image_pos[1] = 0;
	if(center_x || center_y)
	{
		//Get text size
		for(int i = 0; i < num_text_lines; i++)
		{
			gr_get_string_size(&w, &h, text_line_ptrs[i], text_line_lens[i]);

			if(w > tw)
				tw = w;

			th += h;
		}

		//Get image size
		if(image_id != -1)
		{
			bm_get_info(image_id, &w, &h);
			tw += w;
			if(h > th)
				th = h;
		}

	//	gr_resize_screen_pos(&tw, &th);

		//Center it?
		if(center_x)
		{
			image_pos[0] = (gr_screen.max_w_unscaled - tw)/2;
			gr_resize_screen_pos(&image_pos[0], NULL);
		}
		if(center_y)
		{
			image_pos[1] = (gr_screen.max_h_unscaled - th)/2;
			gr_resize_screen_pos(NULL, &image_pos[1]);
		}
	}
	if(in_x_pos < 0 && !center_x)
		image_pos[0] += gr_screen.max_w + in_x_pos;
	else if(!center_x)
		image_pos[0] += in_x_pos;

	if(in_y_pos < 0 && !center_y)
		image_pos[1] += gr_screen.max_h + in_y_pos;
	else if(!center_y)
		image_pos[1] += in_y_pos;


	text_pos[0] = image_pos[0];

	if(image_id != -1)
		text_pos[0] += w; //Still set from bm_get_info call

//	deltax = text_pos[0] / i2fl(gr_screen.max_w_unscaled);	//MikeStar;
//	text_pos[0] = gr_screen.max_w * deltax;	//MikeStar;

	text_pos[1] = image_pos[1];
//	deltay = text_pos[1] / i2fl(gr_screen.max_h_unscaled);	//MikeStar;
//	text_pos[1] = gr_screen.max_h * deltay;	//MikeStar;

	time_displayed = 0.0f;
	time_displayed_end = 2.0f*fade_time + display_time;
}

void subtitle::do_frame(float frametime)
{
	//Figure out how much alpha
	if(time_displayed < fade_time)
	{
		text_color.alpha = (ubyte)fl2i(255.0f*(time_displayed/fade_time));
	}
	else if(time_displayed > time_displayed_end)
	{
		//We're finished
		return;
	}
	else if((time_displayed - fade_time) > display_time)
	{
		text_color.alpha = (ubyte)fl2i(255.0f*(1-(time_displayed - fade_time - display_time)/fade_time));
	}
	else
	{
		text_color.alpha = 255;
	}

	gr_set_color_fast(&text_color);

	int font_height = gr_get_font_height();
	int x = fl2i(text_pos[0]);
	int y = fl2i(text_pos[1]);

	for(unsigned int i = 0; i < text_lines.size(); i++)
	{
		gr_string(x, y, (char*)text_lines[i].c_str(), false);
		y += font_height;
	}

	if(image_id != -1)
	{
		gr_set_bitmap(image_id, GR_ALPHABLEND_FILTER, GR_BITBLT_MODE_NORMAL, text_color.alpha/255.0f);
		gr_bitmap(image_pos[0], image_pos[1], false);
	}

	time_displayed += frametime;
}

subtitle::~subtitle()
{
	if (image_id != -1) {
		if ( bm_release(image_id) ) {
			image_id = -1;
		}
	}
}

void subtitle::clone(const subtitle &sub)
{
	uint i = 0;

	for (i = 0; i < sub.text_lines.size(); i++) {
		text_lines.push_back(sub.text_lines[i]);
	}

	text_pos[0] = sub.text_pos[0];
	text_pos[1] = sub.text_pos[1];

	display_time = sub.display_time;
	fade_time = sub.fade_time;
	memcpy( &text_color, &sub.text_color, sizeof(color) );

	if ( strlen(sub.imageanim) ) {
		memcpy( imageanim, sub.imageanim, MAX_FILENAME_LEN );
		// we bm_load() again here to kick the refcount up
		image_id = bm_load(imageanim);
	} else {
		memset( imageanim, 0, MAX_FILENAME_LEN );
		image_id = -1;
	}

	image_pos[0] = sub.image_pos[0];
	image_pos[1] = sub.image_pos[1];

	time_displayed = sub.time_displayed;
	time_displayed_end = sub.time_displayed_end;
}

const subtitle &subtitle::operator=(const subtitle &sub)
{
	if (this != &sub) {
		if (image_id != -1) {
			if ( bm_release(image_id) ) {
				image_id = -1;
			}
		}

		clone(sub);
	}

	return *this;
}

void cameras_init()
{
	//Preset cameras
	Cameras.push_back(camera("Free"));
	Free_camera = &Cameras[Cameras.size()-1];
	Current_camera = Free_camera;
}

void cameras_close()
{
	Cameras.clear();

	//Preset cameras
	Free_camera = NULL;
	Current_camera = NULL;
}

void cameras_do_frame(float frametime)
{
	unsigned int i,size=Cameras.size();
	for(i = 0; i < size; i++)
	{
		Cameras[i].do_frame(frametime);
	}
}

//Looks up camera by name, returns -1 on failure
int cameras_lookup(char *name)
{
	unsigned int i,size=Cameras.size();
	for(i = 0; i < size; i++)
	{
		if(!stricmp(Cameras[i].get_name(), name))
			return i;
	}

	return -1;
}

void subtitles_init()
{
	//Do nothing!!
}

void subtitles_close()
{
	Subtitles.clear();
}

void subtitles_do_frame(float frametime)
{
	unsigned int i,size=Subtitles.size();
	for(i = 0; i < size; i++)
	{
		Subtitles[i].do_frame(frametime);
	}
}

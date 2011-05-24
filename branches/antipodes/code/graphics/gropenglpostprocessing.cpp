
#include "graphics/gropengl.h"
#include "graphics/gropenglextension.h"
#include "graphics/gropenglpostprocessing.h"
#include "graphics/gropenglshader.h"
#include "graphics/gropenglstate.h"

#include "io/timer.h"
#include "nebula/neb.h"
#include "parse/parselo.h"
#include "cmdline/cmdline.h"
#include "globalincs/def_files.h"


extern bool PostProcessing_override;
extern int opengl_check_framebuffer();

//Needed to track where the FXAA shaders are
size_t fxaa_shader_id;
//In case we don't find the shaders at all, this override is needed
bool fxaa_unavailable = false;


#define SDR_POST_FLAG_MAIN		(1<<0)
#define SDR_POST_FLAG_BRIGHT	(1<<1)
#define SDR_POST_FLAG_BLUR		(1<<2)
#define SDR_POST_FLAG_PASS1		(1<<3)
#define SDR_POST_FLAG_PASS2		(1<<4)

static SCP_vector<opengl_shader_t> GL_post_shader;

// NOTE: The order of this list *must* be preserved!  Additional shaders can be
//       added, but the first 5 are used with magic numbers so their position
//       is assumed to never change.
static opengl_shader_file_t GL_post_shader_files[] = {
	// NOTE: the main post-processing shader has any number of uniforms, but
	//       these few should always be present
	{ "post-v.sdr", "post-f.sdr", SDR_POST_FLAG_MAIN,
		4, { "tex", "timer", "bloomed", "bloom_intensity" } },

	{ "post-v.sdr", "blur-f.sdr", SDR_POST_FLAG_BLUR | SDR_POST_FLAG_PASS1,
		2, { "tex", "bsize" } },

	{ "post-v.sdr", "blur-f.sdr", SDR_POST_FLAG_BLUR | SDR_POST_FLAG_PASS2,
		2, { "tex", "bsize" } },

	{ "post-v.sdr", "brightpass-f.sdr", SDR_POST_FLAG_BRIGHT,
		1, { "tex" } },

	{ "fxaa-v.sdr", "fxaa-f.sdr", NULL, 
		4, { "tex0", "rt_w", "rt_h", "fxaa_preset"} }
};

static const unsigned int Num_post_shader_files = sizeof(GL_post_shader_files) / sizeof(opengl_shader_file_t);

typedef struct post_effect_t {
	SCP_string name;
	SCP_string uniform_name;
	SCP_string define_name;

	float intensity;
	float default_intensity;
	float div;
	float add;

	bool always_on;

	post_effect_t() :
		intensity(0.0f), default_intensity(0.0f), div(1.0f), add(0.0f),
		always_on(false)
	{
	}
} post_effect_t;

SCP_vector<post_effect_t> Post_effects;

static int Post_initialized = 0;

static bool Post_in_frame = false;

static int Post_active_shader_index = 0;

static GLuint Post_framebuffer_id[3] = { 0 };
static GLuint Post_renderbuffer_id = 0;
static GLuint Post_screen_texture_id = 0;
static GLuint Post_bloom_texture_id[3] = { 0 };

static int Post_texture_width = 0;
static int Post_texture_height = 0;


static char *opengl_post_load_shader(char *filename, int flags, int flags2);


static bool opengl_post_pass_bloom()
{
	if (Cmdline_bloom_intensity <= 0) {
		return false;
	}

	// we need the scissor test disabled
	GLboolean scissor_test = GL_state.ScissorTest(GL_FALSE);

	// ------  begin bright pass ------

	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[1]);

	// width and height are 1/2 for the bright pass
	int width = Post_texture_width >> 1;
	int height = Post_texture_height >> 1;

	glViewport(0, 0, width, height);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	opengl_shader_set_current( &GL_post_shader[3] );

	vglUniform1iARB( opengl_shader_get_uniform("tex"), 0 );

	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.SetTarget(GL_TEXTURE_2D);
	GL_state.Texture.Enable(Post_screen_texture_id);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, -1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 1.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
	glEnd();

	GL_state.Texture.Disable();

	// ------ end bright pass ------


	// ------ begin blur pass ------

	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.SetTarget(GL_TEXTURE_2D);

	// drop width and height once more for the blur passes
	width >>= 1;
	height >>= 1;

	glViewport(0, 0, width, height);

	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[2]);

	for (int pass = 0; pass < 2; pass++) {
		vglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Post_bloom_texture_id[1+pass], 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		opengl_shader_set_current( &GL_post_shader[1+pass] );

		vglUniform1iARB( opengl_shader_get_uniform("tex"), 0 );
		vglUniform1fARB( opengl_shader_get_uniform("bsize"), (pass) ? (float)width : (float)height );

		GL_state.Texture.Enable(Post_bloom_texture_id[pass]);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(-1.0f, -1.0f);

			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(1.0f, -1.0f);

			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(1.0f, 1.0f);

			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(-1.0f, 1.0f);
		glEnd();
	}

	GL_state.Texture.Disable();

	// ------ end blur pass --------

	// reset viewport, scissor test and exit
	glViewport(0, 0, gr_screen.max_w, gr_screen.max_h);
	GL_state.ScissorTest(scissor_test);

	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return true;
}

void gr_opengl_post_process_begin()
{
	if ( !Post_initialized ) {
		return;
	}

	if (Post_in_frame) {
		return;
	}

	if (PostProcessing_override) {
		return;
	}

	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[0]);
	vglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, Post_renderbuffer_id);

//	vglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, Post_renderbuffer_id);

//	vglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Post_screen_texture_id, 0);

//	Assert( !opengl_check_framebuffer() );

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Post_in_frame = true;
}

void opengl_post_pass_fxaa() {

	// set and configure post shader ..
	opengl_shader_set_current( &GL_post_shader[fxaa_shader_id] );

	// basic/default uniforms
	vglUniform1iARB( opengl_shader_get_uniform("tex0"), 0 );
	vglUniform1fARB( opengl_shader_get_uniform("rt_w"), static_cast<float>(Post_texture_width));
	vglUniform1fARB( opengl_shader_get_uniform("rt_h"), static_cast<float>(Post_texture_height));
	vglUniform1iARB( opengl_shader_get_uniform("fxaa_preset"), Cmdline_fxaa_preset);

	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.SetTarget(GL_TEXTURE_2D);
	GL_state.Texture.Enable(Post_screen_texture_id);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, -1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 1.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
	glEnd();

	GL_state.Texture.Disable();

	opengl_shader_set_current();
}

void gr_opengl_post_process_end()
{
	if ( !Post_in_frame ) {
		return;
	}

	// state switch just the once (for bloom pass and final render-to-screen)
	GLboolean depth = GL_state.DepthTest(GL_FALSE);
	GLboolean depth_mask = GL_state.DepthMask(GL_FALSE);
	GLboolean light = GL_state.Lighting(GL_FALSE);
	GLboolean blend = GL_state.Blend(GL_FALSE);
	GLboolean cull = GL_state.CullFace(GL_FALSE);

	GL_state.Texture.SetShaderMode(GL_TRUE);

	// Do FXAA
	if (Cmdline_fxaa && !fxaa_unavailable) {
		opengl_post_pass_fxaa();
	}

	// done with screen render framebuffer
	vglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// do bloom, hopefully ;)
	bool bloomed = opengl_post_pass_bloom();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// set and configure post shader ...

	opengl_shader_set_current( &GL_post_shader[Post_active_shader_index] );

	// basic/default uniforms
	vglUniform1iARB( opengl_shader_get_uniform("tex"), 0 );
	vglUniform1fARB( opengl_shader_get_uniform("timer"), static_cast<float>(timer_get_milliseconds() % 100 + 1) );

	for (size_t idx = 0; idx < Post_effects.size(); idx++) {
		if ( GL_post_shader[Post_active_shader_index].flags2 & (1<<idx) ) {
			const char *name = Post_effects[idx].uniform_name.c_str();
			float value = Post_effects[idx].intensity;

			vglUniform1fARB( opengl_shader_get_uniform(name), value);
		}
	}

	// bloom uniforms, but only if we did the bloom
	if (bloomed) {
		float intensity = MIN((float)Cmdline_bloom_intensity, 200.0f) * 0.01f;

		if (Neb2_render_mode != NEB2_RENDER_NONE) {
			// we need less intensity for full neb missions, so cut it by 30%
			intensity /= 3.0f;
		}

		vglUniform1fARB( opengl_shader_get_uniform("bloom_intensity"), intensity );

		vglUniform1iARB( opengl_shader_get_uniform("bloomed"), 1 );

		GL_state.Texture.SetActiveUnit(1);
		GL_state.Texture.SetTarget(GL_TEXTURE_2D);
		GL_state.Texture.Enable(Post_bloom_texture_id[2]);
	}

	// now render it to the screen ...

	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.SetTarget(GL_TEXTURE_2D);
	GL_state.Texture.Enable(Post_screen_texture_id);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, -1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 1.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
	glEnd();

	// Done!

	GL_state.Texture.SetActiveUnit(1);
	GL_state.Texture.Disable();
	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.Disable();

	GL_state.Texture.SetShaderMode(GL_FALSE);

	// reset state
	GL_state.DepthTest(depth);
	GL_state.DepthMask(depth_mask);
	GL_state.Lighting(light);
	GL_state.Blend(blend);
	GL_state.CullFace(cull);

	opengl_shader_set_current();

	Post_in_frame = false;
}

void get_post_process_effect_names(SCP_vector<SCP_string> &names) 
{
	int idx;

	for (idx = 0; idx < (int)Post_effects.size(); idx++) {
		names.push_back(Post_effects[idx].name);
	}
}

static bool opengl_post_compile_main_shader(int flags)
{
	char *vert = NULL, *frag = NULL;
	bool in_error = false;
	opengl_shader_t new_shader;
	opengl_shader_file_t *shader_file = &GL_post_shader_files[0];
	int num_main_uniforms = 0;
	int idx;

	for (idx = 0; idx < (int)Post_effects.size(); idx++) {
		if ( flags & (1 << idx) ) {
			num_main_uniforms++;
		}
	}

	// choose appropriate files
	char *vert_name = shader_file->vert;
	char *frag_name = shader_file->frag;

	mprintf(("POST-PROCESSING: Compiling new post-processing shader with flags %d ... \n", flags));

	// read vertex shader
	if ( (vert = opengl_post_load_shader(vert_name, shader_file->flags, flags)) == NULL ) {
		in_error = true;
		goto Done;
	}

	// read fragment shader
	if ( (frag = opengl_post_load_shader(frag_name, shader_file->flags, flags)) == NULL ) {
		in_error = true;
		goto Done;
	}

	Verify( vert != NULL );
	Verify( frag != NULL );

	new_shader.program_id = opengl_shader_create(vert, frag);

	if ( !new_shader.program_id ) {
		in_error = true;
		goto Done;
	}


	new_shader.flags = shader_file->flags;
	new_shader.flags2 = flags;

	opengl_shader_set_current( &new_shader );

	new_shader.uniforms.reserve(shader_file->num_uniforms + num_main_uniforms);

	for (idx = 0; idx < shader_file->num_uniforms; idx++) {
		opengl_shader_init_uniform( shader_file->uniforms[idx] );
	}

	for (idx = 0; idx < (int)Post_effects.size(); idx++) {
		if ( flags & (1 << idx) ) {
			opengl_shader_init_uniform( Post_effects[idx].uniform_name.c_str() );
		}
	}

	opengl_shader_set_current();

	// add it to our list of embedded shaders
	GL_post_shader.push_back( new_shader );

Done:
	if (vert != NULL) {
		vm_free(vert);
		vert = NULL;
	}

	if (frag != NULL) {
		vm_free(frag);
		frag = NULL;
	}

	return in_error;
}

void gr_opengl_post_process_set_effect(const char *name, int value)
{
	if ( !Post_initialized ) {
		return;
	}

	if (name == NULL) {
		return;
	}

	size_t idx;
	int sflags = 0;
	bool need_change = true;

	for (idx = 0; idx < Post_effects.size(); idx++) {
		const char *eff_name = Post_effects[idx].name.c_str();

		if ( !stricmp(eff_name, name) ) {
			Post_effects[idx].intensity = (value / Post_effects[idx].div) + Post_effects[idx].add;
			break;
		}
	}

	// figure out new flags
	for (idx = 0; idx < Post_effects.size(); idx++) {
		if ( Post_effects[idx].always_on || (Post_effects[idx].intensity != Post_effects[idx].default_intensity) ) {
			sflags |= (1<<idx);
		}
	}

	// see if any existing shader has those flags
	for (idx = 0; idx < GL_post_shader.size(); idx++) {
		if (GL_post_shader[idx].flags2 == sflags) {
			// no change required
			need_change = false;

			// set this as the active post shader
			Post_active_shader_index = (int)idx;

			break;
		}
	}

	// if not then add a new shader to the list
	if (need_change) {
		if ( !opengl_post_compile_main_shader(sflags) ) {
			// shader added, set it as active
			Post_active_shader_index = (int)(GL_post_shader.size() - 1);
		} else {
			// failed to load, just go with default
			Post_active_shader_index = 0;
		}
	}
}

void gr_opengl_post_process_set_defaults()
{
	size_t idx, list_size;

	if ( !Post_initialized ) {
		return;
	}

	// reset all effects to their default values
	list_size = Post_effects.size();

	for (idx = 0; idx < Post_effects.size(); idx++) {
		Post_effects[idx].intensity = Post_effects[idx].default_intensity;
	}

	// remove any post shaders created on-demand, leaving only the defaults
	list_size = GL_post_shader.size();

	for (idx = list_size-1; idx > 0; idx--) {
		if ( !(GL_post_shader[idx].flags & SDR_POST_FLAG_MAIN) ) {
			break;
		}

		if (GL_post_shader[idx].program_id) {
			vglDeleteObjectARB(GL_post_shader[idx].program_id);
		}

		GL_post_shader[idx].uniforms.clear();

		GL_post_shader.pop_back();
	}

	Post_active_shader_index = 0;
}

void gr_opengl_post_process_save_zbuffer()
{
	if ( !Post_initialized ) {
		return;
	}
}


static bool opengl_post_init_table()
{
	int rval;
	bool warned = false;

	if ( (rval = setjmp(parse_abort)) != 0 ) {
		mprintf(("Unable to parse 'post_processing.tbl'!  Error code = %d.\n", rval));
		return false;
	}

	if (cf_exists_full("post_processing.tbl", CF_TYPE_TABLES))
		read_file_text("post_processing.tbl", CF_TYPE_TABLES);
	else
		read_file_text_from_array(defaults_get_file("post_processing.tbl"));

	reset_parse();

	required_string("#Effects");

	while ( required_string_either("#End", "$Name:") ) {
		char tbuf[NAME_LENGTH+1] = { 0 };
		post_effect_t eff;

		required_string("$Name:");
		stuff_string(tbuf, F_NAME, NAME_LENGTH);
		eff.name = tbuf;

		required_string("$Uniform:");
		stuff_string(tbuf, F_NAME, NAME_LENGTH);
		eff.uniform_name = tbuf;

		required_string("$Define:");
		stuff_string(tbuf, F_NAME, NAME_LENGTH);
		eff.define_name = tbuf;

		required_string("$AlwaysOn:");
		stuff_boolean(&eff.always_on);

		required_string("$Default:");
		stuff_float(&eff.default_intensity);
		eff.intensity = eff.default_intensity;

		required_string("$Div:");
		stuff_float(&eff.div);

		required_string("$Add:");
		stuff_float(&eff.add);

		// Post_effects index is used for flag checks, so we can't have more than 32
		if (Post_effects.size() < 32) {
			Post_effects.push_back( eff );
		} else if ( !warned ) {
			mprintf(("WARNING: post_processing.tbl can only have a max of 32 effects! Ignoring extra...\n"));
			warned = true;
		}
	}

	required_string("#End");

	return true;
}

static char *opengl_post_load_shader(char *filename, int flags, int flags2)
{
	std::string sflags;

	if (Use_GLSL >= 4) {
		sflags += "#define SHADER_MODEL 4\n";
	} else if (Use_GLSL == 3) {
		sflags += "#define SHADER_MODEL 3\n";
	} else {
		sflags += "#define SHADER_MODEL 2\n";
	}

	for (size_t idx = 0; idx < Post_effects.size(); idx++) {
		if ( flags2 & (1 << idx) ) {
			sflags += "#define ";
			sflags += Post_effects[idx].define_name.c_str();
			sflags += "\n";
		}
	}

	if (flags & SDR_POST_FLAG_PASS1) {
		sflags += "#define PASS_0\n";
	} else if (flags & SDR_POST_FLAG_PASS2) {
		sflags += "#define PASS_1\n";
	}

	const char *shader_flags = sflags.c_str();
	int flags_len = strlen(shader_flags);

	CFILE *cf_shader = cfopen(filename, "rt", CFILE_NORMAL, CF_TYPE_EFFECTS);

	if (cf_shader != NULL) {
		int len = cfilelength(cf_shader);
		char *shader = (char*) vm_malloc(len + flags_len + 1);

		strcpy(shader, shader_flags);
		memset(shader + flags_len, 0, len + 1);
		cfread(shader + flags_len, len + 1, 1, cf_shader);
		cfclose(cf_shader);

		return shader;
	} else {
		mprintf(("Loading built-in default shader for: %s\n", filename));
		char* def_shader = defaults_get_file(filename);
		size_t len = strlen(def_shader);
		char *shader = (char*) vm_malloc(len + flags_len + 1);

		strcpy(shader, shader_flags);
		strcat(shader, def_shader);
		//memset(shader + flags_len, 0, len + 1);

		return shader;
	}

}

static bool opengl_post_init_shader()
{
	char *vert = NULL, *frag = NULL;
	bool rval = true;
	int idx, i;
	int flags2 = 0;
	int num_main_uniforms = 0;

	for (idx = 0; idx < (int)Post_effects.size(); idx++) {
		if (Post_effects[idx].always_on) {
			flags2 |= (1 << idx);
			num_main_uniforms++;
		}
	}

	for (idx = 0; idx < (int)Num_post_shader_files; idx++) {
		bool in_error = false;
		opengl_shader_t new_shader;
		opengl_shader_file_t *shader_file = &GL_post_shader_files[idx];

		// choose appropriate files
		char *vert_name = shader_file->vert;
		char *frag_name = shader_file->frag;

		mprintf(("  Compiling post-processing shader %d ... \n", idx+1));

		// read vertex shader
		if ( (vert = opengl_post_load_shader(vert_name, shader_file->flags, flags2)) == NULL ) {
			in_error = true;
			goto Done;
		}

		// read fragment shader
		if ( (frag = opengl_post_load_shader(frag_name, shader_file->flags, flags2)) == NULL ) {
			in_error = true;
			goto Done;
		}

		Verify( vert != NULL );
		Verify( frag != NULL );

		new_shader.program_id = opengl_shader_create(vert, frag);

		if ( !new_shader.program_id ) {
			in_error = true;
			goto Done;
		}


		new_shader.flags = shader_file->flags;
		new_shader.flags2 = flags2;

		opengl_shader_set_current( &new_shader );

		new_shader.uniforms.reserve(shader_file->num_uniforms + num_main_uniforms);

		for (i = 0; i < shader_file->num_uniforms; i++) {
			opengl_shader_init_uniform( shader_file->uniforms[i] );
		}

		if (idx == 0) {
			for (i = 0; i < (int)Post_effects.size(); i++) {
				if ( flags2 & (1 << i) ) {
					opengl_shader_init_uniform( Post_effects[i].uniform_name.c_str() );
				}
			}

			flags2 = 0; 
			num_main_uniforms = 0;
		}


		opengl_shader_set_current();

		// add it to our list of embedded shaders
		GL_post_shader.push_back( new_shader );

	Done:
		if (vert != NULL) {
			vm_free(vert);
			vert = NULL;
		}

		if (frag != NULL) {
			vm_free(frag);
			frag = NULL;
		}

		if (idx == 4)
			fxaa_shader_id = GL_post_shader.size() - 1;

		if (in_error) {
			if (idx == 0) {
				// only the main/first shader is actually required for post-processing
				rval = false;
				break;
			} else if (idx == 4) {
				Cmdline_fxaa = false;
				fxaa_unavailable = true;
				mprintf(("Error while compiling FXAA shaders. FXAA will be unavailable.\n"));
			} else if ( shader_file->flags & (SDR_POST_FLAG_BLUR|SDR_POST_FLAG_BRIGHT) ) {
				// disable bloom if we don't have those shaders available
				Cmdline_bloom_intensity = 0;
			}
		}
	}

	mprintf(("\n"));

	return rval;
}

// generate and test the framebuffer and textures that we are going to use
static bool opengl_post_init_framebuffer()
{
	bool rval = false;

	// clamp size, if needed
	Post_texture_width = gr_screen.max_w;
	Post_texture_height = gr_screen.max_h;

	if (Post_texture_width > GL_max_renderbuffer_size) {
		Post_texture_width = GL_max_renderbuffer_size;
	}

	if (Post_texture_height > GL_max_renderbuffer_size) {
		Post_texture_height = GL_max_renderbuffer_size;
	}

	// create framebuffer
	vglGenFramebuffersEXT(1, &Post_framebuffer_id[0]);
	vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[0]);

	// create renderbuffer
	vglGenRenderbuffersEXT(1, &Post_renderbuffer_id);
	vglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, Post_renderbuffer_id);
	vglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, Post_texture_width, Post_texture_height);

	vglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, Post_renderbuffer_id);

	// setup main render texture
	glGenTextures(1, &Post_screen_texture_id);

	GL_state.Texture.SetActiveUnit(0);
	GL_state.Texture.SetTarget(GL_TEXTURE_2D);
	GL_state.Texture.Enable(Post_screen_texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Post_texture_width, Post_texture_height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

	vglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Post_screen_texture_id, 0);

	if ( opengl_check_framebuffer() ) {
	//	nprintf(("OpenGL", "Unable to validate FBO!  Disabling post-processing...\n"));

		vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[0]);
		Post_framebuffer_id[0] = 0;

		vglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		vglDeleteRenderbuffersEXT(1, &Post_renderbuffer_id);
		Post_renderbuffer_id = 0;

		GL_state.Texture.Disable();
		glDeleteTextures(1, &Post_screen_texture_id);
		Post_screen_texture_id = 0;

		rval = false;
	} else {
		vglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

		if (Cmdline_bloom_intensity > 0) {
			// two more framebuffers, one each for the two different sized bloom textures
			vglGenFramebuffersEXT(1, &Post_framebuffer_id[1]);
			vglGenFramebuffersEXT(1, &Post_framebuffer_id[2]);

			// need to generate textures for bloom too
			glGenTextures(3, Post_bloom_texture_id);

			// half size
			int width = Post_texture_width >> 1;
			int height = Post_texture_height >> 1;

			for (int tex = 0; tex < 3; tex++) {
				GL_state.Texture.SetActiveUnit(0);
				GL_state.Texture.SetTarget(GL_TEXTURE_2D);
				GL_state.Texture.Enable(Post_bloom_texture_id[tex]);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

				if (tex == 0) {
					// attach to our bright pass framebuffer and make sure it's ok
					vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[1]);
					vglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Post_bloom_texture_id[tex], 0);

					// if not then clean up and disable bloom
					if ( opengl_check_framebuffer() ) {
						vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
						vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[1]);
						vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[2]);
						Post_framebuffer_id[1] = 0;
						Post_framebuffer_id[2] = 0;

						glDeleteTextures(3, Post_bloom_texture_id);
						memset(Post_bloom_texture_id, 0, sizeof(Post_bloom_texture_id));

						Cmdline_bloom_intensity = 0;

						break;
					}

					// width and height are 1/2 for the bright pass, 1/4 for the blur, so drop down
					width >>= 1;
					height >>= 1;
				} else {
					// attach to our blur pass framebuffer and make sure it's ok
					vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, Post_framebuffer_id[2]);
					vglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Post_bloom_texture_id[tex], 0);

					// if not then clean up and disable bloom
					if ( opengl_check_framebuffer() ) {
						vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
						vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[1]);
						vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[2]);
						Post_framebuffer_id[1] = 0;
						Post_framebuffer_id[2] = 0;

						glDeleteTextures(3, Post_bloom_texture_id);
						memset(Post_bloom_texture_id, 0, sizeof(Post_bloom_texture_id));

						Cmdline_bloom_intensity = 0;

						break;
					}
				}
			}
		}

		vglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		GL_state.Texture.Disable();

		rval = true;
	}

	if ( opengl_check_for_errors("post_init_framebuffer()") ) {
		rval = false;
	}

	return rval;
}

void opengl_post_process_init()
{
	Post_initialized = 0;

	//We need to read the tbl first. This is mostly for FRED's benefit, as otherwise the list of post effects for the sexp doesn't get updated.
	if ( !opengl_post_init_table() ) {
		mprintf(("  Unable to read post-processing table! Disabling post-processing...\n\n"));
		Cmdline_postprocess = 0;
		return;
	}

	if ( !Cmdline_postprocess ) {
		return;
	}

	if ( !Use_GLSL || Cmdline_no_fbo || !Is_Extension_Enabled(OGL_EXT_FRAMEBUFFER_OBJECT) ) {
		Cmdline_postprocess = 0;
		return;
	}

	// for ease of use we require support for non-power-of-2 textures in one
	// form or another:
	//    - the NPOT extension
	//    - GL version 2.0+ (which should work for non-reporting ATI cards since we don't use mipmaps)
	if ( !(Is_Extension_Enabled(OGL_ARB_TEXTURE_NON_POWER_OF_TWO) || (GL_version >= 20)) ) {
		Cmdline_postprocess = 0;
		return;
	}

	if ( !opengl_post_init_shader() ) {
		mprintf(("  Unable to initialize post-processing shaders! Disabling post-processing...\n\n"));
		Cmdline_postprocess = 0;
		return;
	}

	if ( !opengl_post_init_framebuffer() ) {
		mprintf(("  Unable to initialize post-processing framebuffer! Disabling post-processing...\n\n"));
		Cmdline_postprocess = 0;
		return;
	}

	Post_initialized = 1;
}

void opengl_post_process_shutdown()
{
	if ( !Post_initialized ) {
		return;
	}

	for (size_t i = 0; i < GL_post_shader.size(); i++) {
		if (GL_post_shader[i].program_id) {
			vglDeleteObjectARB(GL_post_shader[i].program_id);
			GL_post_shader[i].program_id = 0;
		}

		GL_post_shader[i].uniforms.clear();
	}

	GL_post_shader.clear();

	if (Post_screen_texture_id) {
		glDeleteTextures(1, &Post_screen_texture_id);
		Post_screen_texture_id = 0;
	}

	if (Post_bloom_texture_id[0]) {
		glDeleteTextures(3, Post_bloom_texture_id);
		memset(Post_bloom_texture_id, 0, sizeof(Post_bloom_texture_id));
	}

	if (Post_renderbuffer_id) {
		vglDeleteRenderbuffersEXT(1, &Post_renderbuffer_id);
		Post_renderbuffer_id = 0;
	}

	if (Post_framebuffer_id[0]) {
		vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[0]);
		Post_framebuffer_id[0] = 0;

		if (Post_framebuffer_id[1]) {
			vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[1]);
			vglDeleteFramebuffersEXT(1, &Post_framebuffer_id[2]);
			Post_framebuffer_id[1] = 0;
			Post_framebuffer_id[2] = 0;
		}
	}

	Post_effects.clear();

	Post_in_frame = false;
	Post_active_shader_index = 0;

	Post_initialized = 0;
}

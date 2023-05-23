#include <os.h>
#include <crt.h>
#include <GL/gl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <GL/glext.h>

static void (*gl_clear_color)(f32, f32, f32, f32);
static void (*gl_clear)(u32);
static u32  (*gl_create_shader)(u32);
static void (*gl_shader_source)(u32, i32, const i8 **, const i32 *);
static void (*gl_compile_shader)(u32);
static void (*gl_get_shader_iv)(u32, u32, i32 *);
static void (*gl_get_shader_info_log)(u32, i32, i32 *, i8 *);
static u32  (*gl_create_program)(void);
static void (*gl_attach_shader)(u32, u32);
static void (*gl_link_program)(u32);
static void (*gl_get_program_iv)(u32, u32, i32 *);
static void (*gl_get_program_info_log)(u32, i32, i32 *, i8 *);
static void (*gl_use_program)(u32);
static void (*gl_delete_shader)(u32);
static void (*gl_delete_program)(u32);
static void (*gl_gen_vertex_arrays)(i32, u32 *);
static void (*gl_gen_buffers)(i32, u32 *);
static void (*gl_bind_vertex_array)(u32);
static void (*gl_bind_buffer)(u32, u32);
static void (*gl_buffer_data)(u32, i64, const void *, u32);
static void (*gl_vertex_attrib_pointer)(u32, i32, u32, b8, i32, const void *);
static void (*gl_enable_vertex_attrib_array)(u32);
static void (*gl_draw_elements)(u32, i32, u32, const void *);
static void (*gl_delete_vertex_arrays)(i32, u32 *);
static void (*gl_delete_buffers)(i32, u32 *);
static void (*gl_draw_arrays)(u32, i32, i32);
static void (*gl_gen_textures)(i32, u32 *);
static void (*gl_bind_texture)(u32, u32);
static void (*gl_tex_parameter_i)(u32, u32, i32);
static void (*gl_tex_sub_image_2d)(u32, i32, i32, i32, i32, i32, u32, u32, const void *);
static void (*gl_tex_image_2d)(u32, i32, i32, i32, i32, i32, u32, u32, const void *);
static void (*gl_delete_textures)(i32, u32 *);

static u32 shader_program;

static u32 vertex_array;
static u32 vertex_buffer;
static u32 index_buffer;

static u32 screen_buffer[GAME_SIZE*GAME_SIZE];
static u32 screen;

static u32 electron_gun_x;
static u32 electron_gun_y;

static u32
shader_create(u32 type, const i8 *name) {
	i8 *path = resource_path("shaders", name);
	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "error: couldn't load shader %s: %s\n", path, strerror(errno));
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	i32 siz = ftell(file);
	rewind(file);
	i8 *src = malloc(siz + 1);
	if (!src) {
		fprintf(stderr, "error: couldn't allocate memory for shader %s\n", path);
		exit(1);
	}
	src[siz] = '\0';
	fread(src, 1, siz, file);
	i32 success;
	u32 shader = gl_create_shader(type);
	gl_shader_source(shader, 1, (const i8 **)&src, &siz);
	free(src);
	gl_compile_shader(shader);
	gl_get_shader_iv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		i8 info[256];
		gl_get_shader_info_log(shader, 256, NULL, info);
		fprintf(stderr, "error: compile error in %s: %s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", info);
		exit(1);
	}
	return shader;
}

void
crt_begin(void) {
	/* load opengl functions */
	void *(*glfw_get_proc_address)(const i8 *) = glfw_func("glfwGetProcAddress");
	gl_clear_color                = glfw_get_proc_address("glClearColor");
	gl_clear                      = glfw_get_proc_address("glClear");
	gl_create_shader              = glfw_get_proc_address("glCreateShader");
	gl_shader_source              = glfw_get_proc_address("glShaderSource");
	gl_compile_shader             = glfw_get_proc_address("glCompileShader");
	gl_get_shader_iv              = glfw_get_proc_address("glGetShaderiv");
	gl_get_shader_info_log        = glfw_get_proc_address("glGetShaderInfoLog");
	gl_create_program             = glfw_get_proc_address("glCreateProgram");
	gl_attach_shader              = glfw_get_proc_address("glAttachShader");
	gl_link_program               = glfw_get_proc_address("glLinkProgram");
	gl_get_program_iv             = glfw_get_proc_address("glGetProgramiv");
	gl_get_program_info_log       = glfw_get_proc_address("glGetProgramInfoLog");
	gl_use_program                = glfw_get_proc_address("glUseProgram");
	gl_delete_shader              = glfw_get_proc_address("glDeleteShader");
	gl_delete_program             = glfw_get_proc_address("glDeleteProgram");
	gl_gen_vertex_arrays          = glfw_get_proc_address("glGenVertexArrays");
	gl_gen_buffers                = glfw_get_proc_address("glGenBuffers");
	gl_bind_vertex_array          = glfw_get_proc_address("glBindVertexArray");
	gl_bind_buffer                = glfw_get_proc_address("glBindBuffer");
	gl_buffer_data                = glfw_get_proc_address("glBufferData");
	gl_vertex_attrib_pointer      = glfw_get_proc_address("glVertexAttribPointer");
	gl_enable_vertex_attrib_array = glfw_get_proc_address("glEnableVertexAttribArray");
	gl_draw_elements              = glfw_get_proc_address("glDrawElements");
	gl_delete_vertex_arrays       = glfw_get_proc_address("glDeleteVertexArrays");
	gl_delete_buffers             = glfw_get_proc_address("glDeleteBuffers");
	gl_draw_arrays                = glfw_get_proc_address("glDrawArrays");
	gl_gen_textures               = glfw_get_proc_address("glGenTextures");
	gl_bind_texture               = glfw_get_proc_address("glBindTexture");
	gl_tex_parameter_i            = glfw_get_proc_address("glTexParameteri");
	gl_tex_image_2d               = glfw_get_proc_address("glTexImage2D");
	gl_tex_sub_image_2d           = glfw_get_proc_address("glTexSubImage2D");
	gl_delete_textures            = glfw_get_proc_address("glDeleteTextures");
	/* creating shader program */
	i32 success;
	u32 vertex_shader   = shader_create(GL_VERTEX_SHADER,   "vertex.glsl");
	u32 fragment_shader = shader_create(GL_FRAGMENT_SHADER, "fragment.glsl");
	shader_program = gl_create_program();
	gl_attach_shader(shader_program, vertex_shader);
	gl_attach_shader(shader_program, fragment_shader);
	gl_delete_shader(vertex_shader);
	gl_delete_shader(fragment_shader);
	gl_link_program(shader_program);
	gl_get_program_iv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		i8 info[256];
		gl_get_program_info_log(shader_program, 256, NULL, info);
		fprintf(stderr, "error: shader linking error: %s\n", info);
		exit(1);
	}
	gl_use_program(shader_program);
	/* square */
	f32 vertices[] = {
	 -1, -1,  0, 1,
	  1, -1,  1, 1,
	  1,  1,  1, 0,
	 -1,  1,  0, 0
	};
	u32 indices[] = {
		0, 1, 2, 2, 3, 0
	};
	gl_gen_vertex_arrays(1, &vertex_array);
	gl_bind_vertex_array(vertex_array);
	gl_gen_buffers(1, &vertex_buffer);
	gl_gen_buffers(1, &index_buffer);
	gl_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	gl_buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
	gl_bind_buffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl_buffer_data(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
	gl_vertex_attrib_pointer(0, 2, GL_FLOAT, 0, sizeof (f32) * 4, (void *)0);
	gl_enable_vertex_attrib_array(0);
	gl_vertex_attrib_pointer(1, 2, GL_FLOAT, 0, sizeof (f32) * 4, (void *)(sizeof (f32) * 2));
	gl_enable_vertex_attrib_array(1);
	/* create screen texture */
	gl_gen_textures(1, &screen);
	gl_bind_texture(GL_TEXTURE_2D, screen);
	gl_tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl_tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl_tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA, GAME_SIZE, GAME_SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, screen_buffer);
}

void
crt_electron_gun_shoot(u32 rgb) {
	electron_gun_x++;
	if (electron_gun_x >= GAME_SIZE) {
		electron_gun_x = 0;
		electron_gun_y++;
		if (electron_gun_y >= GAME_SIZE) electron_gun_y = 0;
	}
	screen_buffer[electron_gun_y * GAME_SIZE + electron_gun_x] = rgb;
}

void
crt_update(void) {
	gl_tex_sub_image_2d(GL_TEXTURE_2D, 0, 0, 0, GAME_SIZE, GAME_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, screen_buffer);
	gl_draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void
crt_end(void) {
	gl_delete_program(shader_program);
	gl_delete_vertex_arrays(1, &vertex_array);
	gl_delete_buffers(1, &vertex_buffer);
	gl_delete_buffers(1, &index_buffer);
	gl_delete_textures(1, &screen);
}

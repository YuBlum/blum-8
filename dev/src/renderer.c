#include <os.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <renderer.h>

static const i8 vertex_src[] =
"#version 330 core\n"
"layout (location = 0) in vec2 a_pos;\n"
"void\n"
"main() {\n"
"	gl_Position = vec4(a_pos, 0, 1);\n"
"}\n";
static const i32 vertex_siz = sizeof(vertex_src);
static const i8 fragment_src[] =
"#version 330 core\n"
"out vec4 out_color;\n"
"void\n"
"main() {\n"
"	out_color = vec4(1);\n"
"}\n";
static const i32 fragment_siz = sizeof(fragment_src);

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

static u32 shader_program;

static u32 vertex_array;
static u32 vertex_buffer;
static u32 index_buffer;

static u32
shader_create(u32 type, const i8 *src, i32 siz) {
	i32 success;
	u32 shader = gl_create_shader(type);
	gl_shader_source(shader, 1, &src, &siz);
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
renderer_begin(void) {
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
	/* creating shader program */
	i32 success;
	u32 vertex_shader   = shader_create(GL_VERTEX_SHADER,   vertex_src,   vertex_siz);
	u32 fragment_shader = shader_create(GL_FRAGMENT_SHADER, fragment_src, fragment_siz);
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
	 -0.5f, -0.5f,
	  0.5f, -0.5f,
	  0.5f,  0.5f,
	 -0.5f,  0.5f,
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
	gl_vertex_attrib_pointer(0, 2, GL_FLOAT, 0, sizeof (f32) * 2, (void *)0);
	gl_enable_vertex_attrib_array(0);
}

void
renderer_update(void) {
	gl_clear_color(1, 0, 0, 1);
	gl_clear(GL_COLOR_BUFFER_BIT);
	gl_draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void
renderer_end(void) {
	gl_delete_program(shader_program);
	gl_delete_vertex_arrays(1, &vertex_array);
	gl_delete_buffers(1, &vertex_buffer);
	gl_delete_buffers(1, &index_buffer);
}

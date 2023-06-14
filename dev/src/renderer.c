#include <os.h>
#include <renderer.h>
#include <GL/gl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <GL/glext.h>

static struct {
  void (*clear_color)(f32, f32, f32, f32);
  void (*clear)(u32);
  u32  (*create_shader)(u32);
  void (*shader_source)(u32, i32, const i8 **, const i32 *);
  void (*compile_shader)(u32);
  void (*get_shader_iv)(u32, u32, i32 *);
  void (*get_shader_info_log)(u32, i32, i32 *, i8 *);
  u32  (*create_program)(void);
  void (*attach_shader)(u32, u32);
  void (*link_program)(u32);
  void (*get_program_iv)(u32, u32, i32 *);
  void (*get_program_info_log)(u32, i32, i32 *, i8 *);
  void (*use_program)(u32);
  void (*delete_shader)(u32);
  void (*delete_program)(u32);
  void (*gen_vertex_arrays)(i32, u32 *);
  void (*gen_buffers)(i32, u32 *);
  void (*bind_vertex_array)(u32);
  void (*bind_buffer)(u32, u32);
  void (*buffer_data)(u32, i64, const void *, u32);
  void (*vertex_attrib_pointer)(u32, i32, u32, b8, i32, const void *);
  void (*enable_vertex_attrib_array)(u32);
  void (*draw_elements)(u32, i32, u32, const void *);
  void (*delete_vertex_arrays)(i32, u32 *);
  void (*delete_buffers)(i32, u32 *);
  void (*draw_arrays)(u32, i32, i32);
  void (*gen_textures)(i32, u32 *);
  void (*bind_texture)(u32, u32);
  void (*tex_parameter_i)(u32, u32, i32);
  void (*tex_sub_image_2d)(u32, i32, i32, i32, i32, i32, u32, u32, const void *);
  void (*tex_image_2d)(u32, i32, i32, i32, i32, i32, u32, u32, const void *);
  void (*delete_textures)(i32, u32 *);
} gl;

#pragma pack(1)
struct tga_header {
  u8  id_length;
  u8  color_map_type;
  u8  image_type;
  u16 color_map_first;
  u16 color_map_size;
  u8  color_map_depth;
  u16 x, y;
  u16 w, h;
  u8  bpp;
  u8  image_descriptor;
};
#pragma pack(0)

static u32 shader_program;

static u32 vertex_array;
static u32 vertex_buffer;
static u32 index_buffer;

static u32 screen_buffer[GAME_SIZE*GAME_SIZE];
static u32 screen;

static b8 shader_create(u32 type, const i8 *name, u32 *shader);

static void
opengl_load(const struct glfw *glfw) {
  gl.clear_color                = glfw->get_proc_address("glClearColor");
  gl.clear                      = glfw->get_proc_address("glClear");
  gl.create_shader              = glfw->get_proc_address("glCreateShader");
  gl.shader_source              = glfw->get_proc_address("glShaderSource");
  gl.compile_shader             = glfw->get_proc_address("glCompileShader");
  gl.get_shader_iv              = glfw->get_proc_address("glGetShaderiv");
  gl.get_shader_info_log        = glfw->get_proc_address("glGetShaderInfoLog");
  gl.create_program             = glfw->get_proc_address("glCreateProgram");
  gl.attach_shader              = glfw->get_proc_address("glAttachShader");
  gl.link_program               = glfw->get_proc_address("glLinkProgram");
  gl.get_program_iv             = glfw->get_proc_address("glGetProgramiv");
  gl.get_program_info_log       = glfw->get_proc_address("glGetProgramInfoLog");
  gl.use_program                = glfw->get_proc_address("glUseProgram");
  gl.delete_shader              = glfw->get_proc_address("glDeleteShader");
  gl.delete_program             = glfw->get_proc_address("glDeleteProgram");
  gl.gen_vertex_arrays          = glfw->get_proc_address("glGenVertexArrays");
  gl.gen_buffers                = glfw->get_proc_address("glGenBuffers");
  gl.bind_vertex_array          = glfw->get_proc_address("glBindVertexArray");
  gl.bind_buffer                = glfw->get_proc_address("glBindBuffer");
  gl.buffer_data                = glfw->get_proc_address("glBufferData");
  gl.vertex_attrib_pointer      = glfw->get_proc_address("glVertexAttribPointer");
  gl.enable_vertex_attrib_array = glfw->get_proc_address("glEnableVertexAttribArray");
  gl.draw_elements              = glfw->get_proc_address("glDrawElements");
  gl.delete_vertex_arrays       = glfw->get_proc_address("glDeleteVertexArrays");
  gl.delete_buffers             = glfw->get_proc_address("glDeleteBuffers");
  gl.draw_arrays                = glfw->get_proc_address("glDrawArrays");
  gl.gen_textures               = glfw->get_proc_address("glGenTextures");
  gl.bind_texture               = glfw->get_proc_address("glBindTexture");
  gl.tex_parameter_i            = glfw->get_proc_address("glTexParameteri");
  gl.tex_image_2d               = glfw->get_proc_address("glTexImage2D");
  gl.tex_sub_image_2d           = glfw->get_proc_address("glTexSubImage2D");
  gl.delete_textures            = glfw->get_proc_address("glDeleteTextures");
}

static void * 
atlas_load(void) {
  i8 *atlas_path = resource_path("data", "atlas.tga");
  FILE *atlas_file = fopen(atlas_path, "rb");
  if (!atlas_file) {
    fprintf(stderr, "error: couldn't open '%s': %s\n", atlas_path, strerror(errno));
    return NULL;
  }
  struct tga_header header = { 0 };
  fread(&header, 1, sizeof (struct tga_header), atlas_file);
  if (header.color_map_type != 0 || header.image_type != 2 || header.bpp != 32) {
    fprintf(stderr, "error: atlas is not a valid TGA file\n");
    fclose(atlas_file);
    return NULL;
  }
  fseek(atlas_file, header.id_length, SEEK_CUR);
  u8 *atlas_data = malloc(32 * header.w * header.h);
  fclose(atlas_file);
  return atlas_data;
}

b8
renderer_begin(const struct glfw *glfw) {
  opengl_load(glfw);
  /* creating shader program */
  i32 success;
  u32 vertex_shader, fragment_shader;
  if (
    !shader_create(GL_VERTEX_SHADER,   "vertex.glsl", &vertex_shader) ||
    !shader_create(GL_FRAGMENT_SHADER, "fragment.glsl", &fragment_shader)
  ) return 0;
  shader_program = gl.create_program();
  gl.attach_shader(shader_program, vertex_shader);
  gl.attach_shader(shader_program, fragment_shader);
  gl.delete_shader(vertex_shader);
  gl.delete_shader(fragment_shader);
  gl.link_program(shader_program);
  gl.get_program_iv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    i8 info[256];
    gl.get_program_info_log(shader_program, 256, NULL, info);
    fprintf(stderr, "error: shader linking error: %s\n", info);
    return 0;
  }
  gl.use_program(shader_program);
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
  gl.gen_vertex_arrays(1, &vertex_array);
  gl.bind_vertex_array(vertex_array);
  gl.gen_buffers(1, &vertex_buffer);
  gl.gen_buffers(1, &index_buffer);
  gl.bind_buffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  gl.buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);
  gl.bind_buffer(GL_ARRAY_BUFFER, vertex_buffer);
  gl.buffer_data(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
  gl.vertex_attrib_pointer(0, 2, GL_FLOAT, 0, sizeof (f32) * 4, (void *)0);
  gl.enable_vertex_attrib_array(0);
  gl.vertex_attrib_pointer(1, 2, GL_FLOAT, 0, sizeof (f32) * 4, (void *)(sizeof (f32) * 2));
  gl.enable_vertex_attrib_array(1);
  /* create screen texture */
  gl.gen_textures(1, &screen);
  gl.bind_texture(GL_TEXTURE_2D, screen);
  gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl.tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA, GAME_SIZE, GAME_SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, screen_buffer);
  /* tga */
  u8 *atlas_data = atlas_load();
  if (!atlas_data) return 0;
  free(atlas_data);
  return 1;
}

static b8
shader_create(u32 type, const i8 *name, u32 *shader) {
  i8 *path = resource_path("shaders", name);
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "error: couldn't load shader %s: %s\n", path, strerror(errno));
    return 0;
  }
  fseek(file, 0, SEEK_END);
  i32 siz = ftell(file);
  rewind(file);
  i8 *src = malloc(siz + 1);
  if (!src) {
    fprintf(stderr, "error: couldn't allocate memory for shader %s\n", path);
    return 0;
  }
  src[siz] = '\0';
  fread(src, 1, siz, file);
  i32 success;
  *shader = gl.create_shader(type);
  gl.shader_source(*shader, 1, (const i8 **)&src, &siz);
  free(src);
  gl.compile_shader(*shader);
  gl.get_shader_iv(*shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    i8 info[256];
    gl.get_shader_info_log(*shader, 256, NULL, info);
    fprintf(stderr, "error: compile error in %s: %s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", info);
    return 0;
  }
  return 1;
}

void
crt_display_pixel(u32 rgb, u32 x, u32 y) {
  screen_buffer[y * GAME_SIZE + x] = rgb;
}

void
renderer_update(void) {
  gl.tex_sub_image_2d(GL_TEXTURE_2D, 0, 0, 0, GAME_SIZE, GAME_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, screen_buffer);
  gl.draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void
renderer_end(void) {
  gl.delete_program(shader_program);
  gl.delete_vertex_arrays(1, &vertex_array);
  gl.delete_buffers(1, &vertex_buffer);
  gl.delete_buffers(1, &index_buffer);
  gl.delete_textures(1, &screen);
}

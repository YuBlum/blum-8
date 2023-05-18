#version 330 core

out vec4 out_color;
in vec2 texcoord;

uniform sampler2D screen;

void
main() {
	out_color = texture(screen, texcoord);
}

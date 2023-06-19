#version 330 core

out vec4 out_color;
in vec2 texcoord;

uniform vec2 resolution;
uniform sampler2D screen;

void
main() {
  const float curvature = 6;
  vec2 uv = texcoord * 2 - 1;
  vec2 offset = abs(uv.yx) / curvature;
  uv += uv * offset * offset;
  uv = uv * 0.5 + 0.5;
  vec2 vig_vec = uv * (1 - uv.yx);
  float vig = pow(vig_vec.x * vig_vec.y * 200, 0.5);
  if (vig > 1) vig = 1;
  float scanline_intensity = sin(uv.y * 1024 * 3.1415926 * 2);
  scanline_intensity = (0.5 * scanline_intensity + 0.5) * 0.9 + 0.1;
  vec3 scanline = vec3(pow(scanline_intensity, 0.8));
  if (uv.x < 0 || uv.x > 1 || uv.y < 0  || uv.y > 1) out_color = vec4(0, 0, 0, 1);
  else out_color = vec4(pow(texture(screen, uv).rgb * vec3(vig) * scanline, vec3(0.75)), 1);
}

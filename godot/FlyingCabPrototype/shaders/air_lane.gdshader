shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, cull_disabled;
uniform vec4 color : source_color = vec4(0.3, 0.9, 0.8, 1.0);
uniform float length_m = 100.0;
uniform bool vertical = false;
void fragment() {
    float across = vertical ? UV.x : UV.y;
    float along = (vertical ? UV.y : UV.x) * length_m;
    float edge = smoothstep(0.475, 0.495, abs(across - 0.5));
    float centre = 1.0 - smoothstep(0.009, 0.017, abs(across - 0.5));
    float dash = step(0.80, fract(along / 5.0));
    float rail = 1.0 - smoothstep(0.008, 0.015, abs(abs(across - 0.5) - 0.28));
    ALBEDO = color.rgb;
    ALPHA = edge * 0.45 + centre * 0.09 + rail * dash * 0.65 + 0.012;
}

shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, cull_disabled, fog_disabled;
uniform float intensity = 0.0;
uniform float length_m = 16.0;
uniform vec4 tint : source_color = vec4(0.65,0.88,0.86,1.0);
varying float along;
void vertex() { along = (VERTEX.y + length_m*0.5)/length_m; }
void fragment() {
    float edge = pow(abs(dot(normalize(NORMAL),normalize(VIEW))), 1.6);
    float falloff = smoothstep(0.0,0.08,along) * pow(1.0-clamp(along,0.0,1.0),1.3);
    ALBEDO = tint.rgb;
    ALPHA = intensity * edge * falloff * 0.20;
}

shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, cull_disabled;
uniform vec4 tint : source_color = vec4(0.36, 0.9, 0.86, 1.0);
uniform float strength = 0.0;
void fragment() {
    // Local X runs from the fading tail to the car, never along the road.
    float tail = pow(UV.x, 1.3) * (1.0-smoothstep(0.85,1.0,UV.x));
    float width = mix(0.06,0.42,UV.x);
    float feather = 1.0-smoothstep(0.0,width,abs(UV.y-0.5));
    ALBEDO = tint.rgb;
    ALPHA = tail * feather * strength * 0.65;
}

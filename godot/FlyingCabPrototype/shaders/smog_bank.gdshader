shader_type spatial;
render_mode unshaded, blend_mix, depth_draw_never, cull_disabled, fog_disabled;
uniform vec4 tint : source_color = vec4(0.16, 0.32, 0.35, 1.0);
uniform float top_height = -6.0;
uniform float feather = 12.0;
uniform float opacity = 0.24;
uniform float phase = 0.0;
varying vec3 world;
void vertex() { world = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p); f = f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
void fragment() {
    vec2 p = world.xy * vec2(0.055,0.14) + vec2(TIME*0.028+phase,0.0);
    float n = noise(p) * 0.7 + noise(p*2.1-vec2(TIME*0.04,phase))*0.3;
    float height_fade = 1.0-smoothstep(top_height-feather,top_height+4.0,world.y+(n-0.5)*8.0);
    float sides = smoothstep(0.0,0.1,UV.x) * smoothstep(0.0,0.1,1.0-UV.x);
    ALBEDO = tint.rgb * (0.72+n*0.45);
    ALPHA = height_fade * sides * opacity * (0.45+n*0.8);
}

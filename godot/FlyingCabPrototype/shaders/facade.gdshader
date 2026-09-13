shader_type spatial;
uniform vec4 base_color : source_color = vec4(0.06, 0.08, 0.13, 1.0);
uniform vec4 window_color : source_color = vec4(0.5, 0.8, 0.8, 1.0);
uniform vec2 panel_count = vec2(8.0, 24.0);
uniform float seed = 0.0;
uniform float lit_fraction = 0.5;
uniform float window_energy = 0.8;
uniform float glass = 0.5;
varying vec3 world;
void vertex() { world = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7)) + seed) * 43758.5453); }
void fragment() {
    vec2 grid = UV * panel_count;
    vec2 cell = fract(grid);
    float pane = step(0.14, cell.x) * step(cell.x, 0.86) * step(0.12, cell.y) * step(cell.y, 0.80);
    float on = step(1.0 - lit_fraction, hash(floor(grid)));
    float blind = mix(0.52, 1.0, step(0.14, fract(cell.y * 6.0)));
    ALBEDO = mix(base_color.rgb, base_color.rgb * 0.32, pane);
    ROUGHNESS = mix(0.7, 0.18, pane * glass);
    METALLIC = 0.22 + pane * glass * 0.35;
    EMISSION = window_color.rgb * pane * on * blind * window_energy;
}

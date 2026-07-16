#version 330 core

out vec4 FragColor;

struct SceneObject {
    vec4 pos_type;      
    vec4 rot_op;        
    vec4 params;        
    vec4 color_extra;   
};

layout(std140) uniform WorldData {
    int object_count;
    int pad1, pad2, pad3; 
    SceneObject objects[100];
};

uniform vec2 u_resolution;
uniform vec3 u_camera_pos;
uniform vec3 u_camera_front;
uniform vec3 u_camera_up;
uniform vec3 u_camera_right;

#define AA 2
#define MAX_GROUPS 4

// --- Helper: Space Transformation ---
vec3 getLocalSpace(vec3 p, SceneObject obj) {
    vec3 pos = obj.pos_type.xyz;
    vec3 rot = obj.rot_op.xyz;
    vec3 s = sin(-rot);
    vec3 c = cos(-rot);
    
    mat3 rx = mat3(1.0, 0.0, 0.0,  0.0, c.x, s.x,  0.0, -s.x, c.x);
    mat3 ry = mat3(c.y, 0.0, -s.y, 0.0, 1.0, 0.0,  s.y, 0.0, c.y);
    mat3 rz = mat3(c.z, s.z, 0.0, -s.z, c.z, 0.0,  0.0, 0.0, 1.0);
    
    return (rx * ry * rz) * (p - pos);
}

// --- Shape-Specific SDF Functions ---

float sdfSphere(vec3 p_local, SceneObject obj) {
    return length(p_local) - obj.params.x;
}

float sdfBox(vec3 p_local, SceneObject obj) {
    vec3 q = abs(p_local) - obj.params.xyz;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdfRoundBox(vec3 p_local, SceneObject obj) {
    vec3 q = abs(p_local) - obj.params.xyz;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - obj.params.w;
}

float sdfTorus(vec3 p_local, SceneObject obj) {
    vec2 q = vec2(length(p_local.xz) - obj.params.x, p_local.y);
    return length(q) - obj.params.y;
}

float sdfCappedTorus(vec3 p_local, SceneObject obj) {
    vec2 sc = vec2(sin(obj.params.z), cos(obj.params.z));
    vec3 p = p_local;
    p.x = abs(p.x);
    float k = (sc.y * p.x > sc.x * p.y) ? dot(p.xy, sc) : length(p.xy);
    return sqrt(dot(p, p) + obj.params.x * obj.params.x - 2.0 * obj.params.x * k) - obj.params.y;
}

float sdfLink(vec3 p_local, SceneObject obj) {
    vec3 q = vec3(p_local.x, max(abs(p_local.y) - obj.params.x, 0.0), p_local.z);
    return length(vec2(length(q.xy) - obj.params.y, q.z)) - obj.params.z;
}

float sdfCylinderInfinite(vec3 p_local, SceneObject obj) {
    return length(p_local.xz) - obj.params.x;
}

float sdfCylinderCapped(vec3 p_local, SceneObject obj) {
    vec2 d = abs(vec2(length(p_local.xz), p_local.y)) - vec2(obj.params.x, obj.params.y);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdfPlane(vec3 p_local, SceneObject obj) {
    return p_local.y;
}

float sdfHexPrism(vec3 p_local, SceneObject obj) {
    vec3 q = abs(p_local);
    const vec3 k = vec3(-0.8660254, 0.5, 0.57735026);
    q.xy -= 2.0 * min(dot(k.xy, q.xy), 0.0) * k.xy;
    vec2 d2 = vec2(length(q.xy - vec2(clamp(q.x, -k.z * obj.params.x, k.z * obj.params.x), obj.params.x)) * sign(q.y - obj.params.x), q.z - obj.params.y);
    return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0));
}

float sdfCapsule(vec3 p_local, SceneObject obj) {
    vec3 q = p_local;
    q.y -= clamp(q.y, -obj.params.y, obj.params.y);
    return length(q) - obj.params.x;
}

float sdfCylinderRounded(vec3 p_local, SceneObject obj) {
    vec2 d2 = vec2(length(p_local.xz) - obj.params.x + obj.params.z, abs(p_local.y) - obj.params.y);
    return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - obj.params.z;
}

float sdfCutHollowSphere(vec3 p_local, SceneObject obj) {
    vec2 q = vec2(length(p_local.xz), p_local.y);
    float w = sqrt(obj.params.x * obj.params.x - obj.params.y * obj.params.y);
    if (obj.params.y * q.x < w * q.y) {
        return length(q - vec2(w, obj.params.y));
    }
    return abs(length(q) - obj.params.x) - obj.params.z;
}

// 13: Octahedron
float sdfOctahedron(vec3 p_local, SceneObject obj) {
    vec3 q = abs(p_local);
    return (q.x + q.y + q.z - obj.params.x) * 0.57735027;
}

// 14: Pyramid
float sdfPyramid(vec3 p_local, SceneObject obj) {
    float m2 = obj.params.x * obj.params.x + 0.25;
    vec3 q = p_local;
    q.xz = abs(q.xz);
    q.xz = (q.x > q.z) ? q.xz : q.zx;
    q.xz -= 0.5;
    vec3 k = vec3(q.z, obj.params.x * q.y - 0.5 * q.x, obj.params.x * q.x + 0.5 * q.y);
    float s = max(-k.x, 0.0);
    float t = clamp((k.y - 0.5 * q.z) / (m2 + 0.25), 0.0, 1.0);
    float a = m2 * (k.x + s) * (k.x + s) + k.y * k.y;
    float b = m2 * (k.x + 0.5 * t) * (k.x + 0.5 * t) + (k.y - m2 * t) * (k.y - m2 * t);
    float d2 = min(k.y, -k.x * m2 - k.y * 0.5) > 0.0 ? 0.0 : min(a, b);
    return sqrt((d2 + k.z * k.z) / m2) * sign(max(k.z, -q.y));
}

float evaluateSDF(int type, vec3 p_local, SceneObject obj) {
    switch (type) {
        case 0:  return sdfSphere(p_local, obj);
        case 1:  return sdfBox(p_local, obj);
        case 2:  return sdfRoundBox(p_local, obj);
        case 3:  return sdfTorus(p_local, obj);
        case 4:  return sdfCappedTorus(p_local, obj);
        case 5:  return sdfLink(p_local, obj);
        case 6:  return sdfCylinderInfinite(p_local, obj);
        case 7:  return sdfCylinderCapped(p_local, obj);
        case 8:  return sdfPlane(p_local, obj);
        case 9:  return sdfHexPrism(p_local, obj);
        case 10: return sdfCapsule(p_local, obj);
        case 11: return sdfCylinderRounded(p_local, obj);
        case 12: return sdfCutHollowSphere(p_local, obj);
        case 13: return sdfOctahedron(p_local, obj);
        case 14: return sdfPyramid(p_local, obj);
    }
    return 10000.0;
}

// Maps the world using Multi-Pass Group-Isolated CSG Operations
float map(vec3 p, out vec3 hit_color) {
    float global_min_dist = 10000.0;
    hit_color = vec3(0.0);

    // Loop through each isolated operation group
    for (int g = 0; g < MAX_GROUPS; g++) {
        float group_dist = 10000.0;
        vec3 group_color = vec3(0.0);
        bool group_has_objects = false;
        bool is_first_in_group = true;

        for (int i = 0; i < object_count; i++) {
            int obj_group = int(objects[i].color_extra.w);
            if (obj_group != g) continue;

            group_has_objects = true;
            vec3 p_local = getLocalSpace(p, objects[i]);
            int type = int(objects[i].pos_type.w);
            int op = int(objects[i].rot_op.w);
            
            float d = evaluateSDF(type, p_local, objects[i]);
            
            if (is_first_in_group || op == 0) { // UNION
                if (is_first_in_group || d < group_dist) {
                    group_dist = d;
                    group_color = objects[i].color_extra.rgb;
                }
                is_first_in_group = false;
            } else if (op == 1) { // SUBTRACTION
                if (-d > group_dist) {
                    group_dist = -d;
                    group_color = objects[i].color_extra.rgb; 
                }
            } else if (op == 2) { // INTERSECTION
                if (d > group_dist) {
                    group_dist = d;
                    group_color = objects[i].color_extra.rgb;
                }
            }
        }

        // Combine this completed group with the global world map using simple Union
        if (group_has_objects) {
            if (group_dist < global_min_dist) {
                global_min_dist = group_dist;
                hit_color = group_color;
            }
        }
    }
    
    return global_min_dist;
}

void main() {
    vec3 total_color = vec3(0.0);

    for (int m = 0; m < AA; m++) {
        for (int n = 0; n < AA; n++) {
            vec2 offset = vec2(float(m), float(n)) / float(AA) - 0.5;
            vec2 uv = (gl_FragCoord.xy + offset - 0.5 * u_resolution.xy) / u_resolution.y;

            vec3 ro = u_camera_pos; 
            vec3 rd = normalize(uv.x * u_camera_right + uv.y * u_camera_up + 1.0 * u_camera_front); 

            float t = 0.0; 
            vec3 col = vec3(0.05, 0.05, 0.1); 

            for (int i = 0; i < 256; i++) {
                vec3 p = ro + rd * t;
                vec3 obj_color;
                float d = map(p, obj_color);
                
                if (d < 0.001) { 
                    col = obj_color; 
                    break;
                }
                if (t > 100.0) { 
                    break;
                }
                t += d;
            }
            total_color += col;
        }
    }

    total_color /= float(AA * AA);
    FragColor = vec4(total_color, 1.0);
}
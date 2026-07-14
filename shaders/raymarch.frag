#version 330 core

out vec4 FragColor;

struct SceneObject {
    vec4 position_radius; 
    vec4 color_type;      
};

layout(std140) uniform WorldData {
    int object_count;
    int pad1, pad2, pad3; 
    SceneObject objects[100];
};

uniform vec2 u_resolution;
uniform vec3 u_camera_pos;
// NEW: Camera orientation vectors
uniform vec3 u_camera_front;
uniform vec3 u_camera_up;
uniform vec3 u_camera_right;

#define AA 2

float map(vec3 p, out vec3 hit_color) {
    float min_dist = 1000.0;
    hit_color = vec3(0.0);

    for (int i = 0; i < object_count; i++) {
        vec3 obj_pos = objects[i].position_radius.xyz;
        float radius = objects[i].position_radius.w;
        int type = int(objects[i].color_type.w);
        
        float d = 1000.0;
        
        if (type == 0) {
            d = length(p - obj_pos) - radius;
        } else if (type == 1) {
            d = p.y - obj_pos.y; 
        }

        if (d < min_dist) {
            min_dist = d;
            hit_color = objects[i].color_type.rgb;
        }
    }
    return min_dist;
}

void main() {
    vec3 total_color = vec3(0.0);

    for (int m = 0; m < AA; m++) {
        for (int n = 0; n < AA; n++) {
            
            vec2 offset = vec2(float(m), float(n)) / float(AA) - 0.5;
            vec2 uv = (gl_FragCoord.xy + offset - 0.5 * u_resolution.xy) / u_resolution.y;

            vec3 ro = u_camera_pos; 
            
            // NEW: Construct the ray direction using the camera's local axes
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
#version 430

uniform vec4 color;
uniform mat4 view;
//uniform mat4 model;
uniform mat4 projection;
uniform float sphereRadius;

flat in int passInstanceID;

in vec4 passPosition;
in vec4 passColor;
flat in mat4 model;
flat in vec3 center;
in float size;

out vec4 fragColor;
out vec4 InstanceID;

vec3 center_v;

uniform vec4 lightSrc = vec4(0,100,50,1);
bool stop = false;

layout(depth_greater) out float gl_FragDepth;
vec3 view_w;
void hit(vec3 hitPos)
{
    // Normale Berechnen
    vec4 normal = normalize(vec4(hitPos -center_v,0));

    // Beleuchtung
    vec3 light_v = lightSrc.xyz;//vec3(view * lightSrc).xyz;
    vec3 L = normalize(vec3(light_v - hitPos.xyz));
    vec3 finalColor = passColor.xyz * max(dot(normal.xyz,L), 0.0);
    finalColor = clamp(finalColor, 0.0, 1.0);

    float far = 100;
    float near = 1;
    vec4 clip_space_pos = projection * vec4(hitPos,1);
    float ndc_depth = 0.5 * clip_space_pos.z / clip_space_pos.w + 0.5;

    gl_FragDepth = ndc_depth;
    fragColor = vec4(finalColor, 1);
    InstanceID = vec4(passInstanceID);
}

void main() {

    // Kamera in Kamera
    vec4 cam_w = vec4(0,0,0,1);

    // Fragment in Kamera
    vec4 frag_w = passPosition;

    center_v = vec4(view*model*vec4(0,0,0,1)).xyz;

    // Sehstrahl in Welt
    vec3 view_w = normalize((frag_w - cam_w).xyz);

    // Der Impostor ist jetzt quasi in Weltkoordinaten (0,0,0) zentriert (Weltkoordinaten und Impostor/model-Koordinaten decken sich
    // Wenn z.B. eine Kugel gezeichnet werden soll, deren Mittelpunkt dem Ursprung des Impostor entspricht,
    // dann liegt diese jetzt auch im Ursprung der Weltkoordinaten
    // Analog könnte man jetzt auch andere Oberflächen testen, solange man weiß wo diese relativ zum Ursprung des Impostor/Model-Koordinatensystems liegen

    float radius = size;//sphereRadius;

    float a = dot(view_w, -center_v.xyz);
    float b = a * a - length(center_v.xyz) * length(center_v.xyz) + radius * radius;

    if (b < 0)
    {
        discard; // no intersections
    }
    else
    {
        float d = -a - sqrt(b); // just substract (+ lies always behind front point)
        vec3 real_hit_position_cam = d * view_w;
        hit(real_hit_position_cam);
        stop = true;
    }

    // Kugel nicht getroffen, Impostor zeichnen oder verwerfen
    if(!stop)
    {
        discard;
        fragColor = vec4(1,0,0,1);
    }
}

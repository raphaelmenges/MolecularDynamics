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

uniform vec4 lightSrc = vec4(0,100,0,1);
bool stop = false;
bool iterate = false;

layout(depth_greater) out float gl_FragDepth;
vec3 view_w;
void hit(vec3 hitPos)
{
    // Normale Berechnen und dann alles von Welt wieder in View-Koordinaten transformieren
    vec4 normal = normalize(vec4(hitPos -center_v,0));
//    hitPos = vec4(view * model * vec4(hitPos,1)).xyz;
//    normal =  transpose(inverse(view * model)) * normal;

    // Beleuchtung
    vec3 light_v = vec3(view * lightSrc).xyz;
    vec3 L = normalize(vec3(light_v - hitPos.xyz));
    vec3 finalColor = passColor.xyz * max(dot(normal.xyz,L), 0.0);
    finalColor = clamp(finalColor, 0.0, 1.0);

    float far = 100;
    float near = 1;
    vec4 clip_space_pos = projection * vec4(hitPos,1);
    float ndc_depth = 0.5 * clip_space_pos.z / clip_space_pos.w + 0.5;

    gl_FragDepth = ndc_depth;
    //gl_FragDepth = ((ndc_depth + (far + near)/(far - near)) * (far - near))/-2;
    fragColor = vec4(finalColor, 1);
    InstanceID = vec4(passInstanceID);
}

void main() {

    // Kamera in Welt
    vec4 cam_w = /*inverse(view * model) **/ vec4(0,0,0,1);

    // Fragment in Welt
    vec4 frag_w = /*inverse(view * model) **/ passPosition;

    center_v = vec4(view*model*vec4(0,0,0,1)).xyz;//vec4(view * vec4(center,1)).xyz;



    // Sehstrahl in Welt
    //vec3 view_w = normalize((frag_w - cam_w).xyz);
    view_w = vec4(vec4(0,0,-1,0)).xyz;
    // Der Impostor ist jetzt quasi in Weltkoordinaten (0,0,0) zentriert (Weltkoordinaten und Impostor/model-Koordinaten decken sich
    // Wenn z.B. eine Kugel gezeichnet werden soll, deren Mittelpunkt dem Ursprung des Impostor entspricht,
    // dann liegt diese jetzt auch im Ursprung der Weltkoordinaten
    // Analog könnte man jetzt auch andere Oberflächen testen, solange man weiß wo diese relativ zum Ursprung des Impostor/Model-Koordinatensystems liegen

    // Schrittweise durch den Impostor laufen und auf Oberfläche testen (ToDo: pq-Formel)
    float stepSize = 0.1;
    vec3 stepPos = frag_w.xyz;
    float error = 0.1;
    float radius = size/2;//sphereRadius;
    if (iterate)
    for (int i = 0; i < 10; i++)
    {
        // testen ob der gefundene Punkt noch im Impostor liegt
        // dazu müssen Ausmaße in xyz bekannt sein, hier -1..1
//        if( abs(stepPos.x) > radius + error || abs(stepPos.y) > radius  + error || abs(stepPos.z) > radius  + error)
//            break;

        // Abstand Kugel zu aktuellem Testpunkt
        float dist = abs(length(stepPos- center_v));



        // Auf korrekten Abstand testen
        if (abs(dist - radius) < error)
        {
            // getroffen
            hit(stepPos);
            stop = true;
            break;
        }
        stepPos += view_w * stepSize;
    }
    else
    {
        // center_v je nach pixelkoordinate verschieben
//        center_v.x -= 60.0/1280.0*(gl_FragCoord.x - 1280.0/2.0) ;
//        center_v.y += 60.0/720.0*(gl_FragCoord.y - 720.0/2.0);

        // Position des Pixels (Bildebene) in Welt
        float width = 1280.0;
        float height = 720.0;
        float x = (2.0f * gl_FragCoord.x) / width - 1.0f;
        float y = 1.0f - (2.0f * gl_FragCoord.y) / height;
        float z = 1.0f;
        vec3 ray_nds = vec3 (x, y, z);
        vec4 ray_clip = vec4 (ray_nds.xy, 1.0, 1.0);
        vec4 ray_eye = inverse (projection) * ray_clip;

//ray_eye.x = ray_eye.x;
//ray_eye.y = -ray_eye.y;

        center_v.x = center_v.x - 2*ray_eye.x;
        center_v.y = center_v.y + 2*ray_eye.y;

        //view_w = vec3(center_v.xyz/30);//normalize(passPosition.xyz - ray_eye.xyz);

        float a = dot(view_w, -center_v.xyz);
        float b = a * a - length(center_v.xyz) * length(center_v.xyz) + radius * radius;

        if (b < 0)
        {
            /*fragColor = vec4(view_w,1);//*/discard; // no intersections
        }
        else
        {
            float d = -a - sqrt(b); // just substract (+ lies always behind front point)
            vec3 real_hit_position_cam = d * view_w;
            hit(real_hit_position_cam);
            stop = true;
        }

    }

    // Kugel nicht getroffen, Impostor zeichnen oder verwerfen
    if(!stop)
    {
        //discard;
        fragColor = vec4(view_w,1);
        vec4 clip_space_pos = projection * passPosition;
        float ndc_depth = 0.5 * clip_space_pos.z / clip_space_pos.w + 0.5;
        //gl_FragDepth = ndc_depth;
    }
}

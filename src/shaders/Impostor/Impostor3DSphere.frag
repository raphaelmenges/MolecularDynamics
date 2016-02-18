#version 430

uniform vec4 color;
uniform mat4 view;
uniform mat4 model;
uniform mat4 projection;
uniform float sphereRadius;

in vec4 passPosition;

out vec4 fragColor;

uniform vec4 lightSrc = vec4(0,100,0,1);
bool stop = false;

layout(depth_less) out float gl_FragDepth;

void hit(vec3 hitPos)
{
    // Normale Berechnen und dann alles von Welt wieder in View-Koordinaten transformieren
    vec4 normal = normalize(vec4(hitPos,0));
    hitPos = vec4(view * model * vec4(hitPos,1)).xyz;
    normal =  transpose(inverse(view * model)) * normal;

    // Beleuchtung
    vec3 light_v = vec3(view * lightSrc).xyz;
    vec3 L = normalize(vec3(light_v - hitPos.xyz));
    vec3 finalColor = color.xyz * max(dot(normal.xyz,L), 0.0);
    finalColor = clamp(finalColor, 0.0, 1.0);

    float far = 100;
    float near = 1;
    vec4 clip_space_pos = projection * vec4(hitPos,1);
    float ndc_depth = clip_space_pos.z / clip_space_pos.w;

    gl_FragDepth = ndc_depth;
    fragColor = vec4(finalColor, 1);
}

void main() {

    // Kamera in Welt
    vec4 cam_w = inverse(view * model) * vec4(0,0,0,1);

    // Fragment in Welt
    vec4 frag_w = inverse(view * model) * passPosition;

    // Sehstrahl in Welt
    vec3 view_w = normalize((frag_w - cam_w).xyz);

    // Der Impostor ist jetzt quasi in Weltkoordinaten (0,0,0) zentriert (Weltkoordinaten und Impostor/model-Koordinaten decken sich
    // Wenn z.B. eine Kugel gezeichnet werden soll, deren Mittelpunkt dem Ursprung des Impostor entspricht,
    // dann liegt diese jetzt auch im Ursprung der Weltkoordinaten
    // Analog könnte man jetzt auch andere Oberflächen testen, solange man weiß wo diese relativ zum Ursprung des Impostor/Model-Koordinatensystems liegen

    // Schrittweise durch den Impostor laufen und auf Oberfläche testen (ToDo: pq-Formel)
    float stepSize = 0.01;
    vec3 stepPos = frag_w.xyz;
    float error = 0.01;
    float radius = sphereRadius;
    for (int i = 0; i < 100; i++)
    {
        // testen ob der gefundene Punkt noch im Impostor liegt
        // dazu müssen Ausmaße in xyz bekannt sein, hier -1..1
        if( abs(stepPos.x) > 1 + error || abs(stepPos.y) > 1  + error || abs(stepPos.z) > 1  + error)
            break;

        // Abstand toroidalPoint zu aktuellem Testpunkt
        float dist = abs(length(stepPos));

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

    // Kugel nicht getroffen, Impostor zeichnen oder verwerfen
    if(!stop)
    {
        discard;
        fragColor = vec4(1,0,0,0);
    }
}



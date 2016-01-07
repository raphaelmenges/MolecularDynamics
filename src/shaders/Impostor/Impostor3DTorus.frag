#version 430

uniform vec4 color;
uniform mat4 view;
uniform mat4 model;
uniform mat4 projection;

in vec4 passPosition;

out vec4 fragColor;

uniform vec4 lightSrc = vec4(0,100,0,1);
bool stop = false;

layout(depth_any) out float gl_FragDepth;

void hit(vec3 hitPos, vec3 toroidalPoint)
{
    // Normale Berechnen und dann alles von Welt wieder in View-Koordinaten transformieren
    vec4 normal = normalize(vec4(toroidalPoint - hitPos,0));
    hitPos = vec4(view * model * vec4(hitPos,1)).xyz;
    normal =  transpose(inverse(view * model)) * normal;


    // Beleuchtung
    vec3 light_v = vec3(view * lightSrc).xyz;
    vec3 L = normalize(vec3(light_v - hitPos.xyz));
    vec3 finalColor = color.xyz * max(dot(normal.xyz,L), 0.0);
    finalColor = clamp(finalColor, 0.0, 1.0);

   //gl_FragDepth = hitPos.z / 99.0f; // far plane is at 100, near at 1
   float far = 100;
   float near = 1;
   vec4 clip_space_pos = projection * vec4(hitPos,1);

   float ndc_depth = clip_space_pos.z / clip_space_pos.w;

   //float depth = (((far-near) * ndc_depth) + near + far) / 2.0;
   gl_FragDepth = ndc_depth;
   fragColor = vec4(finalColor, 1);
   //fragColor = vec4(vec3(ndc_depth), 1);
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

    // Schrittweise durch den Impostor laufen und auf Oberfläche testen
    float stepSize = 0.001;
    vec3 stepPos = frag_w.xyz;
    float error = 0.001;
    for (int i = 0; i < 10000; i++)
    {
        // testen ob der gefundene Punkt noch im Impostor liegt
        // dazu müssen Ausmaße in xyz bekannt sein, hier -1..1
        if( abs(stepPos.x) > 1 + error || abs(stepPos.y) > 1 + error || abs(stepPos.z) > 1 + error)
            break;


        // für Torus welcher durch Kreis auf YZ Ebene des Impostors läuft
        // toroidal Radius (Kreis)
        float t_radius = 1.6;
        // poloidal Radius (Kugel auf Kreis)
        float p_radius = 1.3;

        // Testpunkt auf YZ Ebene projizieren
        vec3 stepPos_p = vec3(0, stepPos.yz);

        // nächsten Punkt auf toroidal Kreis finden
        vec3 toroidalPoint = normalize(stepPos_p) * t_radius;

        // Abstand toroidalPoint zu aktuellem Testpunkt
        float dist = abs(length(stepPos - toroidalPoint));

        // Auf korrekten Abstand testen
        if (abs(dist - p_radius) < error)
        {
            // getroffen
            hit(stepPos, toroidalPoint);
            stop = true;
            break;
        }
        stepPos += view_w * stepSize;
    }
    // Kugel nicht getroffen, Impostor zeichnen oder verwerfen
    if(!stop)
    {
        //gl_FragDepth = gl_FragCoord.z; // far plane is at 100, near at 1
        discard;
        fragColor = vec4(1,0,1,0);
    }
}



#version 450

coherent layout(rgba16f) uniform image3D intervalBuffer;
coherent layout(r32ui) uniform uimage2D semaphore;

uniform vec4 color;
uniform mat4 view;
uniform mat4 projection;
uniform float sphereRadius;
uniform int width;
uniform int height;
uniform int perPixelDepth;
uniform vec4 lightSrc = vec4(0,100,0,1);

flat in int passInstanceID;

in vec4 passPosition;
in vec4 passColor;
in float size;
flat in mat4 model;
flat in vec3 center;

out vec4 fragColor;
out vec4 InstanceID;

vec3 center_v;
float d_near;
float d_far;
int ID_near;
int ID_far;

bool discardInterval = false;
ivec2 coord;

layout(depth_greater) out float gl_FragDepth;
vec3 view_w;
void hit(vec3 hitPos)
{
// Normale Berechnen
vec4 normal = normalize(vec4(hitPos -center_v,0));

  // Beleuchtung
  vec3 light_v = vec3(view * lightSrc).xyz;
  vec3 L = normalize(vec3(light_v - hitPos.xyz));
  vec3 finalColor = passColor.xyz * max(dot(normal.xyz,L), 0.0);

    float specularCoefficient = 0.0;
    float materialShininess = 1;
    vec3 materialSpecularColor = vec3(0.5);

    specularCoefficient = pow(max(0.0, dot(-normalize(hitPos).xyz, reflect(-L, normal.xyz))), materialShininess);
    vec3 specular = specularCoefficient * materialSpecularColor;

    finalColor += specular;
    finalColor = clamp(finalColor, 0.0, 1.0);


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
  view_w = vec4(vec4(0,0,-1,0)).xyz;

  float radius = size;//sphereRadius;

  // Position des Pixels (Bildebene) in Welt
  float x = (2.0f * gl_FragCoord.x) / width - 1.0f;
  float y = 1.0f - (2.0f * gl_FragCoord.y) / height;
  float z = 1.0f;
  vec3 ray_nds = vec3 (x, y, z);
  vec4 ray_clip = vec4 (ray_nds.xy, 1.0, 1.0);
  vec4 ray_eye = inverse (projection) * ray_clip;

  center_v.x = center_v.x - 2*ray_eye.x;
  center_v.y = center_v.y + 2*ray_eye.y;

  float a = dot(view_w, -center_v.xyz);
  float b = a * a - length(center_v.xyz) * length(center_v.xyz) + radius * radius;

  if (b < 0)
  {
  discard; // no intersections
  }
  else
  {
  float d_n = -a - sqrt(b); // just substract (+ lies always behind front point)
  float d_f= -a + sqrt(b); // for back faces
  vec3 real_hit_position_cam_n = d_n * view_w;
  vec3 real_hit_position_cam_f = d_f * view_w;
  hit(real_hit_position_cam_n);

    d_near = d_n;// -real_hit_position_cam_n.z;
    d_far = d_f;//-real_hit_position_cam_f.z;
    }

  // Kugel wurde getroffen -> kritischer Bereich um belegte Intervalle zu updaten
  bool done = false;
  uint locked = 0;
  coord = ivec2(gl_FragCoord.xy);
  while(!done)
  {
  //locked = imageAtomicCompSwap(semaphore, coord, 0u, 1u);
  locked = imageAtomicExchange(semaphore, coord, 1u);
  if (locked == 0)
  {
  // kritischer Bereich
  memoryBarrier();

      // Anzahl der vorhandenen Intervalle aus letzter Stelle in der 3D Textur lesen
      int numIntervals = int(imageLoad(intervalBuffer, ivec3(coord, perPixelDepth-1)).r);
      int newIntervalPosition = -1; // dem neuen Interval ist noch keine Position im Buffer zugeordnet

      ID_near = ID_far = passInstanceID;
      vec4 newInterval = vec4(d_near, d_far, passInstanceID, passInstanceID);

      for (int i = 0; i < numIntervals; i++)
      {
      vec4 currentIntervalToCheck = imageLoad(intervalBuffer, ivec3(coord, i));
      float check_near = currentIntervalToCheck.x;
      float check_far = currentIntervalToCheck.y;
      int check_near_ID = int(currentIntervalToCheck.z);
      int check_far_ID = int(currentIntervalToCheck.w);

        // Fall 0: das neue Interval liegt komplett in einem anderen
        if(d_far <= check_far && d_near >= check_near)
        {
        discardInterval = true;
        break;
        }
        // Fall 1: das neue Interval umschließt ein anderes komplett
        if (d_far > check_far && d_near < check_near)
        {
        // wurde schon ein neuer Platz gefunden?
        if (newIntervalPosition != -1)
        {
        // wenn ja, dann kann das aktuelle Interval mit dem letzten im Buffer überschrieben werden
        vec4 lastInterval = imageLoad(intervalBuffer, ivec3(coord, numIntervals-1));
        imageStore(intervalBuffer, ivec3(coord, i), lastInterval);

            // wenn der neue Eintrag = dem letzten Eintrag ist, Index updaten:
            if((numIntervals-1) == newIntervalPosition)
            newIntervalPosition = i;

            numIntervals--;
            i--;
            continue;
            }
            else
            {
            // wenn nicht, kann das neue Interval an die aktuelle Stelle geschrieben werden
            imageStore(intervalBuffer, ivec3(coord, i), newInterval);
            newIntervalPosition = i; // neue Position gefunden
            continue;
            }
            }

        // Fall 2: das neue Interval überschneidet sich mit einem anderen "near"
        if ((d_far > check_near) && (check_near > d_near))
        {
        // wurde schon ein neuer Platz gefunden?
        if (newIntervalPosition != -1)
        {
        // wenn ja, dann kann das aktuelle Interval das neue Interval aktualisieren und mit dem letzten im Buffer überschrieben werden
        // aktualisieren
        d_far = check_far;
        ID_far = check_far_ID;
        newInterval = vec4(d_near, d_far, ID_near, check_far_ID);
        imageStore(intervalBuffer, ivec3(coord, newIntervalPosition), newInterval);

            // alten Eintrag löschen
            vec4 lastInterval = imageLoad(intervalBuffer, ivec3(coord, numIntervals-1));
            imageStore(intervalBuffer, ivec3(coord, i), lastInterval);

            // wenn der neue Eintrag = dem letzten Eintrag ist, Index updaten:
            if((numIntervals-1) == newIntervalPosition)
            newIntervalPosition = ++i;
            numIntervals--;
            i--;
            continue;
            }
            else
            {
            // wenn nicht, kann das neue Interval die aktuelle Stelle aktualisieren
            d_far = check_far;
            ID_far = check_far_ID;
            newInterval = vec4(d_near, d_far, ID_near, check_far_ID);
            imageStore(intervalBuffer, ivec3(coord, i), newInterval);
            newIntervalPosition = i; // neue Position gefunden
            continue;
            }
            }

        // Fall 3: das neue Interval überschneidet sich mit einem anderen "far"
        if ((d_far > check_far) && (check_far > d_near))
        {
        // wurde schon ein neuer Platz gefunden?

          if (newIntervalPosition != -1)
          {
          // wenn ja, dann kann das aktuelle Interval das neue Interval aktualisieren und mit dem letzten im Buffer überschrieben werden
          // aktualisieren
          d_near = check_near;
          ID_near = check_near_ID;
          newInterval = vec4(d_near, d_far, check_near_ID, ID_far);
          imageStore(intervalBuffer, ivec3(coord, newIntervalPosition), newInterval);

            // alten Eintrag löschen
            vec4 lastInterval = imageLoad(intervalBuffer, ivec3(coord, numIntervals-1));
            imageStore(intervalBuffer, ivec3(coord, i), lastInterval);

            // wenn der neue Eintrag = dem letzten Eintrag ist, Index updaten:
            if((numIntervals-1) == newIntervalPosition)
            newIntervalPosition = ++i;
            numIntervals--;
            i--;
            continue;
            }
            else
            {
            // wenn nicht, kann das neue Interval die aktuelle Stelle aktualisieren
            d_near = check_near;
            ID_near = check_near_ID;
            newInterval = vec4(d_near, d_far, check_near_ID, ID_far);
            imageStore(intervalBuffer, ivec3(coord, i), newInterval);
            newIntervalPosition = i; // neue Position gefunden
            continue;
            }
            }
            }

      // wurde eine Position gefunden?
      if(newIntervalPosition == -1 && !discardInterval)
      {
      // wenn nicht, dann trage das neue Interval hinten ein ( wenn es passt )
      if(numIntervals < perPixelDepth - 1) // letzter Platz ist für Anzahl der Intervalle reserviert
      {
      imageStore(intervalBuffer, ivec3(coord, numIntervals), newInterval);
      newIntervalPosition = numIntervals;
      numIntervals++;
      }
      }

      // Anzahl der Intervalle speichern
      imageStore(intervalBuffer, ivec3(coord, perPixelDepth-1), vec4(numIntervals));

      memoryBarrier();

      imageAtomicExchange(semaphore, coord, 0u);

      done = true;
      // "Tiefenkomplexität" Rendern
      // fragColor = vec4(int(imageLoad(intervalBuffer, ivec3(coord, perPixelDepth-1)).r)/10.0);
      }
      }
      }

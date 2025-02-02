import * as THREE from "three";
import { FlyControls } from 'three/addons/controls/FlyControls.js';
import { GUI } from 'three/addons/libs/lil-gui.module.min.js';
import Stats from 'three/addons/libs/stats.module.js';

// Create a scene
const scene = new THREE.Scene();

// Create a camera
const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(175, 100, 400);
camera.lookAt(new THREE.Vector3(0.,150.,0.));

// Create a renderer
const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// Set background color
const fogColor = new THREE.Color(0x101030);
renderer.setClearColor(fogColor, 1);
const crystalColor = new THREE.Color(0x87CEEB);

const controls = new FlyControls(camera, renderer.domElement);
controls.movementSpeed = 50.0;
controls.rollSpeed = Math.PI / 24;
controls.autoForward = false;
controls.dragToLook = true;

const stats = new Stats();
document.body.appendChild(stats.dom);

const gui = new GUI();
gui.add(controls, 'movementSpeed');
gui.add(controls, 'autoForward');
const folder = gui.addFolder('cameraPosition');
folder.add(camera.position, 'x').listen();
folder.add(camera.position, 'y').listen();
folder.add(camera.position, 'z').listen();
gui.close();

// Add directional light
const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(1, 1, 1);
scene.add(light);

// Create a ray marching plane
const geometry = new THREE.PlaneGeometry();
const material = new THREE.ShaderMaterial();
const rayMarchPlane = new THREE.Mesh(geometry, material);

// Get the width and height of the near plane
const nearPlaneWidth = camera.near * Math.tan(THREE.MathUtils.degToRad(camera.fov / 2)) * camera.aspect * 2;
const nearPlaneHeight = nearPlaneWidth / camera.aspect;

// Scale the ray marching plane
rayMarchPlane.scale.set(nearPlaneWidth, nearPlaneHeight, 1);

// Add uniforms
const uniforms = {
  u_eps: { value: 0.001 },
  u_maxDis: { value: 1000 },
  u_maxSteps: { value: 120 },

  u_universeRadius: { value: 450. }, // less than 1000
  u_fogColor: { value: fogColor },
  u_crystalColor: { value: crystalColor },

  u_camPos: { value: camera.position },
  u_camToWorldMat: { value: camera.matrixWorld },
  u_camInvProjMat: { value: camera.projectionMatrixInverse },

  u_lightDir: { value: light.position },
  u_lightColor: { value: light.color },

  u_diffIntensity: { value: 0.5 },
  u_specIntensity: { value: 3 },
  u_shininess: { value: 16 },

  u_time: { value: 0 },
};
material.uniforms = uniforms;

// define vertex and fragment shaders and add them to the material
const vertCode = `
out vec2 vUv; // to send to fragment shader

void main() {
    // Compute view direction in world space
    vec4 worldPos = modelViewMatrix * vec4(position, 1.0);
    vec3 viewDir = normalize(-worldPos.xyz);

    // Output vertex position
    gl_Position = projectionMatrix * worldPos;

    vUv = uv;
}`

const fragCode = `
precision mediump float;

// From vertex shader
in vec2 vUv;

// From CPU
uniform float u_eps;
uniform float u_maxDis;
uniform int u_maxSteps;

uniform vec3 u_camPos;
uniform mat4 u_camToWorldMat;
uniform mat4 u_camInvProjMat;

uniform float u_universeRadius; // less than 1000
uniform vec3 u_fogColor;
uniform vec3 u_crystalColor;

uniform vec3 u_lightDir;
uniform vec3 u_lightColor;

uniform float u_diffIntensity;
uniform float u_specIntensity;
uniform float u_shininess;

uniform float u_time;

vec3 universeCenter(vec3 p) {
  if (length(p) < 1000.) return vec3(0.,0.,0.);
  return vec3(10000.,0.,0.);
}

bool isInUniverse(vec3 p) {
  if (length(p) < u_universeRadius) return true;
  return (distance(p, vec3(10000.,0.,0.)) < u_universeRadius);
}

vec3 reflectDirectionBackIntoUniverse(vec3 oldp, vec3 newp) {
  // Don't reflect, just bring us to the other edge of the universe
  // (which, conveniently, has the same normal).
  return (newp - oldp);
}

vec3 reflectPositionBackIntoUniverse(vec3 oldp, vec3 newp) {
  vec3 origin = universeCenter(oldp);
  vec3 delta = newp - oldp;
  vec3 incident = normalize(delta);

  // Find the intersection point by binary search.
  // Invariant: intersectionPoint remains inside the universe.
  vec3 intersectionPoint = oldp;
  float absDelta = length(delta);
  while (true) {
    vec3 mid = intersectionPoint + delta;
    if (distance(origin, mid) < u_universeRadius) {
      intersectionPoint = mid;
    }
    delta *= 0.5;
    absDelta *= 0.5;
    if (absDelta < 0.01) {
      break;
    }
  }

  // Don't reflect, just bring us to the other edge of the universe
  // (which, conveniently, has the same normal).
  vec3 reflectedPoint = (origin - intersectionPoint) + incident * distance(newp, intersectionPoint);
  if (origin.x != 0.) {
    return reflectedPoint;
  } else {
    return reflectedPoint + vec3(10000.,0.,0.);
  }
}

int innerShellForRadius(float d) {
  // u_universeRadius is 450.
  // Shells are evenly spaced at 0, 100, 200, 300, 400, -400, -300, -200, -100, -0.
  if (d < 100.) return 0;
  if (d < 200.) return 1;
  if (d < 300.) return 2;
  if (d < 400.) return 3;
  return 4;
}

float radiusForShell(int s) {
  return float(s) * 100.;
}

int numRingsForShell(int s) {
  if (s == 1) return 5;
  if (s == 2) return 7;
  if (s == 3) return 9;
  if (s == 4) return 13;
}

vec3 northPoleForShell(int s) {
  if (s == 1) return vec3(0.,0.,1.);
  if (s == 2) return normalize(vec3(.123, 0., 1.));
  if (s == 3) return normalize(vec3(0., .06, 1.));
  if (s == 4) return normalize(vec3(-.1,-.1,1.));
  return vec3(0.,0.,1.);
}

vec3 colorForTemporalShell(int s) {
  if (s == 0) return vec3(.32,.41,.21);
  if (s == 1) return vec3(.76, .76, .77);
  if (s == 2) return vec3(.75, .00, .00);
  if (s == 3) return vec3(.50, .93, .50);
  if (s == 4) return vec3(.99, .87, .14);
  return vec3(1., 0., 0.);
}

float scene(vec3 p) {
  vec3 origin = universeCenter(p);
  float d = distance(p, origin);

  float best = 100.0;
  if (d + 100.0 >= u_universeRadius) {
    // Proceed cautiously near the boundary of the universe.
    best = 20.0;
  }

  int innerShell = innerShellForRadius(d);
  int outerShell = innerShell + 1;

  for (int i = 0; i <= 1; ++i) {
    int shell = (i == 0) ? innerShell : outerShell;
    if (shell == 0) {
      float distanceToRing = d - 10.;
      if (distanceToRing < best) {
        best = distanceToRing;
      }
      continue;
    }
    float shellRadius = radiusForShell(shell);
    if (shellRadius > u_universeRadius) {
      continue;
    }
    int numRings = numRingsForShell(shell);
    float numRingsp1 = float(numRings) + 1.;
    for (int j = 0; j <= numRings; ++j) {
      float theta = 3.14159265 * ((float(j) + 0.5) / numRingsp1);

      // https://www.geometrictools.com/Documentation/DistanceToCircle3.pdf
      vec3 circleNormal = northPoleForShell(shell);
      vec3 circleCenter = origin + (shellRadius * cos(theta) * circleNormal);
      float circleRadius = shellRadius * sin(theta);
      vec3 delta = p - circleCenter; // P minus C
      float dotND = dot(circleNormal, delta);
      vec3 QmC = delta - (dotND * circleNormal);
      float distanceToCircle;
      if (length(QmC) > 0.) {
        vec3 crossND = cross(circleNormal, delta);
        float radial = length(crossND) - circleRadius;
        distanceToCircle = length(vec3(0., dotND, radial));
      } else {
        distanceToCircle = sqrt(dot(delta, delta) + (circleRadius * circleRadius));
      }
      float distanceToRing = distanceToCircle - 2.;
      if (distanceToRing < best) {
        best = distanceToRing;
      }
    }
  }
  return best;
}

vec3 sceneCol(vec3 p) {
  vec3 origin = universeCenter(p);
  if (origin.x != 0.) {
    if (distance(p, origin) < 50.) return vec3(8., 8., 8.);
    if (distance(p, origin) < 150.) return vec3(3., 3., 3.);
    if (distance(p, origin) < 250.) return vec3(2., 2., 2.);
    if (distance(p, origin) < 350.) return vec3(1., 1., 1.);
    return vec3(1., 1., 1.);
  }
  int shell = int(floor((length(p) + 50.) / 100.));
  return colorForTemporalShell(shell);
}

vec4 rayMarch(vec3 ro, vec3 rd) {
    // Return the hit point (vec3) and the distance travelled (float),
    // packed into a single vec4.

    float d = 0.; // total distance travelled
    float cd; // current scene distance
    vec3 p = ro; // current position of ray

    for (int i = 0; i < u_maxSteps; ++i) { // main loop
        cd = scene(p); // get scene distance
        
        // if we have hit anything or our distance is too big, break loop
        if (cd < u_eps || d >= u_maxDis) break;

        // otherwise, add new scene distance to total distance
        d += cd;

        vec3 newp = p + cd * rd;
        if (!isInUniverse(newp)) {
          // rd = normalize(reflectDirectionBackIntoUniverse(p, newp));
          newp = reflectPositionBackIntoUniverse(p, newp);
        }
        p = newp;
    }

    return vec4(p, d);
}

vec3 normal(vec3 p) {
  vec3 n = vec3(0, 0, 0);
  vec3 e;
  for (int i = 0; i < 4; i++) {
    e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
    n += e * scene(p + e * u_eps);
  }
  return normalize(n);
}

float scene2(vec3 p) {
  vec3 origin = universeCenter(p);
  float d = distance(p, origin);
  return u_universeRadius - d;
}

vec4 rayMarch2(vec3 ro, vec3 rd) {
    // Return the hit point (vec3) and the distance travelled (float),
    // packed into a single vec4.

    float d = 0.; // total distance travelled
    float cd; // current scene distance
    vec3 p = ro; // current position of ray

    for (int i = 0; i < u_maxSteps; ++i) { // main loop
        cd = scene2(p); // get scene distance

        // if we have hit anything or our distance is too big, break loop
        if (cd < u_eps || d >= u_maxDis) break;

        // otherwise, add new scene distance to total distance
        d += cd;

        vec3 newp = p + cd * rd;
        p = newp;
    }

    return vec4(p, d);
}

vec3 normal2(vec3 p) {
  vec3 n = vec3(0, 0, 0);
  vec3 e;
  for (int i = 0; i < 4; i++) {
    e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
    n += e * scene2(p + e * u_eps);
  }
  return normalize(n);
}

void main() {
    // Get UV from vertex shader
    vec2 uv = vUv.xy;

    // Get ray origin and direction from camera uniforms
    vec3 ro = u_camPos;
    vec3 rd = (u_camInvProjMat * vec4(uv*2.-1., 0, 1)).xyz;
    rd = (u_camToWorldMat * vec4(rd, 0)).xyz;
    rd = normalize(rd);
    
    vec3 sceneContribution;
    vec3 crystalContribution;
    float distanceToScene;
    float distanceToCrystal;

    // Contribution of the scene itself.
    if (true) {
      vec4 rm = rayMarch(ro, rd);
      vec3 hp = vec3(rm.x, rm.y, rm.z);
      distanceToScene = rm.a;
      vec3 n = normal(hp); // Get normal of hit point

      vec3 color;
      float fogFactor;
      if (distanceToScene >= u_maxDis) {
        color = u_fogColor;
        fogFactor = 0.0;
      } else {
        vec3 lightDir = u_lightDir;
        if (length(hp) > 1000.) {
          lightDir = normalize(-rd);
        }
        float dotNL = dot(n, lightDir);
        float diff = max(dotNL, 0.0) * u_diffIntensity;
        float spec = pow(diff, u_shininess) * u_specIntensity;
        float ambient = 0.4;
        
        color = u_lightColor * (sceneCol(hp) * (spec + ambient + diff));
        fogFactor = min(max(0.0, pow(0.6, distanceToScene / 200.0)), 1.0);
      }
      sceneContribution = mix(u_fogColor, color, fogFactor);
    }

    // Contribution of the crystal sphere between the worlds.
    if (true) {
      vec4 rm = rayMarch2(ro, rd);
      vec3 hp = vec3(rm.x, rm.y, rm.z);
      distanceToCrystal = rm.a;
      vec3 n = normal(hp); // Get normal of hit point
      float dotNL = dot(n, u_lightDir);
      float diff = max(dotNL, 0.0) * u_diffIntensity;
      float spec = pow(diff, u_shininess) * u_specIntensity;
      float ambient = 0.25;

      vec3 color2 = u_lightColor * (u_crystalColor * (spec + ambient + diff));
      float fogFactor = min(max(0.0, pow(0.6, distanceToCrystal / 200.0)), 1.0);
      crystalContribution = mix(u_fogColor, u_crystalColor, fogFactor);
    }

    if (distanceToScene > distanceToCrystal) {
      gl_FragColor = vec4(mix(crystalContribution, sceneContribution, 0.75), 1);
    } else {
      gl_FragColor = vec4(sceneContribution, 1);
    }
}
`
material.vertexShader = vertCode;
material.fragmentShader = fragCode;

// Add plane to scene
scene.add(rayMarchPlane);

// Needed inside update function
let cameraForwardPos = new THREE.Vector3(0, 0, -1);
const VECTOR3ZERO = new THREE.Vector3(0, 0, 0);

let time = Date.now();
const clock = new THREE.Clock();

function isInUniverse(p) {
  if (p.length() < uniforms.u_universeRadius.value) return true;
  if (p.distanceTo(new THREE.Vector3(10000, 0, 0)) < uniforms.u_universeRadius.value) return true;
  return false;
}
function reflectCameraBackIntoUniverse(oldp, newp, camera) {
  let origin = (oldp.length() < 1000.) ? new THREE.Vector3(0, 0, 0) : new THREE.Vector3(10000, 0, 0);
  let delta = newp.clone();
  delta.sub(oldp);

  // Find the intersection point by binary search.
  // Invariant: intersectionPoint remains inside the universe.
  let intersectionPoint = oldp.clone();
  let absDelta = delta.length();
  while (true) {
    let mid = intersectionPoint.clone();
    mid.add(delta);
    if (isInUniverse(mid)) {
      intersectionPoint.copy(mid);
    }
    delta.multiplyScalar(0.5);
    absDelta *= 0.5;
    if (absDelta < 0.01) {
      break;
    }
  }

  // Don't reflect, just bring us to the other edge of the universe
  // (which, conveniently, has the same normal).
  let incident = newp.clone();
  incident.sub(oldp);
  let reflectedPoint = origin.clone();
  reflectedPoint.sub(intersectionPoint);
  reflectedPoint.add(incident.normalize().multiplyScalar(newp.distanceTo(intersectionPoint)));

  if (origin.x) {
    // reflectedPoint.sub(new THREE.Vector3(10000, 0, 0));
  } else {
    reflectedPoint.add(new THREE.Vector3(10000, 0, 0));
  }

  camera.position.copy(reflectedPoint);
}

// Render the scene
const animate = () => {
  const delta = clock.getDelta();
  requestAnimationFrame(animate);
  
  // Update screen plane position and rotation
  cameraForwardPos = camera.position.clone().add(camera.getWorldDirection(VECTOR3ZERO).multiplyScalar(camera.near));
  rayMarchPlane.position.copy(cameraForwardPos);
  rayMarchPlane.rotation.copy(camera.rotation);

  renderer.render(scene, camera);
  stats.update();
  
  uniforms.u_time.value = (Date.now() - time) / 1000;
  
  let oldPos = camera.position.clone();
  controls.update(delta);
  let newPos = camera.position.clone();
  console.assert(isInUniverse(oldPos));

  if (isInUniverse(oldPos) && !isInUniverse(newPos)) {
    console.log("camera rotation was", camera.rotation);
    console.log("camera position was", camera.position);
    console.log("camera up was", camera.up);

    reflectCameraBackIntoUniverse(oldPos, newPos, camera);

    console.log("camera rotation is now", camera.rotation);
    console.log("camera position is now", camera.position);
    console.log("camera up is now", camera.up);
  }
}
animate();

// Handle window resize
window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();

  const nearPlaneWidth = camera.near * Math.tan(THREE.MathUtils.degToRad(camera.fov / 2)) * camera.aspect * 2;
  const nearPlaneHeight = nearPlaneWidth / camera.aspect;
  rayMarchPlane.scale.set(nearPlaneWidth, nearPlaneHeight, 1);

  if (renderer) renderer.setSize(window.innerWidth, window.innerHeight);
});

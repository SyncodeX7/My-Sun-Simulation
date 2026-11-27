// Cinematic Solar Fire Simulation — Full Front-Face Roaring 3D Fire
// Features:
// - Entire visible face of the Sun is roaring in living 3D liquid solar fire
// - Multi-scale billowing flame currents, licking fire ridges, and churning magma embers
// - Volumetric flame mantle floating directly in front of the visible disk
// - Spectacular looping magnetic prominence arches & eruptive CME torch on the left limb
// - Dancing rim of fire and radial heat streamers
#version 450

layout(location = 0) in  vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    float time;
    float resX;
    float resY;
    float camX;
    float camY;
    float camZ;
    float yaw;
    float pitch;
} pc;

// ---------------------------------------------------------------------------
// High-Precision Procedural Noise & Flame Curl
// ---------------------------------------------------------------------------
float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash2(i);
    float b = hash2(i + vec2(1.0, 0.0));
    float c = hash2(i + vec2(0.0, 1.0));
    float d = hash2(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm2(vec2 p, int octaves) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(cos(0.55), sin(0.55), -sin(0.55), cos(0.55));
    for (int i = 0; i < octaves; ++i) {
        v += a * noise2(p);
        p = rot * p * 2.04 + vec2(1.3, 0.7);
        a *= 0.5;
    }
    return v;
}

float flameTurb2(vec2 p, float speed, float t) {
    vec2 flow = vec2(0.0, -t * speed);
    vec2 q = vec2(fbm2(p + flow, 3), fbm2(p + flow + vec2(4.3, 1.7), 3));
    return fbm2(p + flow + 1.8 * q, 4);
}

float hash3(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash3(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash3(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash3(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash3(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash3(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash3(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash3(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash3(i + vec3(1.0, 1.0, 1.0));

    return mix(mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
               mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y), f.z);
}

float fbm3(vec3 p, int octaves) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < octaves; ++i) {
        v += a * noise3(p);
        p = p * 2.03 + vec3(1.2, 0.7, 0.9);
        a *= 0.5;
    }
    return v;
}

float turb3(vec3 p, int octaves) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < octaves; ++i) {
        v += a * abs(noise3(p) * 2.0 - 1.0);
        p = p * 2.02 + vec3(1.4, 0.8, 1.1);
        a *= 0.5;
    }
    return v;
}

float voronoi3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    float minDist = 1.0;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                vec3 neighbor = vec3(float(x), float(y), float(z));
                vec3 randPoint = vec3(
                    hash3(i + neighbor + vec3(0.0, 0.0, 0.0)),
                    hash3(i + neighbor + vec3(7.1, 3.2, 5.4)),
                    hash3(i + neighbor + vec3(2.5, 8.9, 1.3))
                );
                vec3 diff = neighbor + randPoint - f;
                minDist = min(minDist, length(diff));
            }
        }
    }
    return minDist;
}

// ---------------------------------------------------------------------------
// Roaring Solar Fire Palette
// Rich saturated embers -> blazing flame red -> liquid gold fire -> white-hot crests
// ---------------------------------------------------------------------------
vec3 solarFireColor(float val) {
    vec3 cSpace     = vec3(0.0, 0.0, 0.0);
    vec3 cCrimson   = vec3(0.40, 0.016, 0.002);
    vec3 cDarkEmber = vec3(0.72, 0.075, 0.006);
    vec3 cDeepFlame = vec3(0.92, 0.22, 0.012);
    vec3 cFlameRed  = vec3(1.0, 0.44, 0.028);
    vec3 cGoldFire  = vec3(1.0, 0.72, 0.10);
    vec3 cYellowHot = vec3(1.0, 0.92, 0.32);
    vec3 cWhiteHot  = vec3(1.18, 1.12, 0.88);

    vec3 col = cSpace;
    if (val < 0.08) {
        col = mix(cSpace, cCrimson, smoothstep(0.0, 0.08, val));
    } else if (val < 0.22) {
        col = mix(cCrimson, cDarkEmber, smoothstep(0.08, 0.22, val));
    } else if (val < 0.40) {
        col = mix(cDarkEmber, cDeepFlame, smoothstep(0.22, 0.40, val));
    } else if (val < 0.60) {
        col = mix(cDeepFlame, cFlameRed, smoothstep(0.40, 0.60, val));
    } else if (val < 0.78) {
        col = mix(cFlameRed, cGoldFire, smoothstep(0.60, 0.78, val));
    } else if (val < 0.90) {
        col = mix(cGoldFire, cYellowHot, smoothstep(0.78, 0.90, val));
    } else {
        col = mix(cYellowHot, cWhiteHot, clamp((val - 0.90) / 0.12, 0.0, 1.0));
    }
    return col;
}

// ---------------------------------------------------------------------------
// Main Shader Program
// ---------------------------------------------------------------------------
void main() {
    // Screen coordinates centered at (0, 0)
    vec2 uv = fragUV * 2.0 - 1.0;
    uv.x *= pc.resX / pc.resY;

    float t = pc.time;

    // --- 3D Camera & Ray Setup (WASD / Mouse Controls) ---
    vec3 ro = vec3(pc.camX, pc.camY, pc.camZ);

    float cy = cos(pc.yaw), sy = sin(pc.yaw);
    float cp = cos(pc.pitch), sp = sin(pc.pitch);
    vec3 forward = normalize(vec3(sy * cp, sp, -cy * cp));
    vec3 right = normalize(vec3(cy, 0.0, sy));
    vec3 up = cross(right, forward);

    vec3 rd = normalize(uv.x * right + uv.y * up + 1.8 * forward);

    // --- Ray-Sphere Intersection with Sun ---
    const float R_sun = 0.88;
    float b = dot(ro, rd);
    float c = dot(ro, ro) - R_sun * R_sun;
    float discr = b * b - c;

    // Closest approach to sun center for coronal atmosphere and limb flares
    float tClose = max(-b, 0.0);
    vec3 pClose = ro + tClose * rd;
    float r = length(pClose);

    // Projected coordinates around sun center relative to camera orientation
    vec2 pScreen = vec2(dot(pClose, right), dot(pClose, up));
    float angle = atan(pScreen.y, pScreen.x);
    if (angle < 0.0) angle += 6.2831853;

    const float sunR = R_sun;

    float fireRadiance = 0.0;
    vec3 flareEmissive = vec3(0.0);

    // =========================================================================
    // 1. Coronal Atmosphere & Heat Streamers (Surrounding space)
    // =========================================================================
    float h = max(r - sunR, 0.0);
    float coronaGlow = exp(-7.5 * h) * 0.35 + exp(-1.8 * h) * 0.07;
    float heatRays = fbm2(vec2(angle * 7.0 + t * 0.04, r * 2.2 - t * 0.08), 3);
    coronaGlow *= (0.65 + 0.55 * heatRays);
    fireRadiance += coronaGlow;

    // =========================================================================
    // 2. Leaping Rim of Fire: Dancing Flame Spires around the Perimeter
    // =========================================================================
    float flameWave = flameTurb2(vec2(angle * 32.0, h * 32.0), 1.8, t);
    float flameReach = 0.065 + 0.045 * fbm2(vec2(angle * 6.0, t * 0.25), 3);
    float flameMask = exp(-max(h - 0.002, 0.0) / flameReach);
    fireRadiance += flameMask * pow(flameWave, 1.6) * 0.85;

    // =========================================================================
    // 3. Cinematic Looping Magnetic Prominence Arches (Space Flares)
    // =========================================================================
    // Arch 1: Upper-Right (~2:00 / angle ~ 0.72 rad)
    {
        float dAng = atan(sin(angle - 0.72), cos(angle - 0.72));
        float archApex = 0.16;
        float loopPhase = dAng / 0.14;
        float archProfile = exp(-loopPhase * loopPhase);
        float expectedH = archApex * max(1.0 - loopPhase * loopPhase, 0.0);
        float distToArch = abs(h - expectedH);
        float archThickness = 0.028 * (1.0 + 0.5 * archProfile);
        float archTube = exp(-distToArch * distToArch / (2.0 * archThickness * archThickness));
        float archTurb = turb3(vec3(uv * 18.0, t * 0.12), 3);
        fireRadiance += archTube * archProfile * (0.6 + 0.8 * archTurb) * 0.95;
    }

    // Arch 2: Lower-Left Magnetic Arch (~7:30 / angle ~ 3.90 rad)
    {
        float dAng = atan(sin(angle - 3.90), cos(angle - 3.90));
        float archApex = 0.18;
        float loopPhase = dAng / 0.16;
        float archProfile = exp(-loopPhase * loopPhase);
        float expectedH = archApex * max(1.0 - loopPhase * loopPhase, 0.0);
        float distToArch = abs(h - expectedH);
        float archThickness = 0.032 * (1.0 + 0.4 * archProfile);
        float archTube = exp(-distToArch * distToArch / (2.0 * archThickness * archThickness));
        float archTurb = turb3(vec3(uv * 16.0, t * 0.10), 3);
        fireRadiance += archTube * archProfile * (0.6 + 0.8 * archTurb) * 0.90;
    }

    // Jet 3: Lower-Right (~4:30 / angle ~ 5.30 rad)
    {
        float dAng = atan(sin(angle - 5.30), cos(angle - 5.30));
        float angProf = exp(-dAng * dAng / (2.0 * 0.075 * 0.075));
        float radProf = exp(-h / 0.14);
        float flameTurb = flameTurb2(vec2(dAng * 30.0, h * 30.0), 2.2, t);
        fireRadiance += angProf * radProf * pow(flameTurb, 1.4) * 0.88;
    }

    // Jet 4: Upper-Left (~10:30 / angle ~ 2.45 rad)
    {
        float dAng = atan(sin(angle - 2.45), cos(angle - 2.45));
        float angProf = exp(-dAng * dAng / (2.0 * 0.070 * 0.070));
        float radProf = exp(-h / 0.12);
        float flameTurb = flameTurb2(vec2(dAng * 28.0, h * 28.0), 2.0, t);
        fireRadiance += angProf * radProf * pow(flameTurb, 1.4) * 0.80;
    }

    // =========================================================================
    // 4. The Giant Solar Flare / CME on the Left Limb (~9:00 / angle ~ pi)
    // =========================================================================
    {
        vec2 footpoint = vec2(-sunR, -0.05);
        vec2 delta = pScreen - footpoint;
        float sDist = -delta.x;
        float sTrans = delta.y;

        if (sDist > -0.04) {
            float w = 0.042 + 0.32 * max(sDist, 0.0);
            float transGauss = exp(-(sTrans * sTrans) / (2.0 * w * w));
            float radialFalloff = exp(-max(sDist, 0.0) / 0.30);

            vec2 flameCoord = vec2(sDist * 18.0 - t * 2.8, sTrans * 22.0);
            float flameWave = fbm2(flameCoord, 4);
            float flameTurb = turb3(vec3(pScreen * 20.0, t * 0.6), 3);

            float footGlow = exp(-dot(delta, delta) / (2.0 * 0.038 * 0.038)) * 1.35;

            float flarePlume = transGauss * radialFalloff * (0.6 + 0.9 * flameTurb) * pow(flameWave, 1.2) * 1.55;
            float totalFlare = flarePlume + footGlow;

            fireRadiance += totalFlare;

            vec3 cFoot = vec3(1.20, 1.12, 0.85);
            vec3 cMid  = vec3(1.0, 0.70, 0.12);
            vec3 cTip  = vec3(0.88, 0.20, 0.018);
            vec3 flareCol = mix(cFoot, cMid, clamp(sDist / 0.08, 0.0, 1.0));
            flareCol = mix(flareCol, cTip, clamp((sDist - 0.08) / 0.24, 0.0, 1.0));

            flareEmissive += flareCol * totalFlare * 0.50;
        }
    }

    // =========================================================================
    // 5. The Entire Visible Front Face: Roaring 3D Fire, Billowing Flame Tongues & Magma
    // The entire hemisphere pointing at the camera is visibly ON FIRE!
    // =========================================================================
    if (discr > 0.0) {
        float tHit = -b - sqrt(discr);
        vec3 pWorld = ro + tHit * rd;
        vec3 pSphere = pWorld / R_sun;

        // Dynamic 3D rotation & convective churning
        float rotA = t * 0.025;
        float cA = cos(rotA), sA = sin(rotA);
        vec3 p = vec3(
            cA * pSphere.x - sA * pSphere.z,
            pSphere.y * 0.99 + pSphere.z * 0.14,
            sA * pSphere.x + cA * pSphere.z
        );

        // --- Multi-Layer Billowing Fire across the Front Face ---
        // Fire is advecting and boiling outward and upward
        vec3 fireFlow1 = p * 4.5 - vec3(0.0, t * 0.45, 0.0);
        vec3 fireFlow2 = p * 8.0 + vec3(t * 0.35, -t * 0.25, 0.0);
        vec3 fireFlow3 = p * 16.0 - vec3(0.0, t * 0.70, 0.0);

        // Dynamic 3D curl domain-warp
        vec3 qCurl = vec3(
            turb3(fireFlow1, 3),
            turb3(fireFlow1 + vec3(4.3, 1.7, 2.5), 3),
            turb3(fireFlow1 + vec3(7.1, 5.2, 1.9), 3)
        );
        vec3 pFire = fireFlow1 + 1.8 * qCurl;

        // Licking flame ridges on the front surface
        float flameTongue1 = 1.0 - abs(turb3(pFire, 4) * 2.0 - 1.0);
        float flameTongue2 = 1.0 - abs(turb3(fireFlow2 + 1.2 * qCurl, 3) * 2.0 - 1.0);
        float flameTongue3 = turb3(fireFlow3 + 0.8 * qCurl, 3);

        // Composite multi-octave front-face fire
        float frontFaceFire = 0.52 * flameTongue1 + 0.34 * flameTongue2 + 0.14 * flameTongue3;

        // --- Magma Convection & Burning Ember Veins ---
        float voroCells = voronoi3(p * 20.0 + 0.6 * qCurl);
        float magmaLanes = smoothstep(0.05, 0.40, voroCells);

        // Dark magnetic filaments / trenches (burning charcoal beneath the fire)
        float filTurb = turb3(pFire * 0.9 + vec3(0.0, 0.0, t * 0.02), 4);
        float filament = smoothstep(0.42, 0.68, filTurb);

        // --- Sizzling Golden Flare Ribbons & Active Solar Furnaces ---
        vec3 act1 = normalize(vec3( 0.42,  0.30, 0.86)); // upper-right
        vec3 act2 = normalize(vec3( 0.08, -0.16, 0.98)); // center complex (facing camera!)
        vec3 act3 = normalize(vec3( 0.28, -0.40, 0.87)); // lower ribbon
        vec3 act4 = normalize(vec3(-0.48,  0.10, 0.87)); // left plage
        vec3 act5 = normalize(vec3(-0.85, -0.06, 0.52)); // flare root

        float d1 = max(dot(p, act1), 0.0);
        float d2 = max(dot(p, act2), 0.0);
        float d3 = max(dot(p, act3), 0.0);
        float d4 = max(dot(p, act4), 0.0);
        float d5 = max(dot(p, act5), 0.0);

        float p1 = pow(d1, 16.0) * (0.6 + 0.8 * turb3(pFire * 1.5, 3));
        float p2 = pow(d2, 14.0) * (0.7 + 0.9 * turb3(pFire * 1.4, 3)); // camera-facing active furnace!
        float p3 = pow(d3, 16.0) * (0.6 + 0.8 * turb3(pFire * 1.6, 3));
        float p4 = pow(d4, 20.0) * (0.5 + 0.8 * turb3(pFire * 1.5, 3));
        float p5 = pow(d5, 18.0) * (0.7 + 0.9 * turb3(pFire * 1.8, 3));

        float totalActiveRibbons = p1 + p2 * 1.4 + p3 * 1.2 + p4 + p5 * 1.3;
        float hotKernels = pow(clamp(totalActiveRibbons * 1.25, 0.0, 2.0), 2.8);

        // --- Front-Face Surface Fire Intensity ---
        // Generous fiery base: [0.48..0.76]
        float faceIntensity = 0.48 + 0.28 * (0.65 * frontFaceFire + 0.35 * magmaLanes);

        // Deep charcoal trenches beneath the flames
        faceIntensity = mix(faceIntensity, faceIntensity * 0.32, filament * 0.80);

        // Add roaring active fire ribbons & blazing kernels
        faceIntensity += totalActiveRibbons * 0.24 + hotKernels * 0.26;

        // Front-Face 3D Volumetric Flame Shell (leaping towards the camera!)
        float cameraFacingFlames = turb3(vec3(pSphere.xy * 8.0, pSphere.z * 8.0 - t * 0.6), 3);
        faceIntensity += 0.08 * cameraFacingFlames;

        // Glowing fiery limb ring
        vec3 normal = normalize(pSphere);
        float cosTheta = clamp(dot(normal, -rd), 0.0, 1.0);
        float limbGlow = mix(1.18, 0.94, pow(cosTheta, 0.35));
        faceIntensity *= limbGlow;

        // Anti-aliased edge
        float edgeAA = smoothstep(sunR, sunR - 0.008, r);
        fireRadiance = mix(fireRadiance, faceIntensity, edgeAA);
    }

    // =========================================================================
    // 6. Color Composition & Tonemapping
    // =========================================================================
    vec3 finalColor = solarFireColor(fireRadiance);
    finalColor += flareEmissive;

    // Contrast tonemap: deep black space, roaring fiery front face and flares
    vec3 x = finalColor * 0.95;
    vec3 tonemapped = (x * (2.5 * x + 0.04)) / (x * (2.4 * x + 0.54) + 0.12);

    tonemapped = clamp(tonemapped, 0.0, 1.0);
    outColor = vec4(tonemapped, 1.0);
}

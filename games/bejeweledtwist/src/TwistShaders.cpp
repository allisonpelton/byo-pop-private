// port: Twist's seventeen used shaders (ps_2_0 and vs_2_0 in the exe, made by the resources' init at 0x73ef19) as GLSL ES 3.00
// in the device's convention (framework/GPUInterface.h): each body is its disassembly instruction by instruction, the
// instruction in the trailing comment. c# is uPS[#] or uVS[#] unless the shader's own def line gives it (then a const).
// The vertex shaders clamp oD0 and oD1 at their end, as D3D does; pixel shaders then read vDiffuse and vSpecular as v0, v1.
#include "TwistD3D.h"

namespace Sexy {

namespace {

// buttoncover (0x85dbd8)
const char* kButtonCover = R"GLSL(
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0), r3 = vec4(0.0);
	r1.y = vTex0.y + uPS[0].x;                              // add r1.y, t0.yyyy, c0.xxxx
	r1.x = vTex0.x;                                         // mov r1.x, t0.xxxx
	r0.y = vTex0.y + -uPS[0].x;                             // add r0.y, t0.yyyy, -c0.xxxx
	r0.x = vTex0.x;                                         // mov r0.x, t0.xxxx
	r1 = texture(uSampler0, r1.xy);                         // texld r1, r1, s0
	r2 = texture(uSampler1, r0.xy);                         // texld r2, r0, s1
	r3 = texture(uSampler1, vTex0.xy);                      // texld r3, t0, s1
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r2.w = r2.w * r3.w;                                     // mul r2.w, r2.wwww, r3.wwww
	r0.xyz = mix(r1.xyz, r2.xyz, r2.www);                   // lrp r0.xyz, r2.wwww, r2, r1
	r0.w = r1.w * r0.w + r2.w;                              // mad r0.w, r1.wwww, r0.wwww, r2.wwww
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// cube_p_final (0x85e714)
const char* kCubeP = R"GLSL(
void main() {
	vec4 r0 = vec4(0.0);
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r0 = r0 * vDiffuse;                                     // mul r0, r0, v0
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// grayscale (0x85e854)
const char* kGrayscale = R"GLSL(
const vec4 c1 = vec4(0.531, 0.27, 0.099, 1.0);              // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0);
	r2 = texture(uSampler0, vTex0.xy);                      // texld r2, t0, s0
	r0.w = r2.y * c1.x;                                     // mul r0.w, r2.yyyy, c1.xxxx
	r0.w = r2.x * c1.y + r0.w;                              // mad r0.w, r2.xxxx, c1.yyyy, r0.wwww
	r1.xyz = r2.zzz * c1.zzz + r0.www;                      // mad r1.xyz, r2.zzzz, c1.zzzz, r0.wwww
	r1.w = c1.w;                                            // mov r1.w, c1.wwww
	r0 = mix(r2, r1, uPS[0].xxxx);                          // lrp r0, c0.xxxx, r1, r2
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// logo (0x85de78)
const char* kLogo = R"GLSL(
const vec4 c1 = vec4(2.0, 2.0, 1.0, 1.1);                   // def c1
const vec4 c2 = vec4(-1.0, -1.0, -0.0, 1.3);                // def c2
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0), r3 = vec4(0.0);
	r1 = texture(uSampler1, vTex0.xy);                      // texld r1, t0, s1
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r2.xy = -vTex0.xy + uPS[0].xy;                          // add r2.xy, -t0, c0
	r2.z = uPS[0].z;                                        // mov r2.z, c0.zzzz
	r3.x = dot(r2.xyz, r2.xyz);                             // dp3 r3.x, r2, r2
	r2.w = inversesqrt(r3.x);                               // rsq r2.w, r3.xxxx
	r3.w = 1.0 / r2.w;                                      // rcp r3.w, r2.wwww
	r2.xyz = r2.xyz * r2.www;                               // mul r2.xyz, r2, r2.wwww
	r2.w = r3.w * c1.w;                                     // mul r2.w, r3.wwww, c1.wwww
	r3.xyz = c1.xyz;                                        // mov r3.xyz, c1
	r1.xyz = r1.xyz * r3.xyz + c2.xyz;                      // mad r1.xyz, r1, r3, c2
	r3.w = r3.w * c2.w;                                     // mul r3.w, r3.wwww, c2.wwww
	r1.x = dot(r1.xyz, r2.xyz);                             // dp3 r1.x, r1, r2
	r2.w = 1.0 / r2.w;                                      // rcp r2.w, r2.wwww
	r2.w = clamp(r1.x * r2.w - r3.w, 0.0, 1.0);             // mad r2.w_sat, r1.xxxx, r2.wwww, -r3.wwww
	r0 = r0 * vDiffuse;                                     // mul r0, r0, v0
	r0.xyz = r2.www * r1.www + r0.xyz;                      // mad r0.xyz, r2.wwww, r1.wwww, r0
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// mask (0x85e30c)
const char* kMask = R"GLSL(
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r1 = texture(uSampler1, vTex1.xy);                      // texld r1, t1, s1
	r0 = r0 * r1.wwww;                                      // mul r0, r0, r1.wwww
	r0.w = r0.w * vDiffuse.w;                               // mul r0.w, r0.wwww, v0.wwww
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// rainbow_cube_p_final (0x85eb94)
const char* kRainbowCubeP = R"GLSL(
const vec4 c0 = vec4(0.85, 0.0, 0.0, 0.0);                  // def c0
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0);
	r1 = texture(uSampler1, vTex0.xy);                      // texld r1, t0, s1
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r2.x = vDiffuse.w;                                      // mov r2.x, v0.wwww
	r1.xyz = c0.xxx;                                        // mov r1.xyz, c0.xxxx
	r0.w = r0.w * r2.x;                                     // mul r0.w, r0.wwww, r2.xxxx
	r0.xyz = r0.xyz * r1.xyz + r1.www;                      // mad r0.xyz, r0, r1, r1.wwww
	r1.xyz = vDiffuse.xyz;                                  // mov r1.xyz, v0
	r1.w = c0.x;                                            // mov r1.w, c0.xxxx
	r0 = r0 * r1 + vSpecular;                               // mad r0, r0, r1, v1
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// rewind (0x85db10)
const char* kRewind = R"GLSL(
const vec4 c0 = vec4(0.0, 0.02, 0.59, 0.3);                 // def c0
const vec4 c1 = vec4(0.11, 0.4, 1.0, 0.0);                  // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0);
	r0.y = vTex0.y;                                         // mov r0.y, t0.yyyy
	r1.x = c0.x;                                            // mov r1.x, c0.xxxx
	r1.y = vTex0.y;                                         // mov r1.y, t0.yyyy
	r1 = texture(uSampler0, r1.xy);                         // texld r1, r1, s0
	r0.x = r1.x * c0.y + vTex0.x;                           // mad r0.x, r1.xxxx, c0.yyyy, t0.xxxx
	r0 = texture(uSampler0, r0.xy);                         // texld r0, r0, s0
	r0.w = r0.y * c0.z;                                     // mul r0.w, r0.yyyy, c0.zzzz
	r0.w = r0.x * c0.w + r0.w;                              // mad r0.w, r0.xxxx, c0.wwww, r0.wwww
	r1.w = r0.z * c1.x + r0.w;                              // mad r1.w, r0.zzzz, c1.xxxx, r0.wwww
	r0.w = r1.x + c1.y;                                     // add r0.w, r1.xxxx, c1.yyyy
	r2.w = r1.w * r0.w;                                     // mul r2.w, r1.wwww, r0.wwww
	r1.w = -r0.w + c1.z;                                    // add r1.w, -r0.wwww, c1.zzzz
	r0.w = r0.x * r1.w + r2.w;                              // mad r0.w, r0.xxxx, r1.wwww, r2.wwww
	r0.x = r1.y + r0.w;                                     // add r0.x, r1.yyyy, r0.wwww
	r0.w = r0.y * r1.w + r2.w;                              // mad r0.w, r0.yyyy, r1.wwww, r2.wwww
	r1.w = r0.z * r1.w + r2.w;                              // mad r1.w, r0.zzzz, r1.wwww, r2.wwww
	r0.y = r1.y + r0.w;                                     // add r0.y, r1.yyyy, r0.wwww
	r0.w = c1.z;                                            // mov r0.w, c1.zzzz
	r0.z = r1.y + r1.w;                                     // add r0.z, r1.yyyy, r1.wwww
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// saucer_window_p (0x85e75c)
const char* kSaucerWindowP = R"GLSL(
const vec4 c0 = vec4(-1.0, 1.0, 0.0, 0.0);                  // def c0
void main() {
	vec4 r0 = vec4(0.0);
	r0.w = -vTex1.z + c0.x;                                 // add r0.w, -t1.zzzz, c0.xxxx
	r0.xyz = abs(r0.www);                                   // abs r0.xyz, r0.wwww
	r0.w = c0.y;                                            // mov r0.w, c0.yyyy
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// screen_distort (0x85e7d0)
const char* kScreenDistort = R"GLSL(
const vec4 c0 = vec4(0.50196, 0.1, 0.15, 4.0);              // def c0
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0), r3 = vec4(0.0);
	r0 = texture(uSampler1, vTex1.xy);                      // texld r0, t1, s1
	r0.w = -r0.x + c0.x;                                    // add r0.w, -r0.xxxx, c0.xxxx
	r0.x = r0.w * c0.y + vTex0.x;                           // mad r0.x, r0.wwww, c0.yyyy, t0.xxxx
	r0.w = -r0.y + c0.x;                                    // add r0.w, -r0.yyyy, c0.xxxx
	r3.w = clamp(r0.z * c0.w, 0.0, 1.0);                    // mul r3.w_sat, r0.zzzz, c0.wwww
	r0.y = r0.w * c0.z + vTex0.y;                           // mad r0.y, r0.wwww, c0.zzzz, t0.yyyy
	r1 = texture(uSampler0, r0.xy);                         // texld r1, r0, s0
	r2 = texture(uSampler0, vTex0.xy);                      // texld r2, t0, s0
	r0 = mix(r2, r1, r3.wwww);                              // lrp r0, r3.wwww, r1, r2
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// sidebar_bump_p (0x85e1c0)
const char* kSidebarBumpP = R"GLSL(
const vec4 c1 = vec4(-4.0, -4.0, -2.0, 0.2);                // def c1
const vec4 c2 = vec4(0.0, 0.866, -0.5, -0.95);              // def c2
const vec4 c3 = vec4(0.5, 0.5, 1.0, 0.6);                   // def c3
const vec4 c4 = vec4(0.5, 0.7, 1.0, 0.0);                   // def c4
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0);
	r1 = texture(uSampler1, vTex0.xy);                      // texld r1, t0, s1
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r1.xyz = -r1.xyz + c3.xyz;                              // add r1.xyz, -r1, c3
	r2.xyz = r1.xyz * c1.xyz + vTex1.xyz;                   // mad r2.xyz, r1, c1, t1
	r1.xyz = normalize(r2.xyz);                             // nrm r1.xyz, r2
	r2.x = dot(r1.xyz, c2.xyz);                             // dp3 r2.x, r1, c2
	r0.w = r2.x + r2.x;                                     // add r0.w, r2.xxxx, r2.xxxx
	r1.xyz = r1.xyz * -r0.www + c2.xyz;                     // mad r1.xyz, r1, -r0.wwww, c2
	r1.x = dot(-r1.xyz, -r1.xyz);                           // dp3 r1.x, -r1, -r1
	r0.w = inversesqrt(r1.x);                               // rsq r0.w, r1.xxxx
	r0.w = -r1.z * r0.w;                                    // mul r0.w, -r1.zzzz, r0.wwww
	r1.w = r0.w * c2.w;                                     // mul r1.w, r0.wwww, c2.wwww
	r0.w = max(r1.w, c2.x);                                 // max r0.w, r1.wwww, c2.xxxx
	r1.w = r0.w * r0.w;                                     // mul r1.w, r0.wwww, r0.wwww
	r0.w = r0.w * r1.w;                                     // mul r0.w, r0.wwww, r1.wwww
	r1.w = r0.z * r0.w;                                     // mul r1.w, r0.zzzz, r0.wwww
	r0.w = r2.x * c3.w + r0.x;                              // mad r0.w, r2.xxxx, c3.wwww, r0.xxxx
	r1.xyz = r1.www * c4.xyz;                               // mul r1.xyz, r1.wwww, c4
	r0.w = r0.w + c1.w;                                     // add r0.w, r0.wwww, c1.wwww
	r1.xyz = r0.xyz * r0.www + r1.xyz;                      // mad r1.xyz, r0, r0.wwww, r1
	r0.xyz = -r1.xyz + c3.zzz;                              // add r0.xyz, -r1, c3.zzzz
	r0.xyz = uPS[0].yyy * r0.xyz + r1.xyz;                  // mad r0.xyz, c0.yyyy, r0, r1
	r0.w = uPS[0].z;                                        // mov r0.w, c0.zzzz
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// sidebar_p (0x85df6c)
const char* kSidebarP = R"GLSL(
const vec4 c1 = vec4(0.0, 0.707, -0.707, 1.0);              // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0);
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r2.x = dot(vTex1.xyz, c1.xyz);                          // dp3 r2.x, t1, c1
	r1.xyz = r0.xyz * -r2.xxx + c1.www;                     // mad r1.xyz, r0, -r2.xxxx, c1.wwww
	r1.xyz = r1.xyz * uPS[0].yyy;                           // mul r1.xyz, r1, c0.yyyy
	r0.xyz = r0.xyz * r2.xxx + r1.xyz;                      // mad r0.xyz, r0, r2.xxxx, r1
	r0.w = uPS[0].z;                                        // mov r0.w, c0.zzzz
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// sidebar_tube_p (0x85e008)
const char* kSidebarTubeP = R"GLSL(
const vec4 c1 = vec4(-1.0, 1.0, 0.0, 0.0);                  // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	r0.x = vTex0.x + uPS[0].w;                              // add r0.x, t0.xxxx, c0.wwww
	r0.y = vTex0.y;                                         // mov r0.y, t0.yyyy
	r1 = texture(uSampler1, r0.xy);                         // texld r1, r0, s1
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r0.xyz = r1.xyz + r0.xyz;                               // add r0.xyz, r1, r0
	r0.xyz = r0.xyz + c1.xxx;                               // add r0.xyz, r0, c1.xxxx
	r1.w = -vTex1.z + c1.x;                                 // add r1.w, -t1.zzzz, c1.xxxx
	r0.xyz = r0.www * r0.xyz + c1.yyy;                      // mad r0.xyz, r0.wwww, r0, c1.yyyy
	r1.w = abs(r1.w);                                       // abs r1.w, r1.wwww
	r1.xyz = r0.xyz + r1.www;                               // add r1.xyz, r0, r1.wwww
	r1.w = r0.w + r1.w;                                     // add r1.w, r0.wwww, r1.wwww
	r0.xyz = -r1.xyz + c1.yyy;                              // add r0.xyz, -r1, c1.yyyy
	r0.w = min(r1.w, c1.y);                                 // min r0.w, r1.wwww, c1.yyyy
	r0.xyz = uPS[0].yyy * r0.xyz + r1.xyz;                  // mad r0.xyz, c0.yyyy, r0, r1
	r0.w = r0.w * uPS[0].z;                                 // mul r0.w, r0.wwww, c0.zzzz
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// under_dialog (0x85df48)
const char* kUnderDialog = R"GLSL(
const vec4 c1 = vec4(0.5, 0.0, 0.0, 0.0);                   // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0), r2 = vec4(0.0), r3 = vec4(0.0), r4 = vec4(0.0);
	r3.xy = vTex0.xy + uPS[0].yz;                           // add r3.xy, t0, c0.yzxw
	r1.x = vTex0.x + uPS[0].z;                              // add r1.x, t0.xxxx, c0.zzzz
	r1.y = vTex0.y + uPS[0].y;                              // add r1.y, t0.yyyy, c0.yyyy
	r2.x = vTex0.x + uPS[0].w;                              // add r2.x, t0.xxxx, c0.wwww
	r2.y = vTex0.y + uPS[0].y;                              // add r2.y, t0.yyyy, c0.yyyy
	r0.x = vTex0.x + uPS[0].z;                              // add r0.x, t0.xxxx, c0.zzzz
	r0.y = vTex0.y + uPS[0].w;                              // add r0.y, t0.yyyy, c0.wwww
	r4 = texture(uSampler0, r3.xy);                         // texld r4, r3, s0
	r1 = texture(uSampler0, r1.xy);                         // texld r1, r1, s0
	r3 = texture(uSampler0, r2.xy);                         // texld r3, r2, s0
	r0 = texture(uSampler0, r0.xy);                         // texld r0, r0, s0
	r2 = mix(r4, r1, c1.xxxx);                              // lrp r2, c1.xxxx, r1, r4
	r1 = mix(r3, r0, c1.xxxx);                              // lrp r1, c1.xxxx, r0, r3
	r0 = mix(r2, r1, c1.xxxx);                              // lrp r0, c1.xxxx, r1, r2
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// wave (0x85e8f8)
const char* kWave = R"GLSL(
const vec4 c1 = vec4(1.0, 0.0, 0.0, 0.0);                   // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r0.xy = r0.xy * uPS[0].xy;                              // mul r0.xy, r0, c0
	r1.w = r0.y + r0.x;                                     // add r1.w, r0.yyyy, r0.xxxx
	r1.w = r0.z * uPS[0].z + r1.w;                          // mad r1.w, r0.zzzz, c0.zzzz, r1.wwww
	r0.xy = vTex0.xy;                                       // mov r0.xy, t0
	r0.w = r0.w * uPS[0].w + r1.w;                          // mad r0.w, r0.wwww, c0.wwww, r1.wwww
	r0.z = c1.x;                                            // mov r0.z, c1.xxxx
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// whiteout_final (0x85e0d8)
const char* kWhiteoutFinal = R"GLSL(
const vec4 c1 = vec4(1.0, 0.0, 0.0, 0.0);                   // def c1
void main() {
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	r0 = texture(uSampler0, vTex0.xy);                      // texld r0, t0, s0
	r1.xyz = r0.xyz * -vDiffuse.xyz + c1.xxx;               // mad r1.xyz, r0, -v0, c1.xxxx
	r0 = r0 * vDiffuse;                                     // mul r0, r0, v0
	r0.xyz = uPS[0].xxx * r1.xyz + r0.xyz;                  // mad r0.xyz, c0.xxxx, r1, r0
	r0.w = r0.w * uPS[0].y;                                 // mul r0.w, r0.wwww, c0.yyyy
	oC0 = r0;                                               // mov oC0, r0
}
)GLSL";

// cube_v_final (0x85e988)
const char* kCubeV = R"GLSL(
const vec4 c0 = vec4(1.0, 0.0, 0.0, 0.0);                   // def c0
void main() {
	vec4 v0 = aPosition, v1 = vec4(aNormal, 1.0), v2 = aTex0;
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	oPos.x = dot(v0, uVS[8]);                               // dp4 oPos0.x, v0, c8
	oPos.y = dot(v0, uVS[9]);                               // dp4 oPos0.y, v0, c9
	r1.x = dot(v1.xyz, uVS[4].xyz);                         // dp3 r1.x, v1, c4
	r1.y = dot(v1.xyz, uVS[5].xyz);                         // dp3 r1.y, v1, c5
	r1.z = dot(v1.xyz, uVS[6].xyz);                         // dp3 r1.z, v1, c6
	oPos.z = dot(v0, uVS[10]);                              // dp4 oPos0.z, v0, c10
	r0.xyz = normalize(r1.xyz);                             // nrm r0.xyz, r1
	oPos.w = dot(v0, uVS[11]);                              // dp4 oPos0.w, v0, c11
	vDiffuse.xyz = -r0.zzz;                                 // mov oD0.xyz, -r0.zzzz
	vTex1.xyz = r0.xyz;                                     // mov o1.xyz, r0
	vDiffuse.w = c0.x;                                      // mov oD0.w, c0.xxxx
	vSpecular = c0.yyyy;                                    // mov oD1, c0.yyyy
	vTex0.xy = v2.xy;                                       // mov o0.xy, v2
	vDiffuse = clamp(vDiffuse, 0.0, 1.0);                   // D3D clamps oD0 and oD1 to [0,1]
	vSpecular = clamp(vSpecular, 0.0, 1.0);
}
)GLSL";

// rainbow_cube_v_final (0x85e388)
const char* kRainbowCubeV = R"GLSL(
const vec4 c0 = vec4(1.0, 0.75, 0.25, 0.1);                 // def c0
const vec4 c1 = vec4(0.0, 0.0, 0.0, 0.0);                   // def c1
void main() {
	vec4 v0 = aPosition, v1 = vec4(aNormal, 1.0), v2 = aTex0, v3 = aDiffuse;
	vec4 r0 = vec4(0.0), r1 = vec4(0.0);
	r1.x = dot(v1.xyz, uVS[4].xyz);                         // dp3 r1.x, v1, c4
	r1.y = dot(v1.xyz, uVS[5].xyz);                         // dp3 r1.y, v1, c5
	r1.z = dot(v1.xyz, uVS[6].xyz);                         // dp3 r1.z, v1, c6
	oPos.x = dot(v0, uVS[8]);                               // dp4 oPos0.x, v0, c8
	r0.x = dot(r1.xyz, r1.xyz);                             // dp3 r0.x, r1, r1
	oPos.y = dot(v0, uVS[9]);                               // dp4 oPos0.y, v0, c9
	r0.w = inversesqrt(r0.x);                               // rsq r0.w, r0.xxxx
	oPos.z = dot(v0, uVS[10]);                              // dp4 oPos0.z, v0, c10
	r0.xyz = r1.xyz * r0.www;                               // mul r0.xyz, r1, r0.wwww
	oPos.w = dot(v0, uVS[11]);                              // dp4 oPos0.w, v0, c11
	r1.xy = abs(r0.xy);                                     // abs r1.xy, r0
	r0.w = r1.z * r0.w + c0.x;                              // mad r0.w, r1.zzzz, r0.wwww, c0.xxxx
	r1.w = r1.y + r1.x;                                     // add r1.w, r1.yyyy, r1.xxxx
	vTex1.xyz = r0.xyz;                                     // mov o1.xyz, r0
	r1.w = r1.w * v3.w;                                     // mul r1.w, r1.wwww, v3.wwww
	vSpecular.xyz = r1.www * c0.www;                        // mul oD1.xyz, r1.wwww, c0.wwww
	r0.w = r0.w * c0.y + c0.z;                              // mad r0.w, r0.wwww, c0.yyyy, c0.zzzz
	vDiffuse.xyz = r0.www * v3.xyz;                         // mul oD0.xyz, r0.wwww, v3
	vDiffuse.w = v3.w;                                      // mov oD0.w, v3.wwww
	vSpecular.w = c1.x;                                     // mov oD1.w, c1.xxxx
	vTex0.xy = v2.xy;                                       // mov o0.xy, v2
	vDiffuse = clamp(vDiffuse, 0.0, 1.0);                   // D3D clamps oD0 and oD1 to [0,1]
	vSpecular = clamp(vSpecular, 0.0, 1.0);
}
)GLSL";

}  // namespace

const char* TwistShaderSource(TwistShaderId theId)
{
	switch (theId) {
	case TWISTSHADER_BUTTONCOVER: return kButtonCover;
	case TWISTSHADER_CUBE_P: return kCubeP;
	case TWISTSHADER_GRAYSCALE: return kGrayscale;
	case TWISTSHADER_LOGO: return kLogo;
	case TWISTSHADER_MASK: return kMask;
	case TWISTSHADER_RAINBOW_CUBE_P: return kRainbowCubeP;
	case TWISTSHADER_REWIND: return kRewind;
	case TWISTSHADER_SAUCER_WINDOW_P: return kSaucerWindowP;
	case TWISTSHADER_SCREEN_DISTORT: return kScreenDistort;
	case TWISTSHADER_SIDEBAR_BUMP_P: return kSidebarBumpP;
	case TWISTSHADER_SIDEBAR_P: return kSidebarP;
	case TWISTSHADER_SIDEBAR_TUBE_P: return kSidebarTubeP;
	case TWISTSHADER_UNDER_DIALOG: return kUnderDialog;
	case TWISTSHADER_WAVE: return kWave;
	case TWISTSHADER_WHITEOUT_FINAL: return kWhiteoutFinal;
	case TWISTSHADER_CUBE_V: return kCubeV;
	case TWISTSHADER_RAINBOW_CUBE_V: return kRainbowCubeV;
	default: return "";
	}
}

const char* TwistShaderName(TwistShaderId theId)
{
	switch (theId) {
	case TWISTSHADER_BUTTONCOVER: return "buttoncover";
	case TWISTSHADER_CUBE_P: return "cube_p_final";
	case TWISTSHADER_GRAYSCALE: return "grayscale";
	case TWISTSHADER_LOGO: return "logo";
	case TWISTSHADER_MASK: return "mask";
	case TWISTSHADER_RAINBOW_CUBE_P: return "rainbow_cube_p_final";
	case TWISTSHADER_REWIND: return "rewind";
	case TWISTSHADER_SAUCER_WINDOW_P: return "saucer_window_p";
	case TWISTSHADER_SCREEN_DISTORT: return "screen_distort";
	case TWISTSHADER_SIDEBAR_BUMP_P: return "sidebar_bump_p";
	case TWISTSHADER_SIDEBAR_P: return "sidebar_p";
	case TWISTSHADER_SIDEBAR_TUBE_P: return "sidebar_tube_p";
	case TWISTSHADER_UNDER_DIALOG: return "under_dialog";
	case TWISTSHADER_WAVE: return "wave";
	case TWISTSHADER_WHITEOUT_FINAL: return "whiteout_final";
	case TWISTSHADER_CUBE_V: return "cube_v_final";
	case TWISTSHADER_RAINBOW_CUBE_V: return "rainbow_cube_v_final";
	default: return "";
	}
}

}  // namespace Sexy

#include "license_gate_stub.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
struct Vector3 {float x,y,z;};
struct Pawn {std::array<unsigned char,0x1400> data{};bool alive=true;bool m_IsAlive(){return alive;}};
Pawn pawn;
namespace GamePlay {Pawn* get_LocalPawn(){return &pawn;}}
namespace Tools {bool IsPtrValid(void* value){return value==&pawn;}}
struct {struct {bool NoGravity=true;float NoGravityScale=.2f;} ExtraMenu;} Config;
#include "../src/main/jni/Hacks/Feature/NoGravity.h"
int main() {
    auto& gravity=*reinterpret_cast<float*>(pawn.data.data()+NoGravityCfg::LocalGravity);
    auto& air=*reinterpret_cast<float*>(pawn.data.data()+NoGravityCfg::AirControl);
    gravity=.98f;air=.75f;license_test::allowed=false;
    ApplyNoGravityRuntime();assert(gravity==.98f&&air==.75f);
    license_test::allowed=true;ApplyNoGravityRuntime();assert(gravity==.2f&&air!=.75f);
    license_test::allowed=false;ApplyNoGravityRuntime();assert(gravity==.98f&&air==.75f);
    license_test::allowed=true;ApplyNoGravityRuntime();Config.ExtraMenu.NoGravity=false;
    ApplyNoGravityRuntime();assert(gravity==.98f&&air==.75f);
    std::puts("License revocation restores owned persistent gravity/air-control fields");
}

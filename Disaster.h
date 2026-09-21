#pragma once
// A self-contained story layered over the original recovery campaign.
enum DisasterPhase { D_NORMAL, D_PRELUDE, D_WARNING, D_QUAKE, D_LAVA, D_METEORS,
    D_STORM, D_RUIN, D_LAST_CUP, D_COLLAPSE, D_SILENCE };
enum DisasterParticleKind { D_EMBER, D_DUST, D_SMOKE, D_CHIP };
struct DisasterParticle { Vec3 p,v; float age=0,life=0,size=0; int kind=0; };
struct Meteor { Vec3 p,start,target; float age=0,duration=3,size=.5f; bool active=false; };
struct Crater { Vec3 p; float size=0,age=0; };
struct DisasterWorld {
    DisasterPhase phase=D_NORMAL;
    float time=0,totalTime=0,damage=0,intensity=0,quake=0,lava=0,storm=0;
    float impactFlash=0,impactShake=0,meteorTimer=1,eruptionTimer=0,restTime=0,standTime=0;
    float fear=0,lookYaw=0,lookPitch=0,fade=0;
    bool reducedMotion=false,escapePlanned=false,finalSip=false,finalReturned=false;
    int playbackSpeed=1;
    unsigned seed=92171;int particleCursor=0,craterCursor=0,impactCount=0,spawnCount=0;
    Vec3 impactPosition;
    std::array<DisasterParticle,480> particles;
    std::array<Meteor,10> meteors;
    std::array<Crater,12> craters;
} disaster;
constexpr Vec3 BATON_POSITION{-3.6f,1.15f,3.8f};
const std::array<Vec3,5> eruptionPoints{{{-19,0,-6},{12,0,-12},{-14,0,23},{20,0,25},{-25,0,-25}}};
bool disasterActive();
bool startDisaster();
void resetDisaster();
void enterDisasterPhase(DisasterPhase phase);
void updateDisaster(float dt);
bool updateDisasterActor(float dt);
void updateDisasterCamera(float dt);
Vec3 disasterCameraShake();
float disasterGround(float x,float z);
float disasterTreeFall(size_t index);
float disasterBridgeDrop(int plank);
Vec3 disasterCupOffset();
void drawBaton();
void drawDisasterOpaque();
void drawDisasterTransparent();
void drawDisasterHUD();
void setupDisasterLighting();
void setDisasterCapture(int stage);
void spawnMeteor();
void impactMeteor(Meteor& meteor);

// A LAST CUP OF TEA / A Campaign for Nature
// C++17, compatibility OpenGL, GLU and FreeGLUT. All geometry is procedural.
// Sections: model -> simulation -> interaction -> geometry -> world -> HUD -> callbacks.
#include <GL/freeglut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

constexpr float PI=3.14159265359f;
float clamp(float v,float a=0,float b=1){return std::max(a,std::min(b,v));}
float mix(float a,float b,float t){return a+(b-a)*t;}
float smooth(float t){t=clamp(t);return t*t*(3-2*t);}
struct Vec3 {float x=0,y=0,z=0;};
Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 operator*(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
float length(Vec3 a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
struct Color{float r,g,b;};
Color blend(Color a,Color b,float t){return {mix(a.r,b.r,t),mix(a.g,b.g,t),mix(a.b,b.b,t)};}
enum EnvironmentState{HEALTHY,WARNING,DAMAGED,RECOVERING,RESTORED};
enum TreeState{NORMAL,SHAKE,FALLING,REMOVED};
enum Kind{TEA,TREE,RIVER,FLOWERS,BIRD,WARNING_SIGN,PLANT,GARBAGE,SWITCH,HABITAT,HOME_LAMP,FARM_FEED};
struct Tree{Vec3 p;float size,delay;TreeState state=NORMAL;float angle=0;};
struct Interactive{Kind kind;Vec3 p;std::string name,verb;int item=-1;};
struct SmokeParticle{float x,y,z,size,alpha;};
struct World {
    EnvironmentState state=HEALTHY;
    float time=0,damageTime=0,environmentProgress=0,recoveryProgress=0,growth=0,shutdownTime=0;
    bool teaVisited=false,planted=false,pollutionStopped=false,habitat=false,finished=false;
    std::array<bool,3> litter{{false,false,false}};
    std::vector<Tree> trees;
    std::vector<SmokeParticle> smoke;
} world;
float cameraX=0,cameraY=2.4f,cameraZ=5.5f,cameraYaw=0,cameraPitch=-12;
int width=1440,height=900,lastTick=0,selected=-1;
bool keys[256]{},paused=false,mouseCaptured=false,seated=false,showHelp=true,warping=false;
bool renderCheck=false;int captureStage=0;
bool benchmarkMode=false;int benchmarkFrames=0;
std::chrono::steady_clock::time_point benchmarkStart;
float seatTime=0,messageUntil=0;
std::string message="Walk softly. There is a whole world in this moment.";
std::string captureDir;
std::vector<Interactive> interactions;
GLUquadric* quadric=nullptr;
float birdPhase=0,butterflyPhase=0,cloudPhase=0,waterPhase=0,steamPhase=0,leafPhase=0;
#include "LivingWorld.h"
#include "Homestead.h"

float terrain(float x,float z){return .075f*std::sin(x*.23f)*std::cos(z*.19f);}
Vec3 camera(){return {cameraX,cameraY,cameraZ};}
int actions(){return int(world.planted)+int(world.pollutionStopped)+int(world.habitat)+int(world.litter[0])+int(world.litter[1])+int(world.litter[2]);}
float damage(){return world.environmentProgress*(1-world.recoveryProgress);}
float ecology(float threshold){
    if(world.state==RECOVERING || world.state==RESTORED)
        return smooth((world.recoveryProgress-threshold+.12f)/.20f);
    return 1-world.environmentProgress;
}
void say(const std::string& text,float seconds=6){message=text;messageUntil=world.time+seconds;}
void reset(){
    world=World{};
    // Fixed placement makes the world and tests reproducible; every tree has a unique fall delay.
    const Vec3 positions[]={{-7,0,-5},{7,0,-7},{-16,0,-4},{14,0,3},{-21,0,-15},{3,0,-19},
        {15,0,-24},{-26,0,3},{26,0,3},{-9,0,23},{9,0,25},{-22,0,29},{22,0,28},
        {-29,0,-25},{-14,0,-29},{30,0,-31},{-32,0,20},{31,0,20},{0,0,33},{-22,0,38},{23,0,38}};
    for(int i=0;i<21;i++)world.trees.push_back({positions[i],.8f+(i%4)*.17f,float(i%7)*1.5f});
    interactions={{TEA,{0,1.25f,0},"The tea table","Sit and drink tea"},
        {TREE,{-7,1,-5},"A living tree","Observe tree"},
        {RIVER,{-4,1,8},"The river","Observe water"},
        {FLOWERS,{5,1,4},"Wildflower meadow","Inspect flowers"},
        {BIRD,{4,1,22},"Birdwatching perch","Observe birds"},
        {WARNING_SIGN,{12,1,-8},"A warning from the valley","Investigate"},
        {PLANT,{-9,1,1},"Planting circle","Plant a tree"},
        {GARBAGE,{-5,1,8},"Riverbank litter","Collect garbage",0},
        {GARBAGE,{6,1,8},"Discarded bottles","Collect garbage",1},
        {GARBAGE,{5,1,19},"Downstream waste","Collect garbage",2},
        {SWITCH,{21,1,-15},"Factory shutoff","Stop pollution"},
        {HABITAT,{-12,1,23},"Wildlife refuge","Restore habitat"},
        {HOME_LAMP,{-15.3f,1.5f,-13.5f},"Home lighting","Toggle warm lights"},
        {FARM_FEED,{18.3f,1,-6.4f},"Cow feed trough","Put out fresh hay"}};
    cameraX=0;cameraY=2.4f;cameraZ=5.5f;cameraYaw=0;cameraPitch=-12;
    paused=false;seated=false;seatTime=0;selected=-1;
    std::fill(std::begin(keys),std::end(keys),false);
    birdPhase=butterflyPhase=cloudPhase=waterPhase=steamPhase=leafPhase=0;
    resetHomestead();resetLivingWorld();
    say("Walk softly. There is a whole world in this moment.",8);
}
bool taskTime(){return world.state==DAMAGED || world.state==RECOVERING;}
bool available(const Interactive& o){
    switch(o.kind){
    case PLANT:return taskTime()&&!world.planted;
    case GARBAGE:return taskTime()&&!world.litter[o.item];
    case SWITCH:return taskTime()&&!world.pollutionStopped;
    case HABITAT:return taskTime()&&!world.habitat;
    case WARNING_SIGN:return world.state==HEALTHY;
    case HOME_LAMP:return insideHouse(human.position.x,human.position.z);
    default:return true;
    }
}
int nearest(){
    if(human.action!=FREE_WALK)return -1;
    float best=3.15f;int index=-1;
    for(size_t i=0;i<interactions.size();i++)if(available(interactions[i])){
        float d=length(interactionOrigin()-interactions[i].p);
        if(d<best){best=d;index=int(i);}
    }
    return index;
}
bool box(float x,float z,float cx,float cz,float sx,float sz){return std::abs(x-cx)<sx && std::abs(z-cz)<sz;}
bool checkCollision(float x,float z){
    if(std::abs(x)>36 || z>40 || z< -34)return true; // Mountain foothills bound the playable valley.
    if(z>9 && z<18 && std::abs(x)>1.5f)return true; // River crossed only on the bridge.
    if(z>9 && z<18 && std::abs(x)>1.25f)return true; // Rail clearance.
    if(box(x,z,0,0,1.85f,1.2f)||houseCollision(x,z)||farmCollision(x,z)||cowCollision(x,z)||box(x,z,26,-22,5,4))return true;
    const Vec3 rocks[]={{-18,0,7},{18,0,6},{-18,0,21},{16,0,-14}};
    for(auto p:rocks)if(std::hypot(x-p.x,z-p.z)<1.6f)return true;
    for(const auto& t:world.trees)if(t.state!=REMOVED && std::hypot(x-t.p.x,z-t.p.z)<.62f*t.size+.28f)return true;
    return false;
}
void updatePlayer(float dt){
    updateHuman(dt);
}
void updateTrees(float){
    for(size_t i=0;i<world.trees.size();i++){
        auto& t=world.trees[i];
        if(i%3==2){t.state=NORMAL;t.angle=0;continue;} // Some trees remain as bare habitat silhouettes.
        if(world.state==RECOVERING||world.state==RESTORED){
            if(world.recoveryProgress>.4f){t.state=NORMAL;t.angle=0;}continue;
        }
        float age=world.damageTime-t.delay-3;
        if(world.state==HEALTHY||age<0){t.state=NORMAL;t.angle=0;}
        else if(age<2){t.state=SHAKE;t.angle=std::sin(age*23)*3;}
        else if(age<6){t.state=FALLING;t.angle=smooth((age-2)/4)*88;}
        else {t.state=REMOVED;t.angle=88;}
    }
}
void updateEnvironment(float dt){
    if(world.state==WARNING){world.damageTime+=dt;world.environmentProgress=clamp(world.damageTime/24);
        if(world.environmentProgress>=1){world.state=DAMAGED;say("The valley is hurting. Six small actions can bring it back.",9);}}
}
void updateRecovery(float dt){
    if(taskTime()){
        float target=actions()/6.0f;
        world.recoveryProgress=std::min(target,world.recoveryProgress+dt*.065f);
        if(actions()>0)world.state=RECOVERING;
        if(actions()==6 && world.recoveryProgress>=.9999f){world.recoveryProgress=1;world.state=RESTORED;
            say("Life has returned. Go back to the table for one more cup.",10);}
    }
    if(world.planted)world.growth=std::min(1.0f,world.growth+dt*.13f);
}
void updateBirds(float dt){birdPhase+=dt;}
void updateButterflies(float dt){butterflyPhase+=dt;}
void updateClouds(float dt){cloudPhase+=dt*(.22f+windStrength*.65f);}
void updateWater(float dt){waterPhase+=dt*(1+rainAmount()*.9f);}
void updateSteam(float dt){steamPhase+=dt;}
void updateFallingLeaves(float dt){leafPhase+=dt;}
void updateSmoke(float){
    world.smoke.clear();
    // Turning off the source stops new emissions; its remaining plume clears in a few seconds.
    float strength=world.environmentProgress*(world.pollutionStopped?std::exp(-(world.time-world.shutdownTime)/2.5f):1);
    for(int i=0;i<28;i++){
        float age=std::fmod(world.time*.42f+i*.22f,6.2f);
        world.smoke.push_back({28+age*(.5f+windStrength)+std::sin(i*2.1f+world.time)*.15f,8+age*1.4f,-22+std::sin(age)*.5f,
            .45f+age*.18f,strength*(1-age/6.2f)*.28f});
    }
}
void update(float dt){
    if(paused)return;
    world.time+=dt;updateAtmosphere(dt);updateHomestead(dt);updatePlayer(dt);updateEnvironment(dt);updateRecovery(dt);updateTrees(dt);
    updateBirds(dt);updateButterflies(dt);updateClouds(dt);updateWater(dt);updateSteam(dt);updateSmoke(dt);updateFallingLeaves(dt);
    selected=nearest();
}
bool interact(int index){
    if(paused || human.action!=FREE_WALK || index<0 || index>=int(interactions.size()))return false;
    const auto& o=interactions[index];
    if(!available(o)||length(interactionOrigin()-o.p)>=3.15f)return false;
    switch(o.kind){
    case HOME_LAMP:homestead.lampOn=!homestead.lampOn;say(homestead.lampOn?"Warm lights on.":"Home lights off.");break;
    case FARM_FEED:homestead.feedTime=12;say("Fresh hay for the cows. A quiet moment on the farm.");break;
    case TEA:
        return beginTea();
    case TREE:say("A tree is more than wood. It is shelter, oxygen, shade and life.");break;
    case RIVER:say("Clean water is not endless.");break;
    case FLOWERS:say("A healthy ecosystem is made of countless small lives.");break;
    case BIRD:say("Listen to the wings above us. Every species needs a place to belong.");break;
    case WARNING_SIGN:
        if(!world.teaVisited)say("First, enjoy the tea. Remember how this place feels.");
        else {world.state=WARNING;say("An upstream factory has opened. Watch what happens to the valley.",9);}break;
    case PLANT:world.planted=true;say("A seed today. Shelter tomorrow.");break;
    case GARBAGE:world.litter[o.item]=true;say("One less piece of waste. One more chance for the river.");break;
    case SWITCH:world.pollutionStopped=true;world.shutdownTime=world.time;say("Pollution stopped. The air can begin to heal.");break;
    case HABITAT:world.habitat=true;say("A refuge for small lives. The meadow will bloom again.");break;
    }
    return true;
}

// ------------------------------- BASIC GEOMETRY -------------------------------
void material(Color c,float shine=0,float alpha=1){
    glColor4f(c.r,c.g,c.b,alpha);
    GLfloat spec[]={shine,shine,shine,1};glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec);
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shine>0?56:0);
}
void cube(float x,float y,float z,float sx,float sy,float sz,Color c){
    glPushMatrix();glTranslatef(x,y,z);glScalef(sx,sy,sz);material(c);glutSolidCube(1);glPopMatrix();
}
void sphere(float x,float y,float z,float sx,float sy,float sz,Color c,float alpha=1){
    glPushMatrix();glTranslatef(x,y,z);glScalef(sx,sy,sz);material(c,0,alpha);glutSolidSphere(1,12,8);glPopMatrix();
}
void cylinder(float radius,float top,float h,Color c){
    glPushMatrix();glRotatef(-90,1,0,0);material(c);gluCylinder(quadric,radius,top,h,16,1);
    glPushMatrix();glTranslatef(0,0,h);gluDisk(quadric,0,top,16,1);glPopMatrix();glPopMatrix();
}
void disk(float x,float y,float z,float radius,Color c,float alpha=1){
    glPushMatrix();glTranslatef(x,y,z);glRotatef(-90,1,0,0);material(c,0,alpha);gluDisk(quadric,0,radius,32,1);glPopMatrix();
}
void ring(float x,float y,float z,float radius,Color c){
    glDisable(GL_LIGHTING);glColor3f(c.r,c.g,c.b);glLineWidth(2);glBegin(GL_LINE_LOOP);
    for(int i=0;i<48;i++){float a=i*2*PI/48;glVertex3f(x+std::cos(a)*radius,y,z+std::sin(a)*radius);}glEnd();
    glLineWidth(1);glEnable(GL_LIGHTING);
}
void shadow(float x,float z,float sx,float sz){
    glEnable(GL_BLEND);glDepthMask(GL_FALSE);glDisable(GL_LIGHTING);
    glPushMatrix();glTranslatef(x,terrain(x,z)+.09f,z);glScalef(sx,1,sz);disk(0,0,0,1,{.06f,.10f,.08f},.18f);glPopMatrix();
    glEnable(GL_LIGHTING);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}
Color wood{.30f,.18f,.11f},lightWood{.56f,.37f,.21f},cream{.94f,.89f,.72f},teal{.15f,.40f,.34f};

void drawGround(){
    Color green=groundColor();
    for(int z=-45;z<45;z+=2){glBegin(GL_TRIANGLE_STRIP);
        for(int x=-48;x<=48;x+=2)for(int k=1;k>=0;k--){float zz=float(z+k*2);
            float shade=.95f+.055f*std::sin(x*1.3f+zz*.7f);glColor3f(green.r*shade,green.g*shade,green.b*shade);
            float dx=.075f*.23f*std::cos(x*.23f)*std::cos(zz*.19f),dz=-.075f*.19f*std::sin(x*.23f)*std::sin(zz*.19f);
            glNormal3f(-dx,1,-dz);glVertex3f(float(x),terrain(float(x),zz),zz);
        }glEnd();}
    // Warm gravel footpath, with separate stretches leading across the bridge.
    material(blend({.62f,.54f,.38f},{.80f,.86f,.87f},atmosphere.seasons[WINTER]*.75f));
    for(int z=-12;z<33;z++){if(z>=9&&z<18)continue;glBegin(GL_QUADS);glNormal3f(0,1,0);
        glVertex3f(-1.1f,.10f,float(z));glVertex3f(-1.1f,.10f,float(z+1));glVertex3f(1.1f,.10f,float(z+1));glVertex3f(1.1f,.10f,float(z));glEnd();}
    // Grass blades use 3D triangles, with a small time-dependent wind offset.
    material(blend(groundColor(),{.52f,.40f,.19f},(1-ecology(.2f))*.7f));
    glBegin(GL_TRIANGLES);glNormal3f(0,1,0);
    for(int i=0;i<850;i++){
        float x=float((i*97)%710)/10-35.5f,z=float((i*53)%700)/10-32;
        if(std::abs(x)<2 || (z>8&&z<19) || homeFloorHeight(x,z)>=0 || box(x,z,25,-6.9f,4.6f,2.3f))continue;
        float y=terrain(x,z)+.02f,w=std::sin(world.time*(1.3f+windStrength)+i)*(.04f+windStrength*.10f);
        glVertex3f(x-.07f,y,z);glVertex3f(x+.07f,y,z);glVertex3f(x+w,y+.25f+(i%3)*.08f,z+.03f);
    }glEnd();
}
void drawMountain(float x,float z,float radius,float h){
    glPushMatrix();glTranslatef(x,-.5f,z);glRotatef(-90,1,0,0);material({.35f,.43f,.40f});glutSolidCone(radius,h,7,1);
    glTranslatef(0,0,h*.65f);material({.79f,.83f,.74f});glutSolidCone(radius*.36f,h*.37f,7,1);glPopMatrix();
}
void drawDeadTree(float x,float y,float z){
    glPushMatrix();glTranslatef(x,y,z);cylinder(.23f,.12f,3.4f,wood);
    for(int i=0;i<3;i++){glPushMatrix();glTranslatef(0,1.8f+i*.4f,0);glRotatef(float(i*120),0,1,0);glRotatef(45,0,0,1);cylinder(.10f,.035f,1.1f,wood);glPopMatrix();}glPopMatrix();
}
void drawTree(float x,float y,float z,float scale){
    glPushMatrix();glTranslatef(x,y,z);glScalef(scale,scale,scale);
    cylinder(.28f,.14f,3.7f,wood);
    float alive=ecology(.4f);Color leaves=foliageColor();
    glPushMatrix();glTranslatef(0,2.1f,0);glRotatef(std::sin(world.time*(1.25f+windStrength)*.9f+x)*(1+windStrength*3),0,0,1);
    if(alive>.12f){
        sphere(0,1.5f,0,1.4f,1.7f,1.3f,leaves);sphere(-.85f,.65f,.15f,.9f,1.2f,1,leaves);
        sphere(.75f,1.1f,.1f,1,1.3f,1,blend(leaves,{.42f,.55f,.25f},.25f));
        float snow=atmosphere.seasons[WINTER];
        if(snow>.05f){Color white=blend(leaves,{.91f,.95f,.95f},snow);
            sphere(0,2.7f,0,1.12f,.55f* snow,1.06f,white);sphere(-.85f,1.45f,.15f,.73f,.43f*snow,.8f,white);
            sphere(.75f,2.12f,.1f,.79f,.4f*snow,.78f,white);}
    }else{for(int j=0;j<3;j++){glPushMatrix();glRotatef(float(j*120),0,1,0);glRotatef(48,0,0,1);cylinder(.10f,.03f,1.5f,wood);glPopMatrix();}}
    glPopMatrix();glPopMatrix();
}
void drawHouse(){drawHouseExterior();drawHouseInterior();}
void drawTeaCup(){
    glPushMatrix();glTranslatef(.12f,1.37f,0);
    disk(0,.025f,0,.34f,cream); // Saucer and raised rim.
    glPushMatrix();glRotatef(90,1,0,0);material(cream,.65f);glutSolidTorus(.025,.31,8,32);glPopMatrix();
    glPopMatrix(); // The saucer stays on the table when the cup is lifted.
    glPushMatrix();glTranslatef(human.cupPosition.x,human.cupPosition.y,human.cupPosition.z);glRotatef(human.cupTilt,1,0,0);
    glScalef(CUP_SCALE,CUP_SCALE,CUP_SCALE);
    // Outer and inner walls, plus a ring at the lip: the cup is hollow, not a capped cylinder.
    glPushMatrix();glRotatef(-90,1,0,0);material(cream,.65f);gluCylinder(quadric,.20,.29,.43,32,1);
    glTranslatef(0,0,.43f);gluDisk(quadric,.25,.29,32,1);glPopMatrix();
    glPushMatrix();glTranslatef(0,.07f,0);glRotatef(-90,1,0,0);gluQuadricOrientation(quadric,GLU_INSIDE);
    gluCylinder(quadric,.17,.25,.36,32,1);gluQuadricOrientation(quadric,GLU_OUTSIDE);glPopMatrix();
    disk(0,.40f,0,.245f,{.40f,.17f,.045f});
    glPushMatrix();glTranslatef(.29f,.23f,0);material(cream,.65f);glutSolidTorus(.048,.15,10,24);glPopMatrix();
    if(seated||selected==0)ring(0,.44f,0,.31f,{.94f,.71f,.32f});
    glPopMatrix();
}
void drawTable(){
    shadow(0,0,2.2f,1.35f);
    for(float x:{-1.35f,1.35f})for(float z:{-.65f,.65f})cube(x,.65f,z,.17f,1.3f,.17f,wood);
    for(int i=0;i<6;i++)cube(0,1.28f,-.75f+i*.30f,3.3f,.16f,.28f,lightWood);
    cube(-.55f,1.38f,0,.07f,.035f,.65f,{.62f,.66f,.63f});
    sphere(-.55f,1.40f,-.33f,.12f,.035f,.18f,{.69f,.74f,.70f});
    cube(.95f,1.385f,.08f,.63f,.025f,.70f,{.84f,.80f,.63f});
    drawTeaCup();
}
void drawChair(){
    glPushMatrix();glTranslatef(0,0,teaActive()?-.78f*human.sitBlend:0);
    cube(0,.68f,2.05f,1.05f,.13f,1,lightWood);
    for(float x:{-.43f,.43f})for(float z:{1.63f,2.47f})cube(x,.34f,z,.1f,.68f,.1f,wood);
    for(float x:{-.43f,.43f})cube(x,1.1f,2.47f,.10f,1.45f,.1f,wood);
    for(float y:{1.15f,1.45f,1.72f})cube(0,y,2.47f,.96f,.17f,.1f,lightWood);
    glPopMatrix();
}
void drawBridge(){
    for(int i=0;i<25;i++)cube(0,.32f,8.3f+i*.43f,3.15f,.18f,.39f,lightWood);
    for(float x:{-1.55f,1.55f}){
        for(int i=0;i<5;i++)cube(x,.92f,8.3f+i*2.5f,.14f,1.5f,.14f,wood);
        cube(x,1.54f,13.3f,.11f,.13f,10.5f,wood);cube(x,.95f,13.3f,.09f,.09f,10.5f,wood);
    }
}
void drawRock(float x,float z,float s){shadow(x,z,s*1.2f,s);sphere(x,.45f*s,z,s,.7f*s,.8f*s,{.46f,.48f,.41f});}
void drawFlower(float x,float z,float s=1){
    float y=terrain(x,z);glPushMatrix();glTranslatef(x,y,z);glScalef(s,s,s);glRotatef(std::sin(world.time*2+x)*(3+windStrength*5),0,0,1);
    cylinder(.022f,.018f,.43f,{.23f,.40f,.17f});
    Color petals=(int(std::abs(x*7+z))%2)?Color{.91f,.68f,.38f}:Color{.81f,.71f,.83f};
    for(int i=0;i<5;i++){float a=i*2*PI/5;sphere(std::cos(a)*.10f,.45f,std::sin(a)*.10f,.08f,.04f,.08f,petals);}
    sphere(0,.47f,0,.055f,.04f,.055f,{.90f,.73f,.25f});glPopMatrix();
}
void drawBird(float x,float y,float z,float phase){
    glPushMatrix();glTranslatef(x,y,z);glRotatef(-phase*23,0,1,0);material({.16f,.21f,.20f});
    sphere(0,0,0,.11f,.11f,.30f,{.16f,.21f,.20f});
    for(int side:{-1,1}){glPushMatrix();glRotatef(std::sin(phase*7)*30*side,0,0,1);glBegin(GL_TRIANGLES);
        glNormal3f(0,1,0);glVertex3f(0,0,.14f);glVertex3f(float(side)*.85f,.07f,.12f);glVertex3f(0,0,-.18f);glEnd();glPopMatrix();}glPopMatrix();
}
void drawButterfly(float x,float y,float z,float phase){
    glPushMatrix();glTranslatef(x,y,z);glRotatef(phase*30,0,1,0);
    for(int side:{-1,1}){glPushMatrix();glRotatef(side*(35+std::sin(phase*11)*32),0,0,1);
        sphere(side*.10f,0,0,.14f,.022f,.18f,{.91f,.63f,.18f});glPopMatrix();}
    sphere(0,0,0,.025f,.025f,.14f,wood);glPopMatrix();
}
void drawFactory(){
    Color brick=blend({.54f,.48f,.39f},{.34f,.34f,.32f},world.environmentProgress);
    cube(26,2,-22,8,4,6,brick);cube(26,4.1f,-22,8.4f,.2f,6.4f,{.24f,.29f,.28f});
    glPushMatrix();glTranslatef(28,4,-22);cylinder(.65f,.5f,4,wood);glPopMatrix();
    for(int i=0;i<4;i++)cube(23+i*1.9f,2.5f,-18.94f,1.1f,.85f,.08f,{.26f,.36f,.35f});
    cube(21,.7f,-15,.14f,1.4f,.14f,wood);cube(21,1.4f,-15,.8f,.9f,.4f,{.24f,.31f,.30f});
    sphere(21,1.55f,-14.74f,.14f,.14f,.08f,world.pollutionStopped?Color{.36f,.76f,.40f}:Color{.91f,.38f,.21f});
    cube(21,1.2f,-14.74f,.12f,.22f,.1f,cream);
}
void drawGarbage(float x,float z,int i){
    for(int j=0;j<5;j++){glPushMatrix();glTranslatef(x+(j%3)*.32f-.3f,.16f,z+(j/3)*.36f);glRotatef(25+j*43,0,1,0);
        if((j+i)%2==0){glRotatef(75,1,0,0);cylinder(.10f,.075f,.35f,{.48f,.63f,.61f});}
        else cube(0,0,0,.24f,.16f,.32f,{.69f,.57f,.39f});
        glPopMatrix();}
}
void drawPlant(){
    disk(-9,.11f,1,.85f,{.30f,.23f,.14f});
    if(world.planted){
        sphere(-9,.13f,1,.09f,.065f,.07f,wood);
        // The growing sapling is green even while mature forest recovery is still low.
        float s=smooth(world.growth);glPushMatrix();glTranslatef(-9,.1f,1);glScalef(s,s,s);
        cylinder(.15f,.055f,2.7f,wood);
        for(int i=0;i<3;i++)sphere((i-1)*.35f,1.8f+i*.25f,0,.65f,.9f,.65f,{.32f,.52f,.22f});
        glPopMatrix();
    }else{cube(-9,.20f,1,.35f,.35f,.12f,{.65f,.49f,.26f});}
}
void drawCampfire(){
    for(int i=0;i<8;i++){float a=i*PI/4;sphere(-5+std::cos(a)*.65f,.17f,-6.5f+std::sin(a)*.65f,.21f,.17f,.20f,{.43f,.43f,.37f});}
    for(int i=0;i<3;i++){glPushMatrix();glTranslatef(-5,.22f,-6.5f);glRotatef(i*60.0f,0,1,0);cube(0,0,0,.17f,.17f,1,wood);glPopMatrix();}
    // A contained campfire, with two animated nested flame cones.
    glDisable(GL_LIGHTING);
    glPushMatrix();glTranslatef(-5,.24f,-6.5f);glRotatef(-90,1,0,0);material({.94f,.41f,.12f});glutSolidCone(.28,.55+.12*std::sin(world.time*6),10,1);
    material({1,.76f,.25f});glutSolidCone(.16,.4,10,1);glPopMatrix();glEnable(GL_LIGHTING);
}
void drawHabitat(){
    cube(-12,.55f,23,1.3f,1.1f,.9f,lightWood);cube(-12,1.2f,23,1.6f,.18f,1.2f,teal);
    sphere(-12,.75f,23.47f,.23f,.23f,.03f,wood);
    if(world.habitat)for(int i=0;i<20;i++)drawFlower(-12+std::sin(i*2.4f)*2,23+std::cos(i*2.4f)*2.1f,.9f);
}
void drawWorldLabel(Vec3 p,const std::string& text,Color c){
    glDisable(GL_LIGHTING);glColor3f(c.r,c.g,c.b);glRasterPos3f(p.x,p.y,p.z);
    for(unsigned char ch:text)glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,ch);
    glEnable(GL_LIGHTING);
}
void drawOpaqueWorld(){
    drawGround();
    for(int i=0;i<11;i++)drawMountain(-52+i*10.0f,-47,10+float(i%3)*3,14+float(i%4)*4);
    for(int i=0;i<6;i++){drawMountain(-49, -25+i*15.0f,12,15+float(i%3)*5);drawMountain(49,-25+i*15.0f,12,17+float(i%3)*4);}
    for(size_t i=0;i<world.trees.size();i++){
        const auto& t=world.trees[i];
        if(t.state==REMOVED){glPushMatrix();glTranslatef(t.p.x,0,t.p.z);cylinder(.3f,.27f,.28f,lightWood);glPopMatrix();continue;}
        shadow(t.p.x,t.p.z,t.size*1.7f,t.size*1.4f);
        glPushMatrix();glTranslatef(t.p.x,terrain(t.p.x,t.p.z),t.p.z);glRotatef(t.angle,1,0,.3f);
        float regrowth=(world.state==RECOVERING && i%3!=2)?smooth((world.recoveryProgress-.4f)/.18f):1;
        drawTree(0,0,0,t.size*std::max(.05f,regrowth));glPopMatrix();
    }
    drawHomesteadPaths();drawFarm();drawHouse();drawShelter();drawTable();drawChair();drawBridge();drawFactory();drawPlant();drawCampfire();drawHabitat();drawHuman();
    drawRock(-18,7,1.2f);drawRock(18,6,1.3f);drawRock(-18,21,1.2f);drawRock(16,-14,1.1f);
    if(ecology(.2f)>.25f)for(int i=0;i<int(85*(1-atmosphere.seasons[WINTER])*(1-.4f*atmosphere.seasons[AUTUMN]));i++){float a=i*2.4f,r=.3f+float(i%10)*.21f;drawFlower(5+std::cos(a)*r,4+std::sin(a)*r,.8f+float(i%3)*.15f);}
    // Warning plaque and birdwatching perch.
    cube(12,1,-8,.13f,2,.13f,wood);cube(12,1.85f,-8,1.75f,1,.13f,{.82f,.66f,.31f});
    drawWorldLabel({11.4f,1.9f,-7.91f},"!  UPSTREAM",wood);
    cube(4,.8f,22,.14f,1.6f,.14f,wood);cube(4,1.6f,22,1.2f,.12f,.16f,lightWood);
    if(world.environmentProgress>.4f)for(const auto& o:interactions)if(o.kind==GARBAGE&&!world.litter[o.item])drawGarbage(o.p.x,o.p.z,o.item);
    int birds=int(10*ecology(.8f)*seasonalWildlife());for(int i=0;i<birds;i++){float t=birdPhase*.23f+i*.628f;drawBird(std::sin(t)*17,7+std::sin(t*2+i)*1.6f,std::cos(t)*15-3,birdPhase+i);}
    int butterflies=int(18*ecology(.6f)*seasonalWildlife()*(1-atmosphere.seasons[WINTER]));for(int i=0;i<butterflies;i++){float t=butterflyPhase*.65f+i;drawButterfly(5+std::sin(t)*2.5f+std::sin(world.time)*windStrength*.4f,1+std::sin(t*1.7f)*.45f,4+std::cos(t*.8f)*2.5f,butterflyPhase+i);}
    glDisable(GL_LIGHTING);sphere(-24,32,-42,2.6f,2.6f,2.6f,blend({1,.89f,.60f},{.64f,.63f,.53f},damage()));glEnable(GL_LIGHTING);
    drawSeasonalClouds();
    // Animated leaves descend when the valley is being damaged.
    if(world.state==WARNING)for(int i=0;i<35;i++){
        float y=4-std::fmod(leafPhase*.7f+i*.23f,4.0f);
        glPushMatrix();glTranslatef(-7+std::sin(i*3.0f+leafPhase)*2,y,-5+std::cos(i*2.0f)*2);glRotatef(leafPhase*80+i*17.0f,1,1,0);
        cube(0,0,0,.13f,.025f,.22f,{.58f,.40f,.17f});glPopMatrix();}
    if(selected>=0){const auto& o=interactions[selected];ring(o.p.x,.15f,o.p.z,o.kind==TEA?1.95f:.85f,{.95f,.77f,.40f});}
    for(const auto& o:interactions)if(available(o)&&o.kind>=PLANT&&o.kind<=HABITAT){
        ring(o.p.x,.14f,o.p.z,.8f,{.65f,.75f,.43f});
        drawWorldLabel({o.p.x-.45f,2.5f+std::sin(world.time*2)*.08f,o.p.z},o.kind==GARBAGE?"CLEAN":o.kind==PLANT?"PLANT":o.kind==SWITCH?"STOP":"RESTORE",cream);
    }
}
void drawRiver(){
    // Water is tiled with animated vertices; the translucent surface is rendered after fish.
    Color c=blend(seasonColor({.19f,.58f,.60f},{.16f,.53f,.63f},{.30f,.49f,.49f},{.47f,.68f,.75f}),{.27f,.30f,.18f},damage());
    glDisable(GL_LIGHTING);glBegin(GL_QUADS);glColor3f(.24f,.35f,.30f);
    glVertex3f(-44,.12f,9);glVertex3f(-44,.12f,18);glVertex3f(44,.12f,18);glVertex3f(44,.12f,9);glEnd();glEnable(GL_LIGHTING);
    for(int i=0;i<int(12*(1-damage()));i++){
        float x=std::fmod(waterPhase*.8f+i*6+44,88)-44;
        sphere(x,.19f,11+float(i%4)*1.4f,.28f,.06f,.10f,{.76f,.57f,.28f});}
    glEnable(GL_BLEND);glDepthMask(GL_FALSE);material(c,.5f,.66f);
    for(int x=-44;x<44;x+=2){glBegin(GL_TRIANGLE_STRIP);glNormal3f(0,1,0);
        for(int z=9;z<=18;z++)for(int k=0;k<2;k++){float xx=float(x+k*2);glVertex3f(xx,.25f+(.027f+rainAmount()*.015f)*std::sin(xx*1.4f+z+waterPhase*2),float(z));}
        glEnd();}
    glDisable(GL_LIGHTING);glColor4f(.75f,.88f,.79f,.32f*(1-damage()));
    glBegin(GL_LINES);for(int i=0;i<110;i++){float x=std::fmod(i*3.91f+waterPhase*1.3f+44,88)-44,z=9.3f+std::fmod(i*1.73f,8.3f);
        glVertex3f(x,.31f,z);glVertex3f(x+.4f+std::sin(i+waterPhase)*.15f,.31f,z);}glEnd();
    glEnable(GL_LIGHTING);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    if(damage()>.25f)for(int i=0;i<int(damage()*12);i++){
        float x=std::fmod(i*6.7f+waterPhase*.20f+42,84)-42;
        cube(x,.34f,11+float(i%4)*1.7f,.20f,.12f,.3f,{.56f,.48f,.32f});}
}
void drawTeaSteam(){
    drawLivingSteam();
}
void drawSmoke(){
    auto particles=world.smoke;
    // Back-to-front ordering reduces blending artifacts in overlapping translucent particles.
    std::sort(particles.begin(),particles.end(),[](const SmokeParticle&a,const SmokeParticle&b){
        return length(Vec3{a.x,a.y,a.z}-camera())>length(Vec3{b.x,b.y,b.z}-camera());});
    for(auto p:particles)if(p.alpha>.005f)sphere(p.x,p.y,p.z,p.size,p.size*.8f,p.size,{.27f,.29f,.27f},p.alpha);
}

// ------------------------------------ HUD ------------------------------------
void rect(float x,float y,float w,float h,Color c,float alpha=1){glColor4f(c.r,c.g,c.b,alpha);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
void text(float x,float y,const std::string&s,Color c={.91f,.92f,.83f},void* font=GLUT_BITMAP_HELVETICA_12){
    glColor3f(c.r,c.g,c.b);glRasterPos2f(x,y);for(unsigned char ch:s)glutBitmapCharacter(font,ch);
}
int textWidth(const std::string&s,void* font=GLUT_BITMAP_HELVETICA_18){return glutBitmapLength(font,reinterpret_cast<const unsigned char*>(s.c_str()));}
void centered(float y,const std::string&s,Color c=cream,void* font=GLUT_BITMAP_HELVETICA_18){text((width-textWidth(s,font))*.5f,y,s,c,font);}
void wrapped(float x,float y,const std::string&s,float maxWidth,void* font=GLUT_BITMAP_HELVETICA_18){
    std::istringstream words(s);std::string word,line;float yy=y;
    while(words>>word){std::string next=line.empty()?word:line+" "+word;
        if(textWidth(next,font)>maxWidth&&!line.empty()){text(x,yy,line,cream,font);yy+=24;line=word;}else line=next;}
    if(!line.empty())text(x,yy,line,cream,font);
}
std::string objective(){
    if(world.finished)return "Carry this care beyond the valley.";
    if(!world.teaVisited)return "01 / Sit at the table. Take a moment.";
    if(world.state==HEALTHY)return "02 / Investigate the upstream warning.";
    if(world.state==WARNING)return "03 / Witness the cost of neglect.";
    if(world.state==RESTORED)return "05 / Return to the tea table.";
    if(actions()==6)return "04 / Let the valley heal. Then return.";
    return "04 / Restore the valley. Every action counts.";
}
void drawMinimap(){
    float x=width-204.0f,y=30;rect(x,y,174,178,{.055f,.11f,.105f},.87f);
    text(x+12,y+20,"VALLEY GUIDE",{.73f,.80f,.65f});
    auto point=[&](float wx,float wz){return Vec3{x+87+wx*1.65f,y+89+wz*1.5f,0};};
    Vec3 river=point(0,13.5f);rect(x+8,river.y-6,158,12,{.27f,.48f,.47f});rect(x+84,river.y-9,6,18,lightWood);
    for(const auto& o:interactions){
        if(o.kind==TREE||o.kind==RIVER||o.kind==FLOWERS||o.kind==BIRD)continue;
        if(!available(o)&&o.kind!=TEA)continue;
        auto p=point(o.p.x,o.p.z);text(p.x-3,p.y+4,o.kind==TEA?"T":o.kind==WARNING_SIGN?"!":o.kind==HOME_LAMP?"L":o.kind==FARM_FEED?"C":"G",o.kind==TEA?cream:Color{.91f,.68f,.34f});}
    auto h=point(-10,-19);text(h.x,h.y,"H");auto f=point(26,-22);text(f.x,f.y,"F");
    auto p=point(human.position.x,human.position.z);rect(p.x-2,p.y-2,4,4,{1,.96f,.80f});
    float a=human.yaw*PI/180;glColor3f(1,.96f,.8f);glBegin(GL_LINES);glVertex2f(p.x,p.y);glVertex2f(p.x+std::sin(a)*9,p.y-std::cos(a)*9);glEnd();
    text(x+12,y+164,"T tea  H home  C cows");
}
void drawHUD(){
    // Save both matrix stacks. Orthographic HUD pixels must not change the 3D camera.
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,width,height,0);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glDisable(GL_FOG);
    glEnable(GL_BLEND);
    rect(0,0,float(width),4,{.78f,.72f,.46f});
    rect(30,30,335,153,{.055f,.11f,.105f},.90f);
    text(48,58,"A LAST CUP OF TEA",cream,GLUT_BITMAP_HELVETICA_18);
    text(48,80,"A   C A M P A I G N   F O R   N A T U R E",{.66f,.77f,.66f});
    int health=int(std::round((1-damage())*100));
    text(48,111,"NATURE HEALTH",{.76f,.81f,.72f});text(302,111,std::to_string(health)+"%",cream);
    rect(48,122,298,4,{.26f,.32f,.26f});rect(48,122,298*(1-damage()),4,{.71f,.77f,.46f});
    const char* phases[]={"I.  A quiet beginning","II.  A fragile balance","III.  What we stand to lose","IV.  Small acts of care","V.  A living tomorrow"};
    text(48,158,phases[world.state],{.76f,.81f,.72f});
    rect(30,197,335,62,{.055f,.11f,.105f},.84f);text(48,219,"YOUR NEXT STEP",{.66f,.77f,.66f});text(48,242,objective());
    if(world.state>=DAMAGED){
        rect(30,273,335,168,{.055f,.11f,.105f},.87f);
        text(48,296,"NATURE RECOVERY  /  "+std::to_string(int(world.recoveryProgress*100))+"%",cream);
        text(48,321,std::string(world.planted?"[x]":"[ ]")+" Plant a tree");
        int count=int(world.litter[0])+int(world.litter[1])+int(world.litter[2]);
        text(48,344,std::string(count==3?"[x]":"[ ]")+" Clean riverbanks ("+std::to_string(count)+"/3)");
        text(48,367,std::string(world.pollutionStopped?"[x]":"[ ]")+" Stop the pollution source");
        text(48,390,std::string(world.habitat?"[x]":"[ ]")+" Restore the wildlife refuge");
        text(48,423,"Actions completed: "+std::to_string(actions())+" / 6",{.71f,.77f,.46f});
    }
    drawMinimap();
    drawAtmosphereHUD();drawHomeHUD();
    // A tiny crosshair gives orientation without covering the cup.
    if(!seated){rect(width*.5f-1,height*.5f-4,2,8,cream,.7f);rect(width*.5f-4,height*.5f-1,8,2,cream,.7f);}
    if(selected>=0&&!seated&&!paused){
        rect(width*.5f-210,height-164.0f,420,64,{.055f,.11f,.105f},.9f);
        centered(height-139.0f,interactions[selected].name,{.75f,.80f,.67f},GLUT_BITMAP_HELVETICA_12);
        centered(height-113.0f,"[ E ]  "+interactions[selected].verb);
    }
    if(seated){
        std::string line=world.finished?(seatTime<5?"The future is shaped by what we protect today.":"Don't let this be the last cup."):
            (seatTime<5?"Take a moment.":"This is what we are trying to protect.");
        rect(width*.5f-340,height-240.0f,680,110,{.055f,.11f,.105f},.88f);
        centered(height-209.0f,world.finished?"A LAST CUP OF TEA":"A peaceful moment, held in your hands.");
        centered(height-179.0f,human.action==TEA_REST?line:humanStatus());
        centered(height-150.0f,human.leaveRequested?"The cup will be returned before standing up.":"A Campaign for Nature   /   [ E ] Leave the table",{.73f,.80f,.66f},GLUT_BITMAP_HELVETICA_12);
    }else if(world.time<messageUntil){
        float w=std::min(760.0f,width-80.0f);rect((width-w)*.5f,height-258.0f,w,72,{.055f,.11f,.105f},.86f);
        wrapped((width-w)*.5f+20,height-229.0f,message,w-40);
    }
    rect(0,height-65.0f,float(width),65,{.045f,.09f,.08f},.88f);
    text(30,height-39.0f,"WASD Walk  |  Mouse Look  |  V View  |  E Interact  |  F1-F4 Seasons  |  1-6 Weather  |  T Auto seasons",cream);
    text(30,height-18.0f,"Q/C Camera height   P Pause   R Restart   H Help   Tab Release mouse   Esc Exit",{.64f,.73f,.64f});
    if(showHelp){
        float x=width-324.0f,y=349;rect(x,y,294,172,{.055f,.11f,.105f},.88f);
        text(x+16,y+23,"A PERSON IN THE VALLEY",cream);
        text(x+16,y+47,"Click, then move the mouse to orbit.");text(x+16,y+68,"F1 Spring  F2 Summer  F3 Autumn  F4 Winter");
        text(x+16,y+89,"1 Clear   2 Cloudy   3 Rain");text(x+16,y+110,"4 Storm   5 Snow   6 Windy");
        text(x+16,y+131,"WASD overrides a walk to shelter.");text(x+16,y+153,"Approach the table and press E for tea.",{.71f,.77f,.46f});
    }
    if(paused){rect(0,0,float(width),float(height),{.02f,.05f,.04f},.65f);centered(height*.5f,"PAUSED  /  Press P to return to the valley");}
    glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);glEnable(GL_FOG);
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);
}
void setupLighting(){
    applyAtmosphereLighting();setupHomeLighting();
}
void writeCapture(const std::string& filename){
    std::vector<unsigned char> pixels(size_t(width)*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream file(filename,std::ios::binary);file<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int y=height-1;y>=0;y--)file.write(reinterpret_cast<char*>(pixels.data()+size_t(y)*width*3),width*3);
    if(!file){std::cerr<<"Cannot write capture: "<<filename<<"\n";std::exit(2);}
}
void setCaptureFixture();
void display(){
    if(renderCheck)setCaptureFixture();
    if(benchmarkMode&&benchmarkFrames==0)benchmarkStart=std::chrono::steady_clock::now();
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    gluLookAt(cameraX,cameraY,cameraZ,viewAim.x,viewAim.y,viewAim.z,0,1,0);
    setupLighting();glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    drawOpaqueWorld();drawRiver();
    glEnable(GL_BLEND);glDepthMask(GL_FALSE);drawHomeGlass();drawSmoke();drawTeaSteam();drawWeatherParticles();drawBreath();glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    drawHUD();
    if(renderCheck){
        const char* names[]={"01-healthy","02-damaged","03-recovering","04-restored","05-final-tea",
            "06-spring-walk","07-summer","08-autumn","09-winter","10-rain","11-storm","12-summer-snow","13-windy",
            "14-tea-reach","15-tea-lift","16-tea-sip","17-tea-return","18-shelter","19-tea-turn","20-tea-sit","21-home","22-door","23-living-room","24-kitchen","25-bedroom","26-bathroom","27-farm","28-cows"};
        GLenum error=glGetError();if(error!=GL_NO_ERROR){std::cerr<<"OpenGL error "<<error<<"\n";std::exit(3);}
        writeCapture(captureDir+"/"+names[captureStage]+".ppm");
        std::cout<<"Captured "<<names[captureStage]<<"; GL_NO_ERROR\n";
        if(++captureStage==28){glutLeaveMainLoop();return;}
    }
    glutSwapBuffers();
    if(benchmarkMode){
        if(glGetError()!=GL_NO_ERROR){std::cerr<<"Runtime graphics error\n";std::exit(3);}
        if(++benchmarkFrames==240){
            double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-benchmarkStart).count();
            std::cout<<"Runtime check: 240 animated storm frames, "<<std::fixed<<std::setprecision(1)<<240/seconds<<" average FPS; GL_NO_ERROR\n";
            glutLeaveMainLoop();
        }
    }
}
void reshape(int w,int h){width=std::max(1,w);height=std::max(1,h);glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(60.0,double(width)/height,.1,1000);glMatrixMode(GL_MODELVIEW);}
void captureMouse(bool value){mouseCaptured=value;glutSetCursor(value?GLUT_CURSOR_NONE:GLUT_CURSOR_INHERIT);
    if(value){warping=true;glutWarpPointer(width/2,height/2);}}
void keyboard(unsigned char raw,int,int){
    unsigned char key=static_cast<unsigned char>(std::tolower(raw));
    if(key==27){glutLeaveMainLoop();return;}
    if(key=='p'){paused=!paused;std::fill(std::begin(keys),std::end(keys),false);if(paused)captureMouse(false);return;}
    if(key=='r'){reset();return;}if(key=='h'){showHelp=!showHelp;return;}
    if(key==9){captureMouse(!mouseCaptured);return;}if(paused)return;
    if(key>='1'&&key<='6'){selectWeather(Weather(key-'1'));return;}
    if(key=='v'){homestead.firstPerson=!homestead.firstPerson;updateFollowCamera(0,true);return;}
    if(key=='t'){atmosphere.automatic=!atmosphere.automatic;atmosphere.cycleTime=0;return;}
    if(key=='e'&&!keys[key]){
        if(teaActive())leaveTea();
        else interact(nearest());
    }
    keys[key]=true;
}
void keyboardUp(unsigned char key,int,int){keys[static_cast<unsigned char>(std::tolower(key))]=false;}
void mouse(int button,int state,int,int){if(button==GLUT_LEFT_BUTTON&&state==GLUT_DOWN&&!paused)captureMouse(true);}
void mouseMotion(int x,int y){
    if(!mouseCaptured||paused||seated)return;
    int dx=x-width/2,dy=y-height/2;if(warping&&dx==0&&dy==0){warping=false;return;}
    cameraYaw=std::remainder(cameraYaw+dx*.13f,360.0f);cameraPitch=clamp(cameraPitch-dy*.13f,-65,homestead.firstPerson?65.0f:-5.0f);
    if(dx||dy){warping=true;glutWarpPointer(width/2,height/2);}
}
void setCaptureFixture(){
    reset();showHelp=false;world.time=12;world.teaVisited=true;
    cameraX=3;cameraY=6;cameraZ=26;cameraYaw=-4;cameraPitch=-10;
    viewAim={0,1,-5};human.position={1,.02f,21};human.yaw=150;
    if(captureStage>=1&&captureStage<=4){world.state=DAMAGED;world.damageTime=24;world.environmentProgress=1;}
    if(captureStage>=2&&captureStage<=4){world.state=RECOVERING;world.planted=true;world.growth=1;world.pollutionStopped=true;world.shutdownTime=0;world.habitat=true;world.recoveryProgress=.64f;world.litter={true,true,false};}
    if(captureStage>=3&&captureStage<=4){world.state=RESTORED;world.recoveryProgress=1;world.litter={true,true,true};}
    if(captureStage>=5){
        human.position={3,.02f,6};human.yaw=155;human.walkBlend=1;human.walkCycle=1.2f;human.legAngle=25;human.armAngle=21;
        cameraX=7;cameraY=3.3f;cameraZ=11;viewAim={2,1.3f,2};
        Season season=captureStage==6?SUMMER:captureStage==7?AUTUMN:captureStage==8?WINTER:SPRING;
        Weather weather=captureStage==7?WINDY:captureStage==8?SNOW:captureStage==9?RAIN:captureStage==10?STORM:captureStage==11?SNOW:captureStage==12?WINDY:CLEAR;
        if(captureStage==11)season=SUMMER;
        atmosphere.season=season;atmosphere.weather=weather;atmosphere.seasons.fill(0);atmosphere.seasons[season]=1;
        atmosphere.weatherMix.fill(0);atmosphere.weatherMix[weather]=1;
        for(int i=0;i<240;i++){world.time+=1.0f/60;updateAtmosphere(1.0f/60);}
        if(captureStage==10){atmosphere.lightningFlash=.55f;atmosphere.reactionTime=2;}
    }
    if(captureStage==4||captureStage>=13){
        human.walkBlend=0;human.legAngle=0;human.armAngle=0;human.position={0,.02f,1.27f};human.yaw=0;human.sitBlend=1;seated=true;
        human.action=TEA_DRINK;human.actionTime=captureStage==13?1.2f:captureStage==14?2.5f:captureStage==15?4.5f:captureStage==16?6.6f:9.4f;
        animateDrink();cameraX=3.8f;cameraY=2.9f;cameraZ=-1.55f;viewAim={0,1.35f,.95f};
        if(captureStage==4){world.finished=true;human.action=TEA_REST;seatTime=8;}
        if(captureStage==17){human.action=SHELTER_REST;seated=false;human.position={PORCH_SEAT.x,HOME_FLOOR,PORCH_SEAT.z};human.yaw=180;human.reach=0;
            atmosphere.weather=RAIN;atmosphere.weatherMix.fill(0);atmosphere.weatherMix[RAIN]=1;
            cameraX=PORCH_SEAT.x;cameraY=2.4f;cameraZ=-6.5f;viewAim={PORCH_SEAT.x,1.45f,PORCH_SEAT.z};}
        if(captureStage==18){human.action=TEA_TURN;human.position.z=2.05f;human.sitBlend=0;human.yaw=65;}
        if(captureStage==19){human.action=TEA_SIT;human.actionTime=.72f;animateSit();}
    }
    if(captureStage>=20){
        human.action=FREE_WALK;human.sitBlend=0;human.reach=0;seated=false;homestead.doorAngle=100;
        human.position={-10,HOME_FLOOR,-11};human.yaw=0;
        const Vec3 eyes[]={{1,9,2},{-8,2.7f,-7},{-10,2.5f,-18},{-10,2.5f,-17},{-10.5f,2.5f,-21.4f},{-5.8f,2.5f,-21.4f},{12,10,12},{24,3,7}};
        const Vec3 aims[]={{-10,2,-18},{-10,1.8f,-14},{-15,1.4f,-16},{-5,1.5f,-18},{-14,1.1f,-23},{-7,1,-24},{25,1,-2},{21,1.2f,3}};
        Vec3 eye=eyes[captureStage-20];cameraX=eye.x;cameraY=eye.y;cameraZ=eye.z;viewAim=aims[captureStage-20];
        homestead.firstPerson=captureStage>=22&&captureStage<=25;
        if(homestead.firstPerson)human.position={eye.x,HOME_FLOOR,eye.z};
        if(captureStage>=26)human.position={17,.02f,-1};
    }
    updateTrees(0);updateSmoke(0);selected=-1;messageUntil=0;
}
void timer(int){
    int now=glutGet(GLUT_ELAPSED_TIME);float dt=clamp((now-lastTick)/1000.0f,0,.05f);lastTick=now;
    if(renderCheck)setCaptureFixture();else update(dt);
    glutPostRedisplay();glutTimerFunc(16,timer,0);
}

#include "LivingWorld.inl"
#include "Homestead.inl"

// ------------------------- DETERMINISTIC LOGIC TESTS -------------------------
int selfTest(){
    int checks=0;auto require=[&](bool condition,const char* title){checks++;if(!condition){std::cerr<<"FAIL: "<<title<<"\nHuman: "<<humanStatus()<<" at "<<human.position.x<<", "<<human.position.z<<"; waypoint "<<human.waypoint<<"/"<<human.route.size()<<"\n"<<message<<"\n";std::exit(1);}};
    auto advance=[](float seconds){for(int i=0;i<int(seconds*60);i++)update(1.0f/60);};
    auto approach=[](int index){Vec3 p=interactions[index].p;human.position={p.x,.02f,p.z+(p.z==8?-2.0f:2.0f)};human.action=FREE_WALK;human.sitBlend=0;seated=false;};
    reset();require(world.state==HEALTHY&&actions()==0,"reset healthy");
    require(!interact(5),"remote interaction rejected");approach(5);interact(5);require(world.state==HEALTHY,"tea must precede damage");
    approach(0);require(interact(0)&&seated,"sit at tea");advance(14);require(human.action==TEA_REST&&world.teaVisited,"complete animated tea sequence");
    leaveTea();advance(2);approach(5);require(interact(5)&&world.state==WARNING,"begin warning");
    advance(5);require(world.environmentProgress>0&&world.environmentProgress<1,"gradual damage");
    require(world.trees[0].state==SHAKE||world.trees[0].state==FALLING,"tree shakes before falling");
    advance(3);require(world.trees[0].state==FALLING&&world.trees[0].angle>0,"tree rotation");
    require(!available(interactions[6]),"restoration waits for damage event");
    paused=true;float frozen=world.time;advance(3);require(world.time==frozen,"pause freezes simulation");paused=false;
    advance(17);require(world.state==DAMAGED&&world.environmentProgress==1,"damaged state");
    require(world.trees[0].state==REMOVED,"fallen tree removed");
    approach(6);require(interact(6),"plant tree");require(!interact(6)&&actions()==1,"task cannot repeat");advance(5);
    require(world.state==RECOVERING&&world.growth>0,"recovery and growth");require(world.recoveryProgress<.18f,"recovery gated by completed tasks");
    for(int i=7;i<=11;i++){approach(i);require(interact(i),"restoration task reachable");}
    require(actions()==6,"all six actions recorded");advance(20);require(world.state==RESTORED&&world.recoveryProgress==1,"fully restored");
    require(ecology(.8f)>.99f,"birds return");approach(0);interact(0);advance(14);require(world.finished,"final tea completes campaign");
    require(checkCollision(0,0),"table collision");require(checkCollision(-17,-15),"house wall collision");
    require(checkCollision(-18,7),"rock collision");require(checkCollision(40,0),"mountain boundary");
    require(checkCollision(5,13),"river blocks movement");require(!checkCollision(0,13),"bridge permits crossing");
    reset();require(checkCollision(-7,-5),"tree collision");require(!world.finished&&!world.planted&&world.growth==0&&actions()==0,"restart clears campaign");
    human.position.z=5;keys['w']=true;updatePlayer(.1f);require(human.position.z<5,"forward movement");
    reset();human.position={0,.02f,2.8f};require(nearest()==0,"nearby tea selected");
    human.position.y=10;require(nearest()!=0,"interaction uses 3D distance");
    // Flood-fill actual collision-free ground cells to ensure the bridge and every goal
    // have a walkable approach. This catches unreachable tasks despite valid state logic.
    reset();homestead.doorAngle=100;constexpr int nx=145,nz=149;std::vector<bool> visited(nx*nz,false);std::queue<int> frontier;
    int origin=72+79*nx;visited[origin]=true;frontier.push(origin);
    while(!frontier.empty()){
        int node=frontier.front();frontier.pop();int ix=node%nx,iz=node/nx;
        for(auto step:std::array<std::array<int,2>,4>{{{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}}){
            int xx=ix+step[0],zz=iz+step[1];if(xx<0||xx>=nx||zz<0||zz>=nz)continue;
            int next=xx+zz*nx;if(visited[next]||checkCollision(-36+xx*.5f,-34+zz*.5f))continue;
            visited[next]=true;frontier.push(next);
        }
    }
    for(const auto& o:interactions){bool reachable=false;
        for(int iz=0;iz<nz&&!reachable;iz++)for(int ix=0;ix<nx;ix++)if(visited[ix+iz*nx]){
            Vec3 p{-36+ix*.5f,2.4f,-34+iz*.5f};if(length(p-o.p)<3.0f){reachable=true;break;}}
        require(reachable,("walkable approach: "+o.name).c_str());
    }
    // New behaviour is exercised independently of graphics fixtures.
    reset();keys['w']=true;advance(.5f);float cycle=human.walkCycle;
    require(cycle>0&&human.walkBlend>.9f,"distance driven walking");
    keys['w']=false;advance(1);require(human.walkBlend<.001f,"walking returns to idle");
    require(human.walkCycle==cycle,"idle does not advance gait");
    human.position={0,.02f,1.21f};keys['w']=true;advance(.5f);require(human.walkBlend<.01f,"blocked actor does not walk in place");
    reset();cameraX=0;cameraY=1.4f;cameraZ=0;human.position={0,.02f,7};require(nearest()!=0,"camera cannot remotely interact");
    selectSeason(AUTUMN);require(atmosphere.seasons[AUTUMN]==0,"season change starts smoothly");advance(12);
    require(atmosphere.seasons[AUTUMN]>.99f&&foliageColor().r>foliageColor().g,"autumn changes foliage");
    selectSeason(WINTER);advance(12);require(coldAmount()>.99f&&snowAmount()>.9f,"winter brings cold and snow");
    require(groundColor().r>.8f,"winter snow cover");
    selectSeason(SUMMER);advance(12);require(coldAmount()<.01f&&seasonalWildlife()>.98f,"summer restores warm wildlife");
    selectWeather(RAIN,false);require(rainAmount()<.01f,"rain transition is gradual");advance(6);require(rainAmount()>.68f&&atmosphere.wetness>.3f,"rain wets ground");
    paused=true;float y=atmosphere.rain[0].y;float age=human.actionTime;advance(1);require(atmosphere.rain[0].y==y&&human.actionTime==age,"pause freezes particles and character");paused=false;
    selectWeather(STORM,false);advance(5);atmosphere.lightningTimer=.01f;advance(.04f);require(atmosphere.lightningFlash>.5f,"storm lightning flashes");
    advance(.5f);require(atmosphere.lightningFlash==0&&atmosphere.lightningTimer>8,"lightning fades and has cooldown");
    float phase=world.time;selectWeather(CLEAR);advance(5);require(rainAmount()<.02f&&world.time>phase,"weather clears smoothly");
    reset();selectWeather(RAIN);advance(18);require(human.action==SHELTER_REST,"rain routes to covered seat");
    require(underRoof(human.position.x,human.position.y+1.9f,human.position.z),"shelter is beneath a roof");
    selectWeather(CLEAR);advance(2);require(human.action==FREE_WALK&&human.sitBlend==0,"leave shelter in clear weather");
    reset();human.position={12,.02f,25};selectWeather(STORM);advance(30);require(human.action==SHELTER_REST,"shelter navigation crosses bridge");
    reset();selectWeather(RAIN);advance(.2f);keys['d']=true;advance(.2f);require(human.action==FREE_WALK,"manual movement overrides shelter");
    reset();human.position={0,.02f,-2.5f};require(beginTea(),"tea path starts from opposite side");
    Vec3 last=human.cupPosition;bool held=false,returned=false,gripCorrect=true,pathClear=true;float maxStep=0;
    for(int i=0;i<1100;i++){
        update(1.0f/60);maxStep=std::max(maxStep,length(human.cupPosition-last));last=human.cupPosition;
        if(human.cupHeld){held=true;gripCorrect=gripCorrect&&length(rightHandPosition()-cupHandlePosition())<.001f;}
        if(held&&!human.cupHeld&&human.action==TEA_REST)returned=true;
        pathClear=pathClear&&!checkCollision(human.position.x,human.position.z);
    }
    require(held&&returned&&maxStep<.08f,"continuous cup pickup and replacement");
    require(gripCorrect,"gripped hand follows cup");require(pathClear,"tea approach avoids table collision");
    require(length(human.cupPosition-Vec3{.12f,1.37f,0})<.001f,"cup returns to original location");
    leaveTea();advance(2);require(human.action==FREE_WALK&&human.sitBlend==0,"smooth stand up");
    reset();approach(0);interact(0);advance(5);leaveTea();advance(12);
    require(human.action==FREE_WALK&&!human.cupHeld&&length(human.cupPosition-Vec3{.12f,1.37f,0})<.001f,"early exit returns held cup safely");
    reset();atmosphere.automatic=true;advance(91);require(atmosphere.season==SUMMER,"automatic seasonal cycle");
    reset();world.state=DAMAGED;world.environmentProgress=1;selectSeason(SPRING);advance(10);
    require(damage()==1&&actions()==0,"changing seasons never repairs campaign damage");
    reset();approach(0);interact(0);advance(5);selectWeather(RAIN);advance(30);
    require(human.action==SHELTER_REST&&world.teaVisited&&!human.cupHeld&&length(human.cupPosition-Vec3{.12f,1.37f,0})<.001f,"rain during tea returns cup before shelter");
    reset();selectSeason(WINTER);advance(12);float startZ=human.position.z;keys['w']=true;advance(.5f);
    require(startZ-human.position.z<1.3f&&startZ-human.position.z>1.1f,"snow slows character movement");
    reset();human.position={PORCH_SEAT.x,HOME_FLOOR,PORCH_SEAT.z};selectWeather(RAIN);advance(.5f);selectWeather(CLEAR);advance(3);
    require(human.action==FREE_WALK,"clearing rain during shelter sit returns control");
    reset();require(atmosphere.season==SPRING&&atmosphere.weather==CLEAR&&human.action==FREE_WALK&&!human.cupHeld,"reset clears new systems");
    require(atmosphere.rain.size()==500&&atmosphere.snow.size()==400&&atmosphere.leaves.size()==100&&atmosphere.steam.size()==20,"bounded particle budgets");
    reset();require(doorCollision(DOOR_X,HOUSE_FRONT),"closed door blocks passage");
    human.position={DOOR_X,HOME_FLOOR,-9.5f};advance(1.3f);
    require(homestead.doorAngle==100&&!doorCollision(DOOR_X,HOUSE_FRONT),"proximity opens door from outside");
    cameraYaw=0;keys['w']=true;advance(2);keys['w']=false;
    require(insideHouse(human.position.x,human.position.z)&&human.position.y==HOME_FLOOR,"walk through door onto home floor");
    require(!checkCollision(human.position.x,human.position.z),"entrance is unobstructed");
    for(Vec3 goal:std::array<Vec3,4>{{{-14,0,-13.5f},{-8,0,-16},{-10.5f,0,-22},{-5.5f,0,-22}}}){
        require(planWalk(goal),("route to room at "+std::to_string(goal.x)+", "+std::to_string(goal.z)).c_str());
        bool clear=true;for(auto p:human.route)clear=clear&&!checkCollision(p.x,p.z);
        require(clear,"room route avoids furniture and walls");
    }
    require(checkCollision(-15.7f,-16.2f)&&checkCollision(-13.8f,-23.25f),"sofa and bed are solid");
    require(checkCollision(-9.25f,-23)&&checkCollision(-17,-18),"interior and exterior walls are solid");
    human.position={-10,HOME_FLOOR,-18};advance(4);require(homestead.doorAngle==0,"door closes after leaving entrance");
    human.position={-10,HOME_FLOOR,-15.8f};advance(1.3f);require(homestead.doorAngle==100,"proximity opens door from inside");
    human.position.z=HOUSE_FRONT;advance(4);require(homestead.doorAngle==100,"door stays open while occupied");
    cameraYaw=180;keys['w']=true;advance(1.4f);keys['w']=false;
    require(!insideHouse(human.position.x,human.position.z)&&human.position.z>HOUSE_FRONT+3,"walk back outside");
    human.position={-10,HOME_FLOOR,-18};homestead.firstPerson=true;cameraPitch=20;updateFollowCamera(0,true);
    require(length(camera()-(human.position+Vec3{0,2.05f,0}))<.001f&&viewAim.y>cameraY,"first person camera supports looking upward");
    selectWeather(RAIN);advance(3);require(human.action==FREE_WALK&&insideHouse(human.position.x,human.position.z),"indoors already shelters from rain");
    reset();human.position={-10,HOME_FLOOR,-18};cameraYaw=180;keys['w']=true;
    bool safeExit=true;for(int i=0;i<180;i++){update(1.0f/60);safeExit=safeExit&&!doorCollision(human.position.x,human.position.z);}
    keys['w']=false;require(safeExit&&human.position.z>HOUSE_FRONT+2,"continuous walk from inside opens door without collision");
    reset();human.position={-14.5f,HOME_FLOOR,-13.5f};require(interact(12)&&!homestead.lampOn,"home lights toggle");
    human.position={17,0,-6.4f};require(interact(13)&&homestead.feedTime>0&&actions()==0,"feeding cows preserves campaign progress");
    require(!farmCollision(16,-1)&&farmCollision(16,2),"farm has open gate and solid fence");
    reset();Vec3 cowStart=homestead.cows[0].position;advance(5);
    require(length(homestead.cows[0].position-cowStart)>.2f,"cows wander with animated gait");
    bool cowsSafe=true;for(int i=0;i<3600;i++){
        update(1.0f/60);for(const auto& cow:homestead.cows)cowsSafe=cowsSafe&&cow.position.x>17&&cow.position.x<33&&cow.position.z>-9&&cow.position.z<6.5f;
    }
    require(cowsSafe,"cows remain within farm");
    auto cow=homestead.cows[0];require(cowCollision(cow.position.x,cow.position.z),"cows block walking through bodies");
    paused=true;advance(1);require(length(cow.position-homestead.cows[0].position)==0,"pause freezes cows");
    reset();require(homestead.doorAngle==0&&!homestead.firstPerson&&homestead.lampOn&&homestead.feedTime==0,"reset restores home and farm");
    std::cout<<"PASS: "<<checks<<" campaign, human, weather, navigation, home and farm checks\n";return 0;
}
int main(int argc,char** argv){
    if(argc>1&&std::string(argv[1])=="--self-test")return selfTest();
    if(argc>2&&std::string(argv[1])=="--render-check"){renderCheck=true;captureDir=argv[2];}
    if(argc>1&&std::string(argv[1])=="--benchmark")benchmarkMode=true;
    // Keep our command-line arguments away from GLUT's argument parser.
    int glutArgc=1;glutInit(&glutArgc,argv);glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH|GLUT_MULTISAMPLE);
    glutInitWindowSize(width,height);glutCreateWindow("A Last Cup of Tea | A Campaign for Nature");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    // Depth testing ensures a nearby object hides the correct parts of farther geometry.
    glEnable(GL_DEPTH_TEST);glEnable(GL_NORMALIZE);glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
    glEnable(GL_FOG);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glShadeModel(GL_SMOOTH);
    quadric=gluNewQuadric();gluQuadricNormals(quadric,GLU_SMOOTH);reset();if(renderCheck)setCaptureFixture();
    if(benchmarkMode){showHelp=false;selectWeather(STORM);atmosphere.weatherMix={0,0,0,1,0,0};}
    glutDisplayFunc(display);glutReshapeFunc(reshape);glutKeyboardFunc(keyboard);glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouse);glutPassiveMotionFunc(mouseMotion);glutMotionFunc(mouseMotion);glutIgnoreKeyRepeat(1);
    lastTick=glutGet(GLUT_ELAPSED_TIME);glutTimerFunc(16,timer,0);glutMainLoop();gluDeleteQuadric(quadric);return 0;
}

// Walkable home and small cow farm. All dimensions are in the existing world coordinates.
#pragma once
constexpr float HOUSE_LEFT=-17, HOUSE_RIGHT=-3, HOUSE_FRONT=-12.4f, HOUSE_BACK=-25.4f;
constexpr float HOME_FLOOR=.18f, DOOR_X=-10, DOOR_WIDTH=2.5f;
constexpr Vec3 PORCH_SEAT{-14.2f,0,-10.1f};
struct FurnitureSolid {float x,z,halfX,halfZ,height;};
struct Cow {
    Vec3 position,center;
    float angle=0,yaw=0,gait=0,graze=0,scale=1;
    bool walking=false;
};
struct Homestead {
    float doorAngle=0,doorHold=0,feedTime=0;
    bool lampOn=true,firstPerson=false;
    std::array<Cow,4> cows;
} homestead;
const std::array<FurnitureSolid,13> furnitureSolids{{
    {-15.7f,-16.2f,.73f,1.85f,1.6f}, // sofa
    {-13.0f,-17.4f,.85f,1.00f,.75f}, // coffee table
    {-16.35f,-19.2f,.38f,.90f,2.65f}, // bookshelf
    {-3.65f,-17.8f,.5f,1.7f,1.15f}, // kitchen counter
    {-4.05f,-19.6f,.8f,.45f,1.15f}, // back counter and stove
    {-8.1f,-19.1f,.55f,.65f,2.6f}, // fridge
    {-6,-15.6f,1.15f,1.15f,1.2f}, // dining table + tucked chairs
    {-13.8f,-23.25f,1.55f,1.75f,.9f}, // bed
    {-10.6f,-24.5f,.9f,.50f,2.9f}, // wardrobe
    {-16.0f,-24.3f,.45f,.5f,1.0f}, // bedside table
    {-7.75f,-23.65f,1.0f,1.3f,.8f}, // bath
    {-4.0f,-24.45f,.46f,.65f,1.1f}, // toilet
    {-3.7f,-21.8f,.45f,.72f,1.1f} // wash basin
}};
bool insideHouse(float x,float z);
bool homeWallCollision(float x,float z,float margin=.28f);
bool doorCollision(float x,float z,float margin=.28f);
bool houseCollision(float x,float z);
bool houseCameraCollision(Vec3 p);
bool farmCollision(float x,float z,float margin=.28f);
bool cowCollision(float x,float z);
float homeFloorHeight(float x,float z);
void resetHomestead();
void updateHomestead(float dt);
void drawHouseExterior();
void drawHouseInterior();
void drawHomePorch();
void drawHomesteadPaths();
void drawFarm();
void drawCow(const Cow& cow,int index);
void drawHomeGlass();
void setupHomeLighting();
void drawHomeHUD();

// Home and farm implementation. Included after the original geometry helpers.
// The house is a collection of wall segments, not a solid box. Door geometry,
// collision, animation and the camera all use the same doorway dimensions.

bool insideHouse(float x,float z){return x>HOUSE_LEFT&&x<HOUSE_RIGHT&&z>HOUSE_BACK&&z<HOUSE_FRONT;}
float homeFloorHeight(float x,float z){
    if(x>HOUSE_LEFT-.15f&&x<HOUSE_RIGHT+.15f&&z>HOUSE_BACK-.15f&&z<HOUSE_FRONT+.15f)return HOME_FLOOR;
    if(x>-16.8f&&x<-3.2f&&z>=HOUSE_FRONT&&z<-8.7f)return HOME_FLOOR*smooth((-8.7f-z)/1.0f);
    return -1;
}
bool homeWallCollision(float x,float z,float margin){
    float thickness=.12f+margin;
    if(box(x,z,HOUSE_LEFT,-18.9f,thickness,6.5f+margin)||box(x,z,HOUSE_RIGHT,-18.9f,thickness,6.5f+margin))return true;
    if(box(x,z,-10,HOUSE_BACK,7+margin,thickness))return true;
    if(std::abs(z-HOUSE_FRONT)<thickness&&x>HOUSE_LEFT-margin&&x<HOUSE_RIGHT+margin&&std::abs(x-DOOR_X)>DOOR_WIDTH*.5f-margin)return true;
    // Bedroom and bathroom doors are permanent openings in the internal wall.
    if(std::abs(z+20.5f)<.10f+margin&&x>HOUSE_LEFT&&x<HOUSE_RIGHT){
        bool bedroom=x>-11.6f+margin&&x<-9.4f-margin;
        bool bathroom=x>-7.1f+margin&&x<-5.0f-margin;
        if(!bedroom&&!bathroom)return true;
    }
    return box(x,z,-9.25f,-23,.10f+margin,2.4f+margin);
}
bool doorCollision(float x,float z,float margin){
    float a=homestead.doorAngle*PI/180;
    Vec3 hinge{DOOR_X-DOOR_WIDTH*.5f,0,HOUSE_FRONT};
    Vec3 end=hinge+Vec3{std::cos(a)*DOOR_WIDTH,0,-std::sin(a)*DOOR_WIDTH};
    Vec3 d=end-hinge,p=Vec3{x,0,z}-hinge;float t=clamp(dot(p,d)/dot(d,d));
    return length(p-d*t)<margin+.07f;
}
bool houseCollision(float x,float z){
    if(homeWallCollision(x,z)||doorCollision(x,z))return true;
    for(auto b:furnitureSolids)if(box(x,z,b.x,b.z,b.halfX+.25f,b.halfZ+.25f))return true;
    // Porch bench and posts leave the central approach to the door open.

    for(float px:{-16.3f,-3.7f})if(std::hypot(x-px,z+9.1f)<.38f)return true;
    return false;
}
bool houseCameraCollision(Vec3 p){
    if(p.y<HOME_FLOOR)return insideHouse(p.x,p.z);
    if(p.y<4.35f&&homeWallCollision(p.x,p.z,.12f))return true;
    if(p.y<3.5f&&doorCollision(p.x,p.z,.10f))return true;
    if(insideHouse(p.x,p.z)&&p.y>4.08f&&p.y<6.9f)return true;
    for(auto b:furnitureSolids)if(p.y<b.height+HOME_FLOOR&&box(p.x,p.z,b.x,b.z,b.halfX+.10f,b.halfZ+.10f))return true;
    return false;
}
bool farmCollision(float x,float z,float margin){
    float r=margin+.08f;
    bool gate=z>-2.5f+margin&&z<.5f-margin;
    if(std::abs(x-16)<r&&z>-9.8f-margin&&z<7.2f+margin&&!gate)return true;
    if(box(x,z,34,-1.3f,r,8.5f+margin))return true;
    if(box(x,z,25,-9.8f,9+margin,r)||box(x,z,25,7.2f,9+margin,r))return true;
    // Barn back and sides; its front is open. Water and feed troughs are solid.
    if(box(x,z,25,-9.2f,4.6f+margin,r)||box(x,z,20.5f,-6.9f,r,2.3f+margin)||box(x,z,29.5f,-6.9f,r,2.3f+margin))return true;
    if(box(x,z,18.3f,-6.4f,.55f+margin,1.4f+margin)||box(x,z,31.8f,-5.9f,.6f+margin,1.4f+margin))return true;
    return false;
}
bool cowCollision(float x,float z){
    for(const auto& cow:homestead.cows){Vec3 p=rotateY(Vec3{x,0,z}-cow.position,cow.yaw);float sx=.92f*cow.scale,sz=1.8f*cow.scale;
        if(p.x*p.x/(sx*sx)+p.z*p.z/(sz*sz)<1)return true;}
    return false;
}
void resetHomestead(){
    homestead=Homestead{};
    const Vec3 centers[]={{21,0,3.7f},{30,0,2.6f},{20.5f,0,-1.5f},{27,0,-6.1f}};
    for(int i=0;i<4;i++){
        auto& c=homestead.cows[i];c.center=centers[i];c.angle=i*1.9f;c.position=c.center;
        c.yaw=i*70.0f;c.scale=i==2?.78f:1.0f;c.graze=i%2?.0f:1.0f;
    }
}
void updateHomestead(float dt){
    float distance=std::hypot(human.position.x-DOOR_X,human.position.z-HOUSE_FRONT);
    bool inSweep=std::abs(human.position.x-DOOR_X)<2.7f&&human.position.z>HOUSE_FRONT-3.2f&&human.position.z<HOUSE_FRONT+1.8f;
    if(distance<4.7f||inSweep)homestead.doorHold=2.0f;
    else homestead.doorHold=std::max(0.0f,homestead.doorHold-dt);
    float target=homestead.doorHold>0?100.0f:0.0f;
    float oldAngle=homestead.doorAngle;
    bool wasTouching=doorCollision(human.position.x,human.position.z,.36f);
    homestead.doorAngle+=clamp(target-homestead.doorAngle,-dt*140,dt*140);
    if(!wasTouching&&doorCollision(human.position.x,human.position.z,.36f))homestead.doorAngle=oldAngle;
    homestead.feedTime=std::max(0.0f,homestead.feedTime-dt);
    for(int i=0;i<4;i++){
        auto& c=homestead.cows[i];Vec3 old=c.position;
        float cycle=std::fmod(world.time+i*6.5f,27.0f);
        bool walk=cycle<10&&i!=3&&homestead.feedTime<=0;
        if(walk&&std::hypot(human.position.x-c.position.x,human.position.z-c.position.z)>2.8f){
            float angle=c.angle+dt*.18f;Vec3 next=c.center+Vec3{std::cos(angle)*1.25f,0,std::sin(angle)*.72f};
            bool blocked=farmCollision(next.x,next.z,1.1f);
            for(const auto& t:world.trees)if(std::hypot(next.x-t.p.x,next.z-t.p.z)<2.2f)blocked=true;
            for(int j=0;j<4;j++)if(j!=i&&length(next-homestead.cows[j].position)<2.9f)blocked=true;
            if(!blocked){
                Vec3 delta=next-c.position;delta.y=0;float step=std::min(length(delta),dt*.45f);
                c.position=c.position+unit(delta)*step;c.angle=angle;
                if(step>.0001f){float yaw=std::atan2(delta.x,-delta.z)*180/PI;c.yaw+=std::remainder(yaw-c.yaw,360.0f)*(1-std::exp(-dt*3));}
            }
        }
        Vec3 moved=c.position-old;moved.y=0;c.walking=length(moved)>.00001f;c.gait+=length(moved)*5;
        c.graze=mix(c.graze,!c.walking&&homestead.feedTime<=0?1.0f:0.0f,1-std::exp(-dt*1.7f));
        c.position.y=std::max(.02f,terrain(c.position.x,c.position.z));
    }
}

// ---------------- HOUSE EXTERIOR ----------------
Color plaster{.88f,.82f,.68f},trim{.94f,.90f,.77f},roofClay{.45f,.23f,.16f};
void homeWallPiece(float x,float y,float z,float sx,float sy,float sz){
    cube(x,y+HOME_FLOOR,z,sx,sy,sz,plaster);
    if(sz<.4f)for(float yy=y-sy*.5f+.28f;yy<y+sy*.5f;yy+=.38f)cube(x,yy+HOME_FLOOR,z+.132f,sx,.016f,.022f,{.75f,.68f,.52f});
}
void homeFrontWindow(float x){
    homeWallPiece(x,.68f,HOUSE_FRONT,2.2f,1.36f,.24f);
    homeWallPiece(x,3.70f,HOUSE_FRONT,2.2f,1.10f,.24f);
    for(float xx:{x-1.13f,x+1.13f})cube(xx,2.43f,HOUSE_FRONT+.035f,.11f,1.86f,.38f,trim);
    for(float yy:{1.51f,3.36f})cube(x,yy,HOUSE_FRONT+.035f,2.37f,.12f,.38f,trim);
    cube(x,2.43f,HOUSE_FRONT+.025f,.08f,1.85f,.31f,trim);cube(x,2.46f,HOUSE_FRONT+.025f,2.2f,.07f,.31f,trim);
    cube(x,1.45f,HOUSE_FRONT+.25f,2.6f,.12f,.52f,lightWood);
    for(int i=0;i<5;i++){glPushMatrix();glTranslatef(x-.85f+i*.42f,1.5f,HOUSE_FRONT+.38f);cylinder(.14f,.18f,.24f,roofClay);sphere(0,.40f,0,.20f,.24f,.20f,{.27f,.49f,.21f});glPopMatrix();}
}
void pitchedRoof(float cx,float cz,float halfX,float halfZ,float edge,float peak,Color color){
    material(color);glBegin(GL_QUADS);
    for(int side:{-1,1}){glNormal3f(side*.38f,.92f,0);
        glVertex3f(cx,peak,cz-halfZ);glVertex3f(cx,peak,cz+halfZ);glVertex3f(cx+side*halfX,edge,cz+halfZ);glVertex3f(cx+side*halfX,edge,cz-halfZ);}
    glEnd();
    // Repeated raised strips make the roof read as tiles without texture files.
    for(int side:{-1,1})for(int row=1;row<12;row++){
        float t=row/12.0f;cube(cx+side*halfX*t,mix(peak,edge,t)+.025f,cz,.08f,.055f,halfZ*2,color.r>.4f?Color{.36f,.18f,.13f}:wood);
    }
    cube(cx,peak+.025f,cz,.16f,.13f,halfZ*2+.1f,color);
}
void drawHouseExterior(){
    cube(-10,.08f,-18.9f,14.5f,.16f,13.5f,{.48f,.48f,.42f});
    cube(-10,HOME_FLOOR-.035f,-18.9f,14,.07f,13,lightWood);
    // Front wall has three real openings: two windows and the hinged door.
    homeWallPiece(-16.2f,2.12f,HOUSE_FRONT,1.6f,4.24f,.24f);
    homeFrontWindow(-14.3f);homeWallPiece(-12.25f,2.12f,HOUSE_FRONT,1.9f,4.24f,.24f);
    homeWallPiece(-7.775f,2.12f,HOUSE_FRONT,1.95f,4.24f,.24f);homeFrontWindow(-5.7f);
    homeWallPiece(-3.8f,2.12f,HOUSE_FRONT,1.6f,4.24f,.24f);
    homeWallPiece(DOOR_X,3.83f,HOUSE_FRONT,DOOR_WIDTH, .82f,.24f);
    // Side windows also have physical holes in the wall.
    for(float x:{HOUSE_LEFT,HOUSE_RIGHT}){
        homeWallPiece(x,2.12f,-13.7f,.24f,4.24f,2.6f);
        homeWallPiece(x,2.12f,-19.5f,.24f,4.24f,4.0f);
        homeWallPiece(x,2.12f,-24.6f,.24f,4.24f,1.6f);
        for(float z:{-16.25f,-22.65f}){
            homeWallPiece(x,.7f,z,.24f,1.4f,2.5f);homeWallPiece(x,3.7f,z,.24f,1.08f,2.5f);
            for(float zz:{z-1.27f,z+1.27f})cube(x,2.45f,zz,.36f,1.86f,.10f,trim);
            for(float yy:{1.55f,3.35f})cube(x,yy,z,.36f,.10f,2.6f,trim);
            cube(x,2.45f,z,.34f,1.8f,.075f,trim);
        }
    }
    homeWallPiece(-10,2.12f,HOUSE_BACK,14,4.24f,.24f);
    cube(-10,4.25f,-18.9f,14,.12f,13,{.92f,.90f,.82f});
    for(float x:{HOUSE_LEFT,HOUSE_RIGHT})cube(x,2.35f,HOUSE_FRONT,.22f,4.4f,.36f,trim);
    for(float z:{HOUSE_FRONT-.01f,HOUSE_BACK}){material(plaster);glBegin(GL_TRIANGLES);glNormal3f(0,0,z==HOUSE_BACK?-1:1);
        glVertex3f(HOUSE_LEFT,4.4f,z);glVertex3f(HOUSE_RIGHT,4.4f,z);glVertex3f(-10,6.8f,z);glEnd();}
    Color roof=blend(roofClay,{.87f,.93f,.94f},atmosphere.seasons[WINTER]*.92f);
    pitchedRoof(-10,-18.9f,7.7f,7.2f,4.35f,6.95f,roof);
    cube(-15.3f,5.8f,-22.6f,.9f,3.3f,.9f,{.43f,.38f,.31f});cube(-15.3f,7.48f,-22.6f,1.15f,.18f,1.15f,wood);
    for(float x:{DOOR_X-1.31f,DOOR_X+1.31f})cube(x,1.86f,HOUSE_FRONT+.03f,.16f,3.43f,.4f,trim);
    cube(DOOR_X,3.60f,HOUSE_FRONT+.03f,2.78f,.16f,.4f,trim);
    glPushMatrix();glTranslatef(DOOR_X-DOOR_WIDTH*.5f,HOME_FLOOR,HOUSE_FRONT);glRotatef(homestead.doorAngle,0,1,0);
    cube(DOOR_WIDTH*.5f,1.67f,0,DOOR_WIDTH,3.34f,.14f,teal);
    for(float xx:{.60f,1.90f})for(float yy:{.70f,1.97f}){
        cube(xx,yy,.078f,.93f,1.03f,.025f,{.12f,.31f,.28f});cube(xx,yy,-.078f,.93f,1.03f,.025f,{.12f,.31f,.28f});}
    sphere(2.22f,1.50f,.13f,.095f,.095f,.075f,{.81f,.61f,.24f});sphere(2.22f,1.50f,-.13f,.095f,.095f,.075f,{.81f,.61f,.24f});
    glPopMatrix();
    cube(-10,.205f,-11.9f,2.8f,.045f,.65f,{.40f,.32f,.20f});
}

// ---------------- FURNITURE AND EVERYDAY OBJECTS ----------------
void furnitureLegs(float x,float z,float hx,float hz,float h){for(float dx:{-hx,hx})for(float dz:{-hz,hz})cube(x+dx,HOME_FLOOR+h*.5f,z+dz,.09f,h,.09f,wood);}
void smallMug(float x,float y,float z,Color color){
    glPushMatrix();glTranslatef(x,y,z);cylinder(.09f,.105f,.17f,color);disk(0,.172f,0,.084f,{.22f,.10f,.03f});
    glTranslatef(.10f,.10f,0);glutSolidTorus(.019,.05,6,12);glPopMatrix();
}
void housePlant(float x,float z){glPushMatrix();glTranslatef(x,HOME_FLOOR,z);cylinder(.28f,.34f,.48f,roofClay);
    for(int i=0;i<5;i++){float a=i*2*PI/5;limb({0,.45f,0},{std::cos(a)*.35f,1.1f+i*.08f,std::sin(a)*.35f},.025f,teal);
        sphere(std::cos(a)*.35f,1.1f+i*.08f,std::sin(a)*.35f,.24f,.36f,.12f,{.26f,.44f,.20f});}glPopMatrix();}
void wallPicture(float x,float y,float z,float w,float h){
    cube(x,y,z,w+.15f,h+.15f,.08f,wood);cube(x,y,z+.05f,w,h,.035f,{.44f,.68f,.68f});
    material({.30f,.46f,.30f});glBegin(GL_TRIANGLES);glNormal3f(0,0,1);
    glVertex3f(x-w*.5f,y-h*.45f,z+.075f);glVertex3f(x+w*.12f,y+h*.35f,z+.075f);glVertex3f(x+w*.42f,y-h*.45f,z+.075f);glEnd();
    sphere(x+w*.25f,y+h*.23f,z+.08f,.11f,.11f,.015f,{.95f,.79f,.44f});
}
void drawHouseInterior(){
    // Warm floorboards, with seams and occasional nail heads.
    for(int i=0;i<26;i++){
        float z=HOUSE_BACK+.25f+i*.5f;cube(-10,HOME_FLOOR-.01f,z,13.75f,.04f,.48f,i%3==0?Color{.57f,.40f,.24f}:Color{.65f,.47f,.30f});
        for(int j=0;j<4;j++)cube(-16.4f+j*3.8f+(i%2)*.35f,HOME_FLOOR+.012f,z,.022f,.009f,.42f,wood);
    }
    // The central hallway opens to both back rooms.
    for(auto span:std::array<std::array<float,2>,3>{{{{-17,-11.6f}},{{-9.4f,-7.1f}},{{-5,-3}}}})
        cube((span[0]+span[1])*.5f,2.2f,-20.5f,span[1]-span[0],4.0f,.18f,plaster);
    for(float x:{-10.5f,-6.05f})cube(x,3.70f,-20.5f,2.2f,1.0f,.18f,plaster);
    cube(-9.25f,2.2f,-23.0f,.18f,4.0f,4.8f,plaster);
    // Rug and soft living-room furniture.
    cube(-13.75f,.215f,-16.2f,5.4f,.035f,5.4f,{.32f,.43f,.38f});
    for(int i=0;i<13;i++)cube(-13.75f,.236f,-18.6f+i*.4f,5.15f,.006f,.07f,{.70f,.65f,.47f});
    cube(-15.7f,.59f,-16.2f,1.45f,.7f,3.65f,wood);
    cube(-16.27f,1.25f,-16.2f,.31f,1.2f,3.7f,{.27f,.43f,.43f});
    for(float z:{-17.0f,-15.4f})sphere(-15.6f,.99f,z,.66f,.25f,.74f,{.44f,.59f,.54f});
    for(float z:{-18.0f,-14.4f})cube(-15.7f,1.03f,z,1.5f,.7f,.22f,{.27f,.43f,.43f});
    sphere(-15.91f,1.34f,-17.25f,.20f,.37f,.42f,{.83f,.64f,.33f});sphere(-15.9f,1.34f,-15.15f,.20f,.37f,.42f,cream);
    glPushMatrix();glTranslatef(0,0,-1.2f);
    furnitureLegs(-13,-16.2f,.65f,.8f,.5f);cube(-13,.75f,-16.2f,1.65f,.12f,2,lightWood);
    cube(-13,.845f,-16.6f,.8f,.07f,.52f,{.64f,.28f,.15f});cube(-12.95f,.905f,-16.6f,.64f,.05f,.48f,cream);
    smallMug(-13.4f,.815f,-15.8f,cream);smallMug(-12.65f,.815f,-15.7f,teal);
    glPopMatrix();
    cube(-16.35f,1.49f,-19.2f,.65f,2.62f,1.75f,wood);
    for(int shelf=0;shelf<3;shelf++)for(int i=0;i<7;i++)cube(-15.98f,.65f+shelf*.72f,-19.87f+i*.22f,.26f,.38f+(i%3)*.065f,.16f,i%3==0?teal:i%3==1?roofClay:cream);
    // Sideboard, landscape print, and a floor lamp.
    wallPicture(-13.85f,2.65f,-20.38f,2.6f,1.3f);
    glPushMatrix();glTranslatef(-15.3f,HOME_FLOOR,-13.5f);cylinder(.29f,.25f,.07f,wood);cylinder(.035f,.035f,2.0f,{.48f,.41f,.25f});
    glTranslatef(0,1.9f,0);cylinder(.36f,.21f,.43f,cream);glPopMatrix();
    housePlant(-11.85f,-13.45f);
    // Kitchen: lower cupboards, worktop, sink, hob, fridge, kettle and dishes.
    cube(-3.65f,.70f,-17.8f,.9f,1.03f,3.4f,teal);cube(-3.65f,1.25f,-17.8f,1.04f,.1f,3.6f,cream);
    cube(-4.05f,.70f,-19.6f,1.6f,1.03f,.9f,teal);cube(-4.05f,1.25f,-19.6f,1.7f,.1f,1,cream);
    for(int i=0;i<4;i++){cube(-4.12f,.75f,-19.1f+i*.82f,.026f,.79f,.73f,{.24f,.48f,.41f});cube(-4.15f,.87f,-19.1f+i*.82f,.05f,.055f,.3f,wood);}
    cube(-3.68f,1.31f,-17.4f,.68f,.04f,1.0f,{.41f,.54f,.54f});cube(-3.68f,1.335f,-17.4f,.52f,.022f,.82f,{.20f,.35f,.35f});
    limb({-3.3f,1.3f,-17.4f},{-3.3f,1.7f,-17.4f},.035f,{.7f,.73f,.7f});limb({-3.3f,1.7f,-17.4f},{-3.65f,1.7f,-17.4f},.035f,{.7f,.73f,.7f});
    glPushMatrix();glTranslatef(1.45f,0,0);
    cube(-5.5f,1.32f,-19.6f,1.35f,.04f,.85f,{.17f,.20f,.19f});for(float x:{-5.9f,-5.1f})for(float z:{-19.85f,-19.35f})ring(x,1.35f,z,.16f,{.54f,.57f,.51f});
    sphere(-5.9f,1.50f,-19.85f,.19f,.22f,.19f,{.62f,.63f,.57f});limb({-5.75f,1.5f,-19.85f},{-5.58f,1.62f,-19.85f},.05f,{.62f,.63f,.57f});
    glPopMatrix();
    cube(-8.1f,1.48f,-19.1f,1.05f,2.6f,1.2f,{.87f,.88f,.80f});cube(-8.1f,1.68f,-18.485f,1.0f,.035f,.022f,wood);
    for(float y:{1.15f,2.12f})cube(-7.72f,y,-18.46f,.05f,.40f,.06f,wood);
    furnitureLegs(-6,-15.6f,.7f,.7f,.86f);cube(-6,1.09f,-15.6f,1.9f,.12f,1.9f,lightWood);
    for(float z:{-16.62f,-14.58f}){cube(-6,.66f,z,.88f,.13f,.68f,teal);furnitureLegs(-6,z,.32f,.23f,.44f);cube(-6,1.12f,z+(z<-15.6f?-.27f:.27f),.88f,.8f,.10f,teal);}
    disk(-6,1.16f,-15.6f,.32f,cream);for(int i=0;i<4;i++)sphere(-6+std::sin(i*2.4f)*.17f,1.26f,-15.6f+std::cos(i*2.4f)*.17f,.12f,.12f,.12f,i%2?Color{.91f,.64f,.12f}:Color{.69f,.25f,.12f});
    smallMug(-6.5f,1.16f,-15.1f,cream);
    // Bedroom with a made bed, quilt, pillows, wardrobe and bedside lamp.
    cube(-13.8f,.51f,-23.25f,3.05f,.58f,3.5f,wood);cube(-13.8f,.90f,-23.25f,2.94f,.28f,3.45f,cream);
    cube(-13.8f,1.4f,-24.94f,3.25f,1.65f,.18f,lightWood);
    cube(-13.8f,1.07f,-22.85f,2.98f,.11f,2.6f,{.33f,.51f,.50f});
    for(int i=0;i<6;i++)cube(-13.8f,1.13f,-23.9f+i*.42f,2.96f,.013f,.045f,{.76f,.76f,.58f});
    for(float x:{-14.5f,-13.1f})sphere(x,1.14f,-24.42f,.6f,.16f,.38f,{.94f,.91f,.79f});
    cube(-10.6f,1.68f,-24.5f,1.8f,2.9f,.96f,lightWood);cube(-10.6f,1.67f,-24.0f,.035f,2.65f,.025f,wood);
    for(float x:{-10.76f,-10.44f})sphere(x,1.55f,-23.96f,.04f,.04f,.03f,cream);
    cube(-16,.64f,-24.3f,.85f,.85f,.9f,lightWood);sphere(-16,1.25f,-24.3f,.15f,.22f,.15f,teal);
    glPushMatrix();glTranslatef(-16,1.38f,-24.3f);cylinder(.27f,.16f,.32f,cream);glPopMatrix();
    wallPicture(-13.7f,2.75f,HOUSE_BACK+.15f,1.9f,.9f);
    // Bathroom: tiled floor, bath, toilet, vanity and mirror.
    for(int i=0;i<10;i++)for(int j=0;j<7;j++)cube(-8.9f+i*.56f,.205f,-25+j*.6f,.54f,.028f,.58f,(i+j)%2?Color{.70f,.79f,.75f}:Color{.89f,.91f,.83f});
    cube(-7.75f,.42f,-23.65f,1.95f,.40f,2.65f,cream);cube(-7.75f,.64f,-23.65f,1.62f,.06f,2.28f,{.49f,.71f,.74f});
    for(float x:{-8.68f,-6.82f})cube(x,.72f,-23.65f,.16f,.5f,2.65f,cream);
    for(float z:{-24.9f,-22.4f})cube(-7.75f,.72f,z,1.95f,.5f,.16f,cream);
    sphere(-4.0f,.55f,-24.4f,.38f,.43f,.56f,cream);cube(-4.0f,.99f,-24.98f,.78f,.93f,.30f,cream);
    glPushMatrix();glTranslatef(-4,.84f,-24.25f);glRotatef(90,1,0,0);material(cream);glutSolidTorus(.07,.28,10,20);glPopMatrix();
    cube(-3.7f,.7f,-21.8f,.80f,1.0f,1.25f,teal);sphere(-3.7f,1.21f,-21.8f,.48f,.13f,.68f,cream);sphere(-3.7f,1.25f,-21.8f,.32f,.06f,.46f,{.50f,.63f,.63f});
    cube(-3.16f,2.45f,-21.8f,.08f,1.40f,1.4f,wood);cube(-3.21f,2.45f,-21.8f,.026f,1.21f,1.22f,{.60f,.76f,.77f});
    // Pendant lights use the same warm state as the E-interactable floor lamp.
    for(Vec3 p:{Vec3{-10,3.8f,-17},Vec3{-13.8f,3.8f,-23},Vec3{-6,3.8f,-23}}){
        limb({p.x,4.2f,p.z},p,.024f,wood);sphere(p.x,p.y-.15f,p.z,.30f,.16f,.30f,homestead.lampOn?Color{1,.86f,.48f}:cream);}
}
void drawHomePorch(){
    cube(-10,3.55f,-10.5f,13.8f,.20f,4.2f,teal);
    for(float x:{-16.3f,-3.7f})cube(x,1.80f,-9.1f,.20f,3.6f,.20f,lightWood);
    cube(-10,.13f,-10.6f,13.6f,.10f,3.6f,lightWood);
    cube(PORCH_SEAT.x,.87f,PORCH_SEAT.z,2.15f,.14f,.9f,lightWood);
    for(float x:{PORCH_SEAT.x-.8f,PORCH_SEAT.x+.8f})cube(x,.49f,PORCH_SEAT.z,.14f,.72f,.65f,wood);
    cube(PORCH_SEAT.x,1.42f,PORCH_SEAT.z-.46f,2.15f,.80f,.12f,teal);
    if(atmosphere.seasons[WINTER]>.1f)cube(-10,3.7f,-10.5f,13.9f,.08f*atmosphere.seasons[WINTER],4.3f,{.91f,.95f,.95f});
    cube(-7.0f,2.6f,-12.23f,1.45f,.50f,.10f,wood);drawWorldLabel({-7.6f,2.6f,-12.15f},"WELCOME HOME",cream);
}
void drawHomesteadPaths(){
    for(int i=0;i<12;i++)cube(-10,.115f,-12+i*.55f,1.65f,.05f,.49f,{.58f,.58f,.47f});
    for(int i=0;i<16;i++)cube(-9.4f+i*.6f,.10f,-6.0f,.55f,.045f,1.45f,{.62f,.59f,.45f});
    for(int i=0;i<23;i++)cube(2.3f+i*.6f,.11f,-1,.53f,.04f,1.3f,{.63f,.54f,.36f});
}

// ---------------- ANIMATED COW FARM ----------------
void fenceRail(float x1,float z1,float x2,float z2){
    float n=std::hypot(x2-x1,z2-z1);int posts=int(std::ceil(n/2));
    for(int i=0;i<=posts;i++){float t=float(i)/posts;cube(mix(x1,x2,t),.77f,mix(z1,z2,t),.15f,1.54f,.15f,lightWood);}
    for(float y:{.53f,1.13f})limb({x1,y,z1},{x2,y,z2},.065f,lightWood);
}
void drawCow(const Cow& c,int index){
    shadow(c.position.x,c.position.z,.85f*c.scale,1.40f*c.scale);
    glPushMatrix();glTranslatef(c.position.x,c.position.y,c.position.z);glRotatef(-c.yaw,0,1,0);glScalef(c.scale,c.scale,c.scale);
    Color milk{.90f,.87f,.76f},spot=index==1?Color{.38f,.20f,.11f}:Color{.105f,.12f,.105f};
    sphere(0,1.35f,0,.63f,.57f,1.17f,milk);
    // Raised patches follow the rounded body rather than painting a rectangular box.
    for(int side:{-1,1}){sphere(side*.54f,1.5f,-.43f,.12f,.30f,.35f,spot);sphere(side*.57f,1.2f,.49f,.09f,.25f,.42f,spot);}
    sphere(.10f,1.87f,.16f,.31f,.055f,.46f,spot);
    for(int side:{-1,1})for(int end:{-1,1}){
        glPushMatrix();glTranslatef(side*.43f,1.10f,end*.74f);glRotatef(c.walking?std::sin(c.gait+(side==end?PI:0))*19:0,1,0,0);
        limb({0,0,0},{0,-.90f,0},.115f,milk);sphere(0,-.92f,-.035f,.145f,.14f,.18f,spot);glPopMatrix();}
    sphere(0,.82f,.62f,.28f,.20f,.28f,{.80f,.56f,.43f});
    for(float x:{-.13f,.13f})for(float z:{.5f,.72f})limb({x,.76f,z},{x,.59f,z},.035f,{.80f,.56f,.43f});
    glPushMatrix();glTranslatef(0,1.40f,-.88f);glRotatef(-58*c.graze+std::sin(world.time*1.7f+index)*2,1,0,0);
    sphere(0,-.02f,-.38f,.34f,.37f,.48f,milk);sphere(0,-.16f,-.88f,.32f,.24f,.40f,milk);
    sphere(0,-.19f,-1.13f,.33f,.19f,.21f,{.65f,.47f,.37f});
    for(int side:{-1,1}){
        sphere(side*.29f,.07f,-.66f,.045f,.065f,.065f,spot);sphere(side*.305f,.085f,-.69f,.014f,.02f,.019f,cream);
        sphere(side*.43f,.20f,-.32f,.23f,.075f,.13f,spot);
        limb({side*.19f,.28f,-.34f},{side*.31f,.54f,-.22f},.058f,cream);
        sphere(side*.14f,-.16f,-1.32f,.048f,.035f,.022f,spot);
    }
    cube(0,-.30f+std::sin(world.time*5+index)*.012f,-1.25f,.28f,.018f,.12f,spot);glPopMatrix();
    glPushMatrix();glTranslatef(0,1.53f,1.10f);glRotatef(std::sin(world.time*2+index)*22,0,0,1);
    limb({0,0,0},{0,-.68f,.30f},.04f,milk);sphere(0,-.72f,.32f,.09f,.16f,.09f,spot);glPopMatrix();
    glPopMatrix();
}
void drawFarm(){
    // A permanently open pedestrian gate connects the farm to the valley path.
    fenceRail(16,-9.8f,34,-9.8f);fenceRail(34,-9.8f,34,7.2f);fenceRail(34,7.2f,16,7.2f);
    fenceRail(16,-9.8f,16,-2.5f);fenceRail(16,.5f,16,7.2f);fenceRail(16,-2.5f,18.2f,-2.5f);
    cube(16,2.12f,-1, .18f,.54f,3.2f,wood);drawWorldLabel({15.8f,2.2f,.1f},"COW FARM",cream);
    // Barn with red vertical boards, white trim and an open front.
    cube(25,.13f,-6.9f,9.0f,.20f,4.6f,{.47f,.38f,.23f});
    for(int i=0;i<23;i++)cube(20.6f+i*.4f,1.67f,-9.2f,.38f,3.14f,.16f,{.54f,.22f,.14f});
    for(float x:{20.5f,29.5f})for(int i=0;i<12;i++)cube(x,1.67f,-9.1f+i*.4f,.16f,3.14f,.38f,{.60f,.27f,.16f});
    for(float x:{20.45f,29.55f})cube(x,1.75f,-4.60f,.18f,3.5f,.20f,cream);
    cube(25,3.32f,-4.6f,9.25f,.20f,.20f,cream);
    pitchedRoof(25,-6.9f,5.05f,2.8f,3.35f,4.75f,blend({.25f,.31f,.30f},{.87f,.93f,.94f},atmosphere.seasons[WINTER]));
    for(int i=0;i<3;i++)cube(21.6f+i*1.1f,.47f,-8.35f,.98f,.65f,.70f,{.75f,.59f,.24f});
    for(float x:{18.3f,31.8f}){
        cube(x,.39f,-6.2f,1.0f,.60f,2.7f,wood);
        cube(x,.72f,-6.2f,.85f,.05f,2.48f,x<20?Color{.71f,.58f,.23f}:Color{.28f,.58f,.61f});
        for(float xx:{x-.53f,x+.53f})cube(xx,.77f,-6.2f,.10f,.32f,2.8f,lightWood);
    }
    if(homestead.feedTime>0)for(int i=0;i<18;i++)cube(18.3f+std::sin(i*2.4f)*.3f,.82f+(i%3)*.07f,-7.2f+(i%7)*.3f,.28f,.045f,.075f,{.87f,.72f,.27f});
    for(int i=0;i<4;i++)drawCow(homestead.cows[i],i);
}
void drawHomeGlass(){
    glDisable(GL_LIGHTING);glColor4f(.59f,.81f,.85f,.13f);glBegin(GL_QUADS);
    for(float x:{-14.3f,-5.7f}){glVertex3f(x-1.06f,1.58f,HOUSE_FRONT);glVertex3f(x+1.06f,1.58f,HOUSE_FRONT);glVertex3f(x+1.06f,3.30f,HOUSE_FRONT);glVertex3f(x-1.06f,3.30f,HOUSE_FRONT);}
    for(float x:{HOUSE_LEFT,HOUSE_RIGHT})for(float z:{-16.25f,-22.65f}){glVertex3f(x,1.6f,z-1.2f);glVertex3f(x,1.6f,z+1.2f);glVertex3f(x,3.28f,z+1.2f);glVertex3f(x,3.28f,z-1.2f);}
    glEnd();glEnable(GL_LIGHTING);
}
void setupHomeLighting(){
    if(!homestead.lampOn){glDisable(GL_LIGHT1);return;}
    glEnable(GL_LIGHT1);GLfloat position[]={-10,3.65f,-18,1},ambient[]={.12f,.09f,.055f,1},diffuse[]={.95f,.75f,.44f,1};
    glLightfv(GL_LIGHT1,GL_POSITION,position);glLightfv(GL_LIGHT1,GL_AMBIENT,ambient);glLightfv(GL_LIGHT1,GL_DIFFUSE,diffuse);
    glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,1);glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,.025f);glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,.015f);
}
void drawHomeHUD(){
    if(insideHouse(human.position.x,human.position.z)){
        rect(width*.5f-210,24,420,54,{.055f,.11f,.105f},.9f);
        centered(47,"HOME  /  Living room, kitchen, bedroom & bath",cream,GLUT_BITMAP_HELVETICA_12);
        centered(66,"[ V ] Change view    [ E ] Use nearby objects",{.74f,.84f,.77f},GLUT_BITMAP_HELVETICA_12);
    }else if(std::hypot(human.position.x-DOOR_X,human.position.z-HOUSE_FRONT)<5){
        centered(height-85.0f,homestead.doorAngle>85?"Welcome home. Walk through the open door.":"The front door opens as you approach.",cream,GLUT_BITMAP_HELVETICA_12);
    }
}

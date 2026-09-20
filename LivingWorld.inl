// Implementation included by main.cpp: one compile command still builds the whole project.
// Sections: seasons/weather -> navigation -> human state machine -> articulated model -> particles.

float randomUnit(){particleSeed=1664525u*particleSeed+1013904223u;return float(particleSeed>>8)/16777216.0f;}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 unit(Vec3 v){float n=length(v);return n>.00001f?v*(1/n):Vec3{0,1,0};}
Vec3 rotateY(Vec3 p,float angle){float a=angle*PI/180;return {p.x*std::cos(a)+p.z*std::sin(a),p.y,-p.x*std::sin(a)+p.z*std::cos(a)};}
Vec3 rotateX(Vec3 p,float angle){float a=angle*PI/180;return {p.x,p.y*std::cos(a)-p.z*std::sin(a),p.y*std::sin(a)+p.z*std::cos(a)};}
Vec3 humanToWorld(Vec3 p){return human.position+rotateY(p,-human.yaw);}
Vec3 worldToHuman(Vec3 p){return rotateY(p-human.position,human.yaw);}
float floorHeight(float x,float z){float home=homeFloorHeight(x,z);if(home>=0)return home;return (z>8.1f&&z<18.9f&&std::abs(x)<1.6f)?.42f:std::max(.02f,terrain(x,z));}
Vec3 interactionOrigin(){return human.position+Vec3{0,1.25f,0};}
float rainAmount(){return clamp(atmosphere.weatherMix[RAIN]*.7f+atmosphere.weatherMix[STORM]);}
float snowAmount(){return clamp(atmosphere.weatherMix[SNOW]+atmosphere.seasons[WINTER]*.45f*(1-rainAmount()));}
float coldAmount(){return clamp(atmosphere.seasons[WINTER]+atmosphere.weatherMix[SNOW]*.8f);}
float cloudAmount(){return clamp(atmosphere.weatherMix[CLOUDY]*.7f+rainAmount()+atmosphere.weatherMix[SNOW]*.65f+atmosphere.weatherMix[WINDY]*.35f+atmosphere.seasons[AUTUMN]*.15f);}
float seasonalWildlife(){return (1-.88f*atmosphere.seasons[WINTER]-.3f*atmosphere.seasons[AUTUMN])*(1-.85f*rainAmount());}
Color seasonColor(Color a,Color b,Color c,Color d){
    auto s=atmosphere.seasons;
    return {a.r*s[0]+b.r*s[1]+c.r*s[2]+d.r*s[3],a.g*s[0]+b.g*s[1]+c.g*s[2]+d.g*s[3],a.b*s[0]+b.b*s[1]+c.b*s[2]+d.b*s[3]};
}
Color groundColor(){
    Color c=seasonColor({.40f,.60f,.27f},{.43f,.56f,.24f},{.48f,.41f,.23f},{.86f,.91f,.91f});
    c=blend(c,{.36f,.30f,.20f},damage()*(1-.55f*atmosphere.seasons[WINTER]));
    return blend(c,{c.r*.68f,c.g*.73f,c.b*.82f},atmosphere.wetness*.65f);
}
Color foliageColor(){return blend(seasonColor({.32f,.56f,.22f},{.20f,.44f,.24f},{.85f,.39f,.10f},{.40f,.50f,.47f}),{.37f,.29f,.15f},1-ecology(.4f));}

void resetLivingWorld(){
    human=Human{};atmosphere=Atmosphere{};particleSeed=173;windStrength=.22f;cameraDistance=6;
    human.position.y=floorHeight(human.position.x,human.position.z);
    cameraPitch=-16;cameraYaw=0;
    for(auto& p:atmosphere.rain)p={randomUnit()*48-24,randomUnit()*22,randomUnit()*48-18,13+randomUnit()*7};
    for(auto& p:atmosphere.snow)p={randomUnit()*48-24,randomUnit()*19,randomUnit()*48-18,.55f+randomUnit()*.8f,randomUnit()*360};
    for(size_t i=0;i<atmosphere.leaves.size();i++){
        const auto& t=world.trees[i%world.trees.size()];
        atmosphere.leaves[i]={t.p.x+(randomUnit()-.5f)*3,randomUnit()*5+.2f,t.p.z+(randomUnit()-.5f)*3,randomUnit()*360,.45f+randomUnit()*.65f,randomUnit()*6};
    }
    for(auto& p:atmosphere.steam)p={(randomUnit()-.5f)*.15f,0,(randomUnit()-.5f)*.12f,randomUnit(),.035f+randomUnit()*.03f};
    updateFollowCamera(0,true);
}
void selectWeather(Weather weather,bool react){
    atmosphere.weather=weather;atmosphere.reactionTime=react?3.2f:0;
    if(react&&(weather==RAIN||weather==STORM))human.shelterRequested=true;
    else if(weather!=RAIN&&weather!=STORM){
        human.shelterRequested=false;
        if(human.action==SHELTER_WALK){human.action=FREE_WALK;human.route.clear();}
        if(human.action==SHELTER_REST){human.action=SHELTER_STAND;human.actionTime=0;}
    }
}
void selectSeason(Season season,bool manual){
    atmosphere.season=season;atmosphere.cycleTime=0;if(manual)atmosphere.automatic=false;
    selectWeather(season==WINTER?SNOW:season==AUTUMN?WINDY:CLEAR);
    say(std::string(seasonNames[season])+" arrives. The valley changes around you.",4);
}
void updateAtmosphere(float dt){
    atmosphere.cycleTime+=dt;
    if(atmosphere.automatic&&atmosphere.cycleTime>=90)selectSeason(Season((atmosphere.season+1)%4),false);
    float ease=1-std::exp(-dt*.8f);
    for(int i=0;i<4;i++)atmosphere.seasons[i]=mix(atmosphere.seasons[i],i==atmosphere.season?1.0f:0.0f,ease*.6f);
    for(int i=0;i<6;i++)atmosphere.weatherMix[i]=mix(atmosphere.weatherMix[i],i==atmosphere.weather?1.0f:0.0f,ease);
    const float winds[]={.22f,.35f,.65f,1.6f,.4f,1.2f};windStrength=0;
    for(int i=0;i<6;i++)windStrength+=winds[i]*atmosphere.weatherMix[i];
    windStrength+=atmosphere.seasons[AUTUMN]*.12f+disaster.storm*1.8f;
    atmosphere.wetness=mix(atmosphere.wetness,rainAmount(),1-std::exp(-dt*.25f));
    atmosphere.reactionTime=std::max(0.0f,atmosphere.reactionTime-dt);
    atmosphere.lightningFlash=std::max(0.0f,atmosphere.lightningFlash-dt*4.5f);
    if(atmosphere.weather==STORM&&atmosphere.weatherMix[STORM]>.65f){
        atmosphere.lightningTimer-=dt;
        if(atmosphere.lightningTimer<=0){atmosphere.lightningFlash=1;atmosphere.lightningTimer=10+randomUnit()*7;}
    }else atmosphere.lightningTimer=11;
    auto wrap=[](float& p,float center){if(p<center-24)p+=48;if(p>center+24)p-=48;};
    for(auto& p:atmosphere.rain){
        p.y-=p.speed*dt;p.x+=windStrength*dt*3.0f;
        wrap(p.x,human.position.x);wrap(p.z,human.position.z);
        if(p.y<floorHeight(p.x,p.z)){p.y=18+randomUnit()*5;p.x=human.position.x+(randomUnit()-.5f)*48;p.z=human.position.z+(randomUnit()-.5f)*48;}
    }
    for(auto& p:atmosphere.snow){
        p.y-=p.speed*dt;p.x+=(windStrength*.8f+std::sin(world.time+p.z)*.25f)*dt;p.rotation+=dt*27;
        wrap(p.x,human.position.x);wrap(p.z,human.position.z);
        if(p.y<floorHeight(p.x,p.z)){p.y=16+randomUnit()*4;p.x=human.position.x+(randomUnit()-.5f)*48;p.z=human.position.z+(randomUnit()-.5f)*48;}
    }
    for(size_t i=0;i<atmosphere.leaves.size();i++){
        auto& p=atmosphere.leaves[i];p.y-=p.speed*dt;p.x+=(windStrength*.6f+std::sin(world.time+p.drift)*.3f)*dt;
        p.z+=std::cos(world.time*.6f+p.drift)*dt*.3f;p.rotation+=dt*(35+windStrength*45);
        if(p.y<terrain(p.x,p.z)+.04f){const auto& tree=world.trees[i%world.trees.size()];p.x=tree.p.x+(randomUnit()-.5f)*3;p.z=tree.p.z+(randomUnit()-.5f)*3;p.y=3+randomUnit()*3;}
    }
    for(auto& p:atmosphere.steam){p.life+=dt*(.42f-coldAmount()*.12f);if(p.life>1){p.life-=1;p.x=(randomUnit()-.5f)*.15f;p.z=(randomUnit()-.5f)*.12f;}p.y=p.life*1.05f;}
}

// ---------------- NAVIGATION AND FOLLOW CAMERA ----------------
// A small breadth-first search routes both tea approaches and shelter walks through
// the same collision map as WASD movement. It cannot cross the river except at the bridge.
bool planWalk(Vec3 destination){
    constexpr int nx=145,nz=149;
    auto cell=[](Vec3 p){int x=int(std::round((p.x+36)*2)),z=int(std::round((p.z+34)*2));return std::max(0,std::min(nx-1,x))+std::max(0,std::min(nz-1,z))*nx;};
    auto point=[](int n){return Vec3{-36+(n%nx)*.5f,0,-34+(n/nx)*.5f};};
    std::vector<int> previous(nx*nz,-1);std::queue<int> todo;
    int start=cell(human.position),goal=cell(destination);previous[start]=start;todo.push(start);
    while(!todo.empty()&&previous[goal]<0){
        int n=todo.front();todo.pop();
        for(int offset:{1,-1,nx,-nx}){
            int next=n+offset;if(next<0||next>=nx*nz)continue;
            if((offset==1||offset==-1)&&next/nx!=n/nx)continue;
            Vec3 p=point(next),from=point(n),middle=(p+from)*.5f;
            // Keep a margin at the bridge entrances instead of cutting a bank corner.
            bool riverMargin=p.z>=8.75f&&p.z<=18.25f&&std::abs(p.x)>1.0f;
            if(previous[next]>=0||riverMargin||checkCollision(p.x,p.z)||checkCollision(middle.x,middle.z))continue;
            previous[next]=n;todo.push(next);
        }
    }
    human.route.clear();human.waypoint=0;if(previous[goal]<0)return false;
    for(int n=goal;n!=start;n=previous[n])human.route.push_back(point(n));
    std::reverse(human.route.begin(),human.route.end());human.route.push_back(destination);return true;
}
bool moveHuman(Vec3 delta){
    bool moved=false;
    if(!checkCollision(human.position.x+delta.x,human.position.z)){human.position.x+=delta.x;moved=moved||std::abs(delta.x)>.000001f;}
    if(!checkCollision(human.position.x,human.position.z+delta.z)){human.position.z+=delta.z;moved=moved||std::abs(delta.z)>.000001f;}
    human.position.y=floorHeight(human.position.x,human.position.z);return moved;
}
bool followRoute(float dt,float speed){
    float remaining=speed*dt;
    while(human.waypoint<human.route.size()&&remaining>.0001f){
        Vec3 delta=human.route[human.waypoint]-human.position;delta.y=0;float distance=length(delta);
        if(distance<.001f){human.position.x=human.route[human.waypoint].x;human.position.z=human.route[human.waypoint].z;human.waypoint++;continue;}
        float step=std::min(distance,remaining);
        if(!moveHuman(delta*(step/distance))){human.route.clear();human.action=FREE_WALK;say("Path blocked. You can walk around the obstacle and try again.");return false;}
        remaining-=step;
        if(step>=distance-.0001f&&length(Vec3{human.route[human.waypoint].x-human.position.x,0,human.route[human.waypoint].z-human.position.z})<.001f){
            human.position.x=human.route[human.waypoint].x;human.position.z=human.route[human.waypoint].z;human.waypoint++;
        }
    }
    return human.waypoint>=human.route.size();
}
bool teaActive(){return human.action>=TEA_APPROACH&&human.action<=TEA_STAND;}
void updateFollowCamera(float dt,bool snap){
    float yaw=cameraYaw*PI/180,pitch=cameraPitch*PI/180;
    Vec3 target=human.position+Vec3{0,1.4f-human.sitBlend*.12f,0};
    Vec3 direction{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
    Vec3 desired=target-direction*cameraDistance;
    if(teaActive()&&human.action!=TEA_APPROACH){target={0,1.35f,.95f};desired={3.8f,2.9f,-1.55f};}
    if(human.action>=SHELTER_SIT){target=human.position+Vec3{0,1.25f,0};desired={PORCH_SEAT.x,2.4f,-6.5f};}
    if(homestead.firstPerson&&human.action==FREE_WALK){Vec3 eye=human.position+Vec3{0,2.05f,0};cameraX=eye.x;cameraY=eye.y;cameraZ=eye.z;viewAim=eye+direction*3;return;}
    // Shorten the boom before it enters solid scenery. The camera may look across water.
    Vec3 ray=desired-target;
    for(float t=.12f;t<1;t+=.04f){Vec3 p=target+ray*t;
        bool blocked=houseCameraCollision(p)||(box(p.x,p.z,26,-22,4.5f,3.5f)&&p.y<6);
        for(const auto& tree:world.trees)if(tree.state!=REMOVED){
            if(std::hypot(p.x-tree.p.x,p.z-tree.p.z)<.5f&&p.y<4)blocked=true;
            Vec3 crown{(p.x-tree.p.x)/(1.7f*tree.size),(p.y-3.6f*tree.size)/(1.9f*tree.size),(p.z-tree.p.z)/(1.6f*tree.size)};
            if(ecology(.4f)>.12f&&length(crown)<1)blocked=true;
        }
        if(blocked){desired=target+ray*std::max(.1f,t-.08f);break;}}
    desired.y=std::max(desired.y,terrain(desired.x,desired.z)+.55f);
    float amount=snap?1:1-std::exp(-dt*(teaActive()?2.6f:9.0f));
    Vec3 eye=camera()*(1-amount)+desired*amount;cameraX=eye.x;cameraY=eye.y;cameraZ=eye.z;
    viewAim=viewAim*(1-amount)+target*amount;
}
bool beginTea(){
    if(human.action!=FREE_WALK)return false;
    if(!planWalk({0,0,2.05f})){say("Walk around the table to the chair, then try again.");return false;}
    human.action=TEA_APPROACH;human.actionTime=0;human.leaveRequested=false;seated=true;seatTime=0;
    std::fill(std::begin(keys),std::end(keys),false);return true;
}
void leaveTea(){
    if(!teaActive())return;
    human.leaveRequested=true;
    if(human.action==TEA_APPROACH){human.action=FREE_WALK;human.route.clear();seated=false;human.leaveRequested=false;}
    else if(human.action==TEA_REST){human.action=TEA_STAND;human.actionTime=0;}
    // During reach/lift/drink, finish returning the cup before standing up.
}
void animateSit(){
    float t=smooth(human.actionTime/1.45f);human.sitBlend=t;
    if(human.action==TEA_SIT){human.position.z=mix(2.05f,1.27f,t);human.position.y=floorHeight(0,2.05f);}
}
float hipHeight(){return mix(1.18f-human.walkBlend*.06f,.89f,human.sitBlend)+human.bodyBob;}
float teaLean(){return human.reach*(1-human.cupLift*.8f);}
Vec3 upperBodyPoint(Vec3 p){
    return Vec3{0,hipHeight(),-.18f*teaLean()}+rotateX(p,-14*teaLean());
}
Vec3 mouthPosition(){return humanToWorld(upperBodyPoint({0,.88f,-.18f}));}
Vec3 cupHandlePosition(){return human.cupPosition+rotateX(Vec3{.43f,.23f,0}*CUP_SCALE,human.cupTilt);}
Vec3 rightHandPosition(){
    Vec3 rest=humanToWorld({.39f,hipHeight()-.45f,-.03f});
    if(teaActive())return rest*(1-human.reach)+cupHandlePosition()*human.reach;
    return rest;
}
void animateDrink(){
    float t=human.actionTime;
    human.drinkProgress=clamp(t/9.4f);
    float reach=smooth(t/1.5f)*(1-smooth((t-7.8f)/1.6f));
    float lift=smooth((t-1.5f)/2.0f)*(1-smooth((t-5.6f)/2.2f));
    float tilt=smooth((t-3.5f)/.7f)*(1-smooth((t-5.0f)/.6f));
    human.reach=reach;human.cupLift=lift;human.cupTilt=tilt*(27-12*atmosphere.weatherMix[STORM]);
    Vec3 mouth=mouthPosition();
    Vec3 sip=mouth-rotateX(Vec3{0,.42f,.18f}*CUP_SCALE,human.cupTilt);
    human.cupPosition=Vec3{.12f,1.37f,0}*(1-lift)+sip*lift;
    human.cupPosition.y+=std::sin(lift*PI)*.16f;
    human.cupHeld=t>=1.5f&&t<7.8f;
}
void updateHuman(float dt){
    Vec3 before=human.position;
    bool input=keys['w']||keys['a']||keys['s']||keys['d'];
    if(input){
        human.shelterRequested=false;
        if(human.action==SHELTER_WALK){human.action=FREE_WALK;human.route.clear();}
        if(human.action==SHELTER_REST){human.action=SHELTER_STAND;human.actionTime=0;}
    }
    if(human.shelterRequested&&human.action==FREE_WALK){
        if(insideHouse(human.position.x,human.position.z)){human.shelterRequested=false;}
        else {
        if(planWalk(PORCH_SEAT)){human.action=SHELTER_WALK;human.actionTime=0;say("Rain is coming. Heading for the porch. WASD takes control.",5);}
        human.shelterRequested=false;
        }
    }
    human.actionTime+=dt;
    auto change=[](HumanAction action){human.action=action;human.actionTime=0;};
    switch(human.action){
    case FREE_WALK:{
        float a=cameraYaw*PI/180,f=float(keys['w'])-float(keys['s']),r=float(keys['d'])-float(keys['a']);
        float norm=std::max(1.0f,std::hypot(f,r));float speed=3.2f*(1-.23f*coldAmount());
        moveHuman({(std::sin(a)*f+std::cos(a)*r)*speed*dt/norm,0,(-std::cos(a)*f+std::sin(a)*r)*speed*dt/norm});
        cameraPitch=clamp(cameraPitch+(float(keys['c'])-float(keys['q']))*dt*28,-65,homestead.firstPerson?65.0f:-5.0f);
        break;}
    case TEA_APPROACH:if(followRoute(dt,2.25f)){human.turnStart=human.yaw;change(TEA_TURN);}break;
    case TEA_TURN:
        human.yaw=human.turnStart+std::remainder(-human.turnStart,360.0f)*smooth(human.actionTime/.75f);
        if(human.actionTime>=.75f){human.yaw=0;change(TEA_SIT);}break;
    case TEA_SIT:animateSit();if(human.actionTime>=1.45f){human.sitBlend=1;human.position.z=1.27f;change(TEA_DRINK);}break;
    case TEA_DRINK:
        animateDrink();
        if(human.actionTime>=9.4f){human.reach=0;human.cupHeld=false;human.cupLift=0;human.cupTilt=0;human.cupPosition={.12f,1.37f,0};
            world.teaVisited=true;if(world.state==RESTORED)world.finished=true;
            change(TEA_REST);seatTime=0;}
        break;
    case TEA_REST:
        seatTime+=dt;if(human.leaveRequested||human.shelterRequested){change(TEA_STAND);}break;
    case TEA_STAND:{
        float t=smooth(human.actionTime/1.35f);human.sitBlend=1-t;human.position.z=mix(1.27f,2.05f,t);
        if(t>=1){seated=false;human.leaveRequested=false;change(FREE_WALK);}break;}
    case SHELTER_WALK:if(followRoute(dt,4.1f)){human.turnStart=human.yaw;change(SHELTER_SIT);}break;
    case SHELTER_SIT:
        human.yaw=human.turnStart+std::remainder(180-human.turnStart,360.0f)*smooth(human.actionTime/.7f);
        animateSit();if(human.actionTime>=1.45f){human.sitBlend=1;human.yaw=180;change(SHELTER_REST);}break;
    case SHELTER_REST:
        if(atmosphere.weather!=RAIN&&atmosphere.weather!=STORM)change(SHELTER_STAND);
        break;
    case SHELTER_STAND:
        human.sitBlend=1-smooth(human.actionTime/1.2f);
        if(human.actionTime>=1.2f){human.sitBlend=0;change(FREE_WALK);}break;
    }
    Vec3 moved=human.position-before;moved.y=0;
    bool walking=human.action==FREE_WALK||human.action==TEA_APPROACH||human.action==SHELTER_WALK;
    float distance=walking?length(moved):0;
    if(distance>.0001f){
        float target=std::atan2(moved.x,-moved.z)*180/PI;
        human.yaw+=std::remainder(target-human.yaw,360.0f)*(1-std::exp(-dt*14));
        human.walkCycle+=distance*6.0f; // Distance-driven gait stops when collision stops movement.
    }
    human.walkBlend=mix(human.walkBlend,distance>.0001f?1.0f:0.0f,1-std::exp(-dt*15));
    human.legAngle=std::sin(human.walkCycle)*30*human.walkBlend;
    human.armAngle=std::sin(human.walkCycle)*25*human.walkBlend;
    human.bodyBob=std::abs(std::sin(human.walkCycle))*human.walkBlend*.03f;
    updateFollowCamera(dt);
}

// ---------------- HIERARCHICAL HUMAN MODEL ----------------
Color skin{.70f,.44f,.28f},shirt{.16f,.43f,.45f},pants{.18f,.24f,.30f},shoes{.16f,.105f,.075f};
void limb(Vec3 a,Vec3 b,float radius,Color color){
    Vec3 d=b-a;float n=length(d);if(n<.00001f)return;
    glPushMatrix();glTranslatef(a.x,a.y,a.z);
    float angle=std::acos(clamp(d.y/n,-1,1))*180/PI;
    if(std::hypot(d.x,d.z)>.00001f)glRotatef(angle,d.z,0,-d.x);else if(d.y<0)glRotatef(180,1,0,0);
    cylinder(radius,radius*.88f,n,color);glPopMatrix();
}
void drawFoot(){sphere(0,.07f,-.105f,.12f,.10f,.25f,shoes);cube(0,.025f,-.1f,.24f,.045f,.42f,{.24f,.20f,.14f});}
void drawHand(Vec3 p){sphere(p.x,p.y,p.z,.075f,.105f,.085f,skin);}
void drawLeg(int side){
    // During the stance half, the foot moves backward relative to the body by the
    // same distance the body advances. The swing half lifts and returns that foot.
    // Two-link angles keep soles on the ground and knees bending forward.
    float phase=std::fmod(human.walkCycle+(side<0?PI:0),2*PI)/(2*PI);
    float z=0,lift=0;
    if(phase<.5f)z=-.26f+phase*1.04f;
    else{float t=(phase-.5f)*2;z=.26f-.52f*smooth(t);lift=std::sin(t*PI)*.18f;}
    z=mix(z*human.walkBlend,-.50f,human.sitBlend);lift*=human.walkBlend*(1-human.sitBlend);
    float dy=hipHeight()-(.03f+lift),distance=clamp(std::hypot(dy,z),.1f,1.149f);
    float alpha=std::acos(clamp((.58f*.58f+distance*distance-.57f*.57f)/(2*.58f*distance),-1,1));
    float thigh=(std::atan2(-z,dy)+alpha)*180/PI;
    float knee=-std::acos(clamp((distance*distance-.58f*.58f-.57f*.57f)/(2*.58f*.57f),-1,1))*180/PI;
    glPushMatrix();glTranslatef(side*.16f,hipHeight(),0);glRotatef(thigh,1,0,0);
    limb({0,0,0},{0,-.58f,0},.125f,pants);glTranslatef(0,-.58f,0);sphere(0,0,0,.115f,.115f,.115f,pants);
    glRotatef(knee,1,0,0);limb({0,0,0},{0,-.57f,0},.10f,pants);glTranslatef(0,-.57f,0);
    glRotatef(-thigh-knee,1,0,0);drawFoot();glPopMatrix();
}
// Two-bone inverse kinematics keeps the wrist at the cup handle during pickup,
// lifting, sipping and replacement. The elbow bends down/outward naturally.
void drawArm(Vec3 shoulder,Vec3 hand,int side){
    constexpr float upper=.50f,lower=.52f;
    Vec3 direction=hand-shoulder;float distance=clamp(length(direction),.05f,upper+lower-.001f);Vec3 axis=unit(direction);
    float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
    float height=std::sqrt(std::max(0.0f,upper*upper-along*along));
    // Keep the reaching elbow above the tabletop instead of passing through its edge.
    Vec3 pole{side*.65f,mix(-1,.5f,teaActive()?human.reach:0),.1f};Vec3 bend=unit(pole-axis*dot(pole,axis));
    Vec3 elbow=shoulder+axis*along+bend*height;
    glPushMatrix();glTranslatef(shoulder.x,shoulder.y,shoulder.z);
    limb({0,0,0},elbow-shoulder,.10f,shirt);
    glTranslatef(elbow.x-shoulder.x,elbow.y-shoulder.y,elbow.z-shoulder.z);
    sphere(0,0,0,.09f,.09f,.09f,skin);limb({0,0,0},hand-elbow,.073f,skin);
    drawHand(hand-elbow);glPopMatrix();
}
void drawBody(){
    float breathing=1+std::sin(world.time*2)*.012f;
    glPushMatrix();glScalef(1,breathing,1);
    cylinder(.24f,.29f,.72f,shirt);cube(0,.37f,-.236f,.06f,.58f,.035f,{.82f,.75f,.53f});
    // Shirt collar and an independent hem carry a little wind motion.
    cube(-.1f,.70f,-.19f,.14f,.07f,.12f,cream);cube(.1f,.70f,-.19f,.14f,.07f,.12f,cream);
    glPushMatrix();glRotatef(std::sin(world.time*3)*windStrength*2,0,0,1);cube(0,.04f,0,.52f,.10f,.43f,shirt);glPopMatrix();
    glPopMatrix();
}
void drawHead(){
    cylinder(.085f,.08f,.12f,skin);
    glPushMatrix();glTranslatef(0,.26f,0);
    float lookUp=(rainAmount()>.2f&&atmosphere.reactionTime>0)?12*std::abs(std::sin(atmosphere.reactionTime*2)):0;
    glRotatef(lookUp+disaster.lookPitch,1,0,0);glRotatef(disaster.lookYaw,0,1,0);glRotatef(std::sin(world.time*.65f)*(3*(1-human.walkBlend)+coldAmount()*5),0,1,0);
    sphere(0,0,0,.205f,.245f,.20f,skin);
    sphere(0,.155f,.025f,.211f,.13f,.20f,{.10f,.065f,.045f});
    for(int i=0;i<3;i++)sphere(-.14f+i*.12f,.22f+std::sin(world.time*2+i)*windStrength*.009f,-.025f,.085f,.055f,.15f,{.105f,.07f,.045f});
    sphere(-.21f,0,0,.045f,.075f,.06f,skin);sphere(.21f,0,0,.045f,.075f,.06f,skin);
    for(float x:{-.075f,.075f}){sphere(x,.03f,-.181f,.044f,.026f,.025f,{.95f,.90f,.78f});sphere(x,.027f,-.204f,.018f,.022f,.014f,{.055f,.075f,.065f});}
    sphere(0,-.02f,-.208f,.036f,.048f,.047f,skin);cube(0,-.11f,-.177f,.084f,.018f,.021f,{.31f,.15f,.10f});
    glPopMatrix();
}
void drawHuman(){
    if(homestead.firstPerson&&human.action==FREE_WALK)return;
    shadow(human.position.x,human.position.z,.55f,.43f);
    glPushMatrix();glTranslatef(human.position.x,human.position.y,human.position.z);glRotatef(-human.yaw,0,1,0);
    drawLeg(-1);drawLeg(1);
    cube(0,hipHeight()-.05f,0,.44f,.23f,.32f,pants);
    Vec3 upper=upperBodyPoint({0,0,0});
    glPushMatrix();glTranslatef(upper.x,upper.y,upper.z);glRotatef(-14*teaLean(),1,0,0);
    drawBody();glTranslatef(0,.73f,0);drawHead();glPopMatrix();
    for(int side:{-1,1}){
        Vec3 shoulder=upperBodyPoint({side*.31f,.65f,0});
        float swing=-side*human.armAngle;
        Vec3 hand=shoulder+rotateX({side*.06f,-.88f,0},swing);
        if(human.sitBlend>.01f){Vec3 seatedHand{side*.32f,teaActive()?1.43f:1.15f,teaActive()?-.30f:-.43f};hand=hand*(1-human.sitBlend)+seatedHand*human.sitBlend;}
        if(side==1&&teaActive())hand=hand*(1-human.reach)+worldToHuman(cupHandlePosition())*human.reach;
        if(side==-1&&human.cupHeld&&atmosphere.weatherMix[STORM]>.1f){
            Vec3 support=worldToHuman(human.cupPosition+Vec3{-.15f,.14f,0}*CUP_SCALE);float w=atmosphere.weatherMix[STORM]*human.cupLift;
            hand=hand*(1-w)+support*w;
        }else if(side==-1&&!teaActive()){
            float reaction=clamp(atmosphere.reactionTime)*rainAmount();hand=hand*(1-reaction)+Vec3{-.31f,1.98f,-.25f}*reaction;
        }
        if(disaster.fear>0&&!human.cupHeld)hand=hand*(1-disaster.fear)+Vec3{side*.25f,1.95f,-.23f}*disaster.fear;
        drawArm(shoulder,hand,side);
    }
    glPopMatrix();
}
void drawShelter(){drawHomePorch();}

// ---------------- ATMOSPHERE RENDERING ----------------
void drawSeasonalClouds(){
    float cover=cloudAmount();int count=7+int(cover*9);
    Color c=blend({.98f,.97f,.88f},{.31f,.35f,.40f},clamp(cover*.75f+damage()*.4f));
    for(int i=0;i<count;i++){
        float x=std::fmod(i*16.0f+cloudPhase+60,120)-60;
        for(int j=0;j<3;j++)sphere(x+j*2.2f,23+float(i%3)*2,-30+float(i%4)*15,3.7f+cover*1.2f,1.1f+cover*.7f,1.8f+cover,c);
    }
}
bool underRoof(float x,float y,float z){
    return (box(x,z,-10,-10.5f,6.9f,2.1f)&&y<3.7f)||(insideHouse(x,z)&&y<6.9f)||(box(x,z,25,-6.9f,4.6f,2.3f)&&y<4.8f);
}
void drawWeatherParticles(){
    glDisable(GL_LIGHTING);
    float rain=rainAmount();glLineWidth(1);glBegin(GL_LINES);
    for(int i=0;i<int(500*rain);i++){const auto& p=atmosphere.rain[i];if(underRoof(p.x,p.y,p.z))continue;
        glColor4f(.67f,.83f,.91f,.42f*rain);glVertex3f(p.x,p.y,p.z);glVertex3f(p.x-windStrength*.07f,p.y+.65f,p.z);}
    glEnd();
    float snow=snowAmount();glBegin(GL_QUADS);
    for(int i=0;i<int(400*snow);i++){const auto& p=atmosphere.snow[i];if(underRoof(p.x,p.y,p.z))continue;
        float a=p.rotation*PI/180,s=.045f,c=std::cos(a)*s,d=std::sin(a)*s;
        glColor4f(.96f,.99f,1,.82f*snow);
        Vec3 right=unit({cameraZ-p.z,0,p.x-cameraX});
        Vec3 r=right*c+Vec3{0,d,0},u=Vec3{0,c,0}-right*d,center{p.x,p.y,p.z};
        for(Vec3 v:{center-r-u,center+r-u,center+r+u,center-r+u})glVertex3f(v.x,v.y,v.z);}
    glEnd();
    float petals=atmosphere.seasons[SPRING]*(1-damage());
    float leaves=clamp(atmosphere.seasons[AUTUMN]+atmosphere.weatherMix[STORM]*.65f+(world.state==WARNING?.6f:0));
    glBegin(GL_TRIANGLES);
    for(int i=0;i<int(100*std::max(petals*.6f,leaves));i++){
        const auto& p=atmosphere.leaves[i];float a=p.rotation*PI/180,s=leaves>petals*.6f?.11f:.065f;
        Color c=leaves>petals*.6f?(i%2?Color{.94f,.56f,.13f}:Color{.77f,.28f,.08f}):Color{1,.75f,.81f};
        glColor4f(c.r,c.g,c.b,clamp(p.y*2)*.85f);
        glVertex3f(p.x-std::cos(a)*s,p.y-std::sin(a)*s,p.z);glVertex3f(p.x+std::cos(a)*s,p.y+std::sin(a)*s,p.z);
        glVertex3f(p.x,p.y+.05f,p.z+s*1.5f);
    }glEnd();
    // A brief jagged bolt accompanies the infrequent global light flash.
    if(atmosphere.lightningFlash>.1f){glLineWidth(3);glColor4f(.88f,.94f,1,atmosphere.lightningFlash);glBegin(GL_LINE_STRIP);
        glVertex3f(19,30,-25);glVertex3f(16,25,-25);glVertex3f(18,25,-25);glVertex3f(14,19,-25);glEnd();glLineWidth(1);}
    glEnable(GL_LIGHTING);
}
void drawPuff(Vec3 p,float radius,float alpha){
    Vec3 right=unit({cameraZ-p.z,0,p.x-cameraX});
    glBegin(GL_TRIANGLE_FAN);glColor4f(.94f,.96f,.97f,alpha);glVertex3f(p.x,p.y,p.z);glColor4f(.94f,.96f,.97f,0);
    for(int i=0;i<=12;i++){float a=i*PI/6;Vec3 v=p+right*(std::cos(a)*radius)+Vec3{0,std::sin(a)*radius,0};glVertex3f(v.x,v.y,v.z);}glEnd();
}
void drawLivingSteam(){
    if(disaster.phase==D_SILENCE&&disaster.time>14)return;
    float thick=(.36f+coldAmount()*.23f+rainAmount()*.13f+(human.cupHeld?.1f:0))*(disaster.phase==D_SILENCE?1-smooth((disaster.time-7)/7):1);
    // Twenty recycled translucent particles; local coordinates keep steam attached to the cup.
    glDisable(GL_LIGHTING);
    for(const auto& p:atmosphere.steam){float t=p.life;float x=p.x+std::sin(t*7+steamPhase)*.045f+windStrength*t*t*.27f;
        float radius=p.size*(1+t*2);
        Vec3 pos=human.cupPosition+rotateX({x,.43f*CUP_SCALE+p.y,p.z},human.cupTilt);
        drawPuff(pos,radius,(1-t)*thick);}
    glEnable(GL_LIGHTING);
}
void drawBreath(){
    float cold=coldAmount();if(cold<.05f)return;
    Vec3 mouth=mouthPosition();Vec3 forward=rotateY({0,0,-1},-human.yaw);
    glDisable(GL_LIGHTING);
    for(int i=0;i<12;i++){
        float t=std::fmod(world.time*.48f+i*.065f,1.6f);if(t>1)continue;
        Vec3 p=mouth+forward*(.05f+t*.65f)+Vec3{t*windStrength*.06f,t*.22f,0};float r=.028f+t*.075f;
        drawPuff(p,r,cold*(1-t)*.35f);
    }glEnable(GL_LIGHTING);
}
void applyAtmosphereLighting(){
    float d=damage(),cloud=cloudAmount(),flash=atmosphere.lightningFlash;
    Color sky=seasonColor({.52f,.79f,.88f},{.35f,.70f,.89f},{.72f,.73f,.66f},{.67f,.75f,.81f});
    sky=blend(sky,{.23f,.29f,.35f},cloud*.68f);sky=blend(sky,{.34f,.38f,.36f},d*.65f);sky=blend(sky,{.91f,.95f,1},flash*.75f);
    glClearColor(sky.r,sky.g,sky.b,1);GLfloat fog[]={sky.r,sky.g,sky.b,1};glFogfv(GL_FOG_COLOR,fog);
    glFogi(GL_FOG_MODE,GL_LINEAR);glFogf(GL_FOG_START,mix(52,21,clamp(d*.7f+cloud*.5f)));glFogf(GL_FOG_END,mix(135,80,clamp(d*.6f+cloud*.7f)));
    Color sunlight=seasonColor({1,.96f,.81f},{1,.93f,.71f},{1,.80f,.57f},{.77f,.87f,1});
    float strength=(1-cloud*.47f)*(1-d*.32f)+flash*1.6f;
    GLfloat ambient[]={.49f-d*.12f+flash*.3f,.50f-d*.13f+flash*.3f,.45f+atmosphere.seasons[WINTER]*.10f+flash*.3f,1};
    GLfloat diffuse[]={sunlight.r*strength,sunlight.g*strength,sunlight.b*strength,1};
    GLfloat spec[]={.75f,.75f,.72f,1},position[]={-24,40,22,0};
    glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);glLightfv(GL_LIGHT0,GL_SPECULAR,spec);glLightfv(GL_LIGHT0,GL_POSITION,position);
}
std::string humanStatus(){
    const char* names[]={"Exploring the valley","Walking to the chair","Turning toward the table","Taking a seat","A warm sip of tea","Enjoying the moment","Standing up","Heading for shelter","Taking shelter","Waiting under the porch","Leaving the bench"};
    return names[human.action];
}
void drawAtmosphereHUD(){
    float x=width-324.0f;rect(x,223,294,112,{.055f,.11f,.105f},.89f);
    text(x+16,247,std::string("SEASON: ")+seasonNames[atmosphere.season],cream);
    text(x+16,269,std::string("WEATHER: ")+weatherNames[atmosphere.weather],{.74f,.84f,.80f});
    text(x+16,291,humanStatus(),{.86f,.78f,.56f});
    text(x+16,317,atmosphere.automatic?"AUTO SEASONS: ON   [ T ]":"AUTO SEASONS: OFF   [ T ]",{.66f,.77f,.66f});
}
void specialKeyboard(int key,int,int){if(paused||disasterActive())return;if(key>=GLUT_KEY_F1&&key<=GLUT_KEY_F4)selectSeason(Season(key-GLUT_KEY_F1));}

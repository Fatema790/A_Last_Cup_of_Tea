// Timed story, bounded physics effects, and compatibility OpenGL rendering.
bool disasterActive(){return disaster.phase!=D_NORMAL;}
float disasterRandom(){disaster.seed=1664525u*disaster.seed+1013904223u;return float(disaster.seed>>8)/16777216.0f;}
void resetDisaster(){disaster=DisasterWorld{};}
bool startDisaster(){
    if(paused||disasterActive()||human.action!=FREE_WALK||length(interactionOrigin()-BATON_POSITION)>=3.15f)return false;
    if(!beginTea())return false;
    disaster=DisasterWorld{};disaster.phase=D_PRELUDE;
    disaster.seed=unsigned(std::chrono::steady_clock::now().time_since_epoch().count());
    atmosphere.automatic=false;selectWeather(CLEAR,false);human.shelterRequested=false;homestead.firstPerson=false;
    std::fill(std::begin(keys),std::end(keys),false);
    say("The emergency baton is armed. One last peaceful moment.",6);return true;
}
void enterDisasterPhase(DisasterPhase phase){
    disaster.phase=phase;disaster.time=0;disaster.escapePlanned=false;
    if(phase==D_WARNING)selectWeather(CLOUDY,false);
    if(phase==D_QUAKE)leaveTea();
    if(phase==D_LAVA)selectWeather(WINDY,false);
    if(phase==D_METEORS)selectWeather(RAIN,false);
    if(phase==D_STORM||phase==D_COLLAPSE){selectWeather(STORM,false);atmosphere.lightningTimer=.2f;}
    if(phase==D_LAST_CUP){
        selectWeather(CLOUDY,false);disaster.restTime=0;disaster.standTime=0;
        human.action=FREE_WALK;human.sitBlend=0;seated=false;beginTea();
    }
    if(phase==D_SILENCE){selectWeather(CLOUDY,false);human.cupHeld=false;human.cupPosition={.12f,1.37f,0};human.cupTilt=0;}
    human.shelterRequested=false;
}
void emitDisasterParticle(Vec3 p,Vec3 v,float life,float size,int kind){
    auto& q=disaster.particles[disaster.particleCursor++%disaster.particles.size()];q={p,v,0,life,size,kind};
}
bool safeImpactPoint(Vec3 p){
    // Keep the tea, escape route, home and farm free of lethal-looking direct hits.
    if(length(Vec3{p.x,0,p.z})<10||std::abs(p.x)<4)return false;
    if(box(p.x,p.z,-10,-18.9f,10,10)||box(p.x,p.z,25,-1.3f,12,11))return false;
    return std::hypot(p.x-human.position.x,p.z-human.position.z)>6;
}
void spawnMeteor(){
    for(auto& m:disaster.meteors)if(!m.active){
        Vec3 target;bool found=false;
        for(int i=0;i<32;i++){target={-31+disasterRandom()*62,0,-29+disasterRandom()*61};if(safeImpactPoint(target)){found=true;break;}}
        if(!found)target={-27,0,25};
        // The first impact introduces the event inside the main camera composition.
        // Later impacts scatter across the larger safe zones.
        if(disaster.spawnCount==0)target={-12+disasterRandom()*.5f,0,4+disasterRandom()*.5f};
        m=Meteor{};m.active=true;m.target=target;m.start=target+Vec3{-13-disasterRandom()*9,25+disasterRandom()*13,-16};
        m.p=m.start;m.duration=2.8f+disasterRandom()*1.3f;m.size=.45f+disasterRandom()*.45f;disaster.spawnCount++;return;
    }
}
void impactMeteor(Meteor& m){
    m.active=false;disaster.impactCount++;disaster.impactPosition=m.target;disaster.impactFlash=1;disaster.impactShake=1;
    auto& crater=disaster.craters[disaster.craterCursor++%disaster.craters.size()];crater={m.target,1.2f+m.size,0};
    for(int i=0;i<48;i++){
        float a=disasterRandom()*2*PI,s=1+disasterRandom()*7;int kind=i<18?D_EMBER:i<32?D_CHIP:D_DUST;
        emitDisasterParticle(m.target+Vec3{0,.3f,0},{std::cos(a)*s,2+disasterRandom()*8,std::sin(a)*s},1.8f+disasterRandom()*2.5f,.06f+disasterRandom()*.20f,kind);
    }
    for(int i=0;i<12;i++)emitDisasterParticle(m.target+Vec3{0,.5f,0},{disasterRandom()*2,1+disasterRandom()*2,disasterRandom()-.5f},6,.55f+disasterRandom()*.7f,D_SMOKE);
}
void updateDisaster(float dt){
    if(!disasterActive())return;
    disaster.time=std::min(disaster.time+dt,disaster.phase==D_SILENCE?24.0f:999.0f);disaster.totalTime+=dt;
    float t=disaster.time;
    switch(disaster.phase){
    case D_PRELUDE:if(human.action==TEA_REST&&t>2)enterDisasterPhase(D_WARNING);break;
    case D_WARNING:if(t>=6)enterDisasterPhase(D_QUAKE);break;
    case D_QUAKE:if(t>=12)enterDisasterPhase(D_LAVA);break;
    case D_LAVA:if(t>=12)enterDisasterPhase(D_METEORS);break;
    case D_METEORS:if(t>=14)enterDisasterPhase(D_STORM);break;
    case D_STORM:if(t>=14)enterDisasterPhase(D_RUIN);break;
    case D_RUIN:if(t>=8)enterDisasterPhase(D_LAST_CUP);break;
    case D_LAST_CUP:
        if(human.action==TEA_DRINK&&human.cupHeld)disaster.finalSip=true;
        if(human.action==TEA_REST){disaster.finalReturned=true;disaster.restTime+=dt;if(disaster.restTime>=3)leaveTea();}
        if(human.action==FREE_WALK&&disaster.finalReturned){disaster.standTime+=dt;if(disaster.standTime>=2)enterDisasterPhase(D_COLLAPSE);}
        break;
    case D_COLLAPSE:if(t>=16)enterDisasterPhase(D_SILENCE);break;
    default:break;
    }
    const float levels[]={0,0,.08f,.5f,.65f,.78f,.9f,1,.18f,1,.0f};
    disaster.intensity=mix(disaster.intensity,levels[disaster.phase],1-std::exp(-dt*.6f));
    float targetDamage=disaster.phase>=D_COLLAPSE?1:disaster.phase>=D_RUIN?.85f:disaster.phase>=D_STORM?.7f:disaster.phase>=D_METEORS?.5f:disaster.phase>=D_LAVA?.3f:disaster.phase>=D_QUAKE?.12f:0;
    disaster.damage=std::max(disaster.damage,std::min(targetDamage,disaster.damage+dt*.035f));
    disaster.quake=disaster.phase>=D_QUAKE?disaster.intensity:disaster.intensity*.15f;
    if(disaster.phase>=D_LAVA)disaster.lava=std::min(1.0f,disaster.lava+dt*.07f);
    disaster.storm=disaster.phase>=D_METEORS?disaster.intensity:disaster.intensity*.35f;
    disaster.impactFlash=std::max(0.0f,disaster.impactFlash-dt*2.8f);
    disaster.impactShake=std::max(0.0f,disaster.impactShake-dt*1.8f);
    disaster.fade=disaster.phase==D_SILENCE?smooth((disaster.time-10)/10):0;
    human.shelterRequested=false;atmosphere.automatic=false;
    if(disaster.phase==D_STORM||disaster.phase==D_RUIN||disaster.phase==D_COLLAPSE)
        atmosphere.lightningTimer=std::min(atmosphere.lightningTimer,2.8f);
    bool rainingMeteors=disaster.phase>=D_METEORS&&disaster.phase<D_SILENCE;
    if(rainingMeteors){
        disaster.meteorTimer-=dt;
        if(disaster.meteorTimer<=0){spawnMeteor();disaster.meteorTimer=disaster.phase==D_LAST_CUP?6.5f:disaster.phase==D_COLLAPSE?.65f:2.2f+disasterRandom()*1.4f;}
    }
    for(auto& m:disaster.meteors)if(m.active){
        m.age+=dt;float u=clamp(m.age/m.duration);m.p=m.start+(m.target-m.start)*(u*u);
        if(u>=1)impactMeteor(m);
    }
    if(disaster.lava>0&&disaster.fade<.95f){
        disaster.eruptionTimer-=dt;
        if(disaster.eruptionTimer<=0){
            disaster.eruptionTimer=.12f;
            for(auto p:eruptionPoints){
                float a=disasterRandom()*2*PI;
                emitDisasterParticle(p+Vec3{0,.5f,0},{std::cos(a)*1.3f,3+disaster.intensity*6+disasterRandom()*3,std::sin(a)*1.3f},2.1f,.08f+disasterRandom()*.12f,D_EMBER);
                if(disasterRandom()<.35f)emitDisasterParticle(p+Vec3{0,1,0},{.7f+disaster.storm,1.4f,.2f},5,.45f,D_SMOKE);
                if(disasterRandom()<disaster.intensity*.35f)emitDisasterParticle(p+Vec3{0,.4f,0},{std::cos(a)*3,5+disasterRandom()*5,std::sin(a)*3},3,.17f,D_CHIP);
            }
        }
    }
    for(auto& p:disaster.particles)if(p.age<p.life){
        p.age+=dt;p.p=p.p+p.v*dt;
        if(p.kind==D_SMOKE){p.v.x+=dt*disaster.storm*.2f;p.size+=dt*.20f;}
        else {p.v.y-=dt*(p.kind==D_DUST?3:7.5f);if(p.p.y<.09f){p.p.y=.09f;p.v.y=std::abs(p.v.y)*.22f;p.v.x*=.9f;p.v.z*=.9f;}}
    }
    for(auto& c:disaster.craters)if(c.size>0)c.age+=dt;
}
bool updateDisasterActor(float dt){
    if(!disasterActive())return false;
    if(human.action!=FREE_WALK){
        updateHuman(dt*((disaster.phase==D_LAST_CUP&&human.action==TEA_DRINK)?.62f:1));
    }else {
        Vec3 before=human.position;
        bool escaping=(disaster.phase==D_QUAKE&&disaster.time>3.5f)||disaster.phase==D_COLLAPSE;
        if(escaping&&!disaster.escapePlanned){disaster.escapePlanned=planWalk({5.3f,0,5.5f});}
        if(escaping&&disaster.escapePlanned)followRoute(dt,5.4f);
        Vec3 delta=human.position-before;delta.y=0;float moved=length(delta);
        human.walkBlend=mix(human.walkBlend,moved>.0001f?1:0,1-std::exp(-dt*13));human.walkCycle+=moved*7;
        human.armAngle=std::sin(human.walkCycle)*42*human.walkBlend;
        human.bodyBob=std::abs(std::sin(human.walkCycle))*.075f*human.walkBlend;
        float facing=moved>.0001f?std::atan2(delta.x,-delta.z)*180/PI:std::atan2(-human.position.x,human.position.z)*180/PI;
        human.yaw+=std::remainder(facing-human.yaw,360.0f)*(1-std::exp(-dt*4));
    }
    bool tea=teaActive();
    float fearTarget=disaster.phase>=D_WARNING&&disaster.phase<D_SILENCE&&!tea?clamp(disaster.intensity*.7f+disaster.impactShake):0;
    disaster.fear=mix(disaster.fear,fearTarget,1-std::exp(-dt*5));
    float look=disaster.phase>=D_WARNING&&disaster.phase<D_SILENCE?1:0;
    disaster.lookYaw=std::sin(disaster.totalTime*.72f)*24*look*(1-human.reach);
    disaster.lookPitch=(10+std::sin(disaster.totalTime*.5f)*9)*look*(1-human.reach);
    if(look&&!human.cupHeld)for(const auto& m:disaster.meteors)if(m.active){
        Vec3 direction=m.p-(human.position+Vec3{0,2,0});
        disaster.lookYaw=clamp(std::remainder(std::atan2(direction.x,-direction.z)*180/PI-human.yaw,360.0f),-48,48)*(1-human.reach);
        disaster.lookPitch=clamp(std::atan2(direction.y,std::hypot(direction.x,direction.z))*180/PI,0,38)*(1-human.reach);break;
    }
    updateDisasterCamera(dt);return true;
}
void updateDisasterCamera(float dt){
    Vec3 eye,aim;
    if(teaActive()){
        eye={3.8f,2.8f,-1.6f};aim={0,1.35f,.8f};
        if(human.action==TEA_APPROACH){eye=human.position+Vec3{6,3,7};aim=human.position+Vec3{0,1.2f,0};}
        if(disaster.phase==D_LAST_CUP&&human.cupHeld){eye={2.5f,2.35f,-.85f};aim={0,1.5f,.55f};}
    }else if(disaster.phase==D_SILENCE){
        float t=smooth(disaster.time/16);eye=Vec3{7,3.8f,-5}*(1-t)+Vec3{1.1f,1.95f,-1.35f}*t;aim={.12f,1.52f,.12f};
    }else {eye=human.position+Vec3{8,5,8};aim={-2,disaster.phase==D_METEORS?6.0f:2.0f,-6};}
    float k=dt<=0?1:1-std::exp(-dt*2);Vec3 p=camera()*(1-k)+eye*k;
    cameraX=p.x;cameraY=p.y;cameraZ=p.z;viewAim=viewAim*(1-k)+aim*k;
}
Vec3 disasterCameraShake(){
    if(!disasterActive()||disaster.reducedMotion)return {};
    float a=.09f*disaster.quake+.19f*disaster.impactShake,t=disaster.totalTime;
    return {std::sin(t*37)*a,std::sin(t*43+1)*a*.7f,std::sin(t*29)*a*.55f};
}
float disasterGround(float x,float z){
    if(disaster.phase<D_QUAKE||std::abs(x)<5||homeFloorHeight(x,z)>=0||box(x,z,25,-1,10,10))return 0;
    return disaster.quake*.22f*std::sin(world.time*3.7f+std::floor(x/4)*1.7f+std::floor(z/4));
}
float disasterTreeFall(size_t i){
    if(!disasterActive()||i%4==0)return 0;
    float t=smooth((disaster.damage-(.16f+float(i%7)*.07f))/.23f);
    return t*82+std::sin(world.time*8+i)*disaster.quake*6*(1-t);
}
float disasterBridgeDrop(int i){return disasterActive()?smooth((disaster.damage-.65f)/.3f)*(i>13?1.8f:0):0;}
Vec3 disasterCupOffset(){
    if(!disasterActive()||human.cupHeld||human.reach>.01f||disaster.phase==D_SILENCE)return {};
    return {std::sin(world.time*32)*.035f*disaster.quake,std::abs(std::sin(world.time*27))*.025f*disaster.quake,0};
}

// ---------------- GEOMETRY ----------------
void glowMaterial(Color c){material(c);GLfloat emission[]={c.r,c.g,c.b,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission);}
void endGlow(){GLfloat off[]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,off);}
void drawBaton(){
    cube(BATON_POSITION.x,.46f,BATON_POSITION.z,.62f,.92f,.62f,{.17f,.21f,.20f});
    glPushMatrix();glTranslatef(BATON_POSITION.x,.94f,BATON_POSITION.z);
    cylinder(.10f,.10f,.23f,wood);glTranslatef(0,.23f,0);
    cylinder(.13f,.11f,.58f,{.25f,.27f,.24f});
    float pulse=.65f+.35f*std::sin(world.time*5);
    glDisable(GL_LIGHTING);sphere(0,.53f,0,.17f,.18f,.17f,{1,.12f+.25f*pulse,.025f});
    for(float y:{.12f,.28f,.43f}){glPushMatrix();glTranslatef(0,y,0);glRotatef(90,1,0,0);material({1,.36f,.04f});glutSolidTorus(.026,.13,6,12);glPopMatrix();}
    glEnable(GL_LIGHTING);glPopMatrix();
    if(!disasterActive())drawWorldLabel({BATON_POSITION.x-.9f,2.3f,BATON_POSITION.z},"EMERGENCY BATON",{1,.65f,.22f});
}
Vec3 crackPoint(int branch,int n){
    const Vec3 centers[]={{-17,0,-7},{12,0,-12},{-15,0,25},{20,0,26},{-27,0,-25},{-15,0,5},{16,0,-28}};
    Vec3 p=centers[branch];float a=branch*1.9f;
    p.x+=(n-5)*std::cos(a)*1.7f+std::sin(n*2.3f+branch)*.48f;
    p.z+=(n-5)*std::sin(a)*1.7f+std::cos(n*1.7f+branch)*.42f;
    p.y=std::max(.17f,terrain(p.x,p.z)+disasterGround(p.x,p.z)+.10f);return p;
}
void drawDisasterOpaque(){
    if(!disasterActive())return;
    if(disaster.phase>=D_QUAKE){
        float spread=clamp(disaster.damage*1.6f),width=.06f+disaster.damage*.62f;
        for(int b=0;b<7;b++)for(int n=0;n<int(10*spread);n++){
            Vec3 a=crackPoint(b,n),c=crackPoint(b,n+1),side=unit(Vec3{c.z-a.z,0,a.x-c.x});
            material({.075f,.045f,.035f});glBegin(GL_QUADS);glNormal3f(0,1,0);
            for(auto p:{a+side*width,c+side*width,c-side*width,a-side*width}){glVertex3f(p.x,p.y,p.z);}
            glEnd();
            if(disaster.lava>0){
                glowMaterial({.75f,.08f+.1f*std::sin(n+world.time*2),.006f});glBegin(GL_TRIANGLE_STRIP);
                for(int j=0;j<=3;j++){float t=j/3.0f;Vec3 p=a*(1-t)+c*t;p.y+=.015f+disaster.lava*(.045f+.06f*std::sin(world.time*3+n+j));
                    for(int sign:{-1,1}){Vec3 v=p+side*(sign*width*.58f*disaster.lava);glVertex3f(v.x,v.y,v.z);}}
                glEnd();endGlow();
            }
        }
        // Raised, rotating slabs surround fissures while the central walking area stays stable.
        for(int i=0;i<12;i++){
            Vec3 p=crackPoint(i%7,2+i%6);glPushMatrix();glTranslatef(p.x+.8f,.1f+disasterGround(p.x,p.z),p.z+.7f);
            glRotatef(std::sin(world.time*3+i)*disaster.quake*9,1,.3f,0);
            cube(0,0,0,1.6f,.32f,1.4f,blend(groundColor(),{.18f,.13f,.11f},.55f));glPopMatrix();
        }
    }
    for(size_t i=0;i<eruptionPoints.size();i++)if(disaster.lava>0){
        Vec3 p=eruptionPoints[i];float r=1.2f*disaster.lava;
        for(int j=0;j<10;j++){float a=j*2*PI/10;sphere(p.x+std::cos(a)*r,.22f,p.z+std::sin(a)*r,.35f,.35f,.35f,{.18f,.13f,.11f});}
        glowMaterial({1,.18f,.005f});glBegin(GL_TRIANGLE_FAN);glNormal3f(0,1,0);
        glVertex3f(p.x,.4f+disaster.intensity*.8f+std::sin(world.time*4+i)*.14f,p.z);
        for(int j=0;j<=18;j++){float a=j*2*PI/18,rr=r*(1+.13f*std::sin(j*2.1f));glVertex3f(p.x+std::cos(a)*rr,.20f+.1f*std::sin(a*3+world.time*3),p.z+std::sin(a)*rr);}glEnd();endGlow();
        for(int j=0;j<3;j++){float u=std::fmod(world.time*(.8f+disaster.intensity)+j*.33f+i,1.0f);glDisable(GL_LIGHTING);
            sphere(p.x+std::sin(j*2.4f)*u*.7f,.3f+std::sin(u*PI)*(1+disaster.intensity*4),p.z+std::cos(j*2.4f)*u*.7f,.14f,.25f,.14f,{1,.4f,.02f});glEnable(GL_LIGHTING);}
    }
    for(const auto& c:disaster.craters)if(c.size>0){
        disk(c.p.x,.16f,c.p.z,c.size,{.075f,.065f,.055f});
        for(int i=0;i<12;i++){float a=i*PI/6;glPushMatrix();glTranslatef(c.p.x+std::cos(a)*c.size,.15f,c.p.z+std::sin(a)*c.size);glRotatef(i*31.0f,0,1,0);cube(0,0,0,.65f,.25f+(i%3)*.12f,.42f,{.24f,.20f,.15f});glPopMatrix();}
        glDisable(GL_LIGHTING);disk(c.p.x,.175f,c.p.z,c.size*.5f,{.48f,.10f,.02f});glEnable(GL_LIGHTING);
    }
    for(const auto& m:disaster.meteors)if(m.active){
        sphere(m.p.x,m.p.y,m.p.z,m.size,m.size*.86f,m.size,{.19f,.11f,.065f});
        glDisable(GL_LIGHTING);sphere(m.p.x+.12f,m.p.y-.1f,m.p.z+.12f,m.size*.75f,m.size*.65f,m.size*.76f,{1,.32f,.025f});glEnable(GL_LIGHTING);
    }
    for(const auto& p:disaster.particles)if(p.age<p.life&&p.kind==D_CHIP){
        glPushMatrix();glTranslatef(p.p.x,p.p.y,p.p.z);glRotatef(p.age*170,1,1,0);cube(0,0,0,p.size,p.size*.7f,p.size*1.4f,{.24f,.19f,.12f});glPopMatrix();
    }
    if(disaster.damage>.45f){
        // Visible plaster fractures and fallen trim; no inaccessible replacement building.
        glDisable(GL_LIGHTING);glColor3f(.10f,.07f,.055f);glLineWidth(4);glBegin(GL_LINE_STRIP);
        for(Vec3 p:std::array<Vec3,6>{{{-6.8f,4.2f,-12.25f},{-7.3f,3.5f,-12.25f},{-7.1f,2.8f,-12.25f},{-7.8f,2.4f,-12.25f},{-7.6f,1.4f,-12.25f},{-8.1f,.4f,-12.25f}}})glVertex3f(p.x,p.y,p.z);
        glEnd();glLineWidth(1);glEnable(GL_LIGHTING);
        for(int i=0;i<5;i++){glPushMatrix();glTranslatef(-6.5f+i*.46f,.28f,-8.7f);glRotatef(i*29.0f,0,1,0);cube(0,0,0,.65f,.22f,.27f,plaster);glPopMatrix();}
    }
}
void disasterPuff(Vec3 p,float r,Color color,float alpha){
    Vec3 right=unit({cameraZ-p.z,0,p.x-cameraX});glBegin(GL_TRIANGLE_FAN);glColor4f(color.r,color.g,color.b,alpha);glVertex3f(p.x,p.y,p.z);glColor4f(color.r,color.g,color.b,0);
    for(int i=0;i<=12;i++){float a=i*PI/6;Vec3 v=p+right*(std::cos(a)*r)+Vec3{0,std::sin(a)*r,0};glVertex3f(v.x,v.y,v.z);}glEnd();
}
void drawDisasterTransparent(){
    if(!disasterActive())return;
    glDisable(GL_LIGHTING);
    for(const auto& m:disaster.meteors)if(m.active){Vec3 trail=unit(m.start-m.target);
        for(int i=0;i<14;i++){float t=i/14.0f;Vec3 p=m.p+trail*(i*.43f);disasterPuff(p,m.size*(1-t*.7f),i<6?Color{1,.35f,.015f}:Color{.24f,.20f,.18f},(1-t)*.85f);}}
    for(const auto& p:disaster.particles)if(p.age<p.life&&p.kind!=D_CHIP){
        float a=1-p.age/p.life;Color c=p.kind==D_EMBER?Color{1,.38f,.025f}:p.kind==D_SMOKE?Color{.13f,.13f,.14f}:Color{.43f,.34f,.24f};
        disasterPuff(p.p,p.size,c,a*(p.kind==D_SMOKE?.45f:.8f));
    }
    if(disaster.impactFlash>0){auto p=disaster.impactPosition;float radius=(1-disaster.impactFlash)*5;
        glColor4f(1,.55f,.15f,disaster.impactFlash*.65f);glBegin(GL_TRIANGLE_STRIP);
        for(int i=0;i<=40;i++){float a=i*PI/20;for(float r:{radius,radius+.45f})glVertex3f(p.x+std::cos(a)*r,.22f,p.z+std::sin(a)*r);}glEnd();}
    // Wind-borne leaves and splinters remain bounded even during the peak.
    if(disaster.phase>=D_QUAKE&&disaster.phase<D_SILENCE){glBegin(GL_TRIANGLES);
        for(int i=0;i<int(disaster.intensity*80);i++){float x=std::fmod(world.time*(3+disaster.storm*4)+i*3.71f,58)-29,y=.8f+std::fmod(i*1.3f+world.time*.7f,8),z=-25+std::fmod(i*2.61f,50);
            glColor4f(.45f,.27f,.11f,.8f);glVertex3f(x,y,z);glVertex3f(x+.18f,y+.05f,z);glVertex3f(x+.1f,y+.15f,z+.1f);}glEnd();}
    glEnable(GL_LIGHTING);
}
void setupDisasterLighting(){
    glDisable(GL_LIGHT2);glDisable(GL_LIGHT3);if(!disasterActive()||disaster.phase==D_PRELUDE)return;
    float dark=clamp(disaster.damage*.75f+disaster.storm*.25f),flash=std::min(.28f,atmosphere.lightningFlash*.28f)+disaster.impactFlash*.13f;
    Color sky=blend({.38f,.48f,.49f},{.035f,.045f,.064f},dark);sky.r+=flash;sky.g+=flash;sky.b+=flash;
    glClearColor(sky.r,sky.g,sky.b,1);GLfloat fog[]={sky.r,sky.g,sky.b,1};glFogfv(GL_FOG_COLOR,fog);glFogf(GL_FOG_START,24);glFogf(GL_FOG_END,90-dark*25);
    GLfloat ambient[]={.40f-dark*.14f+flash,.39f-dark*.15f+flash,.37f-dark*.12f+flash,1};
    GLfloat diffuse[]={.65f-dark*.38f+flash,.60f-dark*.39f+flash,.54f-dark*.34f+flash,1};
    glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
    if(disaster.lava>0){GLfloat pos[]={12,2,-12,1},color[]={1,.23f,.025f,1};glEnable(GL_LIGHT2);glLightfv(GL_LIGHT2,GL_POSITION,pos);glLightfv(GL_LIGHT2,GL_DIFFUSE,color);glLightf(GL_LIGHT2,GL_QUADRATIC_ATTENUATION,.012f);}
    if(disaster.impactFlash>0){GLfloat pos[]={disaster.impactPosition.x,3,disaster.impactPosition.z,1},color[]={disaster.impactFlash*2,disaster.impactFlash,.12f,1};glEnable(GL_LIGHT3);glLightfv(GL_LIGHT3,GL_POSITION,pos);glLightfv(GL_LIGHT3,GL_DIFFUSE,color);glLightf(GL_LIGHT3,GL_QUADRATIC_ATTENUATION,.008f);}
}
void drawDisasterHUD(){
    if(!disasterActive())return;
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,width,height,0);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glDisable(GL_FOG);glEnable(GL_BLEND);
    rect(0,0,float(width),65,{.015f,.02f,.025f},.92f);rect(0,height-82.0f,float(width),82,{.015f,.02f,.025f},.94f);
    const char* titles[]={"","ONE PEACEFUL CUP","A WARNING IN THE EARTH","THE GROUND REMEMBERS","FIRE BENEATH OUR FEET","THE SKY IS FALLING","THE STORM BREAKS","A WORLD COMING UNDONE","A LAST CUP OF TEA","THE FINAL COLLAPSE","WHAT REMAINS"};
    const char* lines[]={"","The baton is armed. For a moment, everything is still.","A tremor. A ripple in the cup. Something has changed.","He stands, searches the sky, and runs from the table.","The ground opens. Beneath it, a river of fire.","Light crosses the sky. Then the earth answers.","Rain cannot put out what we have set in motion.","There is nowhere untouched. But there is still the table.","He returns. Not to escape the world, but to remember it.","The last quiet moment has passed.","A half-finished cup. A little warmth. Then silence."};
    centered(39,std::string(titles[disaster.phase])+"  /  "+std::to_string(disaster.playbackSpeed)+"x",{.95f,.79f,.53f},GLUT_BITMAP_HELVETICA_18);
    centered(height-50.0f,lines[disaster.phase],{.88f,.88f,.82f},GLUT_BITMAP_HELVETICA_12);
    centered(height-23.0f,disaster.reducedMotion?"F Speed 1x/4x/8x   P Pause   R Restart   Esc Exit   M Shake OFF":"F Speed 1x/4x/8x   P Pause   R Restart   Esc Exit   M Reduce shake",{.57f,.64f,.66f},GLUT_BITMAP_HELVETICA_12);
    if(disaster.fade>0)rect(0,0,float(width),float(height),{0,0,0},disaster.fade);
    if(disaster.fade>.9f){centered(height*.46f,"A LAST CUP OF TEA",{.88f,.82f,.68f},GLUT_BITMAP_TIMES_ROMAN_24);centered(height*.53f,"Protect the world that makes peaceful moments possible.",{.61f,.66f,.61f},GLUT_BITMAP_HELVETICA_12);centered(height*.62f,"R  Begin again     /     Esc  Exit",{.48f,.52f,.50f},GLUT_BITMAP_HELVETICA_12);}
    if(paused){rect(0,0,float(width),float(height),{.01f,.02f,.03f},.5f);centered(height*.5f,"PAUSED  /  P to continue",cream);}
    glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);glEnable(GL_FOG);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);
}
void setDisasterCapture(int stage){
    reset();showHelp=false;human.position={-3.6f,.02f,4};startDisaster();disaster.seed=18473;
    const DisasterPhase phases[]={D_PRELUDE,D_WARNING,D_QUAKE,D_LAVA,D_METEORS,D_STORM,D_RUIN,D_LAST_CUP,D_COLLAPSE,D_SILENCE,D_SILENCE};
    DisasterPhase target=phases[stage];
    for(int i=0;i<18000;i++){
        update(1.0f/60);
        if(stage==4){bool visible=false;for(const auto& m:disaster.meteors)visible=visible||(m.active&&m.p.y>8&&m.p.y<13);
            if(disaster.phase==target&&visible)break;
        }else if(disaster.phase==target&&disaster.time>(stage==9?8:stage==10?22:stage==7?11:stage==0?10:stage==1?3:5))break;
    }
    if(stage>=3&&stage<=6){cameraX=13;cameraY=9;cameraZ=16;viewAim={-4,2,-7};}
    if(stage==8){cameraX=10;cameraY=7;cameraZ=15;viewAim={-3,2,-7};}
}

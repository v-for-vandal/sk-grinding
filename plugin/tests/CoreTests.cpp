#include "Core.h"
#include <iostream>
#include <stdexcept>
using namespace sp;
void check(bool x,const char* s){if(!x)throw std::runtime_error(s);}
void near(double a,double b,double tolerance=1e-5){if(std::abs(a-b)>tolerance)throw std::runtime_error("expected "+std::to_string(b)+", got "+std::to_string(a));}
int main(int argc,char** argv)try {
    check(argc==2,"config path required");Model m;m.loadConfig(argv[1]);check(m.skills.size()==6,"six skills");
    Curve c;LevelXP l;l.add(.001,c,500);near(l.value(c),1);l.add(c.between(1,75.4),c,500);near(l.value(c),75.4);LevelXP floor;floor.set(20.5,c,500);l.subtract(1000,c,floor);near(l.value(c),20.5);l.add(1e30,c,500);near(l.value(c),500);near(l.progress,0);
    for(auto& sk:m.skills){double n=sk.curve.between(sk.start,sk.target);near(sk.curve.levelFromXP(sk.start,n),sk.target);near(sk.curve.levelFromXP(75.3,-sk.curve.between(15.7,75.3)),15.7);double path=0;for(auto& s:sk.subs)if(std::find(sk.path.begin(),sk.path.end(),s.id)!=sk.path.end())path+=s.worth(sk.pathLevel);near(path,n,1e-6);m.initialize(sk,50);near(sk.computed(0,300),50);for(auto& s:sk.subs)near(s.current.value(s.curve),(s.fixed&&s.k==0)?0:70);}
    for(double u:{0.,20.,30.,70.,90.})for(double dt:{.1,10.,80.}){double numeric=0;int steps=100000;for(int i=0;i<steps;++i)numeric+=ramp(u+(i+.5)*dt/steps,24,72)*dt/steps;near(rampIntegral(u,dt,24,72),numeric,1e-6);}
    auto* s=m.find("1h-sword");s->curve={1,10,1};s->current.set(5,s->curve,500);s->floor.set(10,s->curve,500);s->potential.set(20,s->curve,500);s->peak=s->potential;
    double raw=s->curve.between(5,10)/3+s->curve.between(10,20)/1.5+s->curve.between(20,22);m.award(s->id,raw);near(s->current.value(s->curve),22);near(s->potential.value(s->curve),22);check(s->floor.value(s->curve)<=22,"floor<=current");
    auto saved=m.save();saved["subs"]["removed"]={{"test",42}};saved["mains"]["removed"]={{"test",7}};Model loaded;loaded.loadConfig(argv[1]);loaded.restore(saved);check(loaded.save()==saved,"save roundtrip/unknown IDs");
    m.settings.values["debug"]["no_grace"]=true;m.settings.values["debug"]["time_scale"]=1000;m.tick(24);near(s->current.value(s->curve),s->floor.value(s->curve));check(m.skills[0].rust>=0,"rust nonnegative");
    std::cout<<"Core tests passed: six normalization paths, fractional curves, cap, zones, floors, decay integral, initialization, persistence.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

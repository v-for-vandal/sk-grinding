#include "Core.h"
#include <limits>
#include <stdexcept>
namespace sp {
double Curve::cost(int l) const { return std::max(.001,mult*std::pow(std::max(0,l),exponent)+offset); }
double Curve::between(double a,double b) const {
    if(b<a) return -between(b,a);
    double out=0;
    while(a<b) {int n=static_cast<int>(std::floor(a));double step=std::min(b-a,n+1-a);out+=step*cost(n);a+=step;}
    return out;
}
double Curve::levelFromXP(double base,double xp,int cap) const {
    double l=std::clamp(base,0.,double(cap));
    while(xp>0 && l<cap) {int n=int(std::floor(l));double need=(n+1-l)*cost(n);if(xp<need)return l+xp/cost(n);xp-=need;l=n+1.;}
    while(xp<0 && l>0) {int n=int(std::ceil(l))-1;double need=(l-n)*cost(n);if(-xp<need)return l+xp/cost(n);xp+=need;l=n;}
    return l;
}
double LevelXP::value(const Curve& c) const {return level+progress/c.cost(level);}
void LevelXP::set(double x,const Curve& c,int cap) {x=std::clamp(x,0.,double(cap));level=static_cast<std::uint16_t>(x);progress=level==cap?0:float((x-level)*c.cost(level));}
void LevelXP::add(double xp,const Curve& c,int cap) {
    if(!std::isfinite(xp)||xp<=0)return;
    double p=progress+xp;
    while(level<cap && p>=c.cost(level)) {p-=c.cost(level);++level;}
    progress=level>=cap?0:float(p);
}
void LevelXP::subtract(double levels,const Curve& c,const LevelXP& f) {if(value(c)<=f.value(c))return;set(std::max(f.value(c),value(c)-std::max(0.,levels)),c,65535);}
double Sub::shape(double l) const {double x=T1<=0?1:std::clamp(l/T1,0.,1.);double e=(l-T)/w;return e>700?0:x*x*(3-2*x)*(1+std::exp(-T/w))/(1+std::exp(e));}
void Sub::prepare(int cap) {integral.assign(cap+1,0);for(int i=0;i<cap;++i)integral[i+1]=integral[i]+curve.cost(i)*shape(i+.5);}
double Sub::worth(double l) const {l=std::clamp(l,0.,double(integral.size()-1));auto n=size_t(l);return k*(integral[n]+(n+1<integral.size()?(l-n)*(integral[n+1]-integral[n]):0));}
void Skill::solve(int cap) {
    for(auto& s:subs)s.prepare(cap);
    for(auto& [g,b]:budgets) {double sum=0;for(auto& s:subs)if(!s.fixed&&s.group==g&&std::find(path.begin(),path.end(),s.id)!=path.end()){double saved=s.k;s.k=1;sum+=s.worth(pathLevel);s.k=saved;}
        if(sum<=0)throw std::runtime_error(av+": empty normalization path for "+g);
        for(auto& s:subs)if(!s.fixed&&s.group==g)s.k=b/sum;
    }
}
double Skill::computed(double lo,double hi) const {double x=0;for(auto& s:subs)x+=s.worth(std::max(s.current.value(s.curve),s.peak.value(s.curve)-rust))-s.worth(s.initial);return std::clamp(curve.levelFromXP(base,x),lo,hi);}
double Settings::get(const char* s,const char* k) const {return values.at(s).at(k).get<double>();}
bool Settings::flag(const char* s,const char* k) const {return values.at(s).at(k).get<bool>();}
double ramp(double u,double g,double e) {return e<=g?(u>g?1:0):std::clamp((u-g)/(e-g),0.,1.);}
double rampIntegral(double u,double dt,double g,double e) {
    auto primitive=[&](double x){if(e<=g)return std::max(0.,x-g);double y=std::max(0.,x-g),width=e-g;return y<width?y*y/(2*width):y-width/2;};
    return primitive(u+dt)-primitive(u);
}
void Model::initialize(Skill& sk,double b) {
    sk.base=b;sk.written=std::round(b);sk.rust=sk.idle=0;sk.initialized=true;
    double f=std::clamp(settings.get("init","floor_per_level")*(b-settings.get("init","floor_zero_at")),0.,settings.get("xp","floor_cap"));
    for(auto& s:sk.subs){s.initial=(s.fixed&&s.k==0)?0:f;s.current.set(s.initial,s.curve,int(settings.get("xp","hard_cap")));s.floor=s.potential=s.peak=s.current;s.idle=s.lastXP=0;}
}
Sub* Model::find(const std::string& id,Skill** parent) {for(auto& sk:skills)for(auto& s:sk.subs)if(s.id==id){if(parent)*parent=&sk;return &s;}return nullptr;}
void Model::award(const std::string& id,double raw) {
    if(!std::isfinite(raw)||raw<=0)return;Skill* sk=nullptr;Sub* s=find(id,&sk);if(!s||!sk->initialized)return;
    int cap=int(settings.get("xp","hard_cap"));double left=raw;
    while(left>1e-10 && s->current.level<cap) {
        double cur=s->current.value(s->curve),f=s->floor.value(s->curve),p=s->potential.value(s->curve),boundary=cap;
        double z=settings.get("xp","zone_new");
        if(cur<f){boundary=f;z=settings.get("xp","zone_catch_up");}else if(cur<p){boundary=p;z=settings.get("xp","zone_recall");}
        double part=settings.flag("xp","split_zone_gain")?std::min(left,s->curve.between(cur,boundary)/z):left;
        if(part<=1e-10) {s->current.set(boundary,s->curve,cap);continue;}
        s->current.add(part*z,s->curve,cap);left-=part;
    }
    s->floor.add(raw*settings.get("xp","floor_rate"),s->curve,cap);
    s->floor.set(std::min({s->floor.value(s->curve),settings.get("xp","floor_cap"),s->current.value(s->curve)}),s->curve,cap);
    if(s->current.value(s->curve)>s->potential.value(s->curve))s->potential=s->current;
    if(s->current.value(s->curve)>s->peak.value(s->curve))s->peak=s->current;
    double q=settings.get("decay","q");s->idle=std::max(0.,s->idle-raw/(q*sk->paceSub));s->lastXP=raw;
    sk->idle=std::max(0.,sk->idle-raw/(q*sk->paceSkill));sk->rust=std::max(0.,sk->rust-sk->rustRate/24*raw/(q*sk->paceSkill));
}
void Model::tick(double hours) {
    double dt=hours*settings.get("debug","time_scale"),g=settings.get("decay","grace_hours"),e=settings.get("decay","ramp_end_hours"),max=settings.get("decay","idle_max_hours");
    if(settings.flag("debug","no_grace"))g=e=0;
    activeHours+=hours;
    for(auto& sk:skills)if(sk.initialized) {
        double cap=0;
        for(auto& s:sk.subs){double amount=s.decay/24*rampIntegral(s.idle,dt,g,e);s.current.subtract(amount,s.curve,s.floor);s.idle=std::min(max,s.idle+dt);cap=std::max(cap,s.peak.value(s.curve)-s.floor.value(s.curve));}
        sk.rust=std::min(cap,sk.rust+sk.rustRate/24*rampIntegral(sk.idle,dt,g,e));sk.idle=std::min(max,sk.idle+dt);
    }
}
static json lv(const LevelXP& l){return json::array({l.level,l.progress});}
json Model::save() const {
    json out={{"version",1},{"subs",unknownSubs},{"mains",unknownMains},{"activeHours",activeHours},{"lastHours",lastHours},{"pendingInit",pendingInit},{"raceClosed",raceClosed}};
    for(auto& sk:skills){json initials=unknownMains.contains(sk.av)?unknownMains.at(sk.av).value("initial",json::object()):json::object();for(auto& s:sk.subs){out["subs"][s.id]={{"current",lv(s.current)},{"floor",lv(s.floor)},{"potential",lv(s.potential)},{"peak",lv(s.peak)},{"idle",s.idle}};initials[s.id]=s.initial;}
        out["mains"][sk.av]={{"initialized",sk.initialized},{"base",sk.base},{"rust",sk.rust},{"idle",sk.idle},{"written",sk.written},{"initial",initials}};
    }return out;
}
void Model::restore(const json& j) {
    if(j.at("version")!=1)throw std::runtime_error("unsupported save version");
    auto read=[](const json& a){return LevelXP{a.at(0).get<std::uint16_t>(),a.at(1).get<float>()};};
    unknownSubs=j.at("subs");unknownMains=j.at("mains");activeHours=j.at("activeHours");lastHours=j.at("lastHours");pendingInit=j.at("pendingInit");raceClosed=j.at("raceClosed");
    for(auto& sk:skills){sk.initialized=false;if(unknownMains.contains(sk.av)){auto v=unknownMains[sk.av];sk.initialized=v.at("initialized");sk.base=v.at("base");sk.rust=v.at("rust");sk.idle=v.at("idle");sk.written=v.at("written");
        for(auto& s:sk.subs){if(!unknownSubs.contains(s.id)||!v.at("initial").contains(s.id)){double f=(s.fixed&&s.k==0)?0:std::clamp(settings.get("init","floor_per_level")*(sk.base-settings.get("init","floor_zero_at")),0.,settings.get("xp","floor_cap"));s.initial=f;s.current.set(f,s.curve,int(settings.get("xp","hard_cap")));s.floor=s.potential=s.peak=s.current;s.idle=s.lastXP=0;continue;}auto q=unknownSubs[s.id];s.current=read(q.at("current"));s.floor=read(q.at("floor"));s.potential=read(q.at("potential"));s.peak=read(q.at("peak"));s.idle=q.at("idle");s.initial=v["initial"][s.id];
            for(auto l:{s.current,s.floor,s.potential,s.peak})if(l.level>settings.get("xp","hard_cap")||!std::isfinite(l.progress)||l.progress<0)throw std::runtime_error("invalid saved subskill "+s.id);
            if(!std::isfinite(s.idle)||s.idle<0||!std::isfinite(s.initial))throw std::runtime_error("invalid saved dynamics");
        }
        if(!std::isfinite(sk.base)||!std::isfinite(sk.rust)||sk.rust<0||!std::isfinite(sk.idle)||sk.idle<0||!std::isfinite(sk.written))throw std::runtime_error("invalid saved main skill");
    }}
}
json Model::snapshot() const {
    json out={{"state",pendingInit?"pending":"running"},{"activeHours",activeHours},{"debug",settings.values["debug"]},{"skills",json::array()},{"equipped",json::array()}};
    double g=settings.get("decay","grace_hours"),e=settings.get("decay","ramp_end_hours");if(settings.flag("debug","no_grace"))g=e=0;
    for(auto& sk:skills){json a={{"av",sk.av},{"name",sk.av},{"F",sk.computed(settings.get("main","min_level"),settings.get("main","max_level"))},{"written",sk.written},{"base",sk.base},{"R",sk.rust},{"Umain",sk.idle},{"rustPerDay",sk.rustRate*ramp(sk.idle,g,e)},{"external",sk.external},{"pending",!sk.initialized},{"subs",json::array()}};
        for(auto& s:sk.subs){double c=s.current.value(s.curve),f=s.floor.value(s.curve),p=s.potential.value(s.curve);a["subs"].push_back({{"id",s.id},{"ownFloor",f},{"effFloor",f},{"current",c},{"potential",p},{"peak",s.peak.value(s.curve)},{"U",s.idle},{"decayPerDay",s.decay*ramp(s.idle,g,e)},{"zone",settings.get("xp",c<f?"zone_catch_up":c<p?"zone_recall":"zone_new")},{"worth",s.worth(std::max(c,s.peak.value(s.curve)-sk.rust))-s.worth(s.initial)},{"lastXP",s.lastXP}});}out["skills"].push_back(a);
    }return out;
}
}

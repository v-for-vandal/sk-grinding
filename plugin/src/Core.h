#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
namespace sp {
using json = nlohmann::json;
struct Curve {
    double mult=2, offset=0, exponent=1.95;
    double cost(int level) const;
    double between(double from, double to) const;
    double levelFromXP(double base, double xp, int cap=500) const;
};
struct LevelXP {
    std::uint16_t level=0;
    float progress=0;
    double value(const Curve&) const;
    void set(double value,const Curve&,int cap);
    void add(double xp,const Curve&,int cap);
    void subtract(double levels,const Curve&,const LevelXP& floor);
};
struct Sub {
    std::string id, group;
    Curve curve;
    double T=90,w=15,T1=25,k=0,decay=0,initial=0;
    bool fixed=false;
    std::vector<double> integral;
    LevelXP current, floor, potential, peak;
    double idle=0,lastXP=0;
    double shape(double level) const;
    void prepare(int cap);
    double worth(double level) const;
};
struct Skill {
    std::string av;
    Curve curve;
    double start=15,target=100,pathLevel=75,baseMult=1,decay=1,rustRate=.5,paceSub=2000,paceSkill=1000,bashFactor=.5;
    std::vector<std::string> path;
    std::map<std::string,double> budgets;
    std::vector<Sub> subs;
    bool initialized=false;
    double base=15,rust=0,idle=0,written=15;
    unsigned external=0;
    void solve(int cap);
    double computed(double minimum,double maximum) const;
};
struct Settings {
    json values;
    double get(const char* section,const char* key) const;
    bool flag(const char* section,const char* key) const;
};
struct Model {
    Settings settings;
    std::vector<Skill> skills;
    double activeHours=0,lastHours=0;
    bool pendingInit=true,raceClosed=false;
    json unknownSubs=json::object(),unknownMains=json::object();
    void loadConfig(const std::string& root);
    void initialize(Skill&, double base);
    Sub* find(const std::string& id,Skill** parent=nullptr);
    void award(const std::string& id,double raw);
    void tick(double hours);
    json save() const;
    void restore(const json&);
    json snapshot() const;
};
double ramp(double idle,double grace,double end);
double rampIntegral(double idle,double dt,double grace,double end);
}

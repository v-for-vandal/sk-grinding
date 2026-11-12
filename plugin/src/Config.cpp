#include "Core.h"
#include <filesystem>
#include <set>
#include <toml++/toml.hpp>
namespace sp {
namespace {
json convert(const toml::node& n) {
    if(auto t=n.as_table()){json j=json::object();for(auto& [k,v]:*t)j[std::string(k.str())]=convert(v);return j;}
    if(auto a=n.as_array()){json j=json::array();for(auto& v:*a)j.push_back(convert(v));return j;}
    if(n.is_boolean())return *n.value<bool>();
    if(n.is_integer())return *n.value<std::int64_t>();
    if(n.is_floating_point())return *n.value<double>();
    if(n.is_string())return *n.value<std::string>();
    throw std::runtime_error("unsupported TOML value");
}
Curve curve(const json& j,Curve c={}) {c.mult=j.value("improve_mult",c.mult);c.offset=j.value("improve_offset",c.offset);c.exponent=j.value("exponent",c.exponent);if(c.mult<0||c.offset<0||c.exponent<=0)throw std::runtime_error("invalid curve");return c;}
}
void Model::loadConfig(const std::string& root) {
    Model next;
    next.settings.values=convert(toml::parse_file(root+"/SkillPractice.toml"));
    int cap=int(next.settings.get("xp","hard_cap"));
    if(cap<1||cap>65534||next.settings.get("decay","q")<=0||next.settings.get("xp","zone_new")<=0||next.settings.get("xp","zone_recall")<=0||next.settings.get("xp","zone_catch_up")<=0)throw std::runtime_error("invalid XP/decay configuration");
    std::set<std::string> ids,avs;
    std::vector<std::filesystem::path> files;
    for(auto& f:std::filesystem::directory_iterator(root+"/skills"))if(f.path().extension()==".toml")files.push_back(f.path());
    std::sort(files.begin(),files.end());
    for(auto& path:files){auto j=convert(toml::parse_file(path.string()));Skill sk;auto m=j.at("main"),d=j.at("dynamics");
        sk.av=m.at("actor_value");if(!avs.insert(sk.av).second)throw std::runtime_error("duplicate skill "+sk.av);
        sk.curve=curve(m);sk.start=m.at("start_level");sk.target=m.at("target_level");sk.baseMult=d.at("base_mult");sk.decay=d.at("decay_rate");sk.rustRate=d.at("rust_rate");sk.paceSub=d.at("pace_sub");sk.paceSkill=d.at("pace_skill");
        if(sk.paceSub<=0||sk.paceSkill<=0||sk.decay<0||sk.rustRate<0)throw std::runtime_error("invalid dynamics");
        sk.bashFactor=j.value("routing",json::object()).value("bash_shield_factor",.5);
        sk.pathLevel=j.at("reference_path").at("level");sk.path=j.at("reference_path").at("members").get<std::vector<std::string>>();
        double remain=sk.curve.between(sk.start,sk.target),shares=0;
        for(auto& [g,b]:j.at("budget").items())if(b.contains("main_from")){double n=sk.curve.between(b.at("main_from"),b.at("main_to"));sk.budgets[g]=n;remain-=n;}else shares+=b.at("share").get<double>();
        if(remain<0)throw std::runtime_error("absolute budgets exceed target");
        for(auto& [g,b]:j.at("budget").items())if(b.contains("share")){if(shares<=0)throw std::runtime_error("invalid shares");sk.budgets[g]=remain*b.at("share").get<double>()/shares;}
        for(auto& v:j.at("subskill")){Sub s;s.id=v.at("name");if(!ids.insert(s.id).second)throw std::runtime_error("duplicate subskill "+s.id);s.group=v.at("group");s.T=v.at("T");s.w=v.at("w");s.T1=v.value("T1",j.at("conversion").value("rise_T1",25.));s.fixed=v.contains("k");s.k=v.value("k",0.);s.decay=v.value("decay_rate",sk.decay);s.curve=curve(v.value("curve",json::object()),curve(j.at("sub_curve")));if(s.w<=0||s.decay<0)throw std::runtime_error("invalid subskill");if(!s.fixed&&!sk.budgets.contains(s.group))throw std::runtime_error("missing group budget");sk.subs.push_back(s);}
        for(auto& id:sk.path)if(std::none_of(sk.subs.begin(),sk.subs.end(),[&](auto& s){return s.id==id;}))throw std::runtime_error("unknown path member "+id);
        sk.solve(cap);next.skills.push_back(sk);
    }
    next.settings.values["materials"]=convert(toml::parse_file(root+"/smithing-materials.toml"));
    next.settings.values["directions"]=convert(toml::parse_file(root+"/magic-directions.toml"));
    if(!skills.empty())next.restore(save());
    *this=std::move(next);
}
}

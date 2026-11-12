#include "Runtime.h"
#include <fstream>
#include <set>
namespace runtime {
sp::Model model;
namespace {
constexpr std::uint32_t subsTag=0x53554253,mainTag=0x4D41494E,timeTag=0x54494D45,statTag=0x53544154;
bool restored=false,failedSave=false;
std::string root="Data/SKSE/Plugins/SkillPractice";
void write(SKSE::SerializationInterface* out,std::uint32_t tag,const sp::json& j){auto text=j.dump();if(!out->OpenRecord(tag,1)||!out->WriteRecordData(text.data(),static_cast<std::uint32_t>(text.size())))spdlog::error("[save] write failed tag={:X}",tag);}
void save(SKSE::SerializationInterface* out){auto j=model.save();write(out,subsTag,j["subs"]);write(out,mainTag,j["mains"]);write(out,timeTag,{{"activeHours",j["activeHours"]},{"lastHours",j["lastHours"]}});write(out,statTag,{{"pendingInit",j["pendingInit"]},{"raceClosed",j["raceClosed"]}});spdlog::info("[save] saved version=1 subs={} mains={}",j["subs"].size(),j["mains"].size());}
void load(SKSE::SerializationInterface* in){
    sp::json j={{"version",1},{"subs",sp::json::object()},{"mains",sp::json::object()},{"activeHours",0},{"lastHours",0},{"pendingInit",false},{"raceClosed",true}};std::uint32_t type,version,length;bool any=false;
    try{while(in->GetNextRecordInfo(type,version,length)){if(type!=subsTag&&type!=mainTag&&type!=timeTag&&type!=statTag)continue;if(version!=1||length>16*1024*1024)throw std::runtime_error("unsupported/oversized co-save record");std::string text(length,'\0');if(in->ReadRecordData(text.data(),length)!=length)throw std::runtime_error("truncated co-save");auto data=sp::json::parse(text);if(type==subsTag)j["subs"]=data;else if(type==mainTag)j["mains"]=data;else j.update(data);any=true;}
        if(any){auto copy=model;copy.restore(j);model=std::move(copy);restored=true;spdlog::info("[save] loaded version=1 subs={} mains={}",j["subs"].size(),j["mains"].size());}
    }catch(const std::exception& e){failedSave=true;spdlog::critical("[save] load rejected: {}; gameplay disabled until another save is loaded",e.what());}
}
void revert(SKSE::SerializationInterface*){for(auto& sk:model.skills){sk.initialized=false;sk.external=0;for(auto& s:sk.subs){s.current=s.floor=s.potential=s.peak={};s.idle=s.initial=s.lastXP=0;}sk.rust=sk.idle=0;}model.unknownSubs=sp::json::object();model.unknownMains=sp::json::object();model.activeHours=model.lastHours=0;model.raceClosed=false;model.pendingInit=true;restored=failedSave=false;closeUI();spdlog::info("[save] revert");}
void readINI(const std::string& path){std::ifstream in(path);if(!in)return;std::string section,line;
    while(std::getline(in,line)){auto clean=[](std::string v){auto a=v.find_first_not_of(" \t\r");if(a==std::string::npos)return std::string();return v.substr(a,v.find_last_not_of(" \t\r")-a+1);};line=clean(line);if(line.empty()||line[0]==';'||line[0]=='#')continue;if(line[0]=='['){section=line.substr(1,line.find(']')-1);continue;}auto eq=line.find('=');if(eq==std::string::npos)continue;auto key=clean(line.substr(0,eq));int v;try{v=std::stoi(clean(line.substr(eq+1)));}catch(...){spdlog::warn("[cfg] invalid INI {} {}",path,key);continue;}
        auto set=[&](const char* group,const char* name,sp::json value){auto& old=model.settings.values[group][name];if(old!=value)spdlog::info("[cfg] {}.{} {} -> {}",group,name,old.dump(),value.dump());old=value;};
        if(section=="Debug"){if(key=="iXpMult"){constexpr int a[]{1,10,100};if(v>=0&&v<3)set("debug","xp_mult",a[v]);}else if(key=="iTimeScale"){constexpr int a[]{1,10,100,1000};if(v>=0&&v<4)set("debug","time_scale",a[v]);}else if(key=="bFixedXP")set("debug","fixed_xp",v!=0);else if(key=="bNoGrace")set("debug","no_grace",v!=0);else if(key=="bWriteMain")set("main","write_main_skills",v!=0);else if(key=="bPassthrough")set("debug","vanilla_passthrough",v!=0);}
        else if(section=="Aim"){if(key=="bMissile")set("aim","missile_enabled",v!=0);else if(key=="bBeam")set("aim","beam_enabled",v!=0);}
        else if(section=="Log"){if(key=="bXpVerbose")set("log","xp_verbose",v!=0);else if(key=="iLevel"){constexpr const char* a[]{"trace","debug","info","warn"};if(v>=0&&v<4)set("log","level",a[v]);}}
    }
}
}
RE::ActorValue av(std::string_view name){using A=RE::ActorValue;if(name=="OneHanded")return A::kOneHanded;if(name=="TwoHanded")return A::kTwoHanded;if(name=="Block")return A::kBlock;if(name=="Smithing")return A::kSmithing;if(name=="Destruction")return A::kDestruction;if(name=="Restoration")return A::kRestoration;return A::kNone;}
sp::Skill* skill(RE::ActorValue value){for(auto& sk:model.skills)if(av(sk.av)==value)return &sk;return nullptr;}
void reload(){try{model.loadConfig(root);readINI("Data/MCM/Config/SkillPractice/settings.ini");readINI("Data/MCM/Settings/SkillPractice.ini");auto level=model.settings.values["log"]["level"].get<std::string>();spdlog::set_level(spdlog::level::from_str(level));for(auto& [group,values]:model.settings.values.items())for(auto& [key,value]:values.items())spdlog::debug("[cfg] {}.{}={}",group,key,value.dump());resolveForms();}catch(const std::exception& e){spdlog::error("[cfg] reload failed; previous configuration retained: {}",e.what());}}
bool ready(sp::Skill& sk){if(failedSave)return false;if(!sk.initialized){if(!model.raceClosed)return false;auto p=RE::PlayerCharacter::GetSingleton();model.initialize(sk,p->AsActorValueOwner()->GetBaseActorValue(av(sk.av)));spdlog::info("[init] skill={} base={} floor={} count={}",sk.av,sk.base,sk.subs.empty()?0:sk.subs[0].initial,sk.subs.size());}model.pendingInit=std::any_of(model.skills.begin(),model.skills.end(),[](auto& s){return !s.initialized;});return true;}
void initialize(bool force){if(failedSave)return;for(auto& sk:model.skills)if(force||!sk.initialized){sk.initialized=false;bool old=model.raceClosed;model.raceClosed=true;ready(sk);model.raceClosed=old;}synchronize();}
void synchronize(){auto p=RE::PlayerCharacter::GetSingleton();if(!p||failedSave)return;auto owner=p->AsActorValueOwner();for(auto& sk:model.skills)if(sk.initialized){auto value=av(sk.av);double actual=owner->GetBaseActorValue(value);if(std::abs(actual-sk.written)>.01){double d=actual-sk.written;sk.base+=d;sk.written=actual;++sk.external;spdlog::info("[main] external skill={} delta={} base={}",sk.av,d,sk.base);}
        double f=sk.computed(model.settings.get("main","min_level"),model.settings.get("main","max_level")),n=sk.written;
        while(f>=n+model.settings.get("main","hysteresis_up"))++n;
        while(f<n-model.settings.get("main","hysteresis_down"))--n;
        if(n!=sk.written&&model.settings.flag("main","write_main_skills")){owner->SetBaseActorValue(value,static_cast<float>(n));std::string msg=sk.av+(n>sk.written?" increased to ":" decreased to ")+std::to_string(int(n));RE::SendHUDMessage::ShowHUDMessage(msg.c_str());spdlog::info("[main] write skill={} F={} R={} old={} written={}",sk.av,f,sk.rust,sk.written,n);sk.written=n;}
    }}
void award(std::string_view id,double raw,std::string_view origin){sp::Skill* sk=nullptr;auto s=model.find(std::string(id),&sk);if(!s){spdlog::debug("[xp] unmanaged sub={} skipped",id);return;}if(!ready(*sk))return;double before=s->current.value(s->curve);model.award(std::string(id),raw);if(model.settings.flag("log","xp_verbose"))spdlog::info("[xp] t={} src={} skill={} sub={} raw={} cur={}->{} floor={} pot={} peak={} U={} R={} F={}",model.activeHours,origin,sk->av,id,raw,before,s->current.value(s->curve),s->floor.value(s->curve),s->potential.value(s->curve),s->peak.value(s->curve),s->idle,sk->rust,sk->computed(model.settings.get("main","min_level"),model.settings.get("main","max_level")));}
void resetContext();
void message(SKSE::MessagingInterface::Message* m){
    if(m->type==SKSE::MessagingInterface::kDataLoaded){reload();if(model.skills.empty()){spdlog::critical("[boot] disabled: no valid configuration");return;}size_t n=0;for(auto& s:model.skills)n+=s.subs.size();spdlog::info("[boot] managed={} subskills={}",model.skills.size(),n);installTelemetry();installGameplay();setupUI();installAim();}
    else if(m->type==SKSE::MessagingInterface::kPostLoadGame){reload();resetContext();if(!restored)model.raceClosed=true;for(auto& sk:model.skills)if(!sk.initialized)ready(sk);synchronize();}
    else if(m->type==SKSE::MessagingInterface::kNewGame){reload();model.raceClosed=false;model.pendingInit=true;resetContext();spdlog::info("[init] new game pending; awaiting RaceSex close and first use per skill");}
}
void start(){auto si=SKSE::GetSerializationInterface();si->SetUniqueID(0x534B5052);si->SetSaveCallback(save);si->SetLoadCallback(load);si->SetRevertCallback(revert);}
}


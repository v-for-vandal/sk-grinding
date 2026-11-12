#include "Runtime.h"
#include <MinHook.h>
#include <random>
#include <numbers>
namespace runtime {
namespace {
std::mt19937 rng{std::random_device{}()};
using Launch=RE::ProjectileHandle*(*)(RE::ProjectileHandle*,RE::Projectile::LaunchData&) noexcept;
Launch original=nullptr;
double phaseX=0,phaseZ=0,lastX=0,lastZ=0,held=0;
double amplitudeDegrees=0;
bool swaying=false;
RE::ProjectileHandle* launch(RE::ProjectileHandle* result,RE::Projectile::LaunchData& data) noexcept {
    if(GetCurrentThreadId()==mainThread&&data.shooter==RE::PlayerCharacter::GetSingleton()&&data.spell&&model.settings.flag("aim","missile_enabled")){
        auto spell=data.spell->As<RE::SpellItem>();auto c=classify(spell);
        if(c.form=="missile"){auto s=model.find(c.school+"/missile");auto sk=skill(c.actorValue);if(s&&sk&&sk->initialized){double sigma=model.settings.get("aim","missile_sigma_max_deg")*std::max(0.,1-s->current.value(s->curve)/model.settings.get("aim","missile_L0")),fixed=model.settings.get("aim","debug_missile_yaw_deg");if(sigma>0||fixed!=0){std::normal_distribution<double> noise(0,std::max(sigma,1e-9)*std::numbers::pi/180);double x=fixed!=0?0:noise(rng),z=fixed!=0?fixed*std::numbers::pi/180:noise(rng);data.angleX+=float(x);data.angleZ+=float(z);spdlog::debug("[aim] launch form={:08X} school={} sigma={} pitch={} yaw={} autoAim={} target={:08X}",spell->GetFormID(),c.school,sigma,x,z,data.autoAim,data.desiredTarget?data.desiredTarget->GetFormID():0);}}}
    }
    return original(result,data);
}
}
void installAim(){
    auto address=REL::Relocation<std::uintptr_t>{REL::RelocationID(42928,44108)}.address();auto status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED){spdlog::error("[aim] MinHook initialize failed {}",int(status));return;}
    status=MH_CreateHook(reinterpret_cast<void*>(address),reinterpret_cast<void*>(launch),reinterpret_cast<void**>(&original));if(status==MH_OK)status=MH_EnableHook(reinterpret_cast<void*>(address));
    if(status==MH_OK)spdlog::info("[boot] hook=Projectile::Launch AddressLibrary=42928/44108 MinHook trampoline");else spdlog::error("[aim] launch hook failed {}",int(status));
}
void refreshAim(float dt){
    auto p=RE::PlayerCharacter::GetSingleton();if(!p||dt<=0)return;double level=1e9;bool casting=false;
    if(model.settings.flag("aim","beam_enabled"))for(auto hand:{RE::MagicSystem::CastingSource::kLeftHand,RE::MagicSystem::CastingSource::kRightHand,RE::MagicSystem::CastingSource::kOther}){
        auto caster=p->GetMagicCaster(hand);if(!caster||!caster->currentSpell||!caster->state.any(RE::MagicCaster::State::kCharging,RE::MagicCaster::State::kCasting))continue;
        auto spell=caster->currentSpell->As<RE::SpellItem>();auto c=classify(spell);if(c.form!="beam")continue;auto sub=model.find(c.school+"/beam");auto sk=skill(c.actorValue);if(sub&&sk&&sk->initialized){casting=true;level=std::min(level,sub->current.value(sub->curve));}
    }
    if(!casting){if(swaying)spdlog::debug("[aim] sway stop held={}",held);swaying=false;held=lastX=lastZ=amplitudeDegrees=0;return;}
    if(!swaying){std::uniform_real_distribution<double> phase(0,2*std::numbers::pi);phaseX=phase(rng);phaseZ=phase(rng);held=lastX=lastZ=0;swaying=true;spdlog::debug("[aim] sway start level={}",level);}
    held+=dt;double amp=model.settings.get("aim","beam_amp_max_deg")*std::max(0.,1-level/model.settings.get("aim","beam_L0"))*std::min(1.,held/model.settings.get("aim","beam_t_full_sec"))*std::numbers::pi/180;
    amplitudeDegrees=amp*180/std::numbers::pi;
    double t=held*2*std::numbers::pi*model.settings.get("aim","beam_noise_hz"),weight=model.settings.get("aim","beam_secondary_weight");double x=amp*(std::sin(t+phaseX)+weight*std::sin(t*model.settings.get("aim","beam_pitch_secondary_frequency")+phaseZ))/(1+weight),z=amp*(std::sin(t*model.settings.get("aim","beam_yaw_primary_frequency")+phaseZ)+weight*std::sin(t*model.settings.get("aim","beam_yaw_secondary_frequency")+phaseX))/(1+weight);
    p->SetAngle(RE::NiPoint3{p->GetAngleX()+float(x-lastX),p->GetAngleY(),p->GetAngleZ()+float(z-lastZ)});lastX=x;lastZ=z;
}
double beamAmplitude(){return amplitudeDegrees;}
}


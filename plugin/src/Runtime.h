#pragma once
#include "Core.h"
namespace runtime {
extern sp::Model model;
void start();
void message(SKSE::MessagingInterface::Message*);
void installTelemetry();
void installGameplay();
void installAim();
void setupUI();
void refreshUI(float dt);
void closeUI();
void reload();
void synchronize();
void initialize(bool force=false);
void action(const char*);
sp::json snapshot();
std::uint64_t stamp();
extern std::uint64_t frame;
extern DWORD mainThread;
RE::ActorValue av(std::string_view);
sp::Skill* skill(RE::ActorValue);
bool ready(sp::Skill&);
void award(std::string_view,double,std::string_view,double points=-1);
struct Classification {std::string school,form,direction,projectile;RE::ActorValue actorValue=RE::ActorValue::kNone;};
Classification classify(RE::SpellItem*);
std::string weapon(RE::TESObjectWEAP*);
std::string weaponKeyword(RE::TESObjectWEAP*);
double beamAmplitude();
void resolveForms();
void dumpWeapons();
void dumpSpells();
void route(RE::ActorValue,float,RE::TESForm*,std::string_view,bool bash=false);
}

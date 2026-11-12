#include "Runtime.h"
#include "PrismaUI_API.h"
#include "WeaponClasses.h"
namespace runtime {
namespace {
PRISMA_UI_API::IVPrismaUI1* api=nullptr;
PrismaView view=0;bool dom=false,visible=false;
float elapsed=0;
void send(){if(dom&&api&&view){auto data=snapshot().dump();api->InteropCall(view,"spUpdate",data.c_str());}}
void onAction(const char* text){std::string copy=text?text:"";spdlog::debug("[ui] callback thread={} main={}",GetCurrentThreadId(),mainThread);if(GetCurrentThreadId()!=mainThread)SKSE::GetTaskInterface()->AddTask([copy]{action(copy.c_str());});else action(copy.c_str());}
void ready(PrismaView v){view=v;dom=true;api->RegisterJSListener(view,"spAction",onAction);api->Hide(view);spdlog::info("[ui] DOM ready thread={}",GetCurrentThreadId());}
void console(PrismaView,PRISMA_UI_API::ConsoleMessageLevel level,const char* msg){spdlog::info("[ui] js level={} {}",int(level),msg?msg:"");}
void toggle(){if(!api||!dom)return;if(visible){closeUI();return;}if(RE::UI::GetSingleton()->IsMenuOpen(RE::MainMenu::MENU_NAME)||RE::UI::GetSingleton()->IsMenuOpen(RE::Console::MENU_NAME)||RE::UI::GetSingleton()->GameIsPaused())return;visible=true;send();api->Show(view);if(!api->Focus(view,false)){visible=false;api->Hide(view);spdlog::warn("[ui] focus failed");}else spdlog::info("[ui] opened");}
class Input : public RE::BSTEventSink<RE::InputEvent*> {
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events,RE::BSTEventSource<RE::InputEvent*>*) override {
        if(events)for(auto e=*events;e;e=e->next)if(auto b=e->AsButtonEvent();b&&b->GetDevice()==RE::INPUT_DEVICE::kKeyboard&&b->IsDown()&&b->GetIDCode()==static_cast<unsigned>(model.settings.get("ui","toggle_key")))toggle();return RE::BSEventNotifyControl::kContinue;
    }
};Input input;
}
void closeUI(){if(api&&view){api->Unfocus(view);api->Hide(view);}visible=false;}
void setupUI(){auto v2=PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI2>();api=v2?v2:PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI1>();if(!api){spdlog::warn("[ui] PrismaUI not available");return;}view=api->CreateView("SkillPractice/index.html",ready);api->Hide(view);if(v2)v2->RegisterConsoleCallback(view,console);if(model.settings.flag("ui","inspector")){api->CreateInspectorView(view);api->SetInspectorVisibility(view,true);}RE::BSInputDeviceManager::GetSingleton()->AddEventSink(&input);spdlog::info("[boot] PrismaUI API V{} view={}",v2?2:1,view);}
void refreshAim(float);
void refreshUI(float dt){refreshAim(dt);if(!visible||!dom)return;elapsed+=dt;if(elapsed*1000>=model.settings.get("ui","refresh_ms")){elapsed=0;send();}}
sp::json snapshot(){auto j=model.snapshot();j["debug"]["write_main_skills"]=model.settings.flag("main","write_main_skills");auto p=RE::PlayerCharacter::GetSingleton();if(!p)return j;
    for(bool left:{false,true}){auto item=p->GetEquippedObject(left);sp::json e={{"hand",left?"left":"right"},{"kind","none"},{"name",item?item->GetName():"Unarmed"}};if(auto w=item?item->As<RE::TESObjectWEAP>():nullptr){e["kind"]="weapon";e["sub"]=weapon(w);}else if(auto s=item?item->As<RE::SpellItem>():nullptr){auto c=classify(s);e["kind"]="spell";e["form"]=c.form;e["direction"]=c.direction;e["projectile"]=c.projectile;auto sub=model.find(c.school+"/"+c.form);double level=sub?sub->current.value(sub->curve):0;e["formLevel"]=level;e["sigma"]=model.settings.get("aim","missile_sigma_max_deg")*std::max(0.,1-level/model.settings.get("aim","missile_L0"));e["sway"]=model.settings.get("aim","beam_amp_max_deg")*std::max(0.,1-level/model.settings.get("aim","beam_L0"));}else if(item&&item->As<RE::TESObjectARMO>())e["kind"]="shield";j["equipped"].push_back(e);
    }return j;
}
void action(const char* text){try{auto j=sp::json::parse(text);auto name=j.at("action").get<std::string>();spdlog::info("[ui] action={}",j.dump());
    if(name=="close")closeUI();else if(name=="dumpState"){spdlog::info("[ui] state={}",snapshot().dump());spdlog::default_logger()->flush();}else if(name=="dumpWeapons")dumpWeapons();else if(name=="dumpSpells")dumpSpells();else if(name=="reloadConfig")reload();else if(name=="initNow")initialize();else if(name=="reinitAll")initialize(true);
    else if(name=="giveXP"){double raw=j.at("raw");if(std::isfinite(raw)&&raw>=0){award(j.at("id").get<std::string>(),raw,"debug");synchronize();}}
    else if(name=="setCurrent"){sp::Skill* sk=nullptr;auto s=model.find(j.at("id"),&sk);double l=j.at("level");if(s&&std::isfinite(l)&&l>=0&&ready(*sk)){s->current.set(l,s->curve,int(model.settings.get("xp","hard_cap")));if(s->current.value(s->curve)>s->potential.value(s->curve))s->potential=s->current;if(s->current.value(s->curve)>s->peak.value(s->curve))s->peak=s->current;synchronize();}}
    send();
}catch(const std::exception& e){spdlog::warn("[ui] invalid action: {}",e.what());}}
}

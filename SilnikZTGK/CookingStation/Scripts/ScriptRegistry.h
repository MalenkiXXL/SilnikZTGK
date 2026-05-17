#pragma once
#include "CookingStation/Scene/ecs.h" 
#include <unordered_map>
#include <string>
#include <functional>
#include <spdlog/spdlog.h>

// --- INCLUDUJEMY WSZYSTKIE SKRYPTY ---
#include "CookingStation/Scripts/RotationScript.h"
#include "SilnikZTGK/CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "SilnikZTGK/CookingStation/Scripts/Plates/ItemScript.h"
#include "SilnikZTGK/CookingStation/Scripts/ConveyorBelt/BeltVisualScript.h"
#include "CookingStation/Scripts/PotScript.h"
#include "SilnikZTGK/CookingStation/Scripts/Plates/PlateSpawnerScript.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Scripts/SteamEmitterScript.h"
#include "CookingStation/Scripts/DustEmitterScript.h"
#include "SilnikZTGK/CookingStation/Scripts/Delivery/DeliveryCarScript.h"
#include "SilnikZTGK/CookingStation/Scripts/Managers/GameManagerScript.h"
#include "SilnikZTGK/CookingStation/Scripts/Delivery/PackageScript.h"
#include "DragAndDropScript.h"
#include "MushroomAI.h"
#include "CustomerManagerScript.h"
#include "CustomerScript.h"

//globalny slownik ktory mapuke tekst na funkcje
class ScriptRegistry
{
public:
    using ScriptAdder = std::function<void(NativeScriptComponent&)>;

    static std::unordered_map<std::string, ScriptAdder>& GetRegistry()
    {
        static std::unordered_map<std::string, ScriptAdder> registry;
        return registry;
    }

    template<typename T>
    static void Register(const std::string& name)
    {
        GetRegistry()[name] = [name](NativeScriptComponent& nsc) {
            nsc.AddScript<T>(name);
            };
    }

    static void Init()
    {
        // Tu dodajesz ka�dy nowy skrypt
        Register<RotationScript>("RotationScript");
        Register<ConveyorScript>("ConveyorScript");
        Register<ItemScript>("ItemScript");
        Register<BeltVisualScript>("BeltVisualScript");
        Register<PotScript>("PotScript");
        Register<PlateSpawnerScript>("PlateSpawnerScript");
        Register<ParticleEmitterScript>("ParticleEmitterScript");
        Register<DragAndDropScript>("DragAndDropScript");
        Register<MushroomAI>("MushroomAI");
        Register<CustomerManagerScript>("CustomerManagerScript");
        Register<CustomerScript>("CustomerScript");
        Register<SteamEmitterScript>("SteamEmitterScript");
        Register<DustEmitterScript>("DustEmitterScript");
        Register<DeliveryCarScript>("DeliveryCarScript");
        Register<GameManagerScript>("GameManagerScript");
        Register<PackageScript>("PackageScript");
    }

    static void AddScriptToComponent(NativeScriptComponent& nsc, const std::string& name)
    {
        auto& reg = GetRegistry();
        if (reg.find(name) != reg.end())
        {
            reg[name](nsc);
        }
        else
        {
            spdlog::warn("ScriptRegistry: Nie znaleziono skryptu o nazwie '{}'!", name);
        }
    }
}; 

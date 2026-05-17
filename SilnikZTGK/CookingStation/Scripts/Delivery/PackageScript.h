//
// Created by Amelia on 17.05.2026.
//

#ifndef SILNIKZTGK_PACKAGESCRIPT_H
#define SILNIKZTGK_PACKAGESCRIPT_H
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"

class PackageScript : public ScriptableEntity{
public:
    inline static std::vector<Entity> s_ActivePackages;

    IngredientType m_Type = IngredientType::Tomato;

    int m_IngredientAmount = 5;
    void OnCreate() override {
        s_ActivePackages.push_back(m_Entity);
    };
    void OnUpdate(Timestep ts) override {};

    void OnClick() override;
};


#endif //SILNIKZTGK_PACKAGESCRIPT_H

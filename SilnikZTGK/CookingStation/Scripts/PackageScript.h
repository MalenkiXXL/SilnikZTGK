//
// Created by Amelia on 17.05.2026.
//

#ifndef SILNIKZTGK_PACKAGESCRIPT_H
#define SILNIKZTGK_PACKAGESCRIPT_H
#include "CookingStation/Scene/ScriptableEntity.h"

class PackageScript : public ScriptableEntity{
public:
    int m_IngredientAmount = 5;
    void OnCreate() override {};
    void OnUpdate(Timestep ts) override {};

    void OnClick() override;
};


#endif //SILNIKZTGK_PACKAGESCRIPT_H

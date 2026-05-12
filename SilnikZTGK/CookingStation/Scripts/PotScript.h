#pragma once
#include "MachineScript.h"

class PotScript : public MachineScript
{
public:
    void OnCreate() override
    {
        m_CookTime = 10.0f;
    }

    void OnUpdate(Timestep ts) override
    {
        // 1. Wywo³anie Update z klasy bazowej (¿eby garnek mo¿na by³o przenosiæ!)
        MachineScript::OnUpdate(ts);

        // Jeœli maszyna lata za myszk¹, nie gotujemy
        if (m_IsHeld) return;

        // 2. LOGIKA GOTOWANIA
        if (!m_Ingredients.empty() && !m_IsReady)
        {
            m_CurrentTime += ts.GetSeconds();

            if (m_CurrentTime >= m_CookTime)
            {
                m_IsReady = true;
                UpdateVisuals();
            }
        }

        // 3. AUTOMATYZACJA
        if (m_IsAutomated && m_IsReady)
        {
            TryTransferToPlate();
        }
    }

    bool AddIngredient(IngredientType type)
    {
        if (m_IsReady || m_Ingredients.size() >= 2) return false;

        m_Ingredients.push_back(type);
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        UpdateVisuals();

        return true;
    }

protected:
    void UpdateVisuals() override
    {
        // Tu podmienisz kolory/modele zupy w garnku, np.
        /*
        auto* mesh = GetComponent<MeshComponent>();
        if(mesh) {
            if(m_IsReady) mesh->ShaderPtr->SetFloat3("u_Color", {1, 0, 0});
        }
        */
    }
};
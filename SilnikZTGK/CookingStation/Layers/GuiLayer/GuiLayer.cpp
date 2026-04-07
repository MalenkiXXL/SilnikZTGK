#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"

void GuiLayer::OnAttach() {
    Renderer2D::Init();

    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    glEnable(GL_BLEND); // umozliwia mieszanie kolorow

    // mowi jak to mieszanie ma wygladac, dodaje do siebie:
    // GL_SRC_ALPHA = kolor * alpha    oraz 
    // GL_ONE_MINUS_SRC_ALPHA = kolor * (1-alfa)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

    // aktualizacja rozmiaru ekranu 
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);

    // wczytanie czcionki
    Gui::Init("CookingStation/Assets/fonts/ARIAL.ttf", 32);
}

void GuiLayer::OnUpdate(Timestep ts) {

    // pobranie aktualnej sceny
    auto& world = m_ActiveScene->GetWorld();

    // GUI rysujemy zawsze na wierzchu
    glDisable(GL_DEPTH_TEST); 

    // orto
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    Renderer2D::BeginScene(uiProj);

    // rysujemy kwadrat bedacy tlem na suwaki od koloru tla
    Renderer2D::DrawQuad({ 10.f, 20.f }, { 110.f, 165.f }, { 0.5f, 0.5f, 0.2f, 0.1f });

    // pobieramy komponent odpowiedzialny za kolor tla
    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();

    // jezeli komponent nie jest pusty to pobieramy z niego konkretne kolory rgb
    if (colorStorage && !colorStorage->dense.empty()) {
        float* r = &colorStorage->dense[0].bgColor.r;
        float* g = &colorStorage->dense[0].bgColor.g;
        float* b = &colorStorage->dense[0].bgColor.b;

        // tworzymy slidery dla konkretnych kolorow
        Gui::SliderFloat("R", r, 0.0f, 1.0f, { 15, 50 }, { 100, 20 });
        Gui::SliderFloat("G", g, 0.0f, 1.0f, { 15, 100 }, { 100, 20 });
        Gui::SliderFloat("B", b, 0.0f, 1.0f, { 15, 150 }, { 100, 20 });
    }
    

    // HIERARCHIA SCENY

    // pobieramy komponent odpowiadajacy za tagi obiektow
    auto* tagStorage = world.GetComponentVector<TagComponent>();
    if (tagStorage) {

        // pozycja startowa bardziej z lewej strony ekranu
        glm::vec2 startPos = { m_ViewportWidth - 220.0f, 50.0f };

        // rozmiar jednej pozycji
        glm::vec2 itemSize = { 200.0f, 30.0f };
        
        // przestrzeñ pomiêdzy
        float spacing = 5.0f;

        // podpis hierarchii sceny
        Gui::DrawGuiText("Hierarchia sceny:", { startPos.x, startPos.y - 25.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

        // iterujemy po wszystkich elementac posiadaj¹cych tagComponent
        for (size_t i = 0; i < tagStorage->dense.size(); i++) {

            // zmieniamy im position y
            float offset_y = i * (itemSize.y + spacing);
            glm::vec2 currentItemPos = { startPos.x, startPos.y + offset_y };

            //
            auto& tagComp = tagStorage->dense[i]; // pobieramy referencjê do tagu
            Entity owner = tagStorage->reverse[i]; // pobieramy jego wlasciciela

            // je¿eli przycisk jest wcisniety, wysylamy zaznaczony obiekt do sceny
            if (Gui::Button(tagComp.Tag, currentItemPos, itemSize)) {
                m_ActiveScene->SetSelectedEntity(owner);
                spdlog::info("Gui: Wybrano encje {}", tagComp.Tag);
            }
        }
    }

    // BIBLIOTEKA MODELI

    // rysujemy tlo
    Renderer2D::DrawQuad({ 10, 200 }, { 120, 400 }, { 0.1f, 0.1f, 0.1f, 0.7f });

    // rysujemy opis
    Gui::DrawGuiText("Biblioteka:", { 20, 225 }, 0.45f, { 1, 1, 1, 1 });

    float yOffset = 260.0f;

    // przechodzimy po wszystkich pozycjach z pliku otwieranego w AssetLayer
    for (const auto& entry : AssetManager::GetLibrary()) {
        if (Gui::Button(entry.Name, { 20, yOffset }, { 100, 25 })) {

            // wysylamy proœbê o umiejscowienie modelu
            auto& request = m_ActiveScene->GetPlacementRequest();
            request.Name = entry.Name;
            request.Path = entry.Path;
            request.Active = true; // ustawiamy prosbe na aktywna aby EditorLayer mogl odebrac
        }

        yOffset += 35.0f;
    }

 
    // INSPEKTOR ENCJI

    // pobieramy wybran¹ encjê ze sceny (przeslana przez EditorLayer)
    Entity selected = m_ActiveScene->GetSelectedEntity();
    if (selected.id != std::numeric_limits<std::size_t>::max()) {
        glm::vec2 inspPos = { m_ViewportWidth - 175.0f, 250.0f };

        // pobieramy tag
        auto* tag = world.GetComponent<TagComponent>(selected);
        if (tag) {
            // wyswietlamy nazwe 
            Gui::InputGuiText("Nazwa", tag->Tag, { inspPos.x, inspPos.y - 20.0f }, { 200, 25 });
        }

        // wyswietlamy komponenty od transformacji jako slidery, zeby dalo sie modyfikowac
        auto* transform = world.GetComponent<TransformComponent>(selected);
        if (transform) {
            Gui::SliderFloat("Pos X", &transform->Position.x, -100.0f, 100.0f, { inspPos.x, inspPos.y + 40.0f }, { 150, 15 });
            Gui::SliderFloat("Pos Y", &transform->Position.y, -100.0f, 100.0f, { inspPos.x, inspPos.y + 80.0f }, { 150, 15 });
            Gui::SliderFloat("Rot X", &transform->Rotation.x, -360.0f, 360.0f, { inspPos.x, inspPos.y + 120.0f }, { 150, 15 });
            Gui::SliderFloat("Rot Z", &transform->Rotation.z, -360.0f, 360.0f, { inspPos.x, inspPos.y + 160.0f }, { 150, 15 });
            Gui::SliderFloat("Rot Y", &transform->Rotation.y, -360.0f, 360.0f, { inspPos.x, inspPos.y + 200.0f }, { 150, 15 });
            Gui::SliderFloat("Skala", &transform->Scale.x, 0.1f, 5.0f, { inspPos.x, inspPos.y + 240.0f }, { 180, 15 });

            // na razie synchronizacja skali, potem sie zrobi ze mozna wszystkie (brakuje miejsca na ekranie xd)
            transform->Scale.y = transform->Scale.x;
            transform->Scale.z = transform->Scale.x;
        }

        // przycisk od usuwania obiektu
        if (Gui::Button("USUN OBIEKT", { inspPos.x, inspPos.y + 280.0f }, { 200.0f, 30.0f })) {
            world.DestroyEntity(selected);
            m_ActiveScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
        }
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

void GuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& ev) { Gui::OnCharTyped(ev.GetKeyCode()); return false; });
}

bool GuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}

GuiLayer::~GuiLayer() {}
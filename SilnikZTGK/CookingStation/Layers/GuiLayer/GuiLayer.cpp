#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CookingStation/Core/Application.h"
#include <cstdlib> 
#include <cstdio>

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

	std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

	if (!activeScene) {
		spdlog::error("AssetLayer: Brak aktywnej sceny!");
		return;
	}

	// pobranie aktualnej sceny

	auto& world = activeScene->GetWorld();

	// GUI rysujemy zawsze na wierzchu
	glDisable(GL_DEPTH_TEST);

	// orto
	glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

	Renderer2D::BeginScene(uiProj);

	if (m_ViewportFBO) {
		// Odsuwamy o 200px od lewej (na Hierarchiê) i 30px z góry (Pasek menu)
		glm::vec2 viewportPos = { 200.0f, 30.0f };
		// Zostawiamy 300px z prawej (na Inspektor) i 200px z do³u (na Bibliotekê)
		glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

		if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
			if (m_ViewportFBO->GetSpecification().Width != (uint32_t)viewportSize.x ||
				m_ViewportFBO->GetSpecification().Height != (uint32_t)viewportSize.y)
			{
				m_ViewportFBO->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
			}

			uint32_t textureID = m_ViewportFBO->GetColorAttachmentRendererID();
			Renderer2D::DrawQuad(viewportPos, viewportSize, textureID, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
		}
	}

	// --- G£ÓWNY PASEK ZADAÑ (MAIN MENU BAR) ---

// 1. Rysujemy t³o paska (szerokoœæ na ca³y ekran, wysokoœæ np. 30 pikseli)
	Renderer2D::DrawQuad({ 0.0f, 0.0f }, { m_ViewportWidth, 30.0f }, { 0.15f, 0.15f, 0.15f, 1.0f });

	// 2. Tworzymy przyciski otwieraj¹ce menu
	if (Gui::Button("Plik", { 10.0f, 5.0f }, { 80.0f, 20.0f })) {
		m_ShowFileMenu = !m_ShowFileMenu; // Prze³¹cz stan (otwórz/zamknij)
		m_ShowViewMenu = false;      // Zamknij pozosta³e
	}

	if (Gui::Button("Widok", { 100.0f, 5.0f }, { 80.0f, 20.0f })) {
		m_ShowViewMenu = !m_ShowViewMenu;
		m_ShowFileMenu = false;
	}

	glm::vec2 playButtonPos = { 190.0f, 5.0f }; 
	glm::vec2 playButtonSize = { 80.0f, 20.0f };

	if (activeScene->GetState() == SceneState::Edit) {
		if (Gui::Button("PLAY", playButtonPos, playButtonSize)) {
			// Zamiast logiki, generujemy zdarzenie
			ScenePlayEvent e;
			Application::Get().OnEvent(e);
		}
	}
	else {
		if (Gui::Button("STOP", playButtonPos, playButtonSize)) {
			// Zamiast logiki, generujemy zdarzenie
			SceneStopEvent e;
			Application::Get().OnEvent(e);
		}
	}

	auto& gridReq = activeScene->GetGridRequest();
	glm::vec2 gridBtnPos = { 280.0f, 5.0f };
	std::string gridText = gridReq.Active ? "SIATKA: WL" : "SIATKA: WYL";

	if (Gui::Button(gridText, gridBtnPos, { 100.0f, 20.0f }, gridReq.Active)) {
		gridReq.Active = !gridReq.Active;
	}

	// 3. Logika rozwiniêtego menu "Plik"
	if (m_ShowFileMenu) {
		// T³o rozwijanego menu
		Renderer2D::DrawQuad({ 10.0f, 30.0f }, { 150.0f, 90.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });

		// PRZYCISK ZAPISZ
		if (Gui::Button("Zapisz", { 15.0f, 35.0f }, { 140.0f, 25.0f })) {
			m_ShowSaveDialog = true; // Pokazujemy pop-up zapisu
			m_ShowFileMenu = false;  // Zamykamy listê rozwijan¹
		}

		// PRZYCISK WCZYTAJ
		if (Gui::Button("Wczytaj", { 15.0f, 65.0f }, { 140.0f, 25.0f })) {
			m_ShowLoadDialog = true; // Pokazujemy pop-up wczytywania
			m_ShowFileMenu = false;  // Zamykamy listê rozwijan¹
		}
	}

	// 4. Logika rozwiniêtego menu "Widok"
	if (m_ShowViewMenu) {
		Renderer2D::DrawQuad({ 100.0f, 30.0f }, { 160.0f, 160.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });

		if (Gui::Button("Panel Otoczenia", { 105.0f, 35.0f }, { 150.0f, 25.0f }, m_ShowEnvironmentPanel)) {
			m_ShowEnvironmentPanel = !m_ShowEnvironmentPanel;
			m_ShowViewMenu = false;
		}

		if (Gui::Button("Hierarchia", { 105.0f, 65.0f }, { 150.0f, 25.0f }, m_ShowHierarchyPanel)) {
			m_ShowHierarchyPanel = !m_ShowHierarchyPanel;
			m_ShowViewMenu = false;
		}

		if (Gui::Button("Biblioteka", { 105.0f, 95.0f }, { 150.0f, 25.0f }, m_ShowLibraryPanel)) {
			m_ShowLibraryPanel = !m_ShowLibraryPanel;
			m_ShowViewMenu = false;
		}

		if (Gui::Button("Inspektor", { 105.0f, 125.0f }, { 150.0f, 25.0f }, m_ShowInspectorPanel)) {
			m_ShowInspectorPanel = !m_ShowInspectorPanel;
			m_ShowViewMenu = false;
		}

		if (Gui::Button("Diagnostyka", { 105.0f, 155.0f }, { 150.0f, 25.0f }, m_ShowDiagnosticPanel)) {
			m_ShowDiagnosticPanel = !m_ShowDiagnosticPanel;
			m_ShowViewMenu = false;
		}
	}



	// --- PROTOTYP: GENERATOR QUESTÓW ---
	// Rysujemy t³o ma³ego panelu (np. z lewej strony, pod przyciskiem Plik/Widok)
	glm::vec2 questPanelPos = { 10.0f, 150.0f }; // Ustaw pozycjê, ¿eby nie nak³ada³o siê na inne
	Renderer2D::DrawQuad(questPanelPos, { 180.0f, 85.0f }, { 0.15f, 0.15f, 0.15f, 0.9f });
	Gui::DrawGuiText("Generator Questow:", { questPanelPos.x + 5.f, questPanelPos.y + 10.f }, 0.45f, { 1.0f, 0.8f, 0.2f, 1.0f });

	// Przycisk 1: Z tych samych newsów (Tylko ponowny rzut do LLM)
	if (Gui::Button("Generuj (Stary Cache)", { questPanelPos.x + 5.f, questPanelPos.y + 30.f }, { 170.f, 20.f })) {
		spdlog::info("Generowanie z istniejacego cache...");

		// POPRAWIONA ŒCIE¯KA:
		system("python CookingStation/Tools/QuestGenerator/main.py");
		ReloadQuests();

		spdlog::info("Gotowe! Questy czekaja w pliku wygenerowane_quests.json.");
	}

	// Przycisk 2: Pobierz nowe newsy i wygeneruj
	if (Gui::Button("Generuj (Nowe Newsy)", { questPanelPos.x + 5.f, questPanelPos.y + 55.f }, { 170.f, 20.f })) {
		spdlog::info("Czyszczenie cache i pobieranie nowych newsow...");

		// POPRAWIONA ŒCIE¯KA:
		std::remove("CookingStation/Tools/QuestGenerator/news_cache.json");
		system("python CookingStation/Tools/QuestGenerator/main.py");
		ReloadQuests();

		spdlog::info("Gotowe! Wygenerowano questy z pachnacych nowoscia newsow.");
	}
	// ------------------------------------


	if (m_ShowDiagnosticPanel) {
		// 1. AKTUALIZACJA STATYSTYK (Tylko 4 razy na sekundê!)
		m_StatsUpdateTimer += ts.GetSeconds();

		if (m_StatsUpdateTimer >= 0.25f) {
			auto stats = Renderer::GetStats();
			float fps = 1.0f / ts.GetSeconds();
			float frameTime = ts.GetMilliSeconds();

			auto formatFloat = [](float v) {
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "%.2f", v);
				return std::string(buffer);
				};

			// Nadpisujemy nasze zbuforowane stringi z GuiLayer.h
			m_FpsText = "FPS: " + std::to_string((int)fps);
			m_FrameTimeText = "Frame Time: " + formatFloat(frameTime) + " ms";
			m_CpuText = "CPU Logika: " + formatFloat(stats.CPULogicTime) + " ms";
			m_GpuText = "GPU Render: " + formatFloat(stats.GPURenderTime) + " ms";

			m_DrawCalls3DText = "Draw Calls (3D): " + std::to_string(stats.DrawCalls3D);
			m_Tris3DText = "Trojkaty (3D): " + std::to_string(stats.TriangleCount3D);
			m_DrawCallsUIText = "Draw Calls (UI): " + std::to_string(stats.DrawCallsUI);
			m_TrisUIText = "Trojkaty (UI): " + std::to_string(stats.TriangleCountUI);

			m_StatsUpdateTimer = 0.0f; // Reset timera
		}

		// 2. T£O PANELU DIAGNOSTYCZNEGO
		glm::vec2 panelPos(m_ViewportWidth - 290.0f, m_ViewportHeight - 260.0f);
		glm::vec2 panelSize(280.0f, 250.0f);
		glm::vec4 panelColor(0.12f, 0.12f, 0.12f, 0.85f);
		Renderer2D::DrawQuad(panelPos, panelSize, panelColor);

		// 3. WYPISYWANIE TEKSTÓW 
		float textX = panelPos.x + 15.0f;
		float textY = panelPos.y + 5.0f;
		float lineOffset = 25.0f;
		float scale = 0.6f;

		glm::vec4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 titleColor(0.2f, 0.8f, 0.2f, 1.0f);
		glm::vec4 highlightColor(1.0f, 0.8f, 0.2f, 1.0f);

		Gui::DrawGuiText("Diagnostyka Projektu:", { textX, textY }, scale + 0.1f, titleColor);
		textY += lineOffset + 5.0f;

		Gui::DrawGuiText(m_FpsText, { textX, textY }, scale, textColor); textY += lineOffset;
		Gui::DrawGuiText(m_FrameTimeText, { textX, textY }, scale, textColor); textY += lineOffset;
		Gui::DrawGuiText(m_CpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
		Gui::DrawGuiText(m_GpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
		Gui::DrawGuiText(m_DrawCalls3DText, { textX, textY }, scale, textColor); textY += lineOffset;
		Gui::DrawGuiText(m_Tris3DText, { textX, textY }, scale, textColor); textY += lineOffset;
		Gui::DrawGuiText(m_DrawCallsUIText, { textX, textY }, scale, textColor); textY += lineOffset;
		Gui::DrawGuiText(m_TrisUIText, { textX, textY }, scale, textColor);
	}

	if (m_ShowEnvironmentPanel) {
		// T³o panelu na dole po lewej
		Renderer2D::DrawQuad({ 10.f, m_ViewportHeight - 160.f }, { 180.f, 150.f }, { 0.15f, 0.15f, 0.15f, 0.9f });
		Gui::DrawGuiText("Kolor tla:", { 15.f, m_ViewportHeight - 145.f }, 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

		auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
		if (colorStorage && !colorStorage->dense.empty()) {
			float* r = &colorStorage->dense[0].bgColor.r;
			float* g = &colorStorage->dense[0].bgColor.g;
			float* b = &colorStorage->dense[0].bgColor.b;

			// Suwaki lub DragFloaty ustawione relatywnie do do³u ekranu
			Gui::SliderFloat("R", r, 0.0f, 1.0f, { 15.f, m_ViewportHeight - 110.f }, { 150.f, 20.f });
			Gui::SliderFloat("G", g, 0.0f, 1.0f, { 15.f, m_ViewportHeight - 75.f }, { 150.f, 20.f });
			Gui::SliderFloat("B", b, 0.0f, 1.0f, { 15.f, m_ViewportHeight - 40.f }, { 150.f, 20.f });
		}
	}


	// HIERARCHIA SCENY

	if (m_ShowHierarchyPanel) {
		// pobieramy komponent odpowiadajacy za tagi obiektow
		auto* tagStorage = world.GetComponentVector<TagComponent>();
		if (tagStorage) {

			// pozycja startowa bardziej z lewej strony ekranu
			glm::vec2 startPos = { 10.0f, 80.0f };

			// rozmiar jednej pozycji
			glm::vec2 itemSize = { 180.0f, 25.0f };

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
					activeScene->SetSelectedEntity(owner);
					spdlog::info("Gui: Wybrano encje {}", tagComp.Tag);
				}
			}
		}
	}


	// BIBLIOTEKA MODELI

	if (m_ShowLibraryPanel) {
		// Szerokie t³o na dole ekranu, pod gr¹ (szerokoœæ Viewportu, wysokoœæ 200px)
		glm::vec2 libPos = { 200.0f, m_ViewportHeight - 200.0f };
		glm::vec2 libSize = { m_ViewportWidth - 500.0f, 200.0f };
		Renderer2D::DrawQuad(libPos, libSize, { 0.14f, 0.14f, 0.14f, 0.95f });

		Gui::DrawGuiText("Zasoby:", { libPos.x + 10.0f, libPos.y + 15.0f }, 0.45f, { 1, 1, 1, 1 });

		// Ustawienie pocz¹tkowe przycisków
		float xOffset = libPos.x + 10.0f;
		float yOffset = libPos.y + 50.0f;

		for (const auto& entry : AssetManager::GetLibrary()) {
			// Rysujemy przycisk
			if (Gui::Button(entry.Name, { xOffset, yOffset }, { 120, 30 })) {
				auto& request = activeScene->GetPlacementRequest();
				request.Name = entry.Name;
				request.Path = entry.Path;
				request.Active = true;
			}

			// Przesuwamy siê w prawo o szerokoœæ przycisku + margines
			xOffset += 130.0f;

			// Zawijanie wierszy (Jeœli wyjdziemy poza praw¹ krawêdŸ panelu, schodzimy ni¿ej)
			if (xOffset + 120.0f > libPos.x + libSize.x) {
				xOffset = libPos.x + 10.0f;
				yOffset += 40.0f;
			}
		}
	}


	// INSPEKTOR ENCJI

	if (m_ShowInspectorPanel) {
		// pobieramy wybran¹ encjê ze sceny (przeslana przez EditorLayer)
		Entity selected = activeScene->GetSelectedEntity();
		if (selected.id != std::numeric_limits<std::size_t>::max()) {
			glm::vec2 inspPos = { m_ViewportWidth - 280.0f, 70.0f };

			auto* tag = world.GetComponent<TagComponent>(selected);
			if (tag) {
				Gui::InputGuiText("Nazwa", tag->Tag, { inspPos.x, inspPos.y }, { 260.0f, 25.0f });
			}

			// wyswietlamy komponenty od transformacji jako slidery, zeby dalo sie modyfikowac
			auto* transform = world.GetComponent<TransformComponent>(selected);
			if (transform) {

				glm::vec3 posBeforeSliders = transform->Position;

				float dragSpeed = 0.05f;
				float rotSpeed = 0.5f;

				Gui::DragFloat("Pos X", &transform->Position.x, dragSpeed, { inspPos.x, inspPos.y + 40.0f }, { 300, 30 });
				Gui::DragFloat("Pos Y", &transform->Position.y, dragSpeed, { inspPos.x, inspPos.y + 80.0f }, { 300, 30 });

				Gui::DragFloat("Rot X", &transform->Rotation.x, rotSpeed, { inspPos.x, inspPos.y + 120.0f }, { 300, 30 });
				Gui::DragFloat("Rot Y", &transform->Rotation.y, rotSpeed, { inspPos.x, inspPos.y + 160.0f }, { 300, 30 });
				Gui::DragFloat("Rot Z", &transform->Rotation.z, rotSpeed, { inspPos.x, inspPos.y + 200.0f }, { 300, 30 });

				Gui::DragFloat("Skala X", &transform->Scale.x, dragSpeed, { inspPos.x, inspPos.y + 240.0f }, { 300, 30 });
				Gui::DragFloat("Skala Y", &transform->Scale.y, dragSpeed, { inspPos.x, inspPos.y + 280.0f }, { 300, 30 });
				Gui::DragFloat("Skala Z", &transform->Scale.z, dragSpeed, { inspPos.x, inspPos.y + 320.0f }, { 300, 30 });

				// LOGIKA WYKRYWANIA RUCHU
				if (posBeforeSliders != transform->Position && !m_IsDraggingTransform) {
					m_IsDraggingTransform = true;
					m_TransformStartPos = posBeforeSliders; // Zapisujemy pozycjê startow¹
				}

				// JEŒLI PUŒCILIŒMY KLAWISZ MYSZY - WYSY£AMY EVENT
				if (m_IsDraggingTransform && !Input::IsMouseButtonPressed(0)) {
					EntityTransformChangedEvent e(selected, m_TransformStartPos, transform->Position);

					// Strzelamy eventem w g³ówn¹ szynê aplikacji
					Application::Get().OnEvent(e);

					m_IsDraggingTransform = false; // reset
				}

			}

			// przycisk od usuwania obiektu
			if (Gui::Button("USUN OBIEKT", { inspPos.x, inspPos.y + 360.0f }, { 300.0f, 40.0f })) {
				// Wysy³amy event zamiast niszczyæ œwiat
				EntityDeletedEvent e(selected);
				Application::Get().OnEvent(e);

				// mo¿emy tutaj odznaczyæ encjê w GUI, 
				// ¿eby inspektor nie próbowa³ rysowaæ skasowanych danych
				activeScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
			}
		}
	}

	// --- OKNO ZAPISU (SAVE DIALOG) ---
	if (m_ShowSaveDialog) {
		// T³o okienka wyœrodkowane na ekranie
		glm::vec2 dialogSize = { 350.0f, 150.0f };
		glm::vec2 dialogPos = { (m_ViewportWidth - dialogSize.x) / 2.0f, (m_ViewportHeight - dialogSize.y) / 2.0f };

		Renderer2D::DrawQuad(dialogPos, dialogSize, { 0.2f, 0.2f, 0.25f, 1.0f });
		Gui::DrawGuiText("Zapisz scene jako:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

		// Twoje pole tekstowe do wpisywania nazwy
		Gui::InputGuiText("Nazwa", m_SaveFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

		// Przycisk PotwierdŸ
		if (Gui::Button("Zapisz plik", { dialogPos.x + 10.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
			std::string path = "CookingStation/Assets/saves/" + m_SaveFileName + ".json";
			SceneSerializer serializer(activeScene.get());
			serializer.Serialize(path);

			m_ShowSaveDialog = false; // Zamykamy okienko po udanym zapisie
		}

		// Przycisk Anuluj
		if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
			m_ShowSaveDialog = false;
		}
	}


	// --- OKNO WCZYTYWANIA (LOAD DIALOG) ---
	if (m_ShowLoadDialog) {
		// T³o okienka wyœrodkowane na ekranie
		glm::vec2 dialogSize = { 350.0f, 150.0f };
		glm::vec2 dialogPos = { (m_ViewportWidth - dialogSize.x) / 2.0f, (m_ViewportHeight - dialogSize.y) / 2.0f };

		Renderer2D::DrawQuad(dialogPos, dialogSize, { 0.25f, 0.2f, 0.2f, 1.0f });
		Gui::DrawGuiText("Wczytaj scene:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

		Gui::InputGuiText("Nazwa", m_LoadFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

		if (Gui::Button("Wczytaj plik", { dialogPos.x + 10.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
			std::string path = "CookingStation/Assets/saves/" + m_LoadFileName + ".json";

			// MAGIA SCENE MANAGERA:
			// Tworzymy now¹, czyst¹ scenê i nadpisujemy star¹
			std::shared_ptr<Scene> newScene = SceneManager::NewScene();

			// Wczytujemy do niej dane
			SceneSerializer serializer(newScene.get());
			serializer.Deserialize(path);

			spdlog::info("Wczytano scene z: {}", path);
			m_ShowLoadDialog = false;
		}

		if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
			m_ShowLoadDialog = false;
		}
	}

	if (!m_CurrentQuests.empty()) {
		float startY = 100.0f; // odleglosc od gory ekranu
		float startX = m_ViewportWidth - 350.0f; // odleglosc od lewej krawedzi (zeby bylo po prawej)

		// Opcjonalne t³o dla czytelnoœci
		Renderer2D::DrawQuad({ startX - 10.f, startY - 20.f }, { 340.f, m_CurrentQuests.size() * 50.0f + 20.0f }, { 0.1f, 0.1f, 0.1f, 0.8f });

		Gui::DrawGuiText("ZADANIA:", { startX, startY - 15.f }, 0.5f, { 1.0f, 0.5f, 0.2f, 1.0f });

		for (const auto& q : m_CurrentQuests) {
			// Tytu³ na ¿ó³to
			Gui::DrawGuiText(q.title, { startX, startY }, 0.45f, { 1.0f, 0.8f, 0.2f, 1.0f });
			// Opis na bia³o, trochê mniejszy
			Gui::DrawGuiText(q.desc, { startX, startY + 20.0f }, 0.35f, { 1.0f, 1.0f, 1.0f, 1.0f });

			startY += 50.0f; // Przesuwamy w dó³ dla kolejnego questa
		}
	}

	Renderer2D::EndScene();
	glEnable(GL_DEPTH_TEST);
	Gui::EndFrame();
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

void GuiLayer::ReloadQuests() {
	m_CurrentQuests.clear();
	std::ifstream file("CookingStation/Assets/wygenerowane_quests.json");
	if (file.is_open()) {
		try {
			nlohmann::json data = nlohmann::json::parse(file);
			for (auto& q : data) {
				m_CurrentQuests.push_back({ q["title"].get<std::string>(), q["description"].get<std::string>() });
			}
			spdlog::info("Questy zaladowane do interfejsu!");
		}
		catch (...) {
			spdlog::error("Blad parsowania JSONa questow.");
		}
	}
}


GuiLayer::~GuiLayer() {}


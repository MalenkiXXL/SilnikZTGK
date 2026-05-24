#include "EditorGuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Core/Application.h"
#include "GameGuiLayer.h"
#include <cstdlib> 
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <limits>
#include "CookingStation/Scripts/PotScript.h"

#include "CookingStation/Core/VFS/VFS.h"
#include <spdlog/spdlog.h>

namespace {
    enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

    glm::vec2 GetAnchoredPosition(Anchor anchor, float offsetX, float offsetY, float width, float height, float screenWidth, float screenHeight) {
        switch (anchor) {
        case Anchor::TopLeft:     return { offsetX, offsetY };
        case Anchor::TopRight:    return { screenWidth - width - offsetX, offsetY };
        case Anchor::BottomLeft:  return { offsetX, screenHeight - height - offsetY };
        case Anchor::BottomRight: return { screenWidth - width - offsetX, screenHeight - height - offsetY };
        case Anchor::Center:      return { (screenWidth - width) / 2.0f + offsetX, (screenHeight - height) / 2.0f + offsetY };
        }
        return { offsetX, offsetY };
    }
}

void EditorGuiLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    Gui::Init("assets://fonts/ARIAL.TTF", 32);
}

void EditorGuiLayer::OnUpdate(Timestep ts) {
    Gui::BeginFrame();
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (!activeScene) {
        spdlog::error("AssetLayer: Brak aktywnej sceny!");
        return;
    }

    auto& world = activeScene->GetWorld();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    if (m_ViewportFBO) {
        glm::vec2 viewportPos = { 200.0f, 30.0f };
        glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

        if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {

            //Informacja o rozmiarze okna gry potrzebna do wyliczania ruchu kamery
            activeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

            if (m_ViewportFBO->GetSpecification().Width != (uint32_t)viewportSize.x ||
                m_ViewportFBO->GetSpecification().Height != (uint32_t)viewportSize.y)
            {
                m_ViewportFBO->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
                if (m_MsaaFBO) {
                    m_MsaaFBO->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
                }
            }

            uint32_t textureID = m_ViewportFBO->GetColorAttachmentRendererID();
            Renderer2D::DrawQuad(viewportPos, viewportSize, textureID, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
    }

    // --- GŁÓWNY PASEK ZADAŃ ---
    Gui::Panel({ 0.0f, 0.0f }, { m_ViewportWidth, 30.0f }, { 0.15f, 0.15f, 0.15f, 1.0f }, 15.0f);

    if (Gui::Button("Plik", { 10.0f, 5.0f }, { 80.0f, 20.0f })) {
        m_ShowFileMenu = !m_ShowFileMenu;
        m_ShowViewMenu = false;
    }
    if (Gui::Button("Widok", { 100.0f, 5.0f }, { 80.0f, 20.0f })) {
        m_ShowViewMenu = !m_ShowViewMenu;
        m_ShowFileMenu = false;
    }

    glm::vec2 playButtonPos = { 190.0f, 5.0f };
    glm::vec2 playButtonSize = { 80.0f, 20.0f };

    if (activeScene->GetState() == SceneState::Edit) {
        if (Gui::Button("PLAY", playButtonPos, playButtonSize)) {
            ScenePlayEvent e;
            Application::Get().OnEvent(e);
        }
    }
    else {
        if (Gui::Button("STOP", playButtonPos, playButtonSize)) {
            SceneStopEvent e;
            Application::Get().OnEvent(e);
        }
    }

    auto& gridReq = activeScene->GetGridRequest();
    std::string gridText = gridReq.Active ? "SIATKA: WL" : "SIATKA: WYL";
    if (Gui::Button(gridText, { 280.0f, 5.0f }, { 100.0f, 20.0f }, gridReq.Active)) {
        gridReq.Active = !gridReq.Active;
    }

    if (m_ShowFileMenu) {
        Gui::Panel({ 10.0f, 30.0f }, { 150.0f, 120.0f }, { 0.2f, 0.2f, 0.2f, 0.9f }, 15.0f);

        if (Gui::Button("Zapisz", { 15.0f, 35.0f }, { 140.0f, 25.0f })) {
            m_ShowSaveDialog = true;
            m_ShowFileMenu = false;
        }
        if (Gui::Button("Wczytaj", { 15.0f, 65.0f }, { 140.0f, 25.0f })) {
            m_ShowLoadDialog = true;
            m_ShowFileMenu = false;
        }

        if (Gui::Button("Eksportuj", { 15.0f, 95.0f }, { 140.0f, 25.0f })) {
            spdlog::info("[BuildTool] Rozpoczynam automatyczny proces eksportu...");

            namespace fs = std::filesystem;
            std::error_code ec;

            // Zapisujemy oryginalną ścieżkę PRZED jakimkolwiek current_path()
            fs::path originalPath = fs::current_path();

            // Absolutne ścieżki do zasobów — niezależne od current_path
            fs::path absAssetsPath = originalPath / "CookingStation/Assets";
            fs::path absShadersPath = originalPath / "CookingStation/Shaders";
            fs::path absExportDir = originalPath / "Builds/CookingStation_Dystrybucja";

            // Szukamy .sln idąc w górę drzewa katalogów
            fs::path rootDir = originalPath;
            bool foundSln = false;
            std::string slnFilename = "";

            spdlog::info("[BuildTool] Szukam .sln od: {}", originalPath.string());

            for (int i = 0; i < 6; ++i) {
                if (fs::is_directory(rootDir, ec)) {
                    for (const auto& entry : fs::directory_iterator(rootDir, ec)) {
                        if (entry.path().extension() == ".sln") {
                            slnFilename = entry.path().filename().string();
                            foundSln = true;
                            break;
                        }
                    }
                }
                if (foundSln) break;
                if (rootDir.has_parent_path() && rootDir != rootDir.parent_path())
                    rootDir = rootDir.parent_path();
                else
                    break;
            }

            if (!foundSln) {
                spdlog::error("[BuildTool] Nie znaleziono pliku .sln!");
                m_ShowFileMenu = false;
            }
            else {
                fs::current_path(rootDir);
                spdlog::info("[BuildTool] Znaleziono: {} w {}", slnFilename, rootDir.string());

                // Szukamy vcvars64.bat
                std::string vcvarsScript = "";
                std::vector<std::string> suspectedVCVarsPaths = {
                    "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"",
                    "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat\"",
                    "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat\""
                };
                for (const auto& rawPath : suspectedVCVarsPaths) {
                    std::string cleanPath = rawPath;
                    cleanPath.erase(std::remove(cleanPath.begin(), cleanPath.end(), '\"'), cleanPath.end());
                    if (fs::exists(cleanPath)) { vcvarsScript = rawPath; break; }
                }

                std::string fullCommand;
                if (!vcvarsScript.empty()) {
                    fullCommand = "call " + vcvarsScript + " && msbuild " + slnFilename + " /p:Configuration=Distribution /p:Platform=x64 /t:Rebuild > msbuild_log.txt 2>&1";
                    spdlog::info("[BuildTool] Inicjalizuję środowisko MSVC...");
                }
                else {
                    fullCommand = "msbuild " + slnFilename + " /p:Configuration=Distribution /p:Platform=x64 /t:Rebuild > msbuild_log.txt 2>&1";
                    spdlog::warn("[BuildTool] Nie znaleziono vcvars64.bat, próba uproszczona.");
                }

                spdlog::info("[BuildTool] Uruchamiam kompilację...");
                int compileResult = std::system(fullCommand.c_str());

                // Przywracamy ścieżkę ZARAZ po kompilacji, przed operacjami na plikach
                fs::current_path(originalPath);

                if (compileResult != 0) {
                    spdlog::error("[BuildTool] Błąd kompilacji! Kod: {}", compileResult);
                    std::system("notepad msbuild_log.txt");
                }
                else {
                    spdlog::info("[BuildTool] Kompilacja OK! Generuję paczkę...");

                    try {
                        fs::remove_all(absExportDir);
                        fs::create_directories(absExportDir);

                     
                        std::string foundExePath = "";
                        for (const auto& entry : fs::recursive_directory_iterator(rootDir, ec)) {
                            if (ec) break;
                            std::string pathStr = entry.path().string();
                            if (entry.path().extension() == ".exe" &&
                                pathStr.find("Distribution") != std::string::npos) {
                                foundExePath = pathStr;
                                spdlog::info("[BuildTool] Znaleziono .exe: {}", foundExePath);
                                break;
                            }
                        }

                        if (foundExePath.empty()) {
                            spdlog::error("[BuildTool] Nie znaleziono .exe z 'Distribution' w ścieżce!");
                            spdlog::error("[BuildTool] Sprawdź Output Directory w ustawieniach projektu VS.");
                        }
                        else {
                            // Kopiuj .exe
                            fs::copy_file(foundExePath,
                                absExportDir / "CookingStation.exe",
                                fs::copy_options::overwrite_existing);

                            // Kopiuj .dll z tego samego folderu co .exe
                            fs::path exeDir = fs::path(foundExePath).parent_path();
                            for (const auto& entry : fs::directory_iterator(exeDir, ec)) {
                                if (entry.path().extension() == ".dll") {
                                    fs::copy_file(entry.path(),
                                        absExportDir / entry.path().filename(),
                                        fs::copy_options::overwrite_existing);
                                }
                            }
                            spdlog::info("[BuildTool] Skopiowano .exe i .dll.");
                        }

                        // --- ZAMIAST FS::COPY DLA ASSETS, TWORZYMY PLIK DATA.PAK ---
                        if (fs::exists(absAssetsPath)) {
                            spdlog::info("[BuildTool] Pakowanie zasobow do data.pak...");

                            fs::path pakFilePath = absExportDir / "data.pak";
                            std::ofstream pakFile(pakFilePath, std::ios::binary);

                            if (pakFile.is_open()) {
                                struct FileInfo { std::string relativePath; std::string fullPath; uint64_t size; };
                                std::vector<FileInfo> filesToPack;

                                // 1. Zbieramy wszystkie pliki i odrzucamy pliki robocze
                                for (const auto& entry : fs::recursive_directory_iterator(absAssetsPath)) {
                                    if (entry.is_regular_file()) {
                                        std::string ext = entry.path().extension().string();
                                        // Ignorujemy surowe projekty (możesz dopisać .psd, .xcf itp.)
                                        if (ext == ".blend" || ext == ".blend1") continue;

                                        FileInfo info;
                                        info.fullPath = entry.path().string();
                                        // Używamy generic_string() żeby wymusić '/' zamiast '\\' z Windowsa
                                        info.relativePath = fs::relative(entry.path(), absAssetsPath).generic_string();
                                        info.size = fs::file_size(entry.path());
                                        filesToPack.push_back(info);
                                    }
                                }

                                // 2. Zapisujemy ilość plików na samym początku paka
                                uint32_t numFiles = static_cast<uint32_t>(filesToPack.size());
                                pakFile.write(reinterpret_cast<const char*>(&numFiles), sizeof(uint32_t));

                                // Obliczamy, gdzie (w bajtach) zaczną się surowe dane plików (za nagłówkiem)
                                uint64_t currentDataOffset = sizeof(uint32_t); // Rozmiar numFiles
                                for (const auto& f : filesToPack) {
                                    currentDataOffset += sizeof(uint32_t) + f.relativePath.size() + sizeof(uint64_t) + sizeof(uint64_t);
                                }

                                // 3. Zapisujemy "Spis Treści" (TOC - Table of Contents)
                                for (auto& f : filesToPack) {
                                    uint32_t pathLen = static_cast<uint32_t>(f.relativePath.size());
                                    pakFile.write(reinterpret_cast<const char*>(&pathLen), sizeof(uint32_t));
                                    pakFile.write(f.relativePath.c_str(), pathLen);

                                    uint64_t offset = currentDataOffset;
                                    uint64_t size = f.size;
                                    pakFile.write(reinterpret_cast<const char*>(&offset), sizeof(uint64_t));
                                    pakFile.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));

                                    currentDataOffset += size; // Przesuwamy wskaźnik dla następnego pliku
                                }

                                // 4. Zapisujemy surowe bajty plików
                                for (const auto& f : filesToPack) {
                                    std::ifstream inFile(f.fullPath, std::ios::binary);
                                    if (inFile.is_open()) {
                                        std::vector<char> buffer(f.size);
                                        inFile.read(buffer.data(), f.size);
                                        pakFile.write(buffer.data(), f.size);
                                    }
                                    else {
                                        spdlog::error("[BuildTool] Nie udalo sie otworzyc do pakowania: {}", f.fullPath);
                                    }
                                }
                                pakFile.close();
                                spdlog::info("[BuildTool] Sukces! Utworzono data.pak");
                            }
                        }
                        else {
                            spdlog::error("[BuildTool] Nie znaleziono Assets pod: {}", absAssetsPath.string());
                        }

                        if (fs::exists(absShadersPath)) {
                            spdlog::info("[BuildTool] Pakowanie shaderow do shaders.pak...");

                            fs::path pakFilePath = absExportDir / "shaders.pak";
                            std::ofstream pakFile(pakFilePath, std::ios::binary);

                            if (pakFile.is_open()) {
                                struct FileInfo { std::string relativePath; std::string fullPath; uint64_t size; };
                                std::vector<FileInfo> filesToPack;

                                // 1. Zbieramy wszystkie shadery
                                for (const auto& entry : fs::recursive_directory_iterator(absShadersPath)) {
                                    if (entry.is_regular_file()) {
                                        FileInfo info;
                                        info.fullPath = entry.path().string();
                                        info.relativePath = fs::relative(entry.path(), absShadersPath).generic_string();
                                        info.size = fs::file_size(entry.path());
                                        filesToPack.push_back(info);
                                    }
                                }

                                // 2. Nagłówek: liczba plików
                                uint32_t numFiles = static_cast<uint32_t>(filesToPack.size());
                                pakFile.write(reinterpret_cast<const char*>(&numFiles), sizeof(uint32_t));

                                // 3. Obliczamy offsety dla TOC
                                uint64_t currentDataOffset = sizeof(uint32_t);
                                for (const auto& f : filesToPack) {
                                    currentDataOffset += sizeof(uint32_t) + f.relativePath.size() + sizeof(uint64_t) + sizeof(uint64_t);
                                }

                                // 4. Zapisujemy Spis Treści (TOC)
                                for (auto& f : filesToPack) {
                                    uint32_t pathLen = static_cast<uint32_t>(f.relativePath.size());
                                    pakFile.write(reinterpret_cast<const char*>(&pathLen), sizeof(uint32_t));
                                    pakFile.write(f.relativePath.c_str(), pathLen);

                                    uint64_t offset = currentDataOffset;
                                    uint64_t size = f.size;
                                    pakFile.write(reinterpret_cast<const char*>(&offset), sizeof(uint64_t));
                                    pakFile.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));

                                    currentDataOffset += size;
                                }

                                // 5. Zapisujemy dane shaderów
                                for (const auto& f : filesToPack) {
                                    std::ifstream inFile(f.fullPath, std::ios::binary);
                                    if (inFile.is_open()) {
                                        std::vector<char> buffer(f.size);
                                        inFile.read(buffer.data(), f.size);
                                        pakFile.write(buffer.data(), f.size);
                                    }
                                }
                                pakFile.close();
                                spdlog::info("[BuildTool] Sukces! Utworzono shaders.pak");
                            }
                        }
                        else {
                            spdlog::error("[BuildTool] Nie znaleziono Shaders pod: {}", absShadersPath.string());
                        }

                        spdlog::info("[BuildTool] Paczka gotowa w: {}", absExportDir.string());
                        std::system(("explorer \"" + absExportDir.string() + "\"").c_str());
                    }
                    catch (const fs::filesystem_error& e) {
                        spdlog::error("[BuildTool] Błąd operacji plikowych: {}", e.what());
                    }
                }

                m_ShowFileMenu = false;
            }
        }
    }

    // --- PANEL GENERATORA QUESTÓW AI ---
    if (m_ShowQuestsPanel) {
        glm::vec2 questPanelPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 400.0f, 180.0f, 85.0f, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(questPanelPos, { 180.0f, 85.0f }, { 0.15f, 0.15f, 0.15f, 0.9f }, 15.0f);
        Gui::DrawGuiText("Generator Questow:", { questPanelPos.x + 5.f, questPanelPos.y + 10.f }, 0.45f, { 1.0f, 0.8f, 0.2f, 1.0f });

        if (Gui::Button("Generuj (Stary Cache)", { questPanelPos.x + 5.f, questPanelPos.y + 30.f }, { 170.f, 20.f })) {
            spdlog::info("Generowanie z istniejacego cache...");
            system("python CookingStation/Tools/QuestGenerator/main.py");
            GameGuiLayer::s_NeedsQuestReload = true;
        }
        if (Gui::Button("Generuj (Nowe Newsy)", { questPanelPos.x + 5.f, questPanelPos.y + 55.f }, { 170.f, 20.f })) {
            spdlog::info("Czyszczenie cache i pobieranie nowych newsow...");
            std::remove("CookingStation/Assets/news_cache.json");
            system("python CookingStation/Tools/QuestGenerator/main.py");
            GameGuiLayer::s_NeedsQuestReload = true;
        }
    }

    // --- PANEL DIAGNOSTYCZNY (ANCHOR: BOTTOM RIGHT) ---
    if (m_ShowDiagnosticPanel) {
        m_StatsUpdateTimer += ts.GetSeconds();
        if (m_StatsUpdateTimer >= 0.25f) {
            auto stats = Renderer::GetStats();
            float fps = 1.0f / ts.GetSeconds();
            float frameTime = ts.GetMilliSeconds();
            auto formatFloat = [](float v) {
                char buffer[32]; snprintf(buffer, sizeof(buffer), "%.2f", v); return std::string(buffer);
                };

            m_FpsText = "FPS: " + std::to_string((int)fps);
            m_FrameTimeText = "Frame Time: " + formatFloat(frameTime) + " ms";
            m_CpuText = "CPU Logika: " + formatFloat(stats.CPULogicTime) + " ms";
            m_GpuText = "GPU Render: " + formatFloat(stats.GPURenderTime) + " ms";
            m_DrawCalls3DText = "Draw Calls (3D): " + std::to_string(stats.DrawCalls3D);
            m_InstanceBatchesText = "Instanced Batches: " + std::to_string(stats.InstanceBatches);
            m_MatrixCalcText = "CPU Matrix Calcs: " + std::to_string(stats.MatrixCalculations);

            float totalMatrix = (float)(stats.MatrixCalculations + stats.SkippedCalculations);
            float savings = totalMatrix > 0 ? ((float)stats.SkippedCalculations / totalMatrix) * 100.0f : 0.0f;
            m_CpuSavingsText = "CPU Savings: " + formatFloat(savings) + "%";

            m_Tris3DText = "Trojkaty (3D): " + std::to_string(stats.TriangleCount3D);
            m_Culled3DText = "Odrzucone (Culled): " + std::to_string(stats.CulledObjects3D);
            m_DrawCallsUIText = "Draw Calls (UI): " + std::to_string(stats.DrawCallsUI);
            m_TrisUIText = "Trojkaty (UI): " + std::to_string(stats.TriangleCountUI);
            m_StatsUpdateTimer = 0.0f;
        }

        glm::vec2 panelSize(300.0f, 350.0f);
        glm::vec2 panelPos = GetAnchoredPosition(Anchor::BottomRight, 0.0f, 10.0f, panelSize.x, panelSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(panelPos, panelSize, { 0.12f, 0.12f, 0.12f, 0.85f }, 15.0f);

        float textX = panelPos.x + 15.0f; float textY = panelPos.y + 5.0f; float lineOffset = 25.0f; float scale = 0.6f;
        glm::vec4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 highlightColor(1.0f, 0.8f, 0.2f, 1.0f);
        glm::vec4 optColor(0.2f, 0.9f, 0.8f, 1.0f);

        Gui::DrawGuiText("Diagnostyka Projektu:", { textX, textY }, scale + 0.1f, { 0.2f, 0.8f, 0.2f, 1.0f }); textY += lineOffset + 5.0f;
        Gui::DrawGuiText(m_FpsText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_FrameTimeText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_CpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
        Gui::DrawGuiText(m_GpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
        Gui::DrawGuiText(m_DrawCalls3DText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_InstanceBatchesText, { textX, textY }, scale, optColor);     textY += lineOffset;
        Gui::DrawGuiText(m_MatrixCalcText, { textX, textY }, scale, optColor);      textY += lineOffset;
        Gui::DrawGuiText(m_CpuSavingsText, { textX, textY }, scale, optColor);      textY += lineOffset;
        Gui::DrawGuiText(m_Tris3DText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_Culled3DText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_DrawCallsUIText, { textX, textY }, scale, textColor);     textY += lineOffset;
        Gui::DrawGuiText(m_TrisUIText, { textX, textY }, scale, textColor);
    }

    // --- PANEL OTOCZENIA (ANCHOR: BOTTOM LEFT) ---
    if (m_ShowEnvironmentPanel) {
        glm::vec2 envSize = { 180.f, 350.f };
        glm::vec2 envPos = GetAnchoredPosition(Anchor::BottomLeft, 10.f, 10.f, envSize.x, envSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(envPos, envSize, { 0.15f, 0.15f, 0.15f, 0.9f }, 15.0f);
        Gui::DrawGuiText("Kolor tla:", { envPos.x + 5.f, envPos.y + 12.f }, 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

        auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
        if (colorStorage && !colorStorage->dense.empty()) {
            float* r = &colorStorage->dense[0].bgColor.r;
            float* g = &colorStorage->dense[0].bgColor.g;
            float* b = &colorStorage->dense[0].bgColor.b;
            Gui::SliderFloat("R", r, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 40.f }, { 150.f, 20.f });
            Gui::SliderFloat("G", g, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 75.f }, { 150.f, 20.f });
            Gui::SliderFloat("B", b, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 110.f }, { 150.f, 20.f });
        }

        Gui::DrawGuiText("Model Oswietlenia:", { envPos.x + 5.f, envPos.y + 145.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
        if (Gui::Button("Standard", { envPos.x + 5.f,  envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Standard")))   Renderer::ActiveShader = "Standard";
        if (Gui::Button("RAMP", { envPos.x + 90.f, envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "RAMP")))       Renderer::ActiveShader = "RAMP";
        if (Gui::Button("Fake BRDF", { envPos.x + 5.f,  envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "FakeBRDF")))   Renderer::ActiveShader = "FakeBRDF";
        if (Gui::Button("Blinn-Phong", { envPos.x + 90.f, envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "BlinnPhong"))) Renderer::ActiveShader = "BlinnPhong";
        if (Gui::Button("Rim", { envPos.x + 5.f,  envPos.y + 225.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Rim")))        Renderer::ActiveShader = "Rim";

        Gui::DrawGuiText("Wygładzanie MSAA:", { envPos.x + 5.f, envPos.y + 255.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
        uint32_t currentMsaa = Application::Get().GetMsaaSamples();
        if (Gui::Button("Wył", { envPos.x + 5.f,   envPos.y + 275.f }, { 38.f, 20.f }, currentMsaa == 1))  Application::Get().SetMsaaSamples(1);
        if (Gui::Button("2x", { envPos.x + 48.f,  envPos.y + 275.f }, { 38.f, 20.f }, currentMsaa == 2))  Application::Get().SetMsaaSamples(2);
        if (Gui::Button("4x", { envPos.x + 91.f,  envPos.y + 275.f }, { 38.f, 20.f }, currentMsaa == 4))  Application::Get().SetMsaaSamples(4);
        if (Gui::Button("8x", { envPos.x + 134.f, envPos.y + 275.f }, { 38.f, 20.f }, currentMsaa == 8))  Application::Get().SetMsaaSamples(8);
        if (Gui::Button("16x", { envPos.x + 5.f,   envPos.y + 305.f }, { 32.f, 20.f }, currentMsaa == 16)) Application::Get().SetMsaaSamples(16);
    }

    // --- HIERARCHIA SCENY (ANCHOR: TOP LEFT) ---
    if (m_ShowHierarchyPanel) {
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        if (tagStorage) {
            glm::vec2 panelSize = { 180.0f, 300.0f };
            glm::vec2 startPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, panelSize.x, panelSize.y, m_ViewportWidth, m_ViewportHeight);

            Gui::Panel(startPos, panelSize, { 0.12f, 0.12f, 0.12f, 0.8f }, 15.0f);
            Gui::DrawGuiText("Hierarchia sceny:", { startPos.x + 5.0f, startPos.y + 15.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

            Renderer2D::EndScene();
            glEnable(GL_SCISSOR_TEST);
            int scissorY = (int)(m_ViewportHeight - (startPos.y + panelSize.y));
            glScissor((int)startPos.x, scissorY, (int)panelSize.x, (int)panelSize.y);
            Renderer2D::BeginScene(uiProj);

            for (size_t i = 0; i < tagStorage->dense.size(); i++) {
                glm::vec2 currentItemPos = { startPos.x, startPos.y + 35.0f + (i * 30.0f) - m_HierarchyScrollY };
                auto& tagComp = tagStorage->dense[i];
                Entity owner = tagStorage->reverse[i];
                if (Gui::Button(tagComp.Tag, currentItemPos, { 180.0f, 25.0f }))
                    activeScene->SetSelectedEntity(owner);
            }

            Renderer2D::EndScene();
            glDisable(GL_SCISSOR_TEST);
            Renderer2D::BeginScene(uiProj);
        }
    }

    // --- BIBLIOTEKA MODELI ---
    if (m_ShowLibraryPanel) {
        glm::vec2 libSize = { m_ViewportWidth - 500.0f, 200.0f };
        glm::vec2 libPos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, libSize.x, libSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(libPos, libSize, { 0.14f, 0.14f, 0.14f, 0.95f }, 15.0f);
        Gui::DrawGuiText("Zasoby:", { libPos.x + 10.0f, libPos.y + 15.0f }, 0.45f, { 1, 1, 1, 1 });

        Renderer2D::EndScene();
        glEnable(GL_SCISSOR_TEST);
        int scissorY = (int)(m_ViewportHeight - (libPos.y + libSize.y));
        glScissor((int)libPos.x, scissorY, (int)libSize.x, (int)libSize.y);
        Renderer2D::BeginScene(uiProj);

        float xOffset = libPos.x + 10.0f;
        float yOffset = libPos.y + 40.0f - m_LibraryScrollY;

        for (const auto& entry : AssetManager::GetLibrary()) {
            if (Gui::Button(entry.Name, { xOffset, yOffset }, { 120, 30 })) {
                auto& request = activeScene->GetPlacementRequest();
                request.Name = entry.Name; request.Path = entry.Path; request.Active = true;
            }
            xOffset += 130.0f;
            if (xOffset + 120.0f > libPos.x + libSize.x) {
                xOffset = libPos.x + 10.0f;
                yOffset += 40.0f;
            }
        }

        Renderer2D::EndScene();
        glDisable(GL_SCISSOR_TEST);
        Renderer2D::BeginScene(uiProj);
    }

    // --- PANEL PREFABÓW ---
    if (m_ShowPrefabsPanel) {
        glm::vec2 prefSize = { m_ViewportWidth - 500.0f, 120.0f };
        glm::vec2 prefPos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, prefSize.x, prefSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(prefPos, prefSize, { 0.15f, 0.25f, 0.3f, 0.95f }, 15.0f);
        Gui::DrawGuiText("Gotowe Prefaby:", { prefPos.x + 10.0f, prefPos.y + 15.0f }, 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

        float xOffset = prefPos.x + 10.0f;
        float yOffset = prefPos.y + 40.0f;

        if (std::filesystem::exists("CookingStation/Assets/prefabs")) {
            for (const auto& entry : std::filesystem::directory_iterator("CookingStation/Assets/prefabs")) {
                if (entry.path().extension() == ".json") {
                    std::string prefabName = entry.path().stem().string();
                    std::string virtualPrefabPath = "assets://prefabs/" + prefabName + ".json";

                    if (Gui::Button(prefabName, { xOffset, yOffset }, { 120, 30 })) {
                        auto& request = activeScene->GetPlacementRequest();
                        request.Name = prefabName;
                        request.Path = virtualPrefabPath;
                        request.Active = true;
                    }
                    xOffset += 130.0f;
                    if (xOffset + 120.0f > prefPos.x + prefSize.x) {
                        xOffset = prefPos.x + 10.0f;
                        yOffset += 40.0f;
                    }
                }
            }
        }
    }

    // --- INSPEKTOR ENCJI (ANCHOR: TOP RIGHT) ---
    if (m_ShowInspectorPanel) {
        Entity selected = activeScene->GetSelectedEntity();
        if (selected.id != std::numeric_limits<std::size_t>::max()) {
            glm::vec2 inspSize = { 300.0f, 750.0f };
            glm::vec2 inspPos = GetAnchoredPosition(Anchor::TopRight, 0.0f, 70.0f, inspSize.x, inspSize.y, m_ViewportWidth, m_ViewportHeight);

            Gui::Panel(inspPos, inspSize, { 0.15f, 0.15f, 0.15f, 0.95f }, 15.0f);

            float currentY = inspPos.y + 10.0f;
            float padX = inspPos.x + 5.0f;
            float elementW = 290.0f;

            auto* tag = world.GetComponent<TagComponent>(selected);
            if (tag) {
                Gui::InputGuiText("Nazwa", tag->Tag, { padX, currentY }, { elementW, 25.0f });
                currentY += 35.0f;
            }

            // --- NAWIGACJA HIERARCHII (RODZIC / DZIECI) ---
            auto* rel = world.GetComponent<RelationshipComponent>(selected);
            if (rel)
            {
                // 1. PRZYCISK POWROTU DO RODZICA
                if (rel->Parent != std::numeric_limits<std::size_t>::max())
                {
                    if (Gui::Button("<- Wroc do rodzica", { padX, currentY }, { elementW, 25.0f }))
                    {
                        Entity parentEntity = { rel->Parent, 0 };
                        activeScene->SetSelectedEntity(parentEntity);
                    }
                    currentY += 30.0f;
                }

                // 2. LISTA DZIECI
                if (rel->FirstChild != std::numeric_limits<std::size_t>::max())
                {
                    Gui::DrawGuiText("Dzieci:", { padX, currentY }, 0.4f, { 0.8f, 0.8f, 0.8f, 1.0f });
                    currentY += 20.0f;

                    std::size_t currentChildId = rel->FirstChild;

                    while (currentChildId != std::numeric_limits<std::size_t>::max())
                    {
                        Entity childEntity = { currentChildId, 0 };

                        auto* tagComp = world.GetComponent<TagComponent>(childEntity);
                        std::string childName = tagComp ? tagComp->Tag : "Dziecko " + std::to_string(currentChildId);

                        if (Gui::Button("Wybierz: " + childName, { padX, currentY }, { elementW, 25.0f }))
                        {
                            activeScene->SetSelectedEntity(childEntity);
                        }
                        currentY += 30.0f;

                        auto* childRel = world.GetComponent<RelationshipComponent>(childEntity);
                        if (childRel)
                        {
                            currentChildId = childRel->NextSibling;
                        }
                        else
                        {
                            break;
                        }
                    }
                    currentY += 5.0f;
                }
            }

            auto* transform = world.GetComponent<TransformComponent>(selected);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                glm::vec3 rot = transform->GetRotation();
                glm::vec3 scale = transform->GetScale();
                glm::vec3 posBeforeSliders = pos;
                float dS = 0.05f; float rS = 0.5f;

                Gui::DrawGuiText("Transform:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;
                Gui::DragFloat("Pos X", &pos.x, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Pos Y", &pos.y, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Pos Z", &pos.z, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot X", &rot.x, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot Y", &rot.y, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot Z", &rot.z, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale X", &scale.x, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale Y", &scale.y, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale Z", &scale.z, dS, { padX, currentY }, { elementW, 20 }); currentY += 30.0f;

                if (pos != posBeforeSliders || rot != transform->GetRotation() || scale != transform->GetScale()) {
                    transform->SetPosition(pos); transform->SetRotation(rot); transform->SetScale(scale);
                    if (pos != posBeforeSliders && !m_IsDraggingTransform) {
                        m_IsDraggingTransform = true; m_TransformStartPos = posBeforeSliders;
                    }
                }
                if (m_IsDraggingTransform && !Input::IsMouseButtonPressed(0)) {
                    EntityTransformChangedEvent e(selected, m_TransformStartPos, pos);
                    Application::Get().OnEvent(e);
                    m_IsDraggingTransform = false;
                }
            }

            auto* collider = world.GetComponent<BoxColliderComponent>(selected);
            if (collider) {
                Gui::DrawGuiText("Box Collider:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;
                Gui::DragFloat("Size X", &collider->Size.x, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Size Y", &collider->Size.y, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Size Z", &collider->Size.z, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off X", &collider->Offset.x, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off Y", &collider->Offset.y, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off Z", &collider->Offset.z, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 25.0f;
                if (Gui::Button("USUN COLLIDER", { padX, currentY }, { elementW, 25.0f }))
                    world.RemoveComponent<BoxColliderComponent>(selected);
                currentY += 35.0f;
            }
            else {
                if (Gui::Button("+ Dodaj Box Collider", { padX, currentY }, { elementW, 25.0f })) {
                    BoxColliderComponent bc;
                    bc.Size = { 1.0f, 1.0f, 1.0f }; bc.Offset = { 0.0f, 0.0f, 0.0f };
                    world.AddComponent<BoxColliderComponent>(selected, bc);
                }
                currentY += 35.0f;
            }

            auto* scriptComp = world.GetComponent<NativeScriptComponent>(selected);
            if (!scriptComp) {
                if (Gui::Button("+ Dodaj System Skryptow", { padX, currentY }, { elementW, 25.0f }))
                    world.AddComponent<NativeScriptComponent>(selected, NativeScriptComponent{});
                currentY += 35.0f;
            }
            else {
                Gui::DrawGuiText("Skrypty:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;

                std::string scriptList = "";
                for (const auto& s : scriptComp->Scripts) {
                    scriptList += "- " + s.Name + "\n";
                    if (s.Name == "ParticleEmitterScript" && s.Instance) {
                        auto* emitter = static_cast<ParticleEmitterScript*>(s.Instance);
                        Gui::DrawGuiText("Ustawienia Pary/Dymu:", { padX, currentY }, 0.4f, { 0.4f, 0.8f, 1.0f, 1.0f }); currentY += 20.0f;
                        Gui::DragFloat("Predkosc Y", &emitter->ParticleTemplate.Velocity.y, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                        Gui::DragFloat("Rozrzut X", &emitter->ParticleTemplate.VelocityVariation.x, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                        Gui::DragFloat("Rozmiar", &emitter->ParticleTemplate.SizeBegin, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                        Gui::DragFloat("Czas Zycia", &emitter->ParticleTemplate.LifeTime, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                    }
                }
                if (!scriptList.empty()) {
                    Gui::DrawGuiText(scriptList, { padX, currentY }, 0.35f, { 0.2f, 0.9f, 0.2f, 1.0f });
                    currentY += (scriptComp->Scripts.size() * 15.0f) + 10.0f;
                }

                if (Gui::Button("+ Rot", { padX,         currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<RotationScript>("RotationScript");
                if (Gui::Button("+ Conv", { padX + 70.0f, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<ConveyorScript>("ConveyorScript");
                if (Gui::Button("+ Item", { padX + 140.0f,currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<ItemScript>("ItemScript");
                if (Gui::Button("+ Pot", { padX + 210.0f,currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<PotScript>("PotScript");
                currentY += 25.0f;
                if (Gui::Button("+ BeltVis", { padX,         currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<BeltVisualScript>("BeltVisualScript");
                if (Gui::Button("+ SteamParticle", { padX + 70.0f, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<SteamEmitterScript>("SteamEmitterScript");
                currentY += 25.0f;
                if (Gui::Button("WYCZYSC SKRYPTY", { padX, currentY }, { elementW - 90.0f, 20.0f })) scriptComp->Scripts.clear();
                currentY += 25.0f;
                if (Gui::Button("USUN SYSTEM SKRYPTOW", { padX, currentY }, { elementW, 25.0f })) world.RemoveComponent<NativeScriptComponent>(selected);
                currentY += 35.0f;
            }

            currentY += 10.0f;
            if (Gui::Button("ZAPISZ JAKO PREFAB", { padX, currentY }, { elementW, 30.0f })) {
                std::string prefabName = tag ? tag->Tag : "NowyPrefab";
                std::string path = "assets://prefabs/" + prefabName + ".json";
                PrefabSerializer::Serialize(activeScene.get(), selected, path);
            }
            currentY += 35.0f;

            if (Gui::Button("USUN OBIEKT Z MAPY", { padX, currentY }, { elementW, 30.0f })) {
                EntityDeletedEvent e(selected);
                Application::Get().OnEvent(e);
                activeScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
            }
        }
    }

    // --- OKNO ZAPISU ---
    if (m_ShowSaveDialog) {
        glm::vec2 dialogSize = { 350.0f, 150.0f };
        glm::vec2 dialogPos = GetAnchoredPosition(Anchor::Center, 0.0f, 0.0f, dialogSize.x, dialogSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(dialogPos, dialogSize, { 0.2f, 0.2f, 0.25f, 1.0f }, 15.0f);
        Gui::DrawGuiText("Zapisz scene jako:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });
        Gui::InputGuiText("Nazwa", m_SaveFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

        if (Gui::Button("Zapisz plik", { dialogPos.x + 10.0f,  dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
            SceneSerializer serializer(activeScene.get());
            serializer.Serialize("assets://saves/" + m_SaveFileName + ".json");
            m_ShowSaveDialog = false;
        }
        if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) m_ShowSaveDialog = false;
    }

    // --- OKNO WCZYTYWANIA ---
    if (m_ShowLoadDialog) {
        glm::vec2 dialogSize = { 350.0f, 150.0f };
        glm::vec2 dialogPos = GetAnchoredPosition(Anchor::Center, 0.0f, 0.0f, dialogSize.x, dialogSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(dialogPos, dialogSize, { 0.25f, 0.2f, 0.2f, 1.0f }, 15.0f);
        Gui::DrawGuiText("Wczytaj scene:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });
        Gui::InputGuiText("Nazwa", m_LoadFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

        if (Gui::Button("Wczytaj plik", { dialogPos.x + 10.0f,  dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
            std::shared_ptr<Scene> newScene = SceneManager::NewScene();
            SceneSerializer serializer(newScene.get());
            serializer.Deserialize("assets://saves/" + m_LoadFileName + ".json");
            m_ShowLoadDialog = false;
        }
        if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) m_ShowLoadDialog = false;
    }

    // --- MENU WIDOKU ---
    if (m_ShowViewMenu) {
        Gui::Panel({ 100.0f, 30.0f }, { 160.0f, 220.0f }, { 0.2f, 0.2f, 0.2f, 0.9f }, 15.0f);
        if (Gui::Button("Panel Otoczenia", { 105.0f, 35.0f }, { 150.0f, 25.0f }, m_ShowEnvironmentPanel)) { m_ShowEnvironmentPanel = !m_ShowEnvironmentPanel; m_ShowViewMenu = false; }
        if (Gui::Button("Hierarchia", { 105.0f, 65.0f }, { 150.0f, 25.0f }, m_ShowHierarchyPanel)) { m_ShowHierarchyPanel = !m_ShowHierarchyPanel;   m_ShowViewMenu = false; }
        if (Gui::Button("Biblioteka", { 105.0f, 95.0f }, { 150.0f, 25.0f }, m_ShowLibraryPanel)) { m_ShowLibraryPanel = !m_ShowLibraryPanel;     if (m_ShowPrefabsPanel) m_ShowPrefabsPanel = false; m_ShowViewMenu = false; }
        if (Gui::Button("Inspektor", { 105.0f, 125.0f }, { 150.0f, 25.0f }, m_ShowInspectorPanel)) { m_ShowInspectorPanel = !m_ShowInspectorPanel;   m_ShowViewMenu = false; }
        if (Gui::Button("Diagnostyka", { 105.0f, 155.0f }, { 150.0f, 25.0f }, m_ShowDiagnosticPanel)) { m_ShowDiagnosticPanel = !m_ShowDiagnosticPanel;  m_ShowViewMenu = false; }
        if (Gui::Button("Questy", { 105.0f, 185.0f }, { 150.0f, 25.0f }, m_ShowQuestsPanel)) { m_ShowQuestsPanel = !m_ShowQuestsPanel;      m_ShowViewMenu = false; }
        if (Gui::Button("Prefaby", { 105.0f, 215.0f }, { 150.0f, 25.0f }, m_ShowPrefabsPanel)) { m_ShowPrefabsPanel = !m_ShowPrefabsPanel;     if (m_ShowLibraryPanel) m_ShowLibraryPanel = false; m_ShowViewMenu = false; }
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    Gui::EndFrame();
}

void EditorGuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& ev) { Gui::OnCharTyped(ev.GetKeyCode()); return false; });
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
}

bool EditorGuiLayer::OnMouseScrolled(MouseScrolledEvent& e) {
    float scrollAmount = e.GetYOffset() * 30.0f;
    if (m_ShowLibraryPanel) {
        glm::vec2 size(m_ViewportWidth - 500.0f, 200.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (Gui::IsMouseOver(pos, size)) {
            m_LibraryScrollY -= scrollAmount;
            if (m_LibraryScrollY < 0.0f) m_LibraryScrollY = 0.0f;
            return true;
        }
    }
    if (m_ShowHierarchyPanel) {
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, 180.0f, 250.0f, m_ViewportWidth, m_ViewportHeight);
        if (Gui::IsMouseOver(pos, { 180.0f, 300.0f })) {
            m_HierarchyScrollY -= scrollAmount;
            if (m_HierarchyScrollY < 0.0f) m_HierarchyScrollY = 0.0f;
            return true;
        }
    }
    return false;
}

bool EditorGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    if (Gui::WantCaptureMouse()) return true;
    return false;
}

bool EditorGuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}

EditorGuiLayer::~EditorGuiLayer() {}
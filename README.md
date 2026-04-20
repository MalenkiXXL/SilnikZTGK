# CookingStation - Custom 3D Game Engine

Autorski silnik gier 3D tworzony w C++ z wykorzystaniem API OpenGL. Projekt skupia się na wydajnej, niskopoziomowej architekturze oraz modularności, rozwijany jako główny projekt w ramach konkursu ZTGK.

## 🚀 Kluczowe Funkcjonalności

* **Architektura Entity Component System (ECS):** Autorska implementacja systemu encji.
* **System Fizyki i Kolizji:** Implementacja detekcji kolizji dla brył **AABB** oraz algorytmów sprawdzających przecięcia.
* **Zaawansowany Raycasting:** System rzucania promieni z kamery, umożliwiający precyzyjną interakcję z obiektami w przestrzeni 3D.
* **Renderer 3D:** Warstwa renderująca zarządzająca buforami oraz shaderami, wyposażona w system zbierania statystyk wydajnościowych w czasie rzeczywistym.
* **Natywne Skryptowanie:** System pozwalający na podpinanie logiki w C++ bezpośrednio pod encje w silniku.

## 🛠️ Stack Techniczny

* **Język:** C++ (standard 17/20)
* **Grafika:** OpenGL 4.x
* **Matematyka:** GLM (OpenGL Mathematics)
* **Zależności:** GLFW (zarządzanie oknem), Glad (ładowanie rozszerzeń), spdlog (logowanie), nlohmann/json (serializacja scen).

## 📂 Struktura Projektu

* `Core/` – Jądro silnika, obsługa okna, wejścia i fizyki.
* `Scene/` – Zarządzanie światem, ECS i serializacja.
* `Renderer/` – Abstrakcja API graficznego i systemy renderowania.
* `Layers/` – System warstw aplikacji (Editor, Gameplay, UI).
* `Events/` – System zdarzeń (klawiatura, mysz, okno).

## 🚧 Status Projektu
Projekt jest w fazie aktywnego rozwoju. Obecnie pracujemy nad rozbudową narzędzi edytorskich oraz optymalizacją systemu ładowania zasobów.

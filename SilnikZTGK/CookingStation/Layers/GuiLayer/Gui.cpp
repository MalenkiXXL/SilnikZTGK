#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h" 
#include <utility> // Dla std::pair

std::shared_ptr<Font> Gui::s_Font = nullptr;
float Gui::s_ScreenWidth = 800.0f;
float Gui::s_ScreenHeight = 600.0f;
bool Gui::s_AnyActive = false;
std::string Gui::s_CharacterBuffer = "";

/* 
    Sprawdza czy kursor znajduje siê nad obszarem
    Przelicza pozycjê myszki na wspó³rzêdne GUI
*/
bool Gui::IsMouseOver(const glm::vec2& pos, const glm::vec2& size) {
    // pobiera przemapowan¹ pozycjê myszki
    glm::vec2 mappedMouse = GetMappedMousePos();

    // sprawdza, czy wspó³rzêdne myszy mieszcz¹ siê w granicach prostok¹ta
    return (mappedMouse.x >= pos.x && mappedMouse.x <= pos.x + size.x &&
        mappedMouse.y >= pos.y && mappedMouse.y <= pos.y + size.y);
}


/*
    Przelicza pozycje myszki na wspó³rzêdne GUI
    Pomaga w responsywnosci
*/
glm::vec2 Gui::GetMappedMousePos() {

    // pobiera pozycje kursora w pikselach
    auto mousePos = Input::GetMousePosition();

    // pobiera aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();

    // (pozycja myszki / rozmiar okna) * logiczny rozmiar GUI
    // zwraca wspolrzedne w skali 0-800 niezaleznie od rozdzielczosci
    float mouseX = mousePos.first * (s_ScreenWidth / windowSize.first);
    float mouseY = mousePos.second * (s_ScreenHeight / windowSize.second);
    return { mouseX, mouseY };
}


/*
    Interaktywny suwak, zmienia wartoœæ z pozycji Value
    Zwraca true przy poruszaniu w danej klatce
*/
bool Gui::SliderFloat(const std::string& label, float* value, float min, float max, const glm::vec2& pos, const glm::vec2& size) {
    bool changed = false;

    // 0 = lewy przycisk myszy
   if (Input::IsMouseButtonPressed(0)) {
        if (IsMouseOver(pos, size)) {
            //Pobieramy przeliczon¹ pozycjê myszki
            glm::vec2 mappedMouse = GetMappedMousePos();

            // obliczamy w ktorym miejscu szerokosci jest suwak
            float t = (mappedMouse.x - pos.x) / size.x;

            // mapujemy wartoœæ t na zakres min-max
            // interpolacja ze wzoru:
            // wartosc = min * (procent_przesuniêcia * dlugosc_zakresu)

            *value = min + t * (max - min);

            // sprawdzamy, czy wartosc nie wyszla poza zakres
            if (*value < min) *value = min;
            if (*value > max) *value = max;

            changed = true; // mowimy ze dane ulegly zmianie
        }
    }


   // Rysujemy podpis suwaka
    DrawGuiText(label, { pos.x, pos.y - 15.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
   
    // Rysowanie tla
    Renderer2D::DrawQuad(pos, size, { 0.2f, 0.2f, 0.2f, 1.0f });

    // Obliczanie pozycji uchwytu
    float handleWidth = 10.0f;
    float handlePos = ((*value - min) / (max - min)) * size.x;

    // Rysowanie uchwytu
    Renderer2D::DrawQuad({ pos.x + handlePos - (handleWidth / 2.0f), pos.y }, { handleWidth, size.y }, { 0.8f, 0.8f, 0.8f, 1.0f });

    return changed;
}


/*
    Rysuje tekst na ekranie
    Korzysta z atlasu tekstur czcionki i wspolrzednych UV dla kazdej litery
*/
void Gui::DrawGuiText(const std::string& text, glm::vec2 pos, float scale, const glm::vec4& color) {
    // iterujemy przez kazda litere w tekscie
    for (char c : text) {
        // sprawdzamy czy czionka ma definicje dla tego znaku
        if (s_Font->GetCharacters().find(c) != s_Font->GetCharacters().end()) {
            // pobieramy dane dla danego znaku (wymiar, pozycja w atlasie, odstêp)
            auto& ch = s_Font->GetChar(c);

            // skalujemy rozmiar litery na ekranie
            glm::vec2 size = { ch.Size.x * scale, ch.Size.y * scale };

            // u¿ywamy rozbudowanego DrawQuad z UV
            Renderer2D::DrawQuad(pos, size, s_Font->GetTexture(), color, ch.UV_Min, ch.UV_Max);

            // przesuniêcie pozycji w prawo, ch.Advance to szerokoœæ znaku w linii
            pos.x += ch.Advance * scale; 
        }
    }
}

/*
    Obs³uguje pola tekstowe, do których mo¿na wpisywaæ dane 
*/
bool Gui::InputGuiText(const std::string& label, std::string& value, const glm::vec2& pos, const glm::vec2& size) {
    // detekcja kursora
    bool hovered = IsMouseOver(pos, size);
    
    // zarz¹dzanie stanem pola
    static bool active = false;

    if (hovered && Input::IsMouseButtonPressed(0)) active = true;
    if (!hovered && Input::IsMouseButtonPressed(0)) active = false;

    if (active) s_AnyActive = true;
    else s_AnyActive = false;

    // zmiana koloru tla w zaleznosci od stanu
    glm::vec4 bgColor = active ? glm::vec4(0.2f,0.2f,0.2f,1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    // rysowanie tla
    Renderer2D::DrawQuad(pos, size, bgColor);
    
    // rysowanie podpisu z zawratoœci¹ tekstu (margin 5px)
    DrawGuiText(label + ": " + value, { pos.x + 5.0f, pos.y + 5.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

    // jezeli pole jest aktywne, to obsluguje klawiature
    if (active) {
        // A. OBS£UGA BACKSPACE (usuwanie)
        if (Input::IsKeyPressed(259)) {
            if (!value.empty()) {
                value.pop_back();
            }
        }

        // B. OBS£UGA WPISYWANIA (dodawanie)
        if (!s_CharacterBuffer.empty()) {
            value += s_CharacterBuffer; // Dodajemy to, co wpisaliœmy w tej klatce
            s_CharacterBuffer.clear();  // Czyœcimy bufor po zu¿yciu
        }
    }
    else {
        // Jeœli pole nie jest aktywne, czyœcimy bufor, ¿eby litery nie "wskoczy³y" 
        // do pola po jego póŸniejszym klikniêciu.
        s_CharacterBuffer.clear();
    }
    return active;
}


void Gui::Init(const std::string& fontPath, float fontSize) {
    s_Font = std::make_shared<Font>(fontPath, fontSize);
}

bool Gui::Button(const std::string & label, const glm::vec2 & pos, const glm::vec2 & size) {
    bool hovered = IsMouseOver(pos, size);
    bool clicked = false;

    glm::vec4 color = hovered ? glm::vec4(0.4f,0.4f,0.4f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

    if (hovered && Input::IsMouseButtonPressed(0)) {
        color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        clicked = true;
    }

    Renderer2D::DrawQuad(pos, size, color);
    DrawGuiText(label, { pos.x + 10.f, pos.y + 5.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
    
    return clicked;
}

// dopisuje znak z klawiatury do bufora, 
// który jest potem wykorzystywany przez pola InputGuiText
void Gui::OnCharTyped(int charcode) {
    s_CharacterBuffer += (char)charcode;
}
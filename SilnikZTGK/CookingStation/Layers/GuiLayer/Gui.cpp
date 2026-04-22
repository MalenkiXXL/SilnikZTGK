#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h" 
#include <utility> // dla std::pair

std::shared_ptr<Font> Gui::s_Font = nullptr;
float Gui::s_ScreenWidth = 800.0f;
float Gui::s_ScreenHeight = 600.0f;
bool Gui::s_AnyActive = false;
std::string Gui::s_ActiveWidgetID = ""; 
std::string Gui::s_CharacterBuffer = "";
static std::string s_FloatEditBuffer = "";
static float s_DragFloatStartMouseX = 0.0f;
static float s_DragFloatStartValue = 0.0f;
static bool s_IsDraggingFloat = false;
static bool s_IsInTextMode = false;

// laduje czcionke i inicjalizuje atlas dla gui
void Gui::Init(const std::string& fontPath, float fontSize) {
	s_Font = std::make_shared<Font>(fontPath, fontSize);
}

// sprawdza czy kursor znajduje siê nad obszarem
// przelicza pozycjê myszki na wspó³rzêdne GUI
bool Gui::IsMouseOver(const glm::vec2& pos, const glm::vec2& size) {
	// pobiera przemapowan¹ pozycjê myszki
	glm::vec2 mappedMouse = GetMappedMousePos();

	// sprawdza, czy wspó³rzêdne myszy mieszcz¹ siê w granicach prostok¹ta
	return (mappedMouse.x >= pos.x && mappedMouse.x <= pos.x + size.x &&
		mappedMouse.y >= pos.y && mappedMouse.y <= pos.y + size.y);
}


//przelicza pozycje myszki na wspó³rzêdne GUI
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

// innteraktywny suwak, zmienia wartoœæ z pozycji Value
// zwraca true przy poruszaniu w danej klatce
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

			*value = min + t * (max - min);

			// sprawdzamy, czy wartosc nie wyszla poza zakres
			if (*value < min) *value = min;
			if (*value > max) *value = max;

			changed = true; // mowimy ze dane ulegly zmianie
		}
	}

	// rysowanie podpisu suwaka
	DrawGuiText(label, { pos.x, pos.y - 15.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	// rysowanie tla
	Renderer2D::DrawQuad(pos, size, { 0.2f, 0.2f, 0.2f, 1.0f });

	// obliczanie pozycji uchwytu
	float handleWidth = 10.0f;
	float handlePos = ((*value - min) / (max - min)) * size.x;

	// rysowanie uchwytu
	Renderer2D::DrawQuad({ pos.x + handlePos - (handleWidth / 2.0f), pos.y }, { handleWidth, size.y }, { 0.8f, 0.8f, 0.8f, 1.0f });

	return changed;
}


// rysowanie tekstu na ekranie
// korzysta z atlasu tekstur czcionki i wspolrzednych UV dla kazdej litery
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


// obs³uguje pola tekstowe, do których mo¿na wpisywaæ dane 
bool Gui::InputGuiText(const std::string& label, std::string& value, const glm::vec2& pos, const glm::vec2& size) {
	// detekcja kursora
	bool hovered = IsMouseOver(pos, size);

	// Unikalny identyfikator tego konkretnego pola (po³¹czenie labela i ewentualnie pozycji, by unikn¹æ duplikatów nazw)
	// W profesjonalnych silnikach u¿ywa siê do tego hashowania, ale dla nas to wystarczy:
	std::string widgetID = label + std::to_string(pos.x) + std::to_string(pos.y);

	// Jesli kliknieto lewym przyciskiem myszy...
	if (Input::IsMouseButtonPressed(0)) {
		if (hovered) {
			s_ActiveWidgetID = widgetID;
		}
		else {
			if (s_ActiveWidgetID == widgetID) {
				s_ActiveWidgetID = "";
			}
		}
	}

	bool isActive = (s_ActiveWidgetID == widgetID);

	// wysylamy to do kamery, zeby wiedziala kiedy ma przestac odbierac input
	if (isActive) s_AnyActive = true;

	// zmiana koloru tla w zaleznosci od stanu
	glm::vec4 bgColor = isActive ? glm::vec4(0.2f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

	// rysowanie tla
	Renderer2D::DrawQuad(pos, size, bgColor);

	// rysowanie podpisu z zawratoœci¹ tekstu (margin 5px)
	DrawGuiText(label + ": " + value, { pos.x + 5.0f, pos.y + 5.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	// jezeli pole jest aktywne, to obsluguje klawiature
	if (isActive) {
		// backspace czysci calosc 
		if (Input::IsKeyPressed(259)) {
			if (!value.empty()) {
				value.pop_back();
			}
		}

		// obsluga znakow wpisywanych
		if (!s_CharacterBuffer.empty()) {
			value += s_CharacterBuffer;
			// czyscimy bufor, zeby inne rzeczy nie dostaly tych liter
			s_CharacterBuffer.clear();
		}
	}

	return isActive;
}


bool Gui::Button(const std::string& label, const glm::vec2& pos, const glm::vec2& size, bool isActive) {

	bool hovered = IsMouseOver(pos, size);
	bool clicked = false;

	// Je¿eli przycisk jest aktywny, dajemy mu inny bazowy kolor 
	glm::vec4 color = isActive ? glm::vec4(0.5f, 0.5f, 0.6f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

	// Reakcja na najechanie myszk¹ - te¿ rozró¿niamy w³¹czony vs wy³¹czony
	if (hovered) {
		color = isActive ? glm::vec4(0.6f, 0.6f, 0.7f, 1.0f) : glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
	}

	// Reakcja na fizyczne wciœniêcie 
	if (hovered && Input::IsMouseButtonPressed(0)) {
		color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	}

	// Rejestracja klikniêcia z wykorzystaniem JustPressed
	if (hovered && Input::IsMouseButtonJustPressed(0)) {
		clicked = true;
	}

	// rysujemy tlo 
	Renderer2D::DrawQuad(pos, size, color);

	// podpisujemy
	DrawGuiText(label, { pos.x + 10.f, pos.y + 5.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	return clicked;
}

// dopisuje znak z klawiatury do bufora, który jest potem wykorzystywany przez pola InputGuiText
void Gui::OnCharTyped(int charcode) {
	s_CharacterBuffer += (char)charcode;
}

bool Gui::DragFloat(const std::string& label, float* value, float dragSpeed, const glm::vec2& pos, const glm::vec2& size) {
	std::string widgetID = label + std::to_string(pos.x) + std::to_string(pos.y);
	bool hovered = IsMouseOver(pos, size);
	bool changed = false;
	glm::vec2 mousePos = GetMappedMousePos();

	// 1. Wyjœcie z trybu edycji, jeœli klikniêto gdzieœ indziej
	if (Input::IsMouseButtonJustPressed(0) && !hovered && s_ActiveWidgetID == widgetID) {
		if (s_IsInTextMode) {
			try { *value = std::stof(s_FloatEditBuffer); changed = true; }
			catch (...) {}
		}
		s_ActiveWidgetID = "";
		s_IsInTextMode = false;
	}

	// 2. Klikniêcie w nasz widget
	if (Input::IsMouseButtonJustPressed(0) && hovered) {
		s_ActiveWidgetID = widgetID;
		s_IsDraggingFloat = false;
		s_IsInTextMode = false;
		s_DragFloatStartMouseX = mousePos.x;
		s_DragFloatStartValue = *value;
	}

	bool isActive = (s_ActiveWidgetID == widgetID);

	// 3. Obs³uga wciœniêtego przycisku (Drag)
	if (isActive && Input::IsMouseButtonPressed(0) && !s_IsInTextMode) {
		float delta = mousePos.x - s_DragFloatStartMouseX;
		// Jeœli przesunêliœmy choæ trochê myszkê - to jest przeci¹ganie
		if (std::abs(delta) > 2.0f) {
			s_IsDraggingFloat = true;
			*value = s_DragFloatStartValue + (delta * dragSpeed);
			changed = true;
		}
	}

	// 4. Puszczenie przycisku (Wejœcie w tryb tekstowy, jeœli nie by³o przeci¹gania)
	if (isActive && !Input::IsMouseButtonPressed(0)) {
		if (!s_IsDraggingFloat && !s_IsInTextMode) {
			// Zwyk³e, krótkie klikniêcie! W³¹czamy wpisywanie
			s_IsInTextMode = true;
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%.2f", *value);
			s_FloatEditBuffer = buffer;
		}
		else if (s_IsDraggingFloat) {
			// Koniec przeci¹gania myszk¹
			s_ActiveWidgetID = "";
			s_IsDraggingFloat = false;
		}
	}

	// 5. Tryb tekstowy (Wpisywanie z klawiatury)
	if (isActive && s_IsInTextMode) {
		s_AnyActive = true; // Zatrzymuje kamerê z gry

		// Backspace
		if (Input::IsKeyPressed(259) && !s_FloatEditBuffer.empty()) {
			s_FloatEditBuffer.pop_back();
		}

		// Enter - Zatwierdzenie wartoœci
		if (Input::IsKeyPressed(257)) {
			try { *value = std::stof(s_FloatEditBuffer); changed = true; }
			catch (...) {}
			s_ActiveWidgetID = "";
			s_IsInTextMode = false;
		}

		// Przyjmowanie znaków (tylko cyfry, kropka i minus)
		if (!s_CharacterBuffer.empty()) {
			for (char c : s_CharacterBuffer) {
				if (std::isdigit(c) || c == '.' || c == '-') {
					s_FloatEditBuffer += c;
				}
			}
			s_CharacterBuffer.clear();
		}
	}

	// --- RYSOWANIE WIDGETU ---
	// Kolor t³a zale¿ny od stanu
	glm::vec4 bgColor = (isActive && s_IsInTextMode) ? glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) :
		(hovered ? glm::vec4(0.3f, 0.3f, 0.3f, 1.0f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

	Renderer2D::DrawQuad(pos, size, bgColor);

	// Wyœwietlany tekst
	std::string displayStr;
	if (isActive && s_IsInTextMode) {
		displayStr = s_FloatEditBuffer; // Tryb pisania (pokazujemy bufor w czasie rzeczywistym)
	}
	else {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.2f", *value); // Tryb normalny (zaokr¹glone do 2 miejsc po przecinku)
		displayStr = buffer;
	}

	DrawGuiText(label + ": " + displayStr, { pos.x + 5.0f, pos.y + 5.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	return changed;
}
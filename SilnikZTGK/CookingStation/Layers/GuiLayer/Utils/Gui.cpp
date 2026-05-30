#include "Gui.h"
#include "Renderer2D.h"
#include "CookingStation/Core/Input.h" 
#include <utility> // dla std::pair

std::shared_ptr<Font> Gui::s_Font = nullptr;
float Gui::s_ScreenWidth = 800.0f;
float Gui::s_ScreenHeight = 600.0f;
std::string Gui::s_ActiveWidgetID = "";
std::string Gui::s_CharacterBuffer = "";
static std::string s_FloatEditBuffer = "";
static float s_DragFloatStartMouseX = 0.0f;
static float s_DragFloatStartValue = 0.0f;
static bool s_IsDraggingFloat = false;
static bool s_IsInTextMode = false;
float Gui::s_DeltaTime = 0.0f;
bool Gui::s_WantCaptureMouse = false;

// laduje czcionke i inicjalizuje atlas dla gui
void Gui::Init(const std::string& fontPath, float fontSize) {
	s_Font = std::make_shared<Font>(fontPath, fontSize);
}

// sprawdza czy kursor znajduje si� nad obszarem
// przelicza pozycj� myszki na wsp�rz�dne GUI
bool Gui::IsMouseOver(const glm::vec2& pos, const glm::vec2& size) {
	// pobiera przemapowan� pozycj� myszki
	glm::vec2 mappedMouse = GetMappedMousePos();

	// sprawdza, czy wsp�rz�dne myszy mieszcz� si� w granicach prostok�ta
	return (mappedMouse.x >= pos.x && mappedMouse.x <= pos.x + size.x &&
		mappedMouse.y >= pos.y && mappedMouse.y <= pos.y + size.y);
}

//przelicza pozycje myszki na wsp�rz�dne GUI
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

// innteraktywny suwak, zmienia warto�� z pozycji Value
// zwraca true przy poruszaniu w danej klatce
bool Gui::SliderFloat(const std::string& label, float* value, float min, float max, const glm::vec2& pos, const glm::vec2& size) {
	bool changed = false;

	// 0 = lewy przycisk myszy
	if (Input::IsMouseButtonPressed(0)) {
		if (IsMouseOver(pos, size)) {
			//Pobieramy przeliczon� pozycj� myszki
			glm::vec2 mappedMouse = GetMappedMousePos();

			// obliczamy w ktorym miejscu szerokosci jest suwak
			float t = (mappedMouse.x - pos.x) / size.x;

			*value = min + t * (max - min);

			// sprawdzamy, czy wartosc nie wyszla poza zakres
			if (*value < min) *value = min;
			if (*value > max) *value = max;

			changed = true; // mowimy ze dane ulegly zmianie
		}
	}

	// rysowanie podpisu suwaka
	DrawGuiText(label, { pos.x, pos.y - 15.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	// rysowanie tla z wymuszonym zaokr�gleniem 15.0f
	Renderer2D::DrawQuad(pos, size, { 0.2f, 0.2f, 0.2f, 1.0f }, 15.0f);

	// obliczanie pozycji uchwytu
	float handleWidth = 10.0f;
	float handlePos = ((*value - min) / (max - min)) * size.x;

	// rysowanie uchwytu (uchwyt suwaka delikatnie zaokr�glamy dla sp�jno�ci)
	Renderer2D::DrawQuad({ pos.x + handlePos - (handleWidth / 2.0f), pos.y }, { handleWidth, size.y }, { 0.8f, 0.8f, 0.8f, 1.0f }, 4.0f);

	return changed;
}

// rysowanie tekstu na ekranie
void Gui::DrawGuiText(const std::string& text, glm::vec2 pos, float scale, const glm::vec4& color) {
	float baselineOffset = 32.0f * 0.8f * scale;

	for (char c : text) {
		if (s_Font->GetCharacters().find(c) != s_Font->GetCharacters().end()) {
			auto& ch = s_Font->GetChar(c);
			glm::vec2 size = { ch.Size.x * scale, ch.Size.y * scale };

			glm::vec2 charPos = {
				pos.x + (ch.Offset.x * scale),
				pos.y + baselineOffset + (ch.Offset.y * scale)
			};

			uint32_t fontTextureID = s_Font->GetTexture()->GetRendererID();
			Renderer2D::DrawQuad(charPos, size, fontTextureID, color, ch.UV_Min, ch.UV_Max);

			pos.x += ch.Advance * scale;
		}
	}
}

// obs�uguje pola tekstowe, do kt�rych mo�na wpisywa� dane 
bool Gui::InputGuiText(const std::string& label, std::string& value, const glm::vec2& pos, const glm::vec2& size) {
	bool hovered = IsMouseOver(pos, size);
	std::string widgetID = label + std::to_string(pos.x) + std::to_string(pos.y);

	if (Input::IsMouseButtonPressed(0)) {
		if (hovered) {
			s_ActiveWidgetID = widgetID;
			s_WantCaptureMouse = true;
		}
		else {
			if (s_ActiveWidgetID == widgetID) {
				s_ActiveWidgetID = "";
			}
		}
	}

	bool isActive = (s_ActiveWidgetID == widgetID);
	glm::vec4 bgColor = isActive ? glm::vec4(0.2f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

	// Rysowanie t�a pola tekstowego z zaokr�gleniem 15.0f
	Renderer2D::DrawQuad(pos, size, bgColor, 15.0f);

	DrawGuiText(label + ": " + value, { pos.x + 10.0f, pos.y + 5.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	if (isActive) {
		if (Input::IsKeyPressed(259)) {
			if (!value.empty()) {
				value.pop_back();
			}
		}

		if (!s_CharacterBuffer.empty()) {
			value += s_CharacterBuffer;
			s_CharacterBuffer.clear();
		}
	}

	return isActive;
}

bool Gui::Button(const std::string& label, const glm::vec2& pos, const glm::vec2& size, bool isActive) {
	bool hovered = IsMouseOver(pos, size);
	bool clicked = false;

	glm::vec4 color = isActive ? glm::vec4(0.5f, 0.5f, 0.6f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

	if (hovered) {
		color = isActive ? glm::vec4(0.6f, 0.6f, 0.7f, 1.0f) : glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
		s_WantCaptureMouse = true;
	}

	if (hovered && Input::IsMouseButtonPressed(0)) {
		color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	}

	if (hovered && Input::IsMouseButtonJustPressed(0)) {
		clicked = true;
	}

	// Rysujemy t�o przycisku z wymuszonym zaokr�gleniem 15.0f
	Renderer2D::DrawQuad(pos, size, color, 15.0f);

	DrawGuiText(label, { pos.x + 10.f, pos.y + 5.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	return clicked;
}

void Gui::OnCharTyped(int charcode) {
	s_CharacterBuffer += (char)charcode;
}

bool Gui::DragFloat(const std::string& label, float* value, float dragSpeed, const glm::vec2& pos, const glm::vec2& size) {
	std::string widgetID = label + std::to_string(pos.x) + std::to_string(pos.y);
	bool hovered = IsMouseOver(pos, size);
	if (hovered) {
		s_WantCaptureMouse = true;
	}
	bool changed = false;
	glm::vec2 mousePos = GetMappedMousePos();

	if (Input::IsMouseButtonJustPressed(0) && !hovered && s_ActiveWidgetID == widgetID) {
		if (s_IsInTextMode) {
			try { *value = std::stof(s_FloatEditBuffer); changed = true; }
			catch (...) {}
		}
		s_ActiveWidgetID = "";
		s_IsInTextMode = false;
	}

	if (Input::IsMouseButtonJustPressed(0) && hovered) {
		s_ActiveWidgetID = widgetID;
		s_IsDraggingFloat = false;
		s_IsInTextMode = false;
		s_DragFloatStartMouseX = mousePos.x;
		s_DragFloatStartValue = *value;
	}

	bool isActive = (s_ActiveWidgetID == widgetID);

	if (isActive && Input::IsMouseButtonPressed(0) && !s_IsInTextMode) {
		float delta = mousePos.x - s_DragFloatStartMouseX;
		if (std::abs(delta) > 2.0f) {
			s_IsDraggingFloat = true;
			*value = s_DragFloatStartValue + (delta * dragSpeed);
			changed = true;
		}
	}

	if (isActive && !Input::IsMouseButtonPressed(0)) {
		if (!s_IsDraggingFloat && !s_IsInTextMode) {
			s_IsInTextMode = true;
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%.2f", *value);
			s_FloatEditBuffer = buffer;
		}
		else if (s_IsDraggingFloat) {
			s_ActiveWidgetID = "";
			s_IsDraggingFloat = false;
		}
	}

	if (isActive && s_IsInTextMode) {
		if (Input::IsKeyPressed(259) && !s_FloatEditBuffer.empty()) {
			s_FloatEditBuffer.pop_back();
		}

		if (Input::IsKeyPressed(257)) {
			try { *value = std::stof(s_FloatEditBuffer); changed = true; }
			catch (...) {}
			s_ActiveWidgetID = "";
			s_IsInTextMode = false;
		}

		if (!s_CharacterBuffer.empty()) {
			for (char c : s_CharacterBuffer) {
				if (std::isdigit(c) || c == '.' || c == '-') {
					s_FloatEditBuffer += c;
				}
			}
			s_CharacterBuffer.clear();
		}
	}

	glm::vec4 bgColor = (isActive && s_IsInTextMode) ? glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) :
		(hovered ? glm::vec4(0.3f, 0.3f, 0.3f, 1.0f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

	// Rysujemy pole DragFloat z zaokr�gleniem 15.0f
	Renderer2D::DrawQuad(pos, size, bgColor, 15.0f);

	std::string displayStr;
	if (isActive && s_IsInTextMode) {
		displayStr = s_FloatEditBuffer;
	}
	else {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.2f", *value);
		displayStr = buffer;
	}

	DrawGuiText(label + ": " + displayStr, { pos.x + 10.0f, pos.y + 5.0f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });

	return changed;
}

void Gui::BeginFrame() {
	s_WantCaptureMouse = false;
}

void Gui::Panel(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, float radius) {
	if (IsMouseOver(pos, size)) {
		s_WantCaptureMouse = true;
	}
	// Zawsze przekazujemy promie� do quada
	Renderer2D::DrawQuad(pos, size, color, radius);
}

float Gui::MeasureTextWidth(const std::string& text, float scale) {
	float width = 0.0f;
	for (char c : text) {
		if (s_Font->GetCharacters().find(c) != s_Font->GetCharacters().end()) {
			width += s_Font->GetChar(c).Advance * scale;
		}
	}
	return width;
}

float Gui::MeasureTextHeight(const std::string& text, float scale) {
	float maxHeight = 0.0f;
	for (char c : text) {
		if (s_Font->GetCharacters().find(c) != s_Font->GetCharacters().end()) {
			float h = s_Font->GetChar(c).Size.y * scale;
			if (h > maxHeight) maxHeight = h;
		}
	}
	return maxHeight;
}
bool Gui::ScaledButton(const std::string& label,
	glm::vec2 basePos, glm::vec2 baseSize,
	float btnScale, float bsc,
	glm::vec4 colorNormal, glm::vec4 colorHover,
	bool hovered)
{
	// Skalowane wymiary, wyrownane do srodka bazowego prostokata
	glm::vec2 scaledSize = baseSize * btnScale;
	glm::vec2 scaledPos = {
		basePos.x + (baseSize.x - scaledSize.x) * 0.5f,
		basePos.y + (baseSize.y - scaledSize.y) * 0.5f
	};

	// Kolor: hover, pressed (przyciemnienie), normalny
	glm::vec4 bgColor = hovered ? colorHover : colorNormal;
	if (hovered && Input::IsMouseButtonPressed(0))
		bgColor = bgColor * glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);

	if (hovered) s_WantCaptureMouse = true;

	// Cien przycisku
	Gui::Panel(scaledPos + glm::vec2(4.0f * bsc, 5.0f * bsc),
		scaledSize, { 0.0f, 0.0f, 0.0f, 0.35f }, 15.0f * bsc);

	// Tlo przycisku
	Gui::Panel(scaledPos, scaledSize, bgColor, 15.0f * bsc);

	// Tekst wyśrodkowany z poprawnym baselineOffset
	float textScale = 1.3f * bsc;
	float textW = MeasureTextWidth(label, textScale);
	float textH = MeasureTextHeight(label, textScale);
	float baselineOffset = 32.0f * 0.8f * textScale;
	glm::vec2 textPos = {
		scaledPos.x + (scaledSize.x - textW) * 0.5f,
		scaledPos.y + (scaledSize.y - textH) * 0.5f - baselineOffset + (textH * 0.5f)
	};

	// Cien tekstu
	DrawGuiText(label, textPos + glm::vec2(2.0f, 2.0f), textScale, { 0.0f, 0.0f, 0.0f, 0.7f });
	// Tekst
	DrawGuiText(label, textPos, textScale, { 1.0f, 1.0f, 1.0f, 1.0f });

	return hovered && Input::IsMouseButtonJustPressed(0);
}
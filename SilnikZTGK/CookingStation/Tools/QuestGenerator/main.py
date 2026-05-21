import os
import json
import time
import re
import ApiKeys
from google import genai
from google.genai import types
from newsapi import NewsApiClient

# KONFIGURACJA I KLUCZE
api_my_key = ApiKeys.NEWS_API_KEY
gemini_key = ApiKeys.GEMINI_API_KEY

cashe_file = "CookingStation/Assets/news_cache.json"
cache_expiry_seconds = 3600

# Tylko te dania mogą zostać wylosowane jako cel questa
ALLOWED_DISHES = [
    "pomidorowa", 
]

# "kanapka", "babeczka", "caprese", 
# "kopytka", "kopytka-zlote", "kawa", "kawa-mleko"

client = genai.Client(api_key=gemini_key)


# 1: PRE-PROCESSING 

def remove_polish_chars(text):
    """Deterministyczne usuwanie polskich znaków - oszczędza tokeny i błędy AI."""
    replacements = {'ą': 'a', 'ć': 'c', 'ę': 'e', 'ł': 'l', 'ń': 'n', 'ó': 'o', 'ś': 's', 'ź': 'z', 'ż': 'z',
                    'Ą': 'A', 'Ć': 'C', 'Ę': 'E', 'Ł': 'L', 'Ń': 'N', 'Ó': 'O', 'Ś': 'S', 'Ź': 'Z', 'Ż': 'Z'}
    for pl, lat in replacements.items():
        text = text.replace(pl, lat)
    return text

def get_news():
    os.makedirs(os.path.dirname(cashe_file), exist_ok=True)
    if os.path.exists(cashe_file) and os.path.getsize(cashe_file) > 0 and (time.time() - os.path.getmtime(cashe_file)) < cache_expiry_seconds:
        print("[System] Wczytywanie newsow z cache...")
        with open(cashe_file, "r", encoding='utf-8') as f:
            return json.load(f)

    print("[System] Pobieranie nowych danych z NewsAPI...")
    try:
        newsapi = NewsApiClient(api_key=api_my_key)
        data = newsapi.get_top_headlines(category="general", page_size=10)
        with open(cashe_file, "w", encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        return data
    except Exception as e:
        print(f"[Błąd] Nie udało się pobrać newsów: {e}")
        return None


# 2: GENERATOR (LLM + Few-Shot)

def generate_quests(news_context, feedback=""):
    print("[Generator] Tworzenie wstepnego zadania (kreatywnosc: wysoka)...")
    
    # Dodajemy ewentualny feedback z pętli samo-naprawczej
    feedback_instruction = f"\nOSTATNIA PRÓBA ZOSTAŁA ODRZUCONA. POPRAW BŁĘDY: {feedback}\n" if feedback else ""

    prompt = f"""
    Jesteś projektantem narracji w absurdalnej grze kulinarnej (styl Monty Pythona).
    Wygeneruj dokladnie 5 zadań na podstawie wiadomości.
    {feedback_instruction}
    
    ZASADY (Restrykcje silnika):
    1. Brak polskich znakow (zastap a, e, l, o itd.).
    2. Pola "dish_id" MUSZA pochodzic z tej dokladnej listy: {ALLOWED_DISHES}.
    3. Musisz zwrocic poprawny format JSON (array z 1 obiektem).

    LIMIT CHARAKTEROW:
    - "title": Maksymalnie 22 znaki (krótki, mocny tytuł).
    - "description": Maksymalnie 65 znaków (dokładnie jedno-dwa, bardzo krótkie zdania).
    
    WZÓR STRUKTURY (Few-Shot Prompting):
    [
      {{
        "title": "Kosmiczna Pomidorowa NASA",
        "description": "Naukowcy potrzebuja rakietowego paliwa, zalej ich serwery goraca zupa!",
        "dish_id": "pomidorowa",
        "portions": 15,
        "frequency": 8,
        "reward": "500 Monet, Kask Astronauty"
      }}
    ]
    
    WIADOMOŚCI:
    {news_context}
    """
    
    try:
        response = client.models.generate_content(
            model="gemini-3.1-flash-lite",
            contents=prompt,
            config=types.GenerateContentConfig(
                response_mime_type="application/json",
                temperature=0.8 # Wysoka kreatywność dla generatora
            )
        )
        return remove_polish_chars(response.text)
    except Exception as e:
        print(f"[Błąd Generatora] {e}")
        return None

# 4: SĘDZIA (LLM-as-a-judge + Direct Scoring)

def evaluate_quests_with_judge(quests_json, news_context):
    print("[Sedzia] Trwa ewaluacja semantyczna zadania...")
    
    # Czyszczenie całego promptu sędziego z polskich znaków, aby sam siebie nie triggerował!
    prompt_judge = f"""
    Jestes Glownym Projektantem Gry (Lead Game Designer). Oceniasz plik z zadaniem dla silnika.
    
    Zadanie wygenerowane: {quests_json}
    Zrodlo newsa: {news_context}
    
    PRZEPROWADZ ANALIZE W ZNACZNIKACH <sketchpad>. 
    Sprawdz krok po kroku:
    1. Czy "dish_id" jest logicznie i sensownie dopasowane do opisu wydarzenia?
    2. Czy zadanie w zabawny i absurdalny sposob laczy wiadomosc ze swiata z motywem kulinarnym?
    3. Czy tekst jest bezpieczny (brak polityki, nsfw, drastycznych scen)?
    
    Po analizie, wystaw oceny (1 = Zaliczone/Dobrze, 0 = Odrzucone/Zle) w formacie XML:
    <sketchpad>Twoj proces myslowy i uzasadnienie...</sketchpad>
    <news_anchoring>1 lub 0</news_anchoring>
    <creative_abstraction>1 lub 0</creative_abstraction>
    <safety_check>1 lub 0</safety_check>
    """
    
    try:
        response = client.models.generate_content(
            model="gemini-2.5-flash", 
            contents=prompt_judge,
            config=types.GenerateContentConfig(
                temperature=0.0 # Całkowity determinizm sędziego
            )
        )
        text = remove_polish_chars(response.text)
        
        print("\n================== LOG SEDZIEGO ================================")
        print(text)
        print("===============================================================\n")
        
        try:
            sketchpad = re.search(r'<sketchpad>(.*?)</sketchpad>', text, re.DOTALL).group(1).strip()
            news_anchoring = int(re.search(r'<news_anchoring>(\d)</news_anchoring>', text).group(1))
            creative_abstraction = int(re.search(r'<creative_abstraction>(\d)</creative_abstraction>', text).group(1))
            safety_check = int(re.search(r'<safety_check>(\d)</safety_check>', text).group(1))
            
            # format_valid ustawiamy automatycznie na True, bo Python pilnuje tego funkcją i regexem
            passed = all([news_anchoring, creative_abstraction, safety_check])
            return {"passed": passed, "feedback": sketchpad}
        except Exception as parse_e:
            return {"passed": False, "feedback": "Sedzia zwrocil niepoprawny format oceny XML."}
            
    except Exception as e:
        print(f"[Blad Sedziego] {e}")
        return {"passed": False, "feedback": "API Sedziego nie odpowiada."}


# GŁÓWNA PĘTLA (Self-Correction Loop)

if __name__ == "__main__":
    news_data = get_news()
    
    if news_data and "articles" in news_data:
        # Zmiana: pobieramy tylko 1, najważniejszy news zamiast 5
        news_text = " ".join([art.get("title", "") for art in news_data["articles"][:10]])
        news_text = remove_polish_chars(news_text)
        
        max_retries = 3
        attempts = 0
        final_quests = None
        current_feedback = ""
        
        while attempts < max_retries:
            print(f"\n--- PRÓBA {attempts + 1}/{max_retries} ---")
            
            # 1. Generacja
            quests_json_str = generate_quests(news_text, current_feedback)
            if not quests_json_str:
                attempts += 1
                continue
                
            # Faza 3: SZYBKA WALIDACJA LOGIKI 
            try:
                quests_obj = json.loads(quests_json_str)
                logic_failed = False
                
                # PANCERNY TEST PYTHONOWY NA POLSKIE ZNAKI
                raw_text_to_check = json.dumps(quests_obj)
                if any(char in raw_text_to_check for char in "ąęćłńóśźżĄĆĘŁŃÓŚŹŻ"):
                    current_feedback = "BLAD FORMATU: W wygenerowanym JSON wyryto polskie litery. Usun je i zastap znakami l, o, e, a, z..."
                    logic_failed = True
                
                # Test dozwolonych dań
                if not logic_failed:
                    for q in quests_obj:
                        if q.get("dish_id") not in ALLOWED_DISHES:
                            current_feedback = f"BLAD KRYTYCZNY SILNIKA: '{q.get('dish_id')}' nie istnieje w grze! Uzyj tylko listy dozwolonych dan."
                            logic_failed = True
                            break
                            
                if logic_failed:
                    print(f"[Walidator Python] Odrzucono lokalnie: {current_feedback}")
                    attempts += 1
                    continue
            except json.JSONDecodeError:
                current_feedback = "BLAD: Zwrocony tekst to nie jest poprawny JSON."
                attempts += 1
                continue

            # 2. Ewaluacja (LLM-as-a-judge)
            evaluation = evaluate_quests_with_judge(quests_json_str, news_text)
            
            if evaluation["passed"]:
                print("[Sędzia] ZAAKCEPTOWANO! Zadanie przeszlo rygorystyczne metryki.")
                final_quests = quests_json_str
                break
            else:
                print(f"[Sędzia] ODRZUCONO. Feedback dla generatora: {evaluation['feedback']}")
                current_feedback = evaluation["feedback"]
                attempts += 1
                
        # 
        # 5: ZAPIS 
        # 
        output_dir = "CookingStation/Assets"
        os.makedirs(output_dir, exist_ok=True)
        final_path = os.path.join(output_dir, "wygenerowane_quests.json")
        temp_path = os.path.join(output_dir, "temp_quests.json")

        if final_quests:
            # Zapis atomowy - silnik nigdy nie odczyta uszkodzonego pliku
            with open(temp_path, "w", encoding='utf-8') as f:
                f.write(final_quests)
            os.replace(temp_path, final_path)
            print(f"\n[SUKCES] Zapisano bezpiecznie i atomowo do: {final_path}")
        else:
            print("\n[!] Awaria potoku. Inicjowanie awaryjnego zestawu misji (Fallback).")
            fallback_quests = json.dumps([{
                "title": "Bunt Serwerow",
                "description": "Sztuczna inteligencja zglodniala! Podaj szybko ciepla zupe.",
                "dish_id": "pomidorowa",
                "portions": 10,
                "frequency": 5,
                "reward": "100 Monet"
            }], indent=4)
            
            with open(temp_path, "w", encoding='utf-8') as f:
                f.write(fallback_quests)
            os.replace(temp_path, final_path)
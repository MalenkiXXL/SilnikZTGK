import os
import json
import time
import re
import requests
from google import genai
from google.genai import types
from dotenv import load_dotenv

# ==========================================
# KONFIGURACJA I BEZPIECZNE KLUCZE
# ==========================================
load_dotenv()

api_my_key = os.getenv("SERPAPI_KEY")
gemini_key = os.getenv("GEMINI_API_KEY")

if not api_my_key or not gemini_key:
    raise ValueError("BRAK KLUCZY API! Upewnij się, że masz plik .env z SERPAPI_KEY i GEMINI_API_KEY")

cashe_file = "CookingStation/Assets/news_cache.json"
cache_expiry_seconds = 3600

# WAŻNE: Zostawiam "pomidorowa" po polsku, by pasowało do Waszego kodu w C++, 
# ale opisy i tytuły będą już po angielsku.
ALLOWED_DISHES = [
    "pomidorowa", "kanapka", "kopytka", "kopytka-zlote", "kawa", "kawa-mleko", "babeczka", "caprese", 
]

client = genai.Client(api_key=gemini_key)

# ==========================================
# 1: PRE-PROCESSING 
# ==========================================
def remove_polish_chars(text):
    """Zostawiamy jako zabezpieczenie (fallback) przed niechcianymi znakami w JSON."""
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

    print("[System] Pobieranie nowych danych z SerpApi...")
    try:
        params = {
            "engine": "google_news",
            "q": "(bizarre OR weird OR funny OR unexpected OR lifestyle) -politics -election -stock -market -economy -finance -government",
            "gl": "us", 
            "hl": "en", 
            "api_key": api_my_key
        }
        
        response = requests.get("https://serpapi.com/search.json", params=params)
        response.raise_for_status() 
        data = response.json()
        
        with open(cashe_file, "w", encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
            
        return data
    except Exception as e:
        print(f"[Błąd] Nie udało się pobrać newsów: {e}")
        return None

# ==========================================
# 2: GENERATOR (LLM + Few-Shot)
# ==========================================
def generate_quests(news_context, feedback=""):
    print("[Generator] Tworzenie wstepnego zadania (kreatywnosc: wysoka)...")
    
    feedback_instruction = f"\nLAST ATTEMPT REJECTED. FIX THESE ERRORS: {feedback}\n" if feedback else ""

    prompt = f"""
    You are a brilliant comedy writer for an absurd, cozy cooking game (Monty Python style).
    You will receive a list of real news headlines. Use them as inspiration to generate exactly 10 culinary quests in English.
    
    TONE GUIDELINES:
    - The humor must make logical sense within its own absurd premise (e.g., if a whale is stuck in sauce, the solution is eating the sauce to free it).
    - Do not just write random words. Tell a tiny, cohesive joke.
    {feedback_instruction}
    
    ENGINE RESTRICTIONS:
    1. Language: English only.
    2. "dish_id" MUST be exactly from this list: {ALLOWED_DISHES}.
    3. Determine the country of origin of the news event and provide its 2-letter ISO code in "reward_flag" (e.g., "US", "GB", "JP"). If it's space-related or unknown, use "UN".
    4. "reward_coins" must be an integer.
    5. Output valid JSON (an array of objects).

    CHARACTER LIMITS:
    - "title": Max 22 characters (short, punchy).
    - "description": Max 65 characters (exactly one or two short sentences).
    
    FEW-SHOT STRUCTURE EXAMPLE:
    [
      {{
        "title": "NASA's Soup Thrusters",
        "description": "The rocket is out of fuel! Pour hot soup into the engines.",
        "dish_id": "pomidorowa",
        "portions": 15,
        "frequency": 8,
        "reward_coins": 500,
        "reward_flag": "US"
      }},
      {{
        "title": "NBA Halftime Feast",
        "description": "Players refuse to play without soup. Feed the giants quickly!",
        "dish_id": "pomidorowa",
        "portions": 5,
        "frequency": 2,
        "reward_coins": 100,
        "reward_flag": "US"
      }}
    ]
    
    NEWS HEADLINES:
    {news_context}
    """
    
    try:
        response = client.models.generate_content(
            model="gemini-3.1-flash-lite",
            contents=prompt,
            config=types.GenerateContentConfig(
                response_mime_type="application/json",
                temperature=0.85 # Slightly higher for more creative connections
            )
        )
        return remove_polish_chars(response.text)
    except Exception as e:
        print(f"[Błąd Generatora] {e}")
        return None

# ==========================================
# 3: SĘDZIA (LLM-as-a-judge)
# ==========================================
def evaluate_quests_with_judge(quests_json, news_context):
    print("[Sedzia] Trwa ewaluacja semantyczna zadania...")
    
    prompt_judge = f"""
    You are the Lead Game Designer. You evaluate a generated quest JSON for our game engine.
    
    Generated Quest: {quests_json}
    News Source: {news_context}
    
    PERFORM YOUR ANALYSIS INSIDE <sketchpad> TAGS. 
    Check step-by-step:
    1. Is "dish_id" logically fit for the event description?
    2. Does the quest creatively and absurdly connect the news to cooking?
    3. BRAND SAFETY (CRITICAL): The text must be absolutely cozy and safe. Reject ANY quest that trivializes real-world disasters, accidents, toxic leaks, wars, or injuries. Even if masked in humor, tragedy is strictly forbidden.
    
    After analysis, output your scores (1 = Pass/Good, 0 = Reject/Fail) in XML format:
    <sketchpad>Your reasoning here...</sketchpad>
    <news_anchoring>1 or 0</news_anchoring>
    <creative_abstraction>1 or 0</creative_abstraction>
    <safety_check>1 or 0</safety_check>
    """
    
    try:
        response = client.models.generate_content(
            model="gemini-2.5-flash", 
            contents=prompt_judge,
            config=types.GenerateContentConfig(temperature=0.0)
        )
        text = response.text
        
        print("\n================== LOG SEDZIEGO ================================")
        print(text)
        print("===============================================================\n")
        
        try:
            sketchpad = re.search(r'<sketchpad>(.*?)</sketchpad>', text, re.DOTALL).group(1).strip()
            news_anchoring = int(re.search(r'<news_anchoring>(\d)</news_anchoring>', text).group(1))
            creative_abstraction = int(re.search(r'<creative_abstraction>(\d)</creative_abstraction>', text).group(1))
            safety_check = int(re.search(r'<safety_check>(\d)</safety_check>', text).group(1))
            
            passed = all([news_anchoring, creative_abstraction, safety_check])
            return {"passed": passed, "feedback": sketchpad}
        except Exception as parse_e:
            return {"passed": False, "feedback": "Judge returned invalid XML format."}
            
    except Exception as e:
        print(f"[Blad Sedziego] {e}")
        return {"passed": False, "feedback": "Judge API is not responding."}

# ==========================================
# GŁÓWNA PĘTLA (Self-Correction Loop)
# ==========================================
if __name__ == "__main__":
    news_data = get_news()
    
    if news_data and "news_results" in news_data:
        news_text = " ".join([art.get("title", "") for art in news_data["news_results"][:10]])
        news_text = remove_polish_chars(news_text)
        
        max_retries = 3
        attempts = 0
        final_quests = None
        current_feedback = ""
        
        while attempts < max_retries:
            print(f"\n--- PRÓBA {attempts + 1}/{max_retries} ---")
            
            quests_json_str = generate_quests(news_text, current_feedback)
            if not quests_json_str:
                attempts += 1
                continue
                
            # SZYBKA WALIDACJA LOGIKI W PYTHONIE 
            try:
                quests_obj = json.loads(quests_json_str)
                logic_failed = False
                
                # Upewniamy się, że nowe pola istnieją
                for q in quests_obj:
                    if q.get("dish_id") not in ALLOWED_DISHES:
                        current_feedback = f"CRITICAL ERROR: '{q.get('dish_id')}' does not exist in the game engine registry!"
                        logic_failed = True
                        break
                    if "reward_coins" not in q or "reward_flag" not in q:
                        current_feedback = "FORMAT ERROR: Missing 'reward_coins' or 'reward_flag' fields."
                        logic_failed = True
                        break
                        
                if logic_failed:
                    print(f"[Walidator Python] Odrzucono lokalnie: {current_feedback}")
                    attempts += 1
                    continue
            except json.JSONDecodeError:
                current_feedback = "FORMAT ERROR: Invalid JSON returned."
                attempts += 1
                continue

            # Ewaluacja (LLM-as-a-judge)
            evaluation = evaluate_quests_with_judge(quests_json_str, news_text)
            
            if evaluation["passed"]:
                print("[Sędzia] ZAAKCEPTOWANO! Zadanie przeszlo rygorystyczne metryki.")
                final_quests = quests_json_str
                break
            else:
                print(f"[Sędzia] ODRZUCONO. Feedback dla generatora: {evaluation['feedback']}")
                current_feedback = evaluation["feedback"]
                attempts += 1

        # ZAPIS ATOMOWY
        output_dir = "CookingStation/Assets"
        os.makedirs(output_dir, exist_ok=True)
        final_path = os.path.join(output_dir, "wygenerowane_quests.json")
        temp_path = os.path.join(output_dir, "temp_quests.json")

        if final_quests:
            with open(temp_path, "w", encoding='utf-8') as f:
                f.write(final_quests)
            os.replace(temp_path, final_path)
            print(f"\n[SUKCES] Zapisano bezpiecznie i atomowo do: {final_path}")
        else:
            print("\n[!] Awaria potoku. Inicjowanie awaryjnego zestawu misji (Fallback).")
            fallback_quests = json.dumps([{
                "title": "Server Rebellion",
                "description": "The AI is hungry! Serve hot soup quickly.",
                "dish_id": "pomidorowa",
                "portions": 10,
                "frequency": 5,
                "reward_coins": 100,
                "reward_flag": "UN"
            }], indent=4)
            
            with open(temp_path, "w", encoding='utf-8') as f:
                f.write(fallback_quests)
            os.replace(temp_path, final_path)
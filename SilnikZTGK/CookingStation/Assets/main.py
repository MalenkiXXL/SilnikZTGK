import os
import json
import time
from google import genai
from google.genai import types
from newsapi import NewsApiClient
import datetime as dt

#konfiguracja kluczy
api_my_key = "3d259c682589472b95edf767d9cb39e0"
gemini_key = "AIzaSyDcIhsCqHYxWtZgQz7jLSqQcDnHlehjf_0"

#konfiguracja plikow zapisu
cashe_file = "news_cache.json"
cache_expiry_seconds = 3600

#konfiguracja modelu gemini
client = genai.Client(api_key = gemini_key)

def get_news():
    #pobiera newsy z cashe lub z api, jesli cashe wygasl
    if os.path.exists(cashe_file) and os.path.getsize(cashe_file) > 0 and (time.time() - os.path.getmtime(cashe_file)) < cache_expiry_seconds:
        print("Loading cached news data...")
        with open(cashe_file, "r", encoding='utf-8') as f:
            return json.load(f)

    print("Caching news data...")
    try:
        newsapi = NewsApiClient(api_key=api_my_key)
        data = newsapi.get_top_headlines(language='en', page_size=10)
        with open(cashe_file, "w", encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=4)
        return data
    except Exception as e:
        print("Failed to get news data: {}".format(e))
        if os.path.exists(cashe_file):
            with open(cashe_file, "r", encoding='utf-8') as f:
                return json.load(f)
        return None

def generate_quests(news_data):
    #Wysyla newsy do gemini i generuje questy w jsonie
    if not news_data or "articles" not in news_data:
        return "No news data"

    #lista potraw z gry
    # dostepne_potrawy = ["SpiacyChlebel", "GwiezdneKopytka", "ZupaZiemniaczana"]
    # potrawy_str = ", ".join(dostepne_potrawy)

    #filtr bezpieczeństwa (narazie blacklista finalnie przepuszczenie przez drugi llm)
    banned_words = ["kill", "dead", "death", "murder", "victim", "crash", "shoot", "blood", "war", "terror", "rape",
                    "prison"]



    news_text_list = []

    for article in news_data["articles"]:
        title = article.get("title", "")
        desc = article.get("description", " ")
        if not desc:
            desc = ""

        #sprawdzanie czy news zawiera blacklistowe slowa
        combined_text = (title + " " + desc).lower()
        if any(bad_word in combined_text for bad_word in banned_words):
            continue # ignorujemy newsa

        if title:
            news_text_list.append(f"Tytul: {title} | Opis: {desc}")

    if not news_text_list:
        return "No safe news data"

    news_content = "\n".join(news_text_list)
    print("\n Ready messeage to AI: \n" + news_content)

    prompt = f""""
    Wcielasz się w postać szalonego kucharza-wizjonera, którego umysł działa jak w abstrakcyjnym, kulinarnym śnie (w stylu Monty Pythona).
    Na podstawie poniższych prawdziwych wiadomości ze świata, wygeneruj listę 5 zadań (eventów) dla graczy w grze restauracyjnej.
    
    ZASADA GŁÓWNA: Twoje opisy mają być totalnie odjechane, absurdalne i zabawne. Łącz poważne tematy (np. giełda, wybory, technologie) z absurdalnymi problemami kulinarnymi (np. politycy kradnący sos, rakieta napędzana gulaszem).
    
    Zwróć wynik jako tabelę (array) w formacie JSON. Każdy obiekt zadania musi zawierać dokładnie poniższe klucze:

    1. "title": String. Tytuł wydarzenia. Maksymalnie 50 znaków, maksymalnie 6 słów. Zawsze zamieniaj kluczowe słowo z newsa na jedzenie. (Przykład z newsa o NASA: "NASA wystrzeliła ziemniaka na orbitę!").
    2. "description": String. Krótki opis. Maksymalnie 150 znaków, jedno zdanie. Ma brzmieć jak z gorączkowego snu (Przykład: "Elon Musk potrzebuje węglowodanów, aby zasilić silniki rakietowe, szybko podaj mu kluski!").
    4. "portions": Liczba całkowita z przedziału 5-20.
    5. "frequency": Liczba całkowita z przedziału 5-15 (co ilu klientów występuje event).
    6. "reward": String. Nagroda za wykonanie (np. "500 Monet, polska flaga").
    
    Wiadomości:
    {news_content}
    """

    print("\n Generating quests...")

    #wymuszenie odpowiedzi w jsonie
    response = client.models.generate_content(
        model = "gemini-2.5-flash",
        contents=prompt,
        config=types.GenerateContentConfig(
            response_mime_type="application/json",
        )
    )

    return response.text

#uruchomienie
if __name__ == "__main__":
    data = get_news()

    if data:
        quests_json = generate_quests(data)

        print("\n -------- Generated Quests -------- ")
        print(quests_json)

        with open("wygenerowane_quests.json", "w", encoding='utf-8') as f:
            f.write(quests_json)
        print("\n Quests saved to wygenerowane_quests.json")


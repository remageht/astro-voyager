import os
import base64

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
audio_dir = os.path.join(root_dir, "res", "audio", "egorka")
female_dir = os.path.join(root_dir, "res", "audio", "crew_female")
out_html = os.path.join(root_dir, "egorka_voice_player.html")
artifact_html = r"C:\Users\Эмир\.gemini\antigravity\brain\61717e81-a8ae-49ec-a1e1-565f37c911aa\egorka_voice_player.html"

cues = [
    {"file": "sys_online.wav", "title": "1. Системы корабля", "text": "«Приветствую на мостике, капитан! Системы корабля в полном порядке, я готов к полёту. Куда держим курс?»", "trigger": "Старт полёта (через 2 сек)", "color": "#00e5ff", "dir": audio_dir},
    {"file": "warp_on.wav", "title": "2. Варп запущен", "text": "«Варп-привод запущен! Пространство поплыло, держись крепче, капитан — погнали!»", "trigger": "Клавиша [X] / DpadUp (Варп-разгон)", "color": "#b388ff", "dir": audio_dir},
    {"file": "warp_off.wav", "title": "3. Выход из варпа", "text": "«Выходим из варпа! Гравитационная волна рассеяна, мы на досветовой скорости.»", "trigger": "Клавиша [X] (Выход в круиз)", "color": "#7c4dff", "dir": audio_dir},
    {"file": "gear_down.wav", "title": "4. Опоры выпущены", "text": "«Опоры выпущены! Гидравлика в замках, заходим на посадку!»", "trigger": "Клавиша [G] / DpadDown (Посадка)", "color": "#00e676", "dir": audio_dir},
    {"file": "gear_up.wav", "title": "5. Опоры убраны", "text": "«Опоры убраны! Тяга максимальная, выходим на оперативный простор!»", "trigger": "Клавиша [G] / DpadDown (Взлёт)", "color": "#69f0ae", "dir": audio_dir},
    {"file": "dock_ok.wav", "title": "6. Стыковка успешна", "text": "«Есть захват! Стыковка прошла идеально. Отличная работа, капитан, можно выдохнуть.»", "trigger": "Клавиша [E] / Button A у станции", "color": "#00b0ff", "dir": audio_dir},
    {"file": "undock_ok.wav", "title": "7. Замки открыты", "text": "«Замки открыты, мы свободны! Маневровые двигатели в норме, счастливого полёта!»", "trigger": "Клавиша [E] / Button B отлёт", "color": "#40c4ff", "dir": audio_dir},
    {"file": "fuel_low.wav", "title": "8. Тревога топлива", "text": "«Тревога! Топливо на исходе, баки почти пустые! Капитан, срочно нужна дозаправка!»", "trigger": "Остаток бака < 15%", "color": "#ff9100", "dir": audio_dir},
    {"file": "shield_low.wav", "title": "9. Пробой щитов", "text": "«Щиты пробиты! Дефлектор перегружен, мы горим, маневрируй!»", "trigger": "Энергия дефлектора < 20%", "color": "#ff5252", "dir": audio_dir},
    {"file": "terrain_warn.wav", "title": "10. Опасность рельефа", "text": "«Осторожно, земля! Тяни штурвал вверх, сейчас врежемся в скалы!»", "trigger": "Уклон > 20° или сближение < 2.5 км", "color": "#ff1744", "dir": audio_dir},
    {"file": "orbit_ok.wav", "title": "11. Кеплерова орбита", "text": "«Орбитальный захват стабилен! Мы на идеальной кеплеровской дуге, красота-то какая!»", "trigger": "Клавиша [R] (Круговая орбита)", "color": "#ffd600", "dir": audio_dir}
]

dialogues = [
    {"file": "dial_oj287.wav", "title": "1. OJ 287 (Бинарная пара)", "text": "«Глянь на эту бинарную пару! Метрика дрожит. Мы разорвём эту петлю времени раз и навсегда!»", "trigger": "Подлёт к OJ 287 (Егорка-6)", "color": "#f59e0b", "dir": audio_dir},
    {"file": "dial_hyperdisk.wav", "title": "2. Гипердиск (Аккреция)", "text": "«Тонкий аккреционный диск в двадцать шесть радиусов Шварцшильда! Идеальный кеплеровский градиент. Варп-ускорители на полную мощность!»", "trigger": "Подлёт к Гипердиску (Егорка-6)", "color": "#8b5cf6", "dir": audio_dir},
    {"file": "dial_gw170817.wav", "title": "3. Килонова GW170817", "text": "«Линии золота и платины в ультрафиолете! Мы воочию наблюдаем эрд-процесс нуклеосинтеза. Гравитационные волны сошлись с квантовыми расчётами!»", "trigger": "Подлёт к GW170817 (Егорка-5)", "color": "#ec4899", "dir": audio_dir},
    {"file": "dial_sn1987a.wav", "title": "4. Сверхновая SN 1987A", "text": "«Сверхновая в Большом Магеллановом Облаке! Тридцать две жемчужины ударной волны. Наша память о Земле так же ярко вспыхивает в кольцах времени.»", "trigger": "Подлёт к SN 1987A (Егорка-6)", "color": "#06b6d4", "dir": audio_dir},
    {"file": "dial_hard_landing.wav", "title": "5. Жёсткая посадка", "text": "«Удар о поверхность за пределами нормы! Но опора выдержала. Срочно проверьте состояние корпуса!»", "trigger": "Посадка со стойками под перегрузкой", "color": "#ef4444", "dir": audio_dir},
    {"file": "dial_drill_eureka.wav", "title": "6. Эврика керна", "text": "«Спектрометрия керна завершена! Образец из скважины каталогизирован во Втором Мозге. Научный отсек ликует!»", "trigger": "Каталогизация керна во Второй Мозг", "color": "#10b981", "dir": audio_dir}
]

female_tracks = [
    {"file": "mia_m16.wav", "title": "1. Миа Сато · Столпы Творения M16", "text": "«Столпы Творения прямо по курсу... Четыре световых года молекулярного водорода. Здесь рождаются новые звёздные миры.»", "trigger": "Подлёт к M16 (Пилот Миа Сато)", "color": "#f43f5e", "dir": female_dir},
    {"file": "mia_m1.wav", "title": "2. Миа Сато · Пульсар M1 Крабовидный", "text": "«Ого, слышите пульсар M1? Строчит как бешеный метроном — тридцать ударов в секунду! Врубаю дефлекторы, держитесь за кофе!»", "trigger": "Подлёт к пульсару M1 (Пилот Миа Сато)", "color": "#fb7185", "dir": female_dir},
    {"file": "mia_at2019qiz.wav", "title": "3. Миа Сато · Приливной разрыв звезды", "text": "«Вы только гляньте в обзорный блистер... Чёрная дыра буквально рвёт звезду на части! Плазма летит огненным вихрем... Жутко красиво, аж дух захватывает!»", "trigger": "Подлёт к AT2019qiz (Пилот Миа Сато)", "color": "#fda4af", "dir": female_dir},
    {"file": "mia_touchdown.wav", "title": "4. Миа Сато · Мягкая посадка на планету", "text": "«Есть мягкое касание! Стойки в замках, сели как по маслу. Ну что, капитан, чай допиваем — и на выход! Планета сама себя не исследует.»", "trigger": "Идеальная посадка судна (Миа Сато)", "color": "#34d399", "dir": female_dir},
    {"file": "elena_sgra.wav", "title": "5. Елена Ростова · Эргосфера Sgr A*", "text": "«Фиксирую эргосферу Стрельца А-звезда. Пространство закручивается вокруг горизонта событий, синхротронный пик достигнут. Включаю спектрометрию, не упустите данные.»", "trigger": "Подлёт к Sgr A* (Астрофизик Елена Ростова)", "color": "#a855f7", "dir": female_dir},
    {"file": "elena_m87.wav", "title": "6. Елена Ростова · Релятивистский джет M87*", "text": "«Внимание на обзорные экраны. Релятивистский джет M87 бьёт на пять тысяч световых лет прямо по траверзу. Мы летим сквозь струю ускоренной плазмы. Поразительное, грандиозное явление...»", "trigger": "Подлёт к джету M87* (Елена Ростова)", "color": "#c084fc", "dir": female_dir},
    {"file": "aurora_sys_online.wav", "title": "7. ИИ Аврора · Женский бортовой голос", "text": "«Бортовые системы судна активированы. Добро пожаловать на мостик, капитан. Все узлы функционируют штатно.»", "trigger": "Женский бортовой навигатор судна", "color": "#38bdf8", "dir": female_dir},
]

def render_cards(items):
    cards = []
    for t in items:
        p = os.path.join(t["dir"], t["file"])
        # Fallback check
        if not os.path.exists(p):
            p = os.path.join(audio_dir, t["file"])
        if os.path.exists(p):
            with open(p, "rb") as f:
                b = f.read()
            b64 = base64.b64encode(b).decode("utf-8")
            data_uri = f"data:audio/wav;base64,{b64}"
            dur = round((len(b) - 44) / (24000 * 2), 2)
            kb = round(len(b) / 1024)
            status = f"{dur}s · {kb} KB"
            audio_tag = f'<audio controls src="{data_uri}" preload="auto"></audio>'
        else:
            status = "⏳ Синтезируется на GPU..."
            audio_tag = '<p style="color:#eab308;font-size:12px;padding:8px;">⏳ Синтезируется...</p>'

        card = f'''
        <div class="card" style="border-left: 4px solid {t["color"]};">
          <div class="card-header">
            <span class="badge" style="background: {t["color"]}22; color: {t["color"]}; border: 1px solid {t["color"]}55;">{t["file"]}</span>
            <span class="trigger">{t["trigger"]}</span>
          </div>
          <div class="card-title">{t["title"]}</div>
          <div class="card-quote">{t["text"]}</div>
          <div class="card-footer">
            {audio_tag}
            <span class="duration-tag">{status}</span>
          </div>
        </div>
        '''
        cards.append(card)
    return "\n".join(cards)

cues_html = render_cards(cues)
dial_html = render_cards(dialogues)
female_html = render_cards(female_tracks)

html_content = f'''<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <title>Голосовая система экипажа Astro-Voyager — Егорка, Миа Сато, Елена Ростова</title>
  <style>
    * {{ box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }}
    body {{ background: #07090e; color: #e2e8f0; padding: 36px 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }}
    .container {{ max-width: 1020px; width: 100%; }}
    .header {{ text-align: center; margin-bottom: 36px; }}
    .header h1 {{ font-size: 28px; font-weight: 800; color: #38bdf8; letter-spacing: 1px; margin-bottom: 8px; text-transform: uppercase; }}
    .header p {{ color: #94a3b8; font-size: 15px; }}
    .section-title {{ font-size: 19px; font-weight: 700; color: #f8fafc; margin: 34px 0 16px 0; display: flex; align-items: center; gap: 10px; border-bottom: 1px solid #1e293b; padding-bottom: 8px; }}
    .grid {{ display: grid; grid-template-columns: 1fr; gap: 16px; }}
    @media (min-width: 720px) {{ .grid {{ grid-template-columns: 1fr 1fr; }} }}
    .card {{ background: #0f172a; border-radius: 10px; padding: 18px 20px; box-shadow: 0 4px 18px rgba(0,0,0,0.5); display: flex; flex-direction: column; gap: 10px; transition: transform 0.15s ease, box-shadow 0.15s ease; }}
    .card:hover {{ transform: translateY(-2px); box-shadow: 0 8px 24px rgba(0,0,0,0.7); }}
    .card-header {{ display: flex; justify-content: space-between; align-items: center; font-size: 12px; }}
    .badge {{ padding: 3px 8px; border-radius: 4px; font-family: monospace; font-size: 11px; }}
    .trigger {{ color: #64748b; font-size: 11px; }}
    .card-title {{ font-size: 16px; font-weight: 600; color: #f1f5f9; }}
    .card-quote {{ font-size: 14px; line-height: 1.45; color: #38bdf8; font-style: italic; background: rgba(56, 189, 248, 0.08); padding: 10px 14px; border-radius: 6px; }}
    .card-footer {{ display: flex; flex-direction: column; gap: 6px; margin-top: 4px; }}
    audio {{ width: 100%; height: 36px; border-radius: 6px; outline: none; }}
    .duration-tag {{ font-size: 11px; color: #64748b; text-align: right; font-family: monospace; }}
    .folder-info {{ margin-top: 36px; background: #0f172a; border: 1px solid #1e293b; border-radius: 8px; padding: 18px 22px; text-align: center; font-size: 13px; color: #94a3b8; }}
    .folder-info code {{ background: #1e293b; color: #38bdf8; padding: 3px 8px; border-radius: 4px; font-family: monospace; }}
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🎙️ ГОЛОСОВАЯ СИСТЕМА ЭКИПАЖА ASTRO-VOYAGER</h1>
      <p>Синтезированная озвучка: Егорка-6 (Бортинженер), Миа Сато (Пилот), Елена Ростова (Астрофизик), ИИ Аврора</p>
    </div>

    <div class="section-title">
      <span>👩</span>
      <span>Женские реплики экипажа: Миа Сато [Пилот] и Елена Ростова [Астрофизик]</span>
    </div>
    <div class="grid">
      {female_html}
    </div>

    <div class="section-title">
      <span>🚀</span>
      <span>11 Эмоциональных бортовых реплик аннонсера (Егорка-6)</span>
    </div>
    <div class="grid">
      {cues_html}
    </div>

    <div class="section-title">
      <span>📻</span>
      <span>6 Сюжетных радиодиалогов Егорки</span>
    </div>
    <div class="grid">
      {dial_html}
    </div>

    <div class="folder-info">
      📁 Женские аудиодорожки: <code>res\\audio\\crew_female\\</code> · Дорожки Егорки: <code>res\\audio\\egorka\\</code>
    </div>
  </div>
</body>
</html>
'''

with open(out_html, "w", encoding="utf-8") as f:
    f.write(html_content)

if os.path.exists(os.path.dirname(artifact_html)):
    with open(artifact_html, "w", encoding="utf-8") as f:
        f.write(html_content)

print(f"Success: Updated HTML player generated ({len(html_content)} bytes)")

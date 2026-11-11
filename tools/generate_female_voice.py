import os
import shutil
import sys
import torch
import numpy as np
import soundfile as sf
from qwen_tts import Qwen3TTSModel

# Monkey patch for transformers meta tensor handling
from transformers.generation import utils as _gu
_orig_init = getattr(_gu.GenerationMixin, "_maybe_initialize_input_ids_for_generation", None)
if _orig_init is not None:
    def _safe_init(self, inputs=None, bos_token_id=None, model_kwargs=None):
        r = _orig_init(self, inputs, bos_token_id, model_kwargs)
        if isinstance(r, torch.Tensor) and r.device.type == "meta":
            dev = torch.device("cuda:0")
            ie = (model_kwargs or {}).get("inputs_embeds")
            if isinstance(ie, torch.Tensor) and ie.device.type != "meta":
                dev = ie.device
            r = torch.ones_like(r, device=dev)
        return r
    _gu.GenerationMixin._maybe_initialize_input_ids_for_generation = _safe_init

import torchaudio

# Женские реплики экипажа: Миа Сато (Пилот), Елена Ростова (Астрофизик), Бортовой ИИ Аврора
FEMALE_TRACKS = [
    # Миа Сато [Пилот] — живой, задорный, душевный тон пилота с юмором и эмоциями
    ("mia_m16.wav", "Столпы Творения прямо по курсу... Четыре световых года молекулярного водорода. Здесь рождаются новые звёздные миры.", 101, 0.0),
    ("mia_m1.wav", "Ого, слышите пульсар M1? Строчит как бешеный метроном — тридцать ударов в секунду! Врубаю дефлекторы, держитесь за кофе!", 102, 0.0),
    ("mia_at2019qiz.wav", "Вы только гляньте в обзорный блистер... Чёрная дыра буквально рвёт звезду на части! Плазма летит огненным вихрем... Жутко красиво, аж дух захватывает!", 103, 0.0),
    ("mia_touchdown.wav", "Есть мягкое касание! Стойки в замках, сели как по маслу. Ну что, капитан, чай допиваем — и на выход! Планета сама себя не исследует.", 104, 0.0),
    # Елена Ростова [Астрофизик] — глубокий, женственный и зрелый тембр (-1.5 полутона pitch shift), научная элегантность
    ("elena_sgra.wav", "Фиксирую эргосферу Стрельца А-звезда. Пространство закручивается вокруг горизонта событий, синхротронный пик достигнут. Включаю спектрометрию, не упустите данные.", 201, -1.5),
    ("elena_m87.wav", "Внимание на обзорные экраны. Релятивистский джет M87 бьёт на пять тысяч световых лет прямо по траверзу. Мы летим сквозь струю ускоренной плазмы. Поразительное, грандиозное явление...", 202, -1.5),
    # Бортовой ИИ Аврора [Женский бортовой навигатор] — чёткий, уверенный, мелодичный тон
    ("aurora_sys_online.wav", "Бортовые системы судна активированы. Добро пожаловать на мостик, капитан. Все узлы функционируют штатно.", 301, 0.0),
]

def trim_silence(audio: np.ndarray, sr: int, threshold: float = 0.015, pad_ms: int = 150) -> np.ndarray:
    """Обрезает тишину в начале и конце дорожки с запасом pad_ms."""
    pad = int(sr * pad_ms / 1000)
    nonzero = np.where(np.abs(audio) > threshold)[0]
    if len(nonzero) == 0:
        return audio
    start = max(0, nonzero[0] - pad)
    end = min(len(audio), nonzero[-1] + pad)
    return audio[start:end]

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = os.path.join(root_dir, "res", "audio", "crew_female")
    os.makedirs(out_dir, exist_ok=True)

    ref_audio_path = r"C:\Users\Эмир\AppData\Roaming\ReVoice\voices\88ab37f2-b789-44cc-9644-f54288cc6817\b983df40-85b2-498c-88fa-fe3161e34b7e.wav"

    if not os.path.exists(ref_audio_path):
        print(f"Error: Female reference audio not found at {ref_audio_path}")
        sys.exit(1)

    print(f"Loading reference female voice sample (Анна Дубляж) from {ref_audio_path}...")
    ref_data, ref_sr = sf.read(ref_audio_path)
    if ref_data.ndim > 1:
        ref_data = ref_data[:, 0]
    ref_data = ref_data.astype(np.float32)

    model_id = "Qwen/Qwen3-TTS-12Hz-1.7B-Base"
    print(f"Loading {model_id} onto cuda:0...")
    model = Qwen3TTSModel.from_pretrained(
        model_id,
        device_map="cuda:0",
        dtype=torch.bfloat16,
        attn_implementation="eager"
    )
    print("Model successfully loaded!")

    for filename, text, seed, pitch_shift in FEMALE_TRACKS:
        target_path = os.path.join(out_dir, filename)

        print(f"\n[SYNTH FEMALE] Generating {filename} (seed={seed}, pitch_shift={pitch_shift}st): \"{text}\"")
        torch.manual_seed(seed)
        torch.cuda.manual_seed_all(seed)

        # Подготовка акустического референса: для Елены Ростовой применяем питч-шифт для более зрелого тембра
        cur_ref = ref_data
        if pitch_shift != 0.0:
            ref_t = torch.tensor(ref_data, dtype=torch.float32).unsqueeze(0)
            shifted_ref_t = torchaudio.functional.pitch_shift(ref_t, ref_sr, pitch_shift)
            cur_ref = shifted_ref_t.squeeze(0).numpy()

        words = len(text.split())
        calc_tokens = max(130, min(260, words * 14))

        res = model.generate_voice_clone(
            text=text,
            language="Russian",
            ref_audio=(cur_ref, ref_sr),
            x_vector_only_mode=True,
            max_new_tokens=calc_tokens,
            temperature=0.35,
            top_p=0.8,
            repetition_penalty=1.15,
            synced_gpus=False,
        )

        if isinstance(res, tuple):
            wavs, sr = res
        else:
            wavs = res
            sr = 24000

        audio = wavs[0] if isinstance(wavs, (list, tuple)) else wavs
        
        # Для Елены Ростовой также применяем легкую постобработку высоты тона для бархатной зрелости
        if pitch_shift != 0.0:
            audio_t = torch.tensor(audio, dtype=torch.float32).unsqueeze(0)
            audio_shifted = torchaudio.functional.pitch_shift(audio_t, sr, pitch_shift)
            audio = audio_shifted.squeeze(0).numpy()

        trimmed_audio = trim_silence(audio, sr, threshold=0.015, pad_ms=150)

        # Save WAV
        sf.write(target_path, trimmed_audio, sr)
        duration = len(trimmed_audio) / sr
        print(f"[DONE] Saved {target_path} ({duration:.2f}s, {os.path.getsize(target_path)} bytes, raw={len(audio)/sr:.2f}s)")

    # Также копируем в build-gl/Release и build/Release
    for build_sub in [os.path.join("build-gl", "Release"), os.path.join("build", "Release")]:
        b_res = os.path.join(root_dir, build_sub, "res", "audio", "crew_female")
        if os.path.exists(os.path.dirname(os.path.dirname(b_res))):
            os.makedirs(b_res, exist_ok=True)
            for filename, _, _, _ in FEMALE_TRACKS:
                src_f = os.path.join(out_dir, filename)
                dst_f = os.path.join(b_res, filename)
                if os.path.exists(src_f):
                    shutil.copy2(src_f, dst_f)
            print(f"Mirrored female voice tracks to {b_res}")

    print(f"\nAll {len(FEMALE_TRACKS)} female voice tracks successfully regenerated and deployed!")

if __name__ == "__main__":
    main()

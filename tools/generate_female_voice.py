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

# Женские реплики экипажа: Миа Сато (Пилот), Елена Ростова (Астрофизик), Бортовой ИИ Аврора
FEMALE_TRACKS = [
    # Миа Сато [Пилот] — энергичный, решительный тон пилота-исследователя
    ("mia_m16.wav", "Столпы Творения прямо по курсу... Четыре световых года молекулярного водорода. Здесь рождаются новые звёздные миры.", 101),
    ("mia_m1.wav", "Фиксирую релятивистский синхротронный ветер пульсара M1. Тридцать импульсов в секунду! Включаю магнитные дефлекторы.", 102),
    ("mia_at2019qiz.wav", "Приливной разрыв звезды! Потоки плазмы падают по спирали. Какая колоссальная гравитационная мощь!", 103),
    ("mia_touchdown.wav", "Касание грунта подтверждено! Гидравлические стойки заблокированы. Приступаем к научной экспедиции.", 104),
    # Елена Ростова [Астрофизик] — вдумчивый, научный тон
    ("elena_sgra.wav", "Фиксирую эргосферу Стрельца А-звезда. Синхротронное излучение на пике. Включаю спектрометр.", 201),
    ("elena_m87.wav", "Внимание! Релятивистский джет M87-звезда бьёт на пять тысяч световых лет прямо по траверзу. Мы летим сквозь струю ускоренной плазмы!", 202),
    # Бортовой ИИ Аврора [Женский бортовой навигатор] — чёткий, уверенный тон
    ("aurora_sys_online.wav", "Бортовые системы судна активированы. Добро пожаловать на мостик, капитан. Все узлы функционируют штатно.", 301),
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

    for filename, text, seed in FEMALE_TRACKS:
        target_path = os.path.join(out_dir, filename)

        print(f"\n[SYNTH FEMALE] Generating {filename} (seed={seed}): \"{text}\"")
        torch.manual_seed(seed)
        torch.cuda.manual_seed_all(seed)

        words = len(text.split())
        calc_tokens = max(130, min(250, words * 14))

        res = model.generate_voice_clone(
            text=text,
            language="Russian",
            ref_audio=(ref_data, ref_sr),
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
            for filename, _, _ in FEMALE_TRACKS:
                src_f = os.path.join(out_dir, filename)
                dst_f = os.path.join(b_res, filename)
                if os.path.exists(src_f):
                    shutil.copy2(src_f, dst_f)
            print(f"Mirrored female voice tracks to {b_res}")

    print(f"\nAll {len(FEMALE_TRACKS)} female voice tracks successfully regenerated and deployed!")

if __name__ == "__main__":
    main()

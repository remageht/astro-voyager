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

CUES = [
    ("warp_on.wav", "Варп-привод активирован. Выход на сверхсветовой режим."),
    ("warp_off.wav", "Варп-привод отключен. Переход на досветовой круиз."),
    ("gear_down.wav", "Посадочные опоры выпущены. Системы готовы к посадке."),
    ("gear_up.wav", "Посадочные опоры убраны. Режим свободного полета."),
    ("dock_ok.wav", "Стыковка со станцией завершена. Системы зафиксированы."),
    ("undock_ok.wav", "Расстыковка выполнена. Двигатели маневрирования в норме."),
    ("fuel_low.wav", "Внимание! Запас топлива критический. Требуется дозаправка."),
    ("shield_low.wav", "Внимание! Пробой дефлекторного щита."),
    ("terrain_warn.wav", "Внимание: опасное сближение с рельефом."),
    ("orbit_ok.wav", "Кеплеровская орбита стабилизирована."),
    ("sys_online.wav", "Бортовой компьютер Егорка-6 в сети. Все системы корабля функционируют штатно."),
]

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = os.path.join(root_dir, "res", "audio", "egorka")
    os.makedirs(out_dir, exist_ok=True)

    ref_audio_path = r"C:\Users\Эмир\AppData\Roaming\ReVoice\voices\118e32d2-ddb1-4f5b-ae91-c25288da3ad6\683a9965-6172-491e-acae-72dc3b6bec22.wav"
    ref_text = "За последние несколько месяцев я испытал боль, которой наградил меня жизни и боролся с ней физически и психологически. Фух, пон..."

    if not os.path.exists(ref_audio_path):
        print(f"Error: Reference audio not found at {ref_audio_path}")
        sys.exit(1)

    print(f"Loading reference voice sample from {ref_audio_path}...")
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

    seed = 100
    for filename, text in CUES:
        target_path = os.path.join(out_dir, filename)
        if os.path.exists(target_path) and os.path.getsize(target_path) > 1000:
            print(f"[SKIP] {filename} already exists ({os.path.getsize(target_path)} bytes)")
            continue

        print(f"\n[SYNTH] Generating {filename}: \"{text}\"")
        torch.manual_seed(seed)
        torch.cuda.manual_seed_all(seed)
        seed += 1

        res = model.generate_voice_clone(
            text=text,
            language="Russian",
            ref_audio=(ref_data, ref_sr),
            ref_text=ref_text,
            max_new_tokens=600,
            synced_gpus=False,
        )

        if isinstance(res, tuple):
            wavs, sr = res
        else:
            wavs = res
            sr = 24000

        audio = wavs[0] if isinstance(wavs, (list, tuple)) else wavs
        # Save WAV
        sf.write(target_path, audio, sr)
        duration = len(audio) / sr
        print(f"[DONE] Saved {target_path} ({duration:.2f}s, {os.path.getsize(target_path)} bytes)")

    # Copy generated cues to build directories if they exist
    for build_sub in [os.path.join("build-gl", "Release"), os.path.join("build", "Release")]:
        b_res = os.path.join(root_dir, build_sub, "res", "audio", "egorka")
        if os.path.exists(os.path.dirname(os.path.dirname(b_res))):
            os.makedirs(b_res, exist_ok=True)
            for filename, _ in CUES:
                src_f = os.path.join(out_dir, filename)
                dst_f = os.path.join(b_res, filename)
                if os.path.exists(src_f):
                    shutil.copy2(src_f, dst_f)
            print(f"Mirrored voice cues to {b_res}")

    print("\nAll 11 Egorka voice cues successfully generated and deployed!")

if __name__ == "__main__":
    main()

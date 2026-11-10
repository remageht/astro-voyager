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

# 11 эмоциональных бортовых реплик аннонсера
CUES = [
    ("warp_on.wav", "Варп-привод запущен! Пространство поплыло, держись крепче, капитан — погнали!"),
    ("warp_off.wav", "Выходим из варпа! Гравитационная волна рассеяна, мы на досветовой скорости."),
    ("gear_down.wav", "Опоры выпущены! Гидравлика в замках, заходим на посадку!"),
    ("gear_up.wav", "Опоры убраны! Тяга максимальная, выходим на оперативный простор!"),
    ("dock_ok.wav", "Есть захват! Стыковка прошла идеально. Отличная работа, капитан, можно выдохнуть."),
    ("undock_ok.wav", "Замки открыты, мы свободны! Маневровые двигатели в норме, счастливого полёта!"),
    ("fuel_low.wav", "Тревога! Топливо на исходе, баки почти пустые! Капитан, срочно нужна дозаправка!"),
    ("shield_low.wav", "Щиты пробиты! Дефлектор перегружен, мы горим, маневрируй!"),
    ("terrain_warn.wav", "Осторожно, земля! Тяни штурвал вверх, сейчас врежемся в скалы!"),
    ("orbit_ok.wav", "Орбитальный захват стабилен! Мы на идеальной кеплеровской дуге, красота-то какая!"),
    ("sys_online.wav", "Приветствую на мостике, капитан! Системы корабля в полном порядке, я готов к полёту. Куда держим курс?"),
]

# 6 сюжетных радиодиалогов с Егоркой
DIALOGUES = [
    ("dial_oj287.wav", "Глянь на эту бинарную пару! Метрика дрожит. Мы разорвём эту петлю времени раз и навсегда!"),
    ("dial_hyperdisk.wav", "Тонкий аккреционный диск в двадцать шесть радиусов Шварцшильда! Идеальный кеплеровский градиент. Варп-ускорители на полную мощность!"),
    ("dial_gw170817.wav", "Линии золота и платины в ультрафиолете! Мы воочию наблюдаем эрд-процесс нуклеосинтеза. Гравитационные волны сошлись с квантовыми расчётами!"),
    ("dial_sn1987a.wav", "Сверхновая в Большом Магеллановом Облаке! Тридцать две жемчужины ударной волны. Наша память о Земле так же ярко вспыхивает в кольцах времени."),
    ("dial_hard_landing.wav", "Удар о поверхность за пределами нормы! Но опора выдержала. Срочно проверьте состояние корпуса!"),
    ("dial_drill_eureka.wav", "Спектрометрия керна завершена! Образец из скважины каталогизирован во Втором Мозге. Научный отсек ликует!"),
]

def main():
    force_gen = "--force" in sys.argv or True

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

    all_tracks = CUES + DIALOGUES
    seed = 200
    for filename, text in all_tracks:
        target_path = os.path.join(out_dir, filename)

        print(f"\n[SYNTH] Generating {filename}: \"{text}\"")
        torch.manual_seed(seed)
        torch.cuda.manual_seed_all(seed)
        seed += 1

        res = model.generate_voice_clone(
            text=text,
            language="Russian",
            ref_audio=(ref_data, ref_sr),
            ref_text=ref_text,
            max_new_tokens=800,
            temperature=0.8,
            top_p=0.9,
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
            for filename, _ in all_tracks:
                src_f = os.path.join(out_dir, filename)
                dst_f = os.path.join(b_res, filename)
                if os.path.exists(src_f):
                    shutil.copy2(src_f, dst_f)
            print(f"Mirrored voice cues to {b_res}")

    print(f"\nAll {len(all_tracks)} emotional Egorka voice cues and dialogues successfully generated and deployed!")

if __name__ == "__main__":
    main()

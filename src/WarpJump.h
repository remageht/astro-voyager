#pragma once

// Кинематографичный варп-прыжок: катсцена поверх варп-статусов (агент J).
// Чистая логика без GL/ImGui (как Tour): время, фазы и переходы живут здесь,
// камеру, корабль и отрисовку ведёт AppGL + SceneManager-хвост. Харнесс
// скриншотов катсцену не видит: по умолчанию контроллер в Idle, дефолтные
// u_Burst = 0 и скрытый корабль дают попиксельно старый кадр.

#include <string>

namespace astro {

enum class JumpPhase {
  Idle,      // катсцена выключена (дефолт; скриншоты чистые)
  Charge,    // зарядка 1.0с (FP стоит, гул нарастает)
  Pullback,  // отъезд FP->TP 1.2с (камера отъезжает за корабль)
  Burst,     // вспышка 0.5с (flash-пик в середине, там же телепорт)
  Travel,    // туннель 3.5с (вид сзади, корабль в кадре)
  Arrival,   // прибытие 0.4с (rumble гаснет, u_Burst спадает)
  Pushin,    // возврат TP->FP 1.2с (интерполяция к запомненному FP)
  Cooldown   // остывание 1.5с (управление уже возвращено)
};

struct JumpEvent {
  bool switchStation = false;  // в этом update пора сменить станцию (пик Burst)
  // v3.6.0: астероидное поле в Travel (~25% прыжков). Enter — предупреждение,
  // Dodged — манёвр удался (накоплено kAstDodgeDwell секунд уклонения),
  // Hit — окно опасности кончилось без достаточного уклонения: урон щиту.
  bool asteroidEnter = false;
  bool asteroidDodged = false;
  bool asteroidHit = false;
  bool finished = false;       // катсцена завершена (конец Cooldown -> Idle)
};

class WarpJump {
 public:
  static constexpr float kChargeDur = 1.0f;    // зарядка, с
  static constexpr float kPullbackDur = 1.2f;  // отъезд камеры, с
  static constexpr float kBurstDur = 0.5f;     // вспышка, с
  static constexpr float kTravelDur = 3.5f;    // туннель, с
  static constexpr float kArrivalDur = 0.4f;   // прибытие, с
  static constexpr float kPushinDur = 1.2f;    // возврат камеры, с
  static constexpr float kCooldownDur = 1.5f;  // остывание, с

  // === v3.6.0: астероидное поле в Travel === (пасс D). Доля прыжков с
  // обломками; детерминированный роул 0..1 подаёт вызывающий (RetailN или
  // тест), так что вся логика чистая и тестируется без GPU.
  static constexpr float kAstChance = 0.25f;   // < роул => поле в прыжке
  static constexpr float kAstEnterT = 0.14f;   // Travel t01 предупреждения
  static constexpr float kAstDangerStart = 0.20f;  // окно опасности, t01
  static constexpr float kAstDangerEnd = 0.62f;
  static constexpr float kAstDodgeDwell = 0.25f;  // суммарный манёвр, с
  static constexpr float kAstDamage = 18.0f;   // урон щиту при столкновении

  WarpJump() = default;

  // Старт прыжка к станции target. Пустой target = остаться в Idle.
  // astRoll01 — детерминированный ролл астероидного поля (0..1; поле
  // стартует при значении < kAstChance). Дефолт 1.0 = поля нет.
  void start(const std::string& target, float astRoll01 = 1.0f);
  // Мгновенное прерывание (ESC): всегда в Idle, зацикливаний нет.
  void stop();

  bool active() const { return m_phase != JumpPhase::Idle; }
  JumpPhase phase() const { return m_phase; }
  float phaseTime() const { return m_t; }
  const std::string& target() const { return m_target; }
  // v3.6.0: астероидное поле запланировано на этот прыжок (Travel).
  bool asteroidField() const { return m_astField; }
  // Направление уклонения от AppGL каждый кадр (Travel): -1 = A (влево),
  // +1 = D (вправо), 0 = без ввода. Дрейф суммируется в kAstDodgeDwell.
  void setDodgeAxis(float axis);

  // 0..1, прогресс текущей фазы (Idle = 0). Длительность фазы — phaseDur().
  float phaseT01() const;
  // Длительность фазы в секундах (Idle = 0).
  static float phaseDur(JumpPhase p);

  // Продвинуть state machine на dt секунд (dt клампится в [0, 0.5]).
  // Idle = no-op. switchStation возвращается ровно одним событием за пролёт
  // (в пик Burst, t = 0.5 длительности). finished — в конце Cooldown.
  JumpEvent update(float dt);

  // Конверты фаз (все принимают 0..1, клампят вход; ease in-out):
  // pullK — 0->1 отъезд камеры (Pullback) и 1->0 возврат (через pushK);
  // burstEnv — 0->1->0 вспышка внутри Burst (пик ровно 1 в середине);
  // travelK — ровная полка 1 на весь Travel (туннель уже на максимуме);
  // pushK — 0->1 возврат камеры (Pushin), зеркало pullK.
  static float pullK(float t01);
  static float burstEnv(float t01);
  static float travelK(float t01);
  static float pushK(float t01);

 private:
  JumpPhase m_phase = JumpPhase::Idle;
  float m_t = 0.0f;
  std::string m_target;
  bool m_switched = false;  // switchStation уже выдан за этот пролёт

  // v3.6.0: состояние астероидного поля этого прыжка (все сбрасываются
  // в start/stop и не выходят за пределы одной катсцены).
  bool m_astField = false;
  bool m_astEntered = false;   // предупреждение уже выдано
  bool m_astDodgedDone = false;  // манёвр засчитан
  bool m_astHitDone = false;   // столкновение уже засчитано
  float m_astDodgeT = 0.0f;    // накопленное уклонение в окне опасности, с
  float m_dodgeAxis = 0.0f;    // последний ввод игрока (см. setDodgeAxis)
};

}  // namespace astro

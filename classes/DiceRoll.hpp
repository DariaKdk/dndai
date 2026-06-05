/**
* @file DiceRoll.hpp
 * @brief Вкладка броска d20 с анимацией.
 */

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <vector>

using namespace ftxui;

/**
 * @brief Вкладка броска d20: ASCII‑анимация пятиугольной кости.
 *
 * При нажатии кнопки генерирует случайное число от 1 до 20,
 * проигрывает анимацию смены чисел и отображает финальный результат.
 */
class DiceRoll : public ComponentBase {
public:
    DiceRoll();

    /**
     * @brief Возвращает результат последнего броска.
     * @return Целое число от 1 до 20, или 0 если бросков ещё не было.
     */
    int GetLastRoll() const;

private:
    Element RenderDie(int number) const;  ///< Рисует ASCII‑кубик с заданным числом.

    int display_number_ = 1;     ///< Число, отображаемое в данный момент (в анимации или финал).
    int last_roll_ = 0;          ///< Результат последнего завершённого броска.
    bool rolling_ = false;       ///< Идёт ли анимация.
    std::vector<int> anim_numbers_;  ///< Массив промежуточных чисел для анимации.
    int anim_result_ = 0;            ///< Финальный результат броска.
    std::chrono::steady_clock::time_point anim_start_;  ///< Момент начала анимации.
    Component roll_btn_;             ///< Кнопка «Бросить d20».
};
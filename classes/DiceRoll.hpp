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

/// @brief Вкладка броска d20: ASCII-пятиугольник, анимация, результат.
class DiceRoll : public ComponentBase {
public:
    DiceRoll();

    /// @brief Возвращает результат последнего броска (0 если ещё не бросали).
    int GetLastRoll() const;

private:
    Element RenderDie(int number) const;

    int display_number_ = 1;
    int last_roll_ = 0;
    bool rolling_ = false;
    std::vector<int> anim_numbers_;
    int anim_result_ = 0;
    std::chrono::steady_clock::time_point anim_start_;
    Component roll_btn_;
};

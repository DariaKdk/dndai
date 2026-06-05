/**
* @file CharList.hpp
 * @brief Вкладка листа персонажей D&D 5e.
 */

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

using namespace ftxui;

/// @brief Данные персонажа D&D 5e.
struct Character {
    std::string name = "Новый персонаж";
    std::string race = "Человек";
    std::string char_class = "Воин";
    int level = 1;

    int str = 10, dex = 10, con = 10;
    int intel = 10, wis = 10, cha = 10;

    int hp_max = 10, hp_current = 10;
    int ac = 10;
    int speed = 30;

    std::vector<std::string> equipment;
    std::string background;
};

/// @brief Вкладка со списком персонажей D&D, редактированием и JSON-сохранением.
class CharList : public ComponentBase {
public:
    CharList();

    /// @brief Сохраняет всех персонажей в JSON-файл.
    void SaveToFile(const std::string& path) const;

    /// @brief Загружает персонажей из JSON-файла.
    void LoadFromFile(const std::string& path);

    /// @brief Возвращает текущего персонажа как JSON-строку.
    std::string SaveToJson() const;

private:
    void SyncToCharacter();
    void SyncFromCharacter();

    std::vector<Character> characters_;
    int selected_ = 0;

    std::string name_buf_, race_buf_, class_buf_, level_buf_;
    std::string str_buf_, dex_buf_, con_buf_, intel_buf_, wis_buf_, cha_buf_;
    std::string hp_max_buf_, hp_cur_buf_, ac_buf_, speed_buf_;
    std::string bg_buf_;
    std::string equip_buf_;
    std::string file_path_;

    std::vector<std::string> equipment_;
    int sel_equip_ = 0;

    Component name_in_, race_in_, class_in_, level_in_;
    Component str_in_, dex_in_, con_in_, intel_in_, wis_in_, cha_in_;
    Component hp_max_in_, hp_cur_in_, ac_in_, speed_in_;
    Component bg_in_;
    Component equip_in_;
    Component file_in_;
    Component equip_menu_;
    Component prev_btn_, next_btn_, add_btn_, del_btn_;
    Component save_btn_, load_btn_;
    Component add_equip_btn_, del_equip_btn_;

    int scroll_offset_ = 0;
    int content_height_ = 0;
};
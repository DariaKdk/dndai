/**
 * @file CharList.hpp
 * @brief Вкладка листа персонажей D&D 5e с редактированием и JSON‑сохранением.
 */

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

using namespace ftxui;

/// @brief Данные персонажа D&D 5e.
struct Character {
    std::string name = "Новый персонаж";       ///< Имя персонажа.
    std::string race = "Человек";              ///< Раса.
    std::string char_class = "Воин";           ///< Класс.
    int level = 1;                             ///< Уровень.

    int str = 10, dex = 10, con = 10;          ///< Сила, Ловкость, Телосложение.
    int intel = 10, wis = 10, cha = 10;        ///< Интеллект, Мудрость, Харизма.

    int hp_max = 10, hp_current = 10;          ///< Максимальные и текущие ОЗ.
    int ac = 10;                               ///< Класс доспеха.
    int speed = 30;                            ///< Скорость (в футах).

    std::vector<std::string> equipment;        ///< Список снаряжения.
    std::string background;                    ///< Предыстория.
};

/**
 * @brief Вкладка со списком персонажей D&D.
 *
 * Позволяет создавать, редактировать, удалять персонажей, сохранять и загружать их в/из JSON‑файла.
 * Является компонентом FTXUI, может быть встроен в интерфейс.
 */
class CharList : public ComponentBase {
public:
    CharList();

    /**
     * @brief Сохраняет всех персонажей в JSON‑файл.
     * @param path Путь к файлу (например, "characters.json").
     * @throws std::runtime_error Если файл не удаётся открыть.
     */
    void SaveToFile(const std::string& path) const;

    /**
     * @brief Загружает персонажей из JSON‑файла.
     * @param path Путь к файлу.
     * @throws std::runtime_error Если файл не удаётся открыть или JSON некорректен.
     */
    void LoadFromFile(const std::string& path);

    /**
     * @brief Возвращает JSON‑строку текущего выбранного персонажа.
     * @return Строка в формате JSON, или "{}" если ни один персонаж не выбран.
     */
    std::string SaveToJson() const;

private:
    void SyncToCharacter();    ///< Сохраняет данные из полей ввода в объект Character.
    void SyncFromCharacter();  ///< Загружает данные из текущего Character в поля ввода.

    std::vector<Character> characters_;  ///< Список всех персонажей.
    int selected_ = 0;                   ///< Индекс выбранного персонажа.

    // Буферы для полей ввода
    std::string name_buf_, race_buf_, class_buf_, level_buf_;
    std::string str_buf_, dex_buf_, con_buf_, intel_buf_, wis_buf_, cha_buf_;
    std::string hp_max_buf_, hp_cur_buf_, ac_buf_, speed_buf_;
    std::string bg_buf_;
    std::string equip_buf_;
    std::string file_path_;

    std::vector<std::string> equipment_;  ///< Временный список снаряжения для редактора.
    int sel_equip_ = 0;                   ///< Выбранный элемент в списке снаряжения.

    // Компоненты FTXUI
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

    int scroll_offset_ = 0;    ///< Текущее смещение прокрутки (для vscroll).
    int content_height_ = 0;   ///< Высота контента (используется для прокрутки).
};
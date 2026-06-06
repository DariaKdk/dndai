/**
 * @file main.cpp
 * @brief Точка входа в приложение D&D AI помощника.
 *
 * Приложение предоставляет три вкладки:
 * - Чат для общения с AI‑мастером подземелий (использует языковую модель Falcon).
 * - Редактор персонажей D&D 5e (CharList).
 * - Бросок d20 с анимацией (DiceRoll).
 *
 * Требования: модель должна находиться в той же папке, что и исполняемый файл,
 * или путь можно задать в коде (см. main.cpp).
 *
 * Для работы необходимо скачать модель Falcon-H1-1.5B-Instruct-Q4_K_s.gguf с Hugging Face
 */

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <vector>

#include "classes/ChatController.hpp"
#include "classes/CharList.hpp"
#include "classes/DiceRoll.hpp"

using namespace ftxui;

/**
 * @brief Главная функция.
 *
 * Создаёт экран FTXUI, инициализирует контроллер чата с указанием пути к модели,
 * создаёт вкладки, объединяет их в меню и запускает основной цикл интерфейса.
 * @return 0 при успешном завершении.
 */
int main() {
    auto screen = ScreenInteractive::TerminalOutput();

    // Заголовки вкладок
    std::vector<std::string> tab_titles = {"Чат", "Чарлист", "d20"};
    int selected_tab = 0;

    // Контроллер чата – путь к модели (файл должен существовать)
    auto controller = std::make_shared<ChatController>("Falcon-H1-1.5B-Instruct-Q4_K_S.gguf");
    auto chat_tab = controller->GetChatTab();

    // Остальные вкладки
    auto char_list_tab = std::make_shared<CharList>();
    auto dice_roll_tab = std::make_shared<DiceRoll>();

    // Передаём контроллеру указатели на другие вкладки (для доступа к их данным)
    controller->SetCharList(char_list_tab.get());
    controller->SetDiceRoll(dice_roll_tab.get());

    // Компоненты вкладок
    std::vector<Component> tab_components = {chat_tab, char_list_tab, dice_roll_tab};

    auto tabs = Container::Tab(tab_components, &selected_tab);
    auto tab_menu = Menu(&tab_titles, &selected_tab, MenuOption::HorizontalAnimated());

    auto layout = Container::Vertical({tab_menu, tabs});

    auto renderer = Renderer(layout, [&] {
        return vbox({
                   tab_menu->Render() | hcenter | border,
                   tabs->Render() | flex,
               }) | flex;
    });

    screen.Loop(renderer);
    return 0;
}

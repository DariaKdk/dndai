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


int main() {
    auto screen = ScreenInteractive::TerminalOutput();

    std::vector<std::string> tab_titles = {"Чат", "Чарлист", "d20"};
    int selected_tab = 0;

    auto controller = std::make_shared<ChatController>("Falcon-H1-1.5B-Instruct-Q4_K_S.gguf");
    auto chat_tab = controller->GetChatTab();

    auto char_list_tab = std::make_shared<CharList>();

    auto dice_roll_tab = std::make_shared<DiceRoll>();

    controller->SetCharList(char_list_tab.get());
    controller->SetDiceRoll(dice_roll_tab.get());

    std::vector<Component> tab_components = { chat_tab, char_list_tab, dice_roll_tab };

    auto tabs = Container::Tab(tab_components, &selected_tab);
    auto tab_menu = Menu(&tab_titles, &selected_tab, MenuOption::HorizontalAnimated());

    auto layout = Container::Vertical({ tab_menu, tabs });

    auto renderer = Renderer(layout, [&] {
        return vbox({
            tab_menu->Render() | hcenter | border,
            tabs->Render() | flex,
        }) | flex;
    });
    screen.Loop(renderer);
    return 0;
}

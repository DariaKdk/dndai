/**
* @file ChatController.hpp
 * @brief Связывает ChatTab (UI) и LLamaInterface.
 */

#pragma once

#include "ChatTab.hpp"
#include "LLamaInterface.hpp"
#include <memory>
#include <string>
#include <thread>

class CharList;
class DiceRoll;

/// @brief Контроллер чата: обрабатывает отправку, запускает генерацию в отдельном потоке.
class ChatController {
public:
    explicit ChatController(const std::string& model_path);
    ~ChatController();

    std::shared_ptr<ChatTab> GetChatTab();
    LLamaInterface& GetLLama();

    void SetCharList(CharList* char_list);
    void SetDiceRoll(DiceRoll* dice_roll);

private:
    void OnUserSend(const std::string& text);
    std::string BuildContext() const;

    LLamaInterface llama_;
    std::shared_ptr<ChatTab> chat_tab_;
    std::thread generation_thread_;

    CharList* char_list_ = nullptr;
    DiceRoll* dice_roll_ = nullptr;
};
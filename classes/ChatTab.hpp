/**
* @file ChatTab.hpp
 * @brief Вкладка чата с полем ввода и прокруткой сообщений.
 */

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

using namespace ftxui;

/// @brief Текстовое сообщение в чате.
struct ChatMessage {
    std::string author;                    ///< Автор.
    std::string text;                      ///< Текст.
    Color author_color = Color::CyanLight; ///< Цвет имени автора.
};

/// @brief Вкладка чата: список сообщений, поле ввода, кнопка отправки.
/// Поддерживает потоковый вывод токенов.
class ChatTab : public ComponentBase {
public:
    ChatTab();

    /// @brief Добавляет сообщение в чат.
    void AddMessage(const std::string& author, const std::string& text,
                    Color author_color = Color::CyanLight);

    /// @brief Добавляет текст к последнему сообщению (для потокового ввода).
    void AppendToLastMessage(const std::string& token);

    /// @brief Включает/выключает режим генерации (блокирует ввод).
    void SetGenerating(bool generating);

    /// @brief Устанавливает callback, вызываемый при отправке сообщения.
    void SetOnSendMessage(std::function<void(const std::string&)> callback);

private:
    void RebuildMessages();
    void SendMessage();
    Element RenderMessages();

    std::vector<ChatMessage> messages_;
    std::string input_text_;
    Component input_component_;
    Component send_button_;
    Component messages_view_;
    std::function<void(const std::string&)> on_send_;

    int scroll_offset_ = 0;
    int content_height_ = 0;
    bool auto_scroll_ = true;

    std::mutex messages_mutex_;
    std::atomic<bool> generating_{false};
};
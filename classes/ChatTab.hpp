/**
 * @file ChatTab.hpp
 * @brief вкладка чата с полем ввода и потоковым выводом сообщений.
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

/// @brief одно сообщение в чате.
struct ChatMessage {
    std::string author; ///< имя автора ("Вы" или "AI").
    std::string text; ///< текст сообщения.
    Color author_color = Color::CyanLight; ///< цвет имени автора.
};

/**
 * @brief вкладка чата: список сообщений, поле ввода, кнопка отправки.
 *
 * поддерживает потоковое добавление текста (по токенам) и автоматическую прокрутку.
 * использует мьютекс для доступа к сообщениям.
 */
class ChatTab : public ComponentBase {
public:
    ChatTab();

    /**
     * @brief добавляет готовое сообщение в чат.
     * @param author имя автора.
     * @param text Ттекст сообщения.
     * @param author_color цвет имени (по умолчанию голубой).
     */
    void AddMessage(const std::string &author, const std::string &text,
                    Color author_color = Color::CyanLight);

    /**
     * @brief добавляет токен к последнему сообщению.
     * используется для потокового вывода ответа.
     * @param token строка-токен.
     */
    void AppendToLastMessage(const std::string &token);

    /**
     * @brief включает/выключает режим генерации.
     * во время генерации блокируется ввод и кнопка отправки.
     * @param generating true – начать генерацию, false – закончить.
     */
    void SetGenerating(bool generating);

    /**
     * @brief устанавливает callback, вызываемый при отправке сообщения.
     * @param callback функция, принимающая текст сообщения.
     */
    void SetOnSendMessage(std::function<void(const std::string &)> callback);

private:
    void RebuildMessages(); ///< вызывает перерисовку интерфейса (отправляет событие).
    void SendMessage(); ///< отправляет сообщение.
    Element RenderMessages(); ///< создаёт элемент со всеми сообщениями.

    std::vector<ChatMessage> messages_; ///< все сообщения чата.
    std::string input_text_; ///< текст в поле ввода.

    Component input_component_; ///< поле ввода.
    Component send_button_; ///< кнопка отправки.
    Component messages_view_; ///< область отображения сообщений.

    std::function<void(const std::string &)> on_send_; ///< callback на отправку.

    int scroll_offset_ = 0; ///< отступ прокрутки.
    int content_height_ = 0; ///< общая высота контента.
    bool auto_scroll_ = true; ///< автоматически прокручивать вниз при новых сообщениях.

    std::mutex messages_mutex_; ///< мьютекс для безопасного доступа к messages_.
    std::atomic<bool> generating_{false}; ///< флаг генерации ответа.
};

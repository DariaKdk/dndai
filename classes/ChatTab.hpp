/**
 * @file ChatTab.hpp
 * @brief Вкладка чата с полем ввода и потоковым выводом сообщений.
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

/// @brief Одно сообщение в чате.
struct ChatMessage {
    std::string author;                    ///< Имя автора (например "Вы" или "AI").
    std::string text;                      ///< Текст сообщения.
    Color author_color = Color::CyanLight; ///< Цвет имени автора.
};

/**
 * @brief Вкладка чата: список сообщений, поле ввода, кнопка отправки.
 *
 * Поддерживает потоковое добавление текста (по токенам) и автоматическую прокрутку.
 * Потокобезопасна (использует мьютекс для доступа к сообщениям).
 */
class ChatTab : public ComponentBase {
public:
    ChatTab();

    /**
     * @brief Добавляет готовое сообщение в чат.
     * @param author Имя автора.
     * @param text Текст сообщения.
     * @param author_color Цвет имени (по умолчанию светло‑голубой).
     */
    void AddMessage(const std::string& author, const std::string& text,
                    Color author_color = Color::CyanLight);

    /**
     * @brief Добавляет фрагмент (токен) к последнему сообщению.
     * Используется для потокового вывода ответа нейросети.
     * @param token Строка-токен (например, слово или часть слова).
     */
    void AppendToLastMessage(const std::string& token);

    /**
     * @brief Включает/выключает режим генерации.
     * Во время генерации блокируется ввод и кнопка отправки.
     * @param generating true – начать генерацию, false – закончить.
     */
    void SetGenerating(bool generating);

    /**
     * @brief Устанавливает callback, вызываемый при отправке сообщения.
     * @param callback Функция, принимающая текст сообщения.
     */
    void SetOnSendMessage(std::function<void(const std::string&)> callback);

private:
    void RebuildMessages();    ///< Вызывает перерисовку интерфейса (отправляет событие).
    void SendMessage();        ///< Отправляет сообщение (вызывается по кнопке или Enter).
    Element RenderMessages();  ///< Создаёт элемент FTXUI со всеми сообщениями.

    std::vector<ChatMessage> messages_;  ///< Все сообщения чата.
    std::string input_text_;             ///< Текущий текст в поле ввода.

    Component input_component_;  ///< Поле ввода.
    Component send_button_;      ///< Кнопка отправки.
    Component messages_view_;    ///< Область отображения сообщений.

    std::function<void(const std::string&)> on_send_;  ///< Callback на отправку.

    int scroll_offset_ = 0;    ///< Отступ прокрутки.
    int content_height_ = 0;   ///< Общая высота контента.
    bool auto_scroll_ = true;  ///< Автоматически прокручивать вниз при новых сообщениях.

    std::mutex messages_mutex_;            ///< Мьютекс для потокобезопасного доступа к messages_.
    std::atomic<bool> generating_{false};  ///< Флаг генерации ответа.
};
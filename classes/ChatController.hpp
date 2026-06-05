/**
* @file ChatController.hpp
 * @brief Контроллер, связывающий ChatTab и LLamaInterface.
 */

#pragma once

#include "ChatTab.hpp"
#include "LLamaInterface.hpp"
#include <memory>
#include <string>
#include <thread>

class CharList;
class DiceRoll;

/**
 * @brief Контроллер чата: обрабатывает отправку, запускает генерацию в отдельном потоке.
 *
 * При получении сообщения от ChatTab формирует контекст (персонаж + бросок),
 * запускает генерацию ответа и передаёт токены обратно в чат.
 */
class ChatController {
public:
    /**
     * @brief Конструктор.
     * @param model_path Путь к GGUF‑файлу языковой модели.
     * @throws LLamaLoadError Если модель не удалось загрузить.
     */
    explicit ChatController(const std::string& model_path);

    ~ChatController();

    /// @brief Возвращает указатель на объект ChatTab.
    std::shared_ptr<ChatTab> GetChatTab();

    /// @brief Возвращает ссылку на внутренний объект LLamaInterface.
    LLamaInterface& GetLLama();

    /// @brief Устанавливает указатель на CharList для получения JSON персонажа.
    void SetCharList(CharList* char_list);

    /// @brief Устанавливает указатель на DiceRoll для получения последнего броска.
    void SetDiceRoll(DiceRoll* dice_roll);

private:
    void OnUserSend(const std::string& text);          ///< Обработчик отправленного сообщения.
    std::string BuildContext() const;                  ///< Формирует строку с персонажем и броском.

    LLamaInterface llama_;                             ///< Интерфейс к LLM.
    std::shared_ptr<ChatTab> chat_tab_;                ///< Вкладка чата.
    std::thread generation_thread_;                    ///< Поток для асинхронной генерации.

    CharList* char_list_ = nullptr;   ///< Указатель на редактор персонажей.
    DiceRoll* dice_roll_ = nullptr;   ///< Указатель на виджет броска кубика.
};
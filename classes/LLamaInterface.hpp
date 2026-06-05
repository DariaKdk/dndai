/**
 * @file LLamaInterface.hpp
 * @brief Обёртка над llama.cpp для чата с GGUF‑моделями.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct llama_model;
struct llama_context;
struct llama_sampler;

// ---------- Исключения ----------
/// @brief Базовое исключение для всех ошибок LLamaInterface.
class LLamaException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Не удалось загрузить модель.
class LLamaLoadError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Модель не загружена, но вызван метод, требующий её.
class LLamaNotLoadedError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка применения chat‑template.
class LLamaTemplateError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка токенизации.
class LLamaTokenizeError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Не хватает места в контексте (переполнение KV‑кэша).
class LLamaContextOverflowError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка при llama_decode.
class LLamaInferenceError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка при сохранении/загрузке состояния.
class LLamaStateError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/**
 * @brief Интерфейс для чата с LLM через llama.h C API.
 *
 * Управляет загрузкой модели, историей диалога, потоковой генерацией.
 * Автоматически ограничивает историю последними 6 сообщениями.
 */
class LLamaInterface {
public:
    /// @brief Параметры модели и генерации.
    struct Config {
        std::string model_path;       ///< Путь к .gguf файлу.
        uint32_t n_ctx       = 4096;  ///< Размер контекста (токенов).
        uint32_t n_batch     = 512;   ///< Размер батча для инференса.
        int32_t  n_gpu_layers = 0;    ///< Количество слоёв на GPU (0 = только CPU).
        float    temperature  = 0.8f; ///< Температура (0 = жадная выборка).
        float    top_p        = 0.95f;///< Top‑P (nucleus sampling).
        int32_t  top_k        = 40;   ///< Top‑K.
        int32_t  n_threads    = -1;   ///< Количество потоков (-1 = авто).
        int32_t  max_tokens   = 4096; ///< Максимум токенов в ответе.
    };

    /// Callback для потокового вывода. Вернуть false, чтобы остановить генерацию.
    using TokenCallback = std::function<bool(const std::string& piece)>;

    LLamaInterface();
    ~LLamaInterface();

    // Запрещаем копирование, разрешаем перемещение
    LLamaInterface(const LLamaInterface&) = delete;
    LLamaInterface& operator=(const LLamaInterface&) = delete;
    LLamaInterface(LLamaInterface&&) noexcept;
    LLamaInterface& operator=(LLamaInterface&&) noexcept;

    /**
     * @brief Загрузить модель из файла.
     * @param config Параметры загрузки.
     * @throws LLamaLoadError Если файл не найден или модель не загрузилась.
     */
    void LoadModel(const Config& config);

    /// @brief Проверить, загружена ли модель.
    bool IsLoaded() const;

    /// @brief Выгрузить модель и освободить ресурсы.
    void Unload();

    /// @brief Установить системный промпт (инструкции для модели).
    void SetSystemPrompt(const std::string& prompt);

    /**
     * @brief Отправить сообщение пользователя и получить ответ.
     * @param user_message Текст сообщения.
     * @param callback Опциональный потоковый вывод (вызывается на каждый токен).
     * @return Полный текст ответа модели.
     * @throws LLamaNotLoadedError Если модель не загружена.
     * @throws LLamaTemplateError, LLamaTokenizeError, LLamaContextOverflowError, LLamaInferenceError.
     */
    std::string Chat(const std::string& user_message,
                     TokenCallback callback = nullptr);

    /**
     * @brief Сохранить состояние (KV‑кэш и историю) в файлы.
     * @param base_path Базовое имя файла (к нему добавятся .state и .history).
     * @throws LLamaNotLoadedError, LLamaStateError.
     */
    void SaveState(const std::string& base_path) const;

    /**
     * @brief Восстановить состояние из файлов.
     * @param base_path Базовое имя файла.
     * @throws LLamaNotLoadedError, LLamaStateError.
     */
    void LoadState(const std::string& base_path);

    /// @brief Очистить историю диалога и KV‑кэш.
    void ClearHistory();

    /// @brief Вернуть путь к загруженной модели.
    const std::string& GetModelPath() const;

    /// @brief Вернуть историю сообщений (пары role, content).
    const std::vector<std::pair<std::string, std::string>>& GetHistory() const;

    /**
     * @brief (Только для тестирования) Добавить сообщение в историю вручную.
     * @param role Роль ("user", "assistant", "system").
     * @param content Текст сообщения.
     */
    void AddTestMessage(const std::string& role, const std::string& content) {
        messages_.push_back({role, content});
    }

private:
    void BuildSampler();          ///< Инициализирует сэмплер (top_k, top_p, temperature).
    void FreeResources() noexcept;///< Освобождает все ресурсы llama.cpp.

    void SaveHistory(const std::string& path) const;   ///< Сохранить историю в бинарный файл.
    void LoadHistory(const std::string& path);         ///< Загрузить историю из бинарного файла.

    llama_model*   model_   = nullptr;  ///< Указатель на загруженную модель.
    llama_context* ctx_     = nullptr;  ///< Контекст (состояние инференса).
    llama_sampler* sampler_ = nullptr;  ///< Сэмплер для выбора токенов.

    Config config_;                               ///< Текущая конфигурация.
    std::string system_prompt_;                   ///< Системный промпт.
    std::vector<std::pair<std::string, std::string>> messages_;  ///< История диалога.

    size_t prev_template_len_ = 0;                ///< Длина предыдущего отформатированного шаблона.
    std::vector<char> formatted_;                 ///< Буфер для форматирования шаблона.

    uint32_t n_ctx_ = 0;                          ///< Реальный размер контекста (после инициализации).

    static int backend_ref_count_;                ///< Счётчик ссылок на глобальную инициализацию llama.cpp.
};
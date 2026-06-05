/**
 * @file LLamaInterface.hpp
 * @brief Обёртка над llama.cpp для чата с GGUF-моделями.
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

/// @brief Модель не загружена, но вызван метод, которому она нужна.
class LLamaNotLoadedError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка применения chat-template.
class LLamaTemplateError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Ошибка токенизации.
class LLamaTokenizeError : public LLamaException {
public:
    using LLamaException::LLamaException;
};

/// @brief Не хватает места в контексте.
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
 */
class LLamaInterface {
public:
    /// @brief Параметры модели и генерации.
    struct Config {
        std::string model_path;       ///< Путь к .gguf файлу.
        uint32_t n_ctx       = 4096;  ///< Размер контекста.
        uint32_t n_batch     = 512;  ///< Размер батча (n_ctx для простоты).
        int32_t  n_gpu_layers = 0;    ///< Слоёв на GPU (0 = только CPU).
        float    temperature  = 0.8f; ///< Температура (0 = greedy).
        float    top_p        = 0.95f;///< Top-P.
        int32_t  top_k        = 40;   ///< Top-K.
        int32_t  n_threads    = -1;   ///< Потоки (-1 = авто).
        int32_t  max_tokens   = 4096; ///< Лимит токенов в ответе.
    };

    /// Callback для потокового вывода. Вернуть false чтобы остановить.
    using TokenCallback = std::function<bool(const std::string& piece)>;

    LLamaInterface();
    ~LLamaInterface();

    LLamaInterface(const LLamaInterface&) = delete;
    LLamaInterface& operator=(const LLamaInterface&) = delete;
    LLamaInterface(LLamaInterface&&) noexcept;
    LLamaInterface& operator=(LLamaInterface&&) noexcept;

    /**
     * @brief Загрузить модель.
     * @throws LLamaLoadError если файл не найден или модель не загрузилась.
     */
    void LoadModel(const Config& config);

    /// @brief Проверить, загружена ли модель.
    bool IsLoaded() const;

    /// @brief Выгрузить модель и освободить память.
    void Unload();

    /// @brief Установить системный промпт.
    void SetSystemPrompt(const std::string& prompt);

    /**
     * @brief Отправить сообщение и получить ответ.
     * @param user_message Текст пользователя.
     * @param callback     Опционально - вызывается на каждый кусок текста.
     * @return Ответ модели.
     */
    std::string Chat(const std::string& user_message,
                     TokenCallback callback = nullptr);

    /**
     * @brief Сохранить состояние в файлы (.state + .history).
     * @throws LLamaNotLoadedError модель не загружена.
     * @throws LLamaStateError     ошибка записи.
     */
    void SaveState(const std::string& base_path) const;

    /**
     * @brief Восстановить состояние из файлов.
     * @throws LLamaNotLoadedError модель не загружена.
     * @throws LLamaStateError     ошибка чтения или несовместимый стейт.
     */
    void LoadState(const std::string& base_path);

    /// @brief Очистить историю и KV-кэш.
    void ClearHistory();

    /// @brief Вернуть путь к модели.
    const std::string& GetModelPath() const;

    /// @brief Вернуть историю сообщений (role, content).
    const std::vector<std::pair<std::string, std::string>>& GetHistory() const;

private:
    void BuildSampler();
    void FreeResources() noexcept;

    void SaveHistory(const std::string& path) const;
    void LoadHistory(const std::string& path);

    llama_model*   model_   = nullptr;
    llama_context* ctx_     = nullptr;
    llama_sampler* sampler_ = nullptr;

    Config config_;
    std::string system_prompt_;
    std::vector<std::pair<std::string, std::string>> messages_;

    size_t prev_template_len_ = 0;

    std::vector<char> formatted_;

    uint32_t n_ctx_ = 0;

    static int backend_ref_count_;
};

#include "LLamaInterface.hpp"

#include <llama.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

int LLamaInterface::backend_ref_count_ = 0;

LLamaInterface::LLamaInterface() = default;

LLamaInterface::~LLamaInterface() {
    FreeResources();
}

LLamaInterface::LLamaInterface(LLamaInterface &&other) noexcept
    : model_(other.model_)
      , ctx_(other.ctx_)
      , sampler_(other.sampler_)
      , config_(std::move(other.config_))
      , system_prompt_(std::move(other.system_prompt_))
      , messages_(std::move(other.messages_))
      , prev_template_len_(other.prev_template_len_)
      , formatted_(std::move(other.formatted_))
      , n_ctx_(other.n_ctx_) {
    other.model_ = nullptr;
    other.ctx_ = nullptr;
    other.sampler_ = nullptr;
    other.n_ctx_ = 0;
    other.prev_template_len_ = 0;
}

LLamaInterface &LLamaInterface::operator=(LLamaInterface &&other) noexcept {
    if (this != &other) {
        FreeResources();
        model_ = other.model_;
        ctx_ = other.ctx_;
        sampler_ = other.sampler_;
        config_ = std::move(other.config_);
        system_prompt_ = std::move(other.system_prompt_);
        messages_ = std::move(other.messages_);
        prev_template_len_ = other.prev_template_len_;
        formatted_ = std::move(other.formatted_);
        n_ctx_ = other.n_ctx_;

        other.model_ = nullptr;
        other.ctx_ = nullptr;
        other.sampler_ = nullptr;
        other.n_ctx_ = 0;
        other.prev_template_len_ = 0;
    }
    return *this;
}

void LLamaInterface::LoadModel(const Config &config) {
    FreeResources();
    config_ = config;

    llama_log_set([](ggml_log_level level, const char *, void *) {
    }, nullptr);

    if (backend_ref_count_ == 0) {
        ggml_backend_load_all();
        llama_backend_init();
    }
    ++backend_ref_count_;

    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = config_.n_gpu_layers;

    model_ = llama_model_load_from_file(config_.model_path.c_str(), model_params);
    if (!model_) {
        --backend_ref_count_;
        if (backend_ref_count_ == 0) llama_backend_free();
        throw LLamaLoadError("Не удалось загрузить модель: " + config_.model_path);
    }

    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config_.n_ctx;
    ctx_params.n_batch = config_.n_ctx;
    ctx_params.n_seq_max = 1;
    ctx_params.n_threads = config_.n_threads > 0
                               ? config_.n_threads
                               : std::max(1, (int32_t) std::thread::hardware_concurrency());
    ctx_params.n_threads_batch = ctx_params.n_threads;

    ctx_ = llama_init_from_model(model_, ctx_params);
    if (!ctx_) {
        llama_model_free(model_);
        model_ = nullptr;
        --backend_ref_count_;
        if (backend_ref_count_ == 0) llama_backend_free();
        throw LLamaLoadError("Не удалось создать контекст: " + config_.model_path);
    }

    n_ctx_ = llama_n_ctx(ctx_);

    try {
        BuildSampler();
    } catch (...) {
        llama_free(ctx_);
        ctx_ = nullptr;
        llama_model_free(model_);
        model_ = nullptr;
        --backend_ref_count_;
        if (backend_ref_count_ == 0) llama_backend_free();
        throw;
    }

    formatted_.resize(n_ctx_ * 4 + 4096);

    llama_log_set(nullptr, nullptr);
}

bool LLamaInterface::IsLoaded() const {
    return model_ != nullptr && ctx_ != nullptr;
}

void LLamaInterface::Unload() { FreeResources(); }

void LLamaInterface::SetSystemPrompt(const std::string &prompt) {
    system_prompt_ = prompt;
}

void LLamaInterface::ClearHistory() {
    messages_.clear();
    prev_template_len_ = 0;
    if (ctx_) {
        auto *mem = llama_get_memory(ctx_);
        if (mem) llama_memory_seq_rm(mem, 0, 0, -1);
    }
}

const std::string &LLamaInterface::GetModelPath() const { return config_.model_path; }

const std::vector<std::pair<std::string, std::string> > &LLamaInterface::GetHistory() const {
    return messages_;
}

std::string LLamaInterface::Chat(const std::string &user_message,
                                 TokenCallback callback) {
    if (!IsLoaded()) {
        throw LLamaNotLoadedError("Chat(): модель не загружена.");
    }

    messages_.push_back({"user", user_message});

    const size_t MAX_MESSAGES = 6;
    while (messages_.size() > MAX_MESSAGES) {
        messages_.erase(messages_.begin());
        prev_template_len_ = 0;
        if (ctx_) {
            auto *mem = llama_get_memory(ctx_);
            if (mem) llama_memory_seq_rm(mem, 0, 0, -1);
        }
    }

    try {
        const auto *vocab = llama_model_get_vocab(model_);
        const char *tmpl = llama_model_chat_template(model_, nullptr);

        std::vector<llama_chat_message> chat;
        std::vector<std::string> role_store;
        std::vector<std::string> content_store;

        if (!system_prompt_.empty()) {
            role_store.push_back("system");
            content_store.push_back(system_prompt_);
        }
        for (const auto &[r, c]: messages_) {
            role_store.push_back(r);
            content_store.push_back(c);
        }
        for (size_t i = 0; i < role_store.size(); ++i) {
            chat.push_back({role_store[i].c_str(), content_store[i].c_str()});
        }
        int new_len = llama_chat_apply_template(
            tmpl, chat.data(), chat.size(), true,
            formatted_.data(), formatted_.size());

        if (new_len < 0) {
            throw LLamaTemplateError("Chat(): шаблон вернул ошибку.");
        }
        if (new_len > static_cast<int>(formatted_.size())) {
            formatted_.resize(new_len + 256);
            new_len = llama_chat_apply_template(
                tmpl, chat.data(), chat.size(), true,
                formatted_.data(), formatted_.size());
            if (new_len < 0) {
                throw LLamaTemplateError("Chat(): шаблон (retry) вернул ошибку.");
            }
        }

        const size_t dstart = std::min(prev_template_len_,
                                       static_cast<size_t>(new_len));
        const std::string delta(
            formatted_.begin() + static_cast<ptrdiff_t>(dstart),
            formatted_.begin() + static_cast<ptrdiff_t>(new_len));

        if (delta.empty()) {
            throw LLamaTokenizeError("Chat(): delta пуст.");
        }

        const bool is_first = (prev_template_len_ == 0);

        const int n_tokens = -llama_tokenize(
            vocab, delta.c_str(), delta.size(),
            nullptr, 0, is_first, true);

        if (n_tokens <= 0) {
            throw LLamaTokenizeError(
                "Chat(): токенизация вернула " + std::to_string(n_tokens) + ".");
        }

        std::vector<llama_token> tokens(n_tokens);
        if (llama_tokenize(
                vocab, delta.c_str(), delta.size(),
                tokens.data(), tokens.size(),
                is_first, true) < 0) {
            throw LLamaTokenizeError("Chat(): llama_tokenize failed.");
        }

        std::string response;
        int n_decoded = 0;

        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        llama_token new_token_id = LLAMA_TOKEN_NULL;

        while (n_decoded < config_.max_tokens) {
            auto *mem = llama_get_memory(ctx_);
            const int n_used = mem
                                   ? llama_memory_seq_pos_max(mem, 0) + 1
                                   : 0;

            if (n_used + batch.n_tokens > static_cast<int>(n_ctx_)) {
                throw LLamaContextOverflowError(
                    "Chat(): контекст переполнен (" +
                    std::to_string(n_used) + "+" +
                    std::to_string(batch.n_tokens) + ">" +
                    std::to_string(n_ctx_) + ").");
            }

            const int ret = llama_decode(ctx_, batch);
            if (ret != 0) {
                throw LLamaInferenceError(
                    "Chat(): llama_decode вернул " + std::to_string(ret) +
                    " (n_decoded=" + std::to_string(n_decoded) + ").");
            }

            new_token_id = llama_sampler_sample(sampler_, ctx_, -1);

            if (llama_vocab_is_eog(vocab, new_token_id)) break;

            char buf[256];
            const int n = llama_token_to_piece(
                vocab, new_token_id, buf, sizeof(buf), 0, true);

            if (n > 0) {
                std::string piece(buf, static_cast<size_t>(n));
                response += piece;
                if (callback && !callback(piece)) break;
            }

            batch = llama_batch_get_one(&new_token_id, 1);
            ++n_decoded;
        }

        messages_.push_back({"assistant", response});

        chat.clear();
        role_store.clear();
        content_store.clear();

        if (!system_prompt_.empty()) {
            role_store.push_back("system");
            content_store.push_back(system_prompt_);
        }
        for (const auto &[r, c]: messages_) {
            role_store.push_back(r);
            content_store.push_back(c);
        }
        for (size_t i = 0; i < role_store.size(); ++i) {
            chat.push_back({role_store[i].c_str(), content_store[i].c_str()});
        }

        const int plen = llama_chat_apply_template(
            tmpl, chat.data(), chat.size(), false, nullptr, 0);
        if (plen >= 0) {
            prev_template_len_ = static_cast<size_t>(plen);
        }

        return response;
    } catch (...) {
        messages_.pop_back();
        throw;
    }
}

void LLamaInterface::SaveState(const std::string &base_path) const {
    if (!IsLoaded()) throw LLamaNotLoadedError("SaveState(): модель не загружена.");

    const std::string state_path = base_path + ".state";
    const std::string history_path = base_path + ".history";

    size_t saved = llama_state_seq_save_file(
        ctx_, state_path.c_str(), 0, nullptr, 0);
    if (saved == 0) {
        throw LLamaStateError("SaveState(): не удалось сохранить: " + state_path);
    }

    SaveHistory(history_path);
}

void LLamaInterface::LoadState(const std::string &base_path) {
    if (!IsLoaded()) throw LLamaNotLoadedError("LoadState(): модель не загружена.");

    const std::string state_path = base_path + ".state";
    const std::string history_path = base_path + ".history";

    LoadHistory(history_path);

    size_t n_token_count = 0;
    size_t loaded = llama_state_seq_load_file(
        ctx_, state_path.c_str(), 0, nullptr, 0, &n_token_count);

    if (loaded == 0) {
        messages_.clear();
        throw LLamaStateError("LoadState(): не удалось загрузить: " + state_path);
    }

    const char *tmpl = llama_model_chat_template(model_, nullptr);

    std::vector<llama_chat_message> chat;
    std::vector<std::string> role_store;
    std::vector<std::string> content_store;

    if (!system_prompt_.empty()) {
        role_store.push_back("system");
        content_store.push_back(system_prompt_);
    }
    for (const auto &[r, c]: messages_) {
        role_store.push_back(r);
        content_store.push_back(c);
    }
    for (size_t i = 0; i < role_store.size(); ++i) {
        chat.push_back({role_store[i].c_str(), content_store[i].c_str()});
    }

    const int plen = llama_chat_apply_template(
        tmpl, chat.data(), chat.size(), false, nullptr, 0);
    if (plen >= 0) prev_template_len_ = static_cast<size_t>(plen);
}

void LLamaInterface::FreeResources() noexcept {
    if (sampler_) {
        llama_sampler_free(sampler_);
        sampler_ = nullptr;
    }
    if (ctx_) {
        llama_free(ctx_);
        ctx_ = nullptr;
    }
    if (model_) {
        llama_model_free(model_);
        model_ = nullptr;
    }

    messages_.clear();
    system_prompt_.clear();
    formatted_.clear();
    prev_template_len_ = 0;
    n_ctx_ = 0;

    if (backend_ref_count_ > 0) {
        --backend_ref_count_;
        if (backend_ref_count_ == 0) llama_backend_free();
    }
}

void LLamaInterface::BuildSampler() {
    sampler_ = llama_sampler_chain_init(llama_sampler_chain_default_params());
    if (!sampler_) {
        throw LLamaLoadError("Не удалось инициализировать сэмплер.");
    }

    llama_sampler_chain_add(sampler_, llama_sampler_init_top_k(config_.top_k));
    llama_sampler_chain_add(sampler_, llama_sampler_init_top_p(config_.top_p, 1));
    llama_sampler_chain_add(sampler_, llama_sampler_init_temp(config_.temperature));

    if (config_.temperature <= 0.0f) {
        llama_sampler_chain_add(sampler_, llama_sampler_init_greedy());
    } else {
        llama_sampler_chain_add(sampler_, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    }
}

void LLamaInterface::SaveHistory(const std::string &path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw LLamaStateError("SaveHistory(): не удалось открыть: " + path);
    }

    const uint32_t count = static_cast<uint32_t>(messages_.size());
    file.write(reinterpret_cast<const char *>(&count), sizeof(count));

    for (const auto &[role, content]: messages_) {
        const uint32_t role_len = static_cast<uint32_t>(role.size());
        const uint32_t content_len = static_cast<uint32_t>(content.size());

        file.write(reinterpret_cast<const char *>(&role_len), sizeof(role_len));
        file.write(role.data(), role_len);

        file.write(reinterpret_cast<const char *>(&content_len), sizeof(content_len));
        file.write(content.data(), content_len);
    }

    if (!file.good()) {
        throw LLamaStateError("SaveHistory(): ошибка записи: " + path);
    }
}

void LLamaInterface::LoadHistory(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LLamaStateError("LoadHistory(): не удалось открыть: " + path);
    }

    uint32_t count = 0;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));
    if (!file) {
        throw LLamaStateError("LoadHistory(): повреждён заголовок: " + path);
    }

    messages_.clear();
    messages_.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t role_len = 0, content_len = 0;

        file.read(reinterpret_cast<char *>(&role_len), sizeof(role_len));
        if (!file)
            throw LLamaStateError(
                "LoadHistory(): ошибка role_len [" + std::to_string(i) + "]: " + path);
        std::string role(role_len, '\0');
        file.read(role.data(), role_len);
        if (!file)
            throw LLamaStateError(
                "LoadHistory(): ошибка role [" + std::to_string(i) + "]: " + path);

        file.read(reinterpret_cast<char *>(&content_len), sizeof(content_len));
        if (!file)
            throw LLamaStateError(
                "LoadHistory(): ошибка content_len [" + std::to_string(i) + "]: " + path);
        std::string content(content_len, '\0');
        file.read(content.data(), content_len);
        if (!file)
            throw LLamaStateError(
                "LoadHistory(): ошибка content [" + std::to_string(i) + "]: " + path);

        messages_.push_back({std::move(role), std::move(content)});
    }
}

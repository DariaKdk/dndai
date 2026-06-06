#include <gtest/gtest.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>

#include "../classes/CharList.hpp"
#include "../classes/DiceRoll.hpp"
#include "../classes/LLamaInterface.hpp"
#include "../classes/ChatController.hpp"
#include "../classes/ChatTab.hpp"

using json = nlohmann::json;

// Тесты для CharList

TEST(CharListTest, InitialStateToJson) {
    CharList charList;
    std::string jsonStr = charList.SaveToJson();

    ASSERT_NO_THROW({
        json j = json::parse(jsonStr);
        EXPECT_EQ(j["name"], "Новый персонаж");
        EXPECT_EQ(j["race"], "Человек");
        EXPECT_EQ(j["level"], 1);
        EXPECT_EQ(j["hp_max"], 10);
        EXPECT_EQ(j["hp_current"], 10);
        EXPECT_EQ(j["ac"], 10);
        EXPECT_EQ(j["speed"], 30);
        });
}

TEST(CharListTest, SaveAndLoadFromFile) {
    CharList charList;
    std::string test_file = "test_characters.json";

    charList.SaveToFile(test_file);

    std::ifstream ifs(test_file);
    ASSERT_TRUE(ifs.is_open());
    ifs.close();

    CharList loadedCharList;
    loadedCharList.LoadFromFile(test_file);

    std::string origJson = charList.SaveToJson();
    std::string loadedJson = loadedCharList.SaveToJson();

    EXPECT_EQ(origJson, loadedJson);

    std::remove(test_file.c_str());
}

TEST(CharListTest, LoadFromNonExistentFile) {
    CharList charList;
    EXPECT_THROW(charList.LoadFromFile("non_existent_characters.json"), std::runtime_error);
}

// Тесты для DiceRoll

TEST(DiceRollTest, InitialRollIsZero) {
    DiceRoll dice;
    EXPECT_EQ(dice.GetLastRoll(), 0);
}

TEST(DiceRollTest, StatePersistence) {
    DiceRoll dice;
    EXPECT_EQ(dice.GetLastRoll(), 0);
    EXPECT_EQ(dice.GetLastRoll(), 0);
}


// Тесты для LLamaInterface (без модели)

TEST(LLamaInterfaceTest, InitiallyNotLoaded) {
    LLamaInterface llama;
    EXPECT_FALSE(llama.IsLoaded());
}

TEST(LLamaInterfaceTest, ThrowsOnInvalidModelPath) {
    LLamaInterface llama;
    LLamaInterface::Config config;
    config.model_path = "non_existent_model.gguf";
    EXPECT_THROW(llama.LoadModel(config), LLamaLoadError);
}

TEST(LLamaInterfaceTest, ThrowsWhenChattingWithoutModel) {
    LLamaInterface llama;
    EXPECT_THROW(llama.Chat("hello"), LLamaNotLoadedError);
}

TEST(LLamaInterfaceTest, ThrowsWhenSavingStateWithoutModel) {
    LLamaInterface llama;
    EXPECT_THROW(llama.SaveState("test_state"), LLamaNotLoadedError);
}

TEST(LLamaInterfaceTest, ThrowsWhenLoadingStateWithoutModel) {
    LLamaInterface llama;
    EXPECT_THROW(llama.LoadState("test_state"), LLamaNotLoadedError);
}

// Тесты для ChatController

TEST(ChatControllerTest, ThrowsOnInvalidModelDuringInit) {
    EXPECT_THROW({
                 ChatController controller("no_model.gguf");
                 }, LLamaLoadError);
}

// Тесты для ChatTab

TEST(ChatTabTest, OnSendMessageCallback) {
    ChatTab chat_tab;
    bool callback_called = false;
    std::string sent_text = "";

    chat_tab.SetOnSendMessage([&](const std::string &text) {
        callback_called = true;
        sent_text = text;
    });

    chat_tab.AddMessage("Test Author", "Test Message", ftxui::Color::White);
    SUCCEED();
}

// тесты LLamaInterface (с моделью)

bool FileExists(const std::string &path) {
    std::ifstream f(path);
    return f.good();
}

TEST(LLamaIntegrationTest, LoadAndGenerate) {
    std::string model_path = "Falcon-H1-1.5B-Instruct-Q4_K_S.gguf";

    if (!FileExists(model_path)) {
        GTEST_SKIP() << "Файл модели " << model_path << " не найден. Пропускаем тест.";
    }

    LLamaInterface llama;
    LLamaInterface::Config config;
    config.model_path = model_path;
    config.n_ctx = 512;
    config.n_threads = 4;
    config.max_tokens = 10;
    config.temperature = 0.1f;

    ASSERT_NO_THROW(llama.LoadModel(config));
    EXPECT_TRUE(llama.IsLoaded());

    llama.SetSystemPrompt("Ты - ИИ-помощник.");

    std::string response;
    ASSERT_NO_THROW({
        response = llama.Chat("Напиши цифру 1.");
        });

    EXPECT_FALSE(response.empty());
}

TEST(LLamaIntegrationTest, StreamGenerationCallback) {
    std::string model_path = "Falcon-H1-1.5B-Instruct-Q4_K_S.gguf";

    if (!FileExists(model_path)) {
        GTEST_SKIP() << "Файл модели " << model_path << " не найден. Пропускаем тест.";
    }

    LLamaInterface llama;
    LLamaInterface::Config config;
    config.model_path = model_path;
    config.n_ctx = 512;
    config.n_threads = 4;
    config.max_tokens = 15;

    llama.LoadModel(config);
    llama.SetSystemPrompt("Ты - ИИ-помощник.");

    int token_count = 0;
    std::string accumulated_response = "";

    auto token_callback = [&](const std::string &piece) -> bool {
        token_count++;
        accumulated_response += piece;
        return true;
    };

    std::string final_response = llama.Chat("Привет!", token_callback);
    EXPECT_GT(token_count, 0);
    EXPECT_EQ(accumulated_response, final_response);
}

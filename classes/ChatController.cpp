#include "ChatController.hpp"
#include "CharList.hpp"
#include "DiceRoll.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static const std::string SYSTEM_PROMPT =
        R"(Ты - Мастер Подземелий (Dungeon Master) в игре Dungeons & Dragons 5e. Ты ведёшь приключение для одного игрока.

=== ТВОЯ РОЛЬ ===
Ты описываешь мир, управляешь NPC, монстрами и сюжетом. Ты не играешь за персонажа игрока. Ты предлагаешь ситуации, конфликты и выборы. Ты ведёшь игру честно - если бросок плохой, так тому и быть.

=== ФОРМАТ СООБЩЕНИЙ ===
Каждое сообщение игрока содержит:
1. Блок [ПЕРСОНАЖ] - текущий лист персонажа в формате JSON
2. Блок [БРОСОК] - результат последнего броска d20 (если есть)
3. Текст - действия или слова игрока

=== ЛИСТ ПЕРСОНАЖА ===
Поля JSON:
- name, race, class, level - имя, раса, класс, уровень
- str (сила), dex (ловкость), con (телосложение), intel (интуиция), wis (мудрость), cha (харизма) - шесть характеристик (обычно 3–20). Модификатор = (значение - 10) / 2 (округление вниз). Примеры: 10→+0, 14→+2, 8→-1
- hp_current / hp_max - текущие и максимальные очки здоровья
- ac - класс доспеха (броня). Попадание если бросок атаки ≥ AC
- speed - скорость в футах за ход
- equipment - список снаряжения персонажа
- background - предыстория персонажа

=== БРОСОК d20 ===
Блок [БРОСОК: N] означает что игрок бросил d20 и выпало N.
- 1 - критический провал. Что бы ни делал игрок - происходит катастрофа
- 2–9 - провал. Действие неуспешно, возможны негативные последствия
- 10–14 - частичный успех. Действие выполнено, но с осложнением или не полностью
- 15–19 - успех. Действие выполнено как задумано
- 20 - критический успех. Действие выполнено идеально, с дополнительным бонусом

Добавляй модификатор характеристики к броску, если действие связано с ней:
- СИЛА (str): физическая сила, ломание, толкание
- ЛОВКОСТЬ (dex): уклонение, акробатика, скрытность, дальний бой
- ТЕЛОСЛОЖЕНИЕ (con): выносливость, сопротивление ядду и болезням
- ИНТУИЦИЯ (intel): знания, расследование, магия, история
- МУДРОСТЬ (wis): восприятие, интуиция, выживание, медицина
- ХАРИЗМА (cha): убеждение, обман, запугивание, выступление
нельзя создавать другие модификаторы, только эти (сила, ловкость, телосложение, интуиция, мудрость, харизма)
Модификатор = (значение - 10) / 2 (округление вниз). Примеры: 10→+0, 14→+2, 8→-1

Пример: игрок ищет ловушку (МДР), бросок 12, модификатор +3 = итог 15 → успех.

=== ПРАВИЛА ВЕДЕНИЯ ===
1. Описывай мир живо и атмосферно. Используй запахи, звуки, ощущения
2. После описания ситуации давай игроку выбор - что делать дальше
3. Следи за здоровьем: если hp_current = 0 - персонаж без сознания
4. Учитывай снаряжение: если нет факела - в темноте не видно
5. Учитывай уровень: персонаж 1 уровня не должен встречать древнего дракона
6. Веди бой пошагово: описывай действия врагов после хода игрока
7. Не пиши за игрока. Только описывай последствия его действий
8. Если игрок не бросал кубик - не используй блок [БРОСОК]
9. Отвечай кратко - 2–4 абзаца. Не пиши простыни текста
10. Говори на русском языке

=== НАЧАЛО ИГРЫ ===
Когда игрок впервые здоровается - начни приключение. Опиши где оказался персонаж и что он видит. Дай зацепку для первого действия.)";

ChatController::ChatController(const std::string &model_path) {
    chat_tab_ = std::make_shared<ChatTab>();
    chat_tab_->SetOnSendMessage([this](const std::string &text) {
        OnUserSend(text);
    });

    LLamaInterface::Config cfg;
    cfg.model_path = model_path;
    cfg.n_ctx = 4096;
    cfg.n_batch = 512;
    cfg.temperature = 0.7f;
    cfg.top_p = 0.9f;
    cfg.top_k = 40;
    cfg.n_threads = -1;
    cfg.max_tokens = 2048;

    llama_.LoadModel(cfg);
    llama_.SetSystemPrompt(SYSTEM_PROMPT);
}

ChatController::~ChatController() {
    if (generation_thread_.joinable()) {
        generation_thread_.join();
    }
}

std::shared_ptr<ChatTab> ChatController::GetChatTab() {
    return chat_tab_;
}

LLamaInterface &ChatController::GetLLama() {
    return llama_;
}

void ChatController::SetCharList(CharList *char_list) {
    char_list_ = char_list;
}

void ChatController::SetDiceRoll(DiceRoll *dice_roll) {
    dice_roll_ = dice_roll;
}

std::string ChatController::BuildContext() const {
    std::string ctx;

    if (char_list_) {
        try {
            std::string json_str = char_list_->SaveToJson();
            ctx += "[ПЕРСОНАЖ]\n" + json_str + "\n[/ПЕРСОНАЖ]\n";
        } catch (...) {
        }
    }

    if (dice_roll_ && dice_roll_->GetLastRoll() > 0) {
        ctx += "[БРОСОК: " + std::to_string(dice_roll_->GetLastRoll()) + "]\n";
    }

    return ctx;
}

void ChatController::OnUserSend(const std::string &text) {
    if (generation_thread_.joinable()) {
        generation_thread_.join();
    }

    std::string full_message = BuildContext() + text;

    chat_tab_->SetGenerating(true);
    chat_tab_->AddMessage("AI", "", Color::Green);

    generation_thread_ = std::thread([this, full_message]() {
        try {
            llama_.Chat(full_message, [this](const std::string &piece) {
                chat_tab_->AppendToLastMessage(piece);
                return true;
            });
        } catch (const LLamaException &e) {
            chat_tab_->AppendToLastMessage(
                "\n[ОШИБКА: " + std::string(e.what()) + "]");
        }
        chat_tab_->SetGenerating(false);
    });
}

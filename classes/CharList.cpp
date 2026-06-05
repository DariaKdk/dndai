#include "CharList.hpp"
#include <ftxui/component/event.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>

using json = nlohmann::json;

static int ParseInt(const std::string& s, int def = 10) {
    try { return std::stoi(s); }
    catch (...) { return def; }
}

static std::string ModStr(int score) {
    int m = (score - 10) / 2;
    return m >= 0 ? ("+" + std::to_string(m)) : std::to_string(m);
}

static std::string ProfStr(int level) {
    int p = (level - 1) / 4 + 2;
    return "+" + std::to_string(p);
}

static Component NumericInput(std::string* content, const std::string& placeholder) {
    auto input = Input(content, placeholder);
    return input | CatchEvent([](Event e) {
        if (e.is_character()) {
            char c = e.character()[0];
            return c < '0' || c > '9';
        }
        return false;
    });
}

CharList::CharList() {
    characters_.push_back(Character());
    file_path_ = "characters.json";
    SyncFromCharacter();

    name_in_ = Input(&name_buf_, "");
    race_in_ = Input(&race_buf_, "");
    class_in_ = Input(&class_buf_, "");
    level_in_ = NumericInput(&level_buf_, "");

    str_in_ = NumericInput(&str_buf_, "");
    dex_in_ = NumericInput(&dex_buf_, "");
    con_in_ = NumericInput(&con_buf_, "");
    intel_in_ = NumericInput(&intel_buf_, "");
    wis_in_ = NumericInput(&wis_buf_, "");
    cha_in_ = NumericInput(&cha_buf_, "");

    hp_max_in_ = NumericInput(&hp_max_buf_, "");
    hp_cur_in_ = NumericInput(&hp_cur_buf_, "");
    ac_in_ = NumericInput(&ac_buf_, "");
    speed_in_ = NumericInput(&speed_buf_, "");

    bg_in_ = Input(&bg_buf_, "");

    equip_in_ = Input(&equip_buf_, "");
    file_in_ = Input(&file_path_, "");

    equip_menu_ = Menu(&equipment_, &sel_equip_);

    prev_btn_ = Button("<", [this] {
        SyncToCharacter();
        selected_ = (selected_ - 1 + (int)characters_.size()) % (int)characters_.size();
        SyncFromCharacter();
    });

    next_btn_ = Button(">", [this] {
        SyncToCharacter();
        selected_ = (selected_ + 1) % (int)characters_.size();
        SyncFromCharacter();
    });

    add_btn_ = Button("+Персонаж", [this] {
        SyncToCharacter();
        characters_.push_back(Character());
        selected_ = (int)characters_.size() - 1;
        SyncFromCharacter();
    });

    del_btn_ = Button("Удалить", [this] {
        if (characters_.size() <= 1) return;
        characters_.erase(characters_.begin() + selected_);
        if (selected_ >= (int)characters_.size())
            selected_ = (int)characters_.size() - 1;
        SyncFromCharacter();
    });

    save_btn_ = Button("Сохранить", [this] {
        SyncToCharacter();
        SaveToFile(file_path_);
    });

    load_btn_ = Button("Загрузить", [this] {
        LoadFromFile(file_path_);
        selected_ = 0;
        SyncFromCharacter();
    });

    add_equip_btn_ = Button("+", [this] {
        if (!equip_buf_.empty()) {
            equipment_.push_back(equip_buf_);
            equip_buf_.clear();
        }
    });

    del_equip_btn_ = Button("-", [this] {
        if (!equipment_.empty() && sel_equip_ >= 0 && sel_equip_ < (int)equipment_.size()) {
            equipment_.erase(equipment_.begin() + sel_equip_);
            if (sel_equip_ >= (int)equipment_.size())
                sel_equip_ = std::max(0, (int)equipment_.size() - 1);
        }
    });

    auto row_nav = Container::Horizontal({prev_btn_, next_btn_, add_btn_, del_btn_});
    auto row_name = Container::Horizontal({name_in_, race_in_});
    auto row_class = Container::Horizontal({class_in_, level_in_});
    auto row_stat1 = Container::Horizontal({str_in_, dex_in_, con_in_});
    auto row_stat2 = Container::Horizontal({intel_in_, wis_in_, cha_in_});
    auto row_combat = Container::Horizontal({hp_cur_in_, hp_max_in_, ac_in_, speed_in_});
    auto row_equip = Container::Horizontal({equip_in_, add_equip_btn_, del_equip_btn_});
    auto row_file = Container::Horizontal({file_in_, save_btn_, load_btn_});

    auto container = Container::Vertical({
        row_nav, row_name, row_class, row_stat1, row_stat2,
        row_combat, row_equip, equip_menu_,
        bg_in_, row_file,
    });

    auto renderer = Renderer(container, [this] {
        int s = ParseInt(str_buf_);
        int d = ParseInt(dex_buf_);
        int c = ParseInt(con_buf_);
        int ii = ParseInt(intel_buf_);
        int w = ParseInt(wis_buf_);
        int ch = ParseInt(cha_buf_);
        int lv = ParseInt(level_buf_, 1);

        std::string counter = " " + std::to_string(selected_ + 1) + "/" +
                              std::to_string(characters_.size()) + " ";
        std::string prof = ProfStr(lv);

        Elements nav_elems;
        nav_elems.push_back(prev_btn_->Render());
        nav_elems.push_back(text(counter) | bold);
        nav_elems.push_back(next_btn_->Render());
        nav_elems.push_back(text("  "));
        nav_elems.push_back(add_btn_->Render());
        nav_elems.push_back(text(" "));
        nav_elems.push_back(del_btn_->Render());
        auto nav = hbox(nav_elems) | borderLight;

        Elements gen1;
        gen1.push_back(text(" Имя: "));
        gen1.push_back(name_in_->Render() | flex);
        gen1.push_back(text(" Раса: "));
        gen1.push_back(race_in_->Render() | flex);

        Elements gen2;
        gen2.push_back(text(" Класс: "));
        gen2.push_back(class_in_->Render() | flex);
        gen2.push_back(text(" Уровень: "));
        gen2.push_back(level_in_->Render() | size(WIDTH, LESS_THAN, 5));
        gen2.push_back(text("  Бонус мастерства: ") | bold);
        gen2.push_back(text(prof) | bold);

        auto general = window(text("Основное"), vbox({hbox(gen1), hbox(gen2)}));

        Elements st1;
        st1.push_back(text(" СИЛ: "));
        st1.push_back(str_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st1.push_back(text("(" + ModStr(s) + ")  "));
        st1.push_back(text("ЛОВ: "));
        st1.push_back(dex_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st1.push_back(text("(" + ModStr(d) + ")  "));
        st1.push_back(text("ТЕЛ: "));
        st1.push_back(con_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st1.push_back(text("(" + ModStr(c) + ")"));

        Elements st2;
        st2.push_back(text(" ИНТ: "));
        st2.push_back(intel_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st2.push_back(text("(" + ModStr(ii) + ")  "));
        st2.push_back(text("МДР: "));
        st2.push_back(wis_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st2.push_back(text("(" + ModStr(w) + ")  "));
        st2.push_back(text("ХАР: "));
        st2.push_back(cha_in_->Render() | size(WIDTH, LESS_THAN, 4));
        st2.push_back(text("(" + ModStr(ch) + ")"));

        auto stats = window(text("Характеристики"), vbox({hbox(st1), hbox(st2)}));

        Elements cb;
        cb.push_back(text(" ОЗ: "));
        cb.push_back(hp_cur_in_->Render() | size(WIDTH, LESS_THAN, 4));
        cb.push_back(text("/"));
        cb.push_back(hp_max_in_->Render() | size(WIDTH, LESS_THAN, 4));
        cb.push_back(text("  КД: "));
        cb.push_back(ac_in_->Render() | size(WIDTH, LESS_THAN, 4));
        cb.push_back(text("  Скорость: "));
        cb.push_back(speed_in_->Render() | size(WIDTH, LESS_THAN, 4));

        auto combat = window(text("Бой"), hbox(cb));

        Elements eq;
        eq.push_back(text(" "));
        eq.push_back(equip_in_->Render() | flex);
        eq.push_back(text(" "));
        eq.push_back(add_equip_btn_->Render());
        eq.push_back(text(" "));
        eq.push_back(del_equip_btn_->Render());

        auto equip_w = window(text("Снаряжение"), vbox({
            hbox(eq),
            equip_menu_->Render() | frame | size(HEIGHT, LESS_THAN, 5),
        }));

        auto bg_w = window(text("Предыстория"), bg_in_->Render() | flex);

        Elements fb;
        fb.push_back(text(" Файл: "));
        fb.push_back(file_in_->Render() | flex);
        fb.push_back(text(" "));
        fb.push_back(save_btn_->Render());
        fb.push_back(text(" "));
        fb.push_back(load_btn_->Render());

        auto file_bar = hbox(fb) | borderLight;

        content_height_ = 3 + 4 + 4 + 3
            + 2 + std::min((int)equipment_.size(), 5)
            + 3 + 1 + 4;

        scroll_offset_ = std::min(scroll_offset_, std::max(0, content_height_ - 1));

        Elements page;
        page.push_back(nav);
        page.push_back(general);
        page.push_back(stats);
        page.push_back(combat);
        page.push_back(equip_w);
        page.push_back(bg_w);
        page.push_back(separator());
        page.push_back(file_bar);
        page.push_back(text(""));

        return vbox(page)
            | focusPosition(0, scroll_offset_)
            | vscroll_indicator
            | frame
            | flex;
    });

    auto scrollable = renderer | CatchEvent([this](Event event) {
        if (event.is_mouse()) {
            int max_scroll = std::max(0, content_height_ - 1);
            if (event.mouse().button == 4) {
                scroll_offset_ = std::max(0, scroll_offset_ - 3);
                return true;
            }
            if (event.mouse().button == 5) {
                scroll_offset_ = std::min(max_scroll, scroll_offset_ + 3);
                return true;
            }
        }
        return false;
    });

    Add(scrollable);
}

void CharList::SyncToCharacter() {
    if (selected_ < 0 || selected_ >= (int)characters_.size()) return;
    auto& ch = characters_[selected_];
    ch.name = name_buf_;
    ch.race = race_buf_;
    ch.char_class = class_buf_;
    ch.level = ParseInt(level_buf_, 1);
    ch.str = ParseInt(str_buf_);
    ch.dex = ParseInt(dex_buf_);
    ch.con = ParseInt(con_buf_);
    ch.intel = ParseInt(intel_buf_);
    ch.wis = ParseInt(wis_buf_);
    ch.cha = ParseInt(cha_buf_);
    ch.hp_max = ParseInt(hp_max_buf_);
    ch.hp_current = ParseInt(hp_cur_buf_);
    ch.ac = ParseInt(ac_buf_);
    ch.speed = ParseInt(speed_buf_, 30);
    ch.background = bg_buf_;
    ch.equipment = equipment_;
}

void CharList::SyncFromCharacter() {
    if (selected_ < 0 || selected_ >= (int)characters_.size()) return;
    const auto& ch = characters_[selected_];
    name_buf_ = ch.name;
    race_buf_ = ch.race;
    class_buf_ = ch.char_class;
    level_buf_ = std::to_string(ch.level);
    str_buf_ = std::to_string(ch.str);
    dex_buf_ = std::to_string(ch.dex);
    con_buf_ = std::to_string(ch.con);
    intel_buf_ = std::to_string(ch.intel);
    wis_buf_ = std::to_string(ch.wis);
    cha_buf_ = std::to_string(ch.cha);
    hp_max_buf_ = std::to_string(ch.hp_max);
    hp_cur_buf_ = std::to_string(ch.hp_current);
    ac_buf_ = std::to_string(ch.ac);
    speed_buf_ = std::to_string(ch.speed);
    bg_buf_ = ch.background;
    equipment_ = ch.equipment;
    sel_equip_ = 0;
    scroll_offset_ = 0;
}

void CharList::SaveToFile(const std::string& path) const {
    json j;
    j["characters"] = json::array();
    for (const auto& c : characters_) {
        json ch;
        ch["name"] = c.name;
        ch["race"] = c.race;
        ch["class"] = c.char_class;
        ch["level"] = c.level;
        ch["str"] = c.str;
        ch["dex"] = c.dex;
        ch["con"] = c.con;
        ch["intel"] = c.intel;
        ch["wis"] = c.wis;
        ch["cha"] = c.cha;
        ch["hp_max"] = c.hp_max;
        ch["hp_current"] = c.hp_current;
        ch["ac"] = c.ac;
        ch["speed"] = c.speed;
        ch["equipment"] = c.equipment;
        ch["background"] = c.background;
        j["characters"].push_back(ch);
    }

    std::ofstream file(path);
    if (!file) throw std::runtime_error("Не удалось открыть файл: " + path);
    file << j.dump(2);
}

std::string CharList::SaveToJson() const {
    if (selected_ < 0 || selected_ >= (int)characters_.size()) return "{}";
    const auto& c = characters_[selected_];

    json ch;
    ch["name"] = c.name;
    ch["race"] = c.race;
    ch["class"] = c.char_class;
    ch["level"] = c.level;
    ch["str"] = c.str;
    ch["dex"] = c.dex;
    ch["con"] = c.con;
    ch["intel"] = c.intel;
    ch["wis"] = c.wis;
    ch["cha"] = c.cha;
    ch["hp_max"] = c.hp_max;
    ch["hp_current"] = c.hp_current;
    ch["ac"] = c.ac;
    ch["speed"] = c.speed;
    ch["equipment"] = c.equipment;
    ch["background"] = c.background;

    return ch.dump(2);
}

void CharList::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Не удалось открыть файл: " + path);

    json j;
    file >> j;

    characters_.clear();
    for (const auto& ch : j["characters"]) {
        Character c;
        c.name = ch.value("name", "Новый персонаж");
        c.race = ch.value("race", "Человек");
        c.char_class = ch.value("class", "Воин");
        c.level = ch.value("level", 1);
        c.str = ch.value("str", 10);
        c.dex = ch.value("dex", 10);
        c.con = ch.value("con", 10);
        c.intel = ch.value("intel", 10);
        c.wis = ch.value("wis", 10);
        c.cha = ch.value("cha", 10);
        c.hp_max = ch.value("hp_max", 10);
        c.hp_current = ch.value("hp_current", 10);
        c.ac = ch.value("ac", 10);
        c.speed = ch.value("speed", 30);
        c.equipment = ch.value("equipment", std::vector<std::string>{});
        c.background = ch.value("background", "");
        characters_.push_back(c);
    }

    if (characters_.empty()) {
        characters_.push_back(Character());
    }
}
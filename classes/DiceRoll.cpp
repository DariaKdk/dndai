#include "DiceRoll.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <random>
#include <thread>

DiceRoll::DiceRoll() {
    roll_btn_ = Button("Бросить d20", [this] {
        if (rolling_) return;
        rolling_ = true;
        anim_start_ = std::chrono::steady_clock::now();

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(1, 20);

        anim_numbers_.clear();
        for (int i = 0; i < 8; i++) {
            anim_numbers_.push_back(dist(rng));
        }
        anim_result_ = dist(rng);

        display_number_ = anim_numbers_[0];

        auto* screen = ScreenInteractive::Active();
        std::thread([this, screen]() {
            for (int i = 1; i < (int)anim_numbers_.size(); i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                int num = anim_numbers_[i];
                screen->PostEvent(Event::Special("d_" + std::to_string(num)));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            int res = anim_result_;
            screen->PostEvent(Event::Special("d_f_" + std::to_string(res)));
        }).detach();
    });

    auto container = Container::Vertical({roll_btn_});

    auto handler = CatchEvent(container, [this](Event event) {
        std::string in = event.input();
        if (in.size() > 2 && in[0] == 'd' && in[1] == '_') {
            if (in.size() > 4 && in[2] == 'f' && in[3] == '_') {
                display_number_ = std::stoi(in.substr(4));
                last_roll_ = display_number_;
                rolling_ = false;
                return true;
            }
            display_number_ = std::stoi(in.substr(2));
            return true;
        }
        return false;
    });

    auto renderer = Renderer(handler, [this] {
        auto now = std::chrono::steady_clock::now();
        if (rolling_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - anim_start_).count();
            if (elapsed > 2000) {
                display_number_ = anim_result_;
                last_roll_ = anim_result_;
                rolling_ = false;
            }
        }

        auto die = RenderDie(display_number_);

        std::string info;
        if (rolling_) {
            info = "Бросаем...";
        } else if (last_roll_ > 0) {
            info = "Результат: " + std::to_string(last_roll_);
        } else {
            info = "Нажмите кнопку";
        }

        Color info_color = rolling_ ? Color::Yellow : Color::GrayLight;

        Elements content;
        content.push_back(filler());
        content.push_back(die);
        content.push_back(text("") | flex);
        content.push_back(text(info) | center | color(info_color));
        content.push_back(text(""));
        content.push_back(roll_btn_->Render() | center);
        content.push_back(filler());

        return vbox(content) | flex | center;
    });

    Add(renderer);
}

int DiceRoll::GetLastRoll() const {
    return last_roll_;
}

Element DiceRoll::RenderDie(int number) const {
    std::string n = std::to_string(number);
    int pad_l = (4 - (int)n.size()) / 2;
    int pad_r = 4 - (int)n.size() - pad_l;
    std::string padded = std::string(pad_l, ' ') + n + std::string(pad_r, ' ');

    Color edge = rolling_ ? Color::Yellow : Color::Cyan;
    Color face;
    if (rolling_) {
        face = Color::Yellow;
    } else if (number == 20) {
        face = Color::Green;
    } else if (number == 1) {
        face = Color::Red;
    } else {
        face = Color::White;
    }

    Elements lines;
    lines.push_back(text("  ____") | color(edge));
    lines.push_back(text(" /    \\") | color(edge));
    lines.push_back(text("/ " + padded + " \\") | color(face) | bold);
    lines.push_back(text("\\      /") | color(edge));
    lines.push_back(text(" \\____/") | color(edge));

    return vbox(lines) | center;
}

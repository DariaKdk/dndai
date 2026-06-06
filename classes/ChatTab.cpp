#include "ChatTab.hpp"
#include <ftxui/screen/terminal.hpp>
#include <algorithm>
#include <sstream>


static std::vector<std::string> wrap_text(const std::string &text, int max_width) {
    if (max_width <= 0) return {text};

    std::vector<std::string> lines;
    std::istringstream line_stream(text);
    std::string paragraph;

    while (std::getline(line_stream, paragraph)) {
        if (paragraph.empty()) {
            lines.push_back("");
            continue;
        }
        std::istringstream word_stream(paragraph);
        std::string word;
        std::string current_line;

        while (word_stream >> word) {
            if (current_line.empty()) {
                current_line = word;
            } else if ((int) (current_line.length() + 1 + word.length()) <= max_width) {
                current_line += " " + word;
            } else {
                lines.push_back(current_line);
                current_line = word;
            }
        }
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }
    return lines;
}

static Element RenderSingleMessage(const ChatMessage &msg, int term_width) {
    int text_width = std::max(10, term_width - 6);
    auto wrapped = wrap_text(msg.text, text_width);

    Elements elements;
    elements.push_back(text(" > " + msg.author) | bold | color(msg.author_color));
    for (const auto &line: wrapped) {
        elements.push_back(text("   " + line) | color(Color::GrayLight));
    }

    return vbox(elements) | borderLight;
}


ChatTab::ChatTab() {
    input_text_ = "";
    input_component_ = Input(&input_text_, "Введите сообщение...");

    send_button_ = Button("Отправить", [this] { SendMessage(); });

    input_component_ |= CatchEvent([this](Event event) {
        if (generating_) {
            if (event == Event::Return) return true;
            return false;
        }
        if (event == Event::Return && !input_text_.empty()) {
            SendMessage();
            return true;
        }
        return false;
    });

    messages_view_ = Renderer([this] {
        return RenderMessages();
    }) | CatchEvent([this](Event event) {
        if (event.is_mouse()) {
            int max_scroll = std::max(0, content_height_ - 1);

            if (event.mouse().button == 4) {
                scroll_offset_ = std::max(0, scroll_offset_ - 3);
                auto_scroll_ = (scroll_offset_ >= max_scroll);
                return true;
            }
            if (event.mouse().button == 5) {
                scroll_offset_ = std::min(max_scroll, scroll_offset_ + 3);
                auto_scroll_ = (scroll_offset_ >= max_scroll);
                return true;
            }
        }
        return false;
    });

    auto input_panel = Container::Horizontal({input_component_, send_button_});

    auto input_renderer = Renderer(input_panel, [this] {
        std::string placeholder = "Введите сообщение...";

        auto input_box = hbox({
            input_component_->Render() | flex,
            separator(),
            send_button_->Render(),
        });

        if (generating_) {
            input_box |= dim;
        }

        return input_box | borderLight;
    });

    auto main_vertical = Container::Vertical({messages_view_, input_renderer});
    Add(main_vertical);
}

void ChatTab::RebuildMessages() {
    auto *screen = ScreenInteractive::Active();
    if (screen) {
        screen->PostEvent(Event::Custom);
    }
}

void ChatTab::AddMessage(const std::string &author, const std::string &text,
                         Color author_color) {
    {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        messages_.push_back({author, text, author_color});
    }
    auto_scroll_ = true;
    RebuildMessages();
}

void ChatTab::AppendToLastMessage(const std::string &token) {
    {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        if (!messages_.empty()) {
            messages_.back().text += token;
        }
    }
    RebuildMessages();
}

void ChatTab::SetGenerating(bool generating) {
    generating_ = generating;
    RebuildMessages();
}

void ChatTab::SetOnSendMessage(std::function<void(const std::string &)> callback) {
    on_send_ = std::move(callback);
}

void ChatTab::SendMessage() {
    if (generating_ || input_text_.empty()) return;

    std::string msg = input_text_;
    messages_.push_back({"Вы", msg});
    input_text_.clear();
    auto_scroll_ = true;
    RebuildMessages();

    if (on_send_) {
        on_send_(msg);
    }
}

Element ChatTab::RenderMessages() {
    int term_width = Terminal::Size().dimx;

    std::lock_guard<std::mutex> lock(messages_mutex_);

    Elements all_messages;
    int total_height = 0;

    for (const auto &msg: messages_) {
        int text_width = std::max(10, term_width - 6);
        auto wrapped = wrap_text(msg.text, text_width);
        total_height += (int) wrapped.size() + 3;

        all_messages.push_back(RenderSingleMessage(msg, term_width));
    }

    content_height_ = total_height;

    if (all_messages.empty()) {
        all_messages.push_back(
            text("  Чат пуст. Напишите сообщение.") | dim | color(Color::GrayLight));
        content_height_ = 1;
    }

    if (auto_scroll_) {
        scroll_offset_ = std::max(0, content_height_ - 1);
    }

    scroll_offset_ = std::min(scroll_offset_, std::max(0, content_height_ - 1));

    auto content = vbox(all_messages);
    return content | focusPosition(0, scroll_offset_)
           | vscroll_indicator
           | frame
           | flex;
}

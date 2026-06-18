/**
 * @file    CharacterSelectPanel.cpp
 * @brief   CharacterSelectPanel 实现
 */

#include "ui/CharacterSelectPanel.h"

#include "ui/UiTheme.h"

namespace
{
constexpr float kRowHeight = 36.f;
constexpr float kListTopOffset = 120.f;

const char* vocationLabel(uint8_t vocation)
{
    switch (vocation)
    {
    case CharacterDef::kVocationMage:
        return u8"法师";
    default:
        return u8"战士";
    }
}

const char* sexLabel(uint8_t sex)
{
    return sex == CharacterDef::kSexFemale ? u8"女" : u8"男";
}
}  // namespace

CharacterSelectPanel::CharacterSelectPanel()
    : m_theme(nullptr)
    , m_status(Status::Loading)
    , m_mode(Mode::Select)
    , m_lastUserId(0)
    , m_selectedIndex(-1)
    , m_selectedVocation(CharacterDef::kVocationWarrior)
    , m_selectedSex(CharacterDef::kSexMale)
{
}

void CharacterSelectPanel::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;

    const sf::FloatRect panel = panelRect();
    const float px = panel.left;
    const float py = panel.top;
    const float panelW = panel.width;

    m_nameInput.setup(theme, u8"道号", px + 40.f, py + 120.f, panelW - 80.f, 36.f);
    m_vocationWarriorBtn.setup(theme, u8"战士", px + 40.f, py + 180.f, 150.f, 36.f);
    m_vocationMageBtn.setup(theme, u8"法师", px + 210.f, py + 180.f, 150.f, 36.f);
    m_sexMaleBtn.setup(theme, u8"男", px + 40.f, py + 230.f, 150.f, 36.f);
    m_sexFemaleBtn.setup(theme, u8"女", px + 210.f, py + 230.f, 150.f, 36.f);

    m_enterGameButton.setup(theme, u8"进入游戏", px + 40.f, py + 420.f, 150.f, 40.f);
    m_createCharButton.setup(theme, u8"创建角色", px + 210.f, py + 420.f, 150.f, 40.f);
    m_backButton.setup(theme, u8"返回登录", px + 40.f, py + 470.f, panelW - 80.f, 36.f);

    m_confirmCreateButton.setup(theme, u8"确认创建", px + 40.f, py + 290.f, 150.f, 40.f);
    m_cancelCreateButton.setup(theme, u8"取消", px + 210.f, py + 290.f, 150.f, 40.f);

    m_vocationWarriorBtn.setOnClick([this]() {
        m_selectedVocation = CharacterDef::kVocationWarrior;
        refreshButtons();
    });
    m_vocationMageBtn.setOnClick([this]() {
        m_selectedVocation = CharacterDef::kVocationMage;
        refreshButtons();
    });
    m_sexMaleBtn.setOnClick([this]() {
        m_selectedSex = CharacterDef::kSexMale;
        refreshButtons();
    });
    m_sexFemaleBtn.setOnClick([this]() {
        m_selectedSex = CharacterDef::kSexFemale;
        refreshButtons();
    });

    m_enterGameButton.setOnClick([this]() {
        if (!m_onEnterGame || m_selectedIndex < 0 ||
            m_selectedIndex >= static_cast<int>(m_characters.size()))
        {
            return;
        }
        m_onEnterGame(m_characters[static_cast<size_t>(m_selectedIndex)].userID);
    });

    m_createCharButton.setOnClick([this]() {
        m_mode = Mode::Create;
        refreshButtons();
    });

    m_backButton.setOnClick([this]() {
        if (m_onBack)
        {
            m_onBack();
        }
    });

    m_confirmCreateButton.setOnClick([this]() {
        std::string err;
        if (!validateCreate(err))
        {
            m_status        = Status::Error;
            m_statusMessage = err;
            return;
        }
        if (m_onCreateCharacter)
        {
            m_status        = Status::Loading;
            m_statusMessage = u8"正在创建角色...";
            m_onCreateCharacter(m_nameInput.text(), m_selectedVocation, m_selectedSex);
        }
    });

    m_cancelCreateButton.setOnClick([this]() {
        if (m_characters.empty())
        {
            if (m_onBack)
            {
                m_onBack();
            }
            return;
        }
        m_mode = Mode::Select;
        refreshButtons();
    });

    refreshButtons();
}

void CharacterSelectPanel::setCharacters(const std::vector<CharacterEntry>& chars, uint64_t lastUserId)
{
    m_characters  = chars;
    m_lastUserId  = lastUserId;
    m_mode        = chars.empty() ? Mode::Create : Mode::Select;
    m_status      = Status::Ready;
    m_statusMessage.clear();
    refreshSelection();
    refreshButtons();
}

void CharacterSelectPanel::setStatus(Status status, const std::string& message)
{
    m_status        = status;
    m_statusMessage = message;
    refreshButtons();
}

void CharacterSelectPanel::reset()
{
    m_characters.clear();
    m_lastUserId    = 0;
    m_selectedIndex = -1;
    m_mode          = Mode::Select;
    m_status        = Status::Loading;
    m_statusMessage.clear();
    m_nameInput.setText("");
    m_selectedVocation = CharacterDef::kVocationWarrior;
    m_selectedSex      = CharacterDef::kSexMale;
    refreshButtons();
}

void CharacterSelectPanel::setOnEnterGame(std::function<void(uint64_t userId)> cb)
{
    m_onEnterGame = std::move(cb);
}

void CharacterSelectPanel::setOnCreateCharacter(
    std::function<void(const std::string&, uint8_t, uint8_t)> cb)
{
    m_onCreateCharacter = std::move(cb);
}

void CharacterSelectPanel::setOnBack(std::function<void()> cb)
{
    m_onBack = std::move(cb);
}

void CharacterSelectPanel::refreshSelection()
{
    m_selectedIndex = -1;
    if (m_characters.empty())
    {
        return;
    }

    if (m_lastUserId != 0)
    {
        for (size_t i = 0; i < m_characters.size(); ++i)
        {
            if (m_characters[i].userID == m_lastUserId)
            {
                m_selectedIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (m_selectedIndex < 0)
    {
        m_selectedIndex = 0;
    }
}

void CharacterSelectPanel::refreshButtons()
{
    const bool ready = m_status == Status::Ready;
    const bool loading = m_status == Status::Loading;

    m_enterGameButton.setEnabled(ready && m_mode == Mode::Select && m_selectedIndex >= 0 && !loading);
    m_createCharButton.setEnabled(ready && m_mode == Mode::Select && !loading);
    m_backButton.setEnabled(!loading);
    m_confirmCreateButton.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_cancelCreateButton.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_vocationWarriorBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_vocationMageBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_sexMaleBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_sexFemaleBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
}

bool CharacterSelectPanel::validateCreate(std::string& err) const
{
    const std::string name = m_nameInput.text();
    if (name.empty())
    {
        err = u8"请输入角色名";
        return false;
    }
    if (name.size() > 31)
    {
        err = u8"角色名过长";
        return false;
    }
    return true;
}

sf::FloatRect CharacterSelectPanel::panelRect() const
{
    const float panelW = 420.f;
    const float panelH = 540.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;
    return sf::FloatRect(px, py, panelW, panelH);
}

sf::FloatRect CharacterSelectPanel::listAreaRect() const
{
    const sf::FloatRect panel = panelRect();
    return sf::FloatRect(panel.left + 20.f,
                         panel.top + kListTopOffset,
                         panel.width - 40.f,
                         220.f);
}

int CharacterSelectPanel::rowAtPosition(const sf::Vector2f& pos) const
{
    const sf::FloatRect list = listAreaRect();
    if (!list.contains(pos))
    {
        return -1;
    }
    const int row = static_cast<int>((pos.y - list.top) / kRowHeight);
    if (row < 0 || row >= static_cast<int>(m_characters.size()))
    {
        return -1;
    }
    return row;
}

void CharacterSelectPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (m_mode == Mode::Create)
    {
        const bool canEdit = m_status == Status::Ready;
        if (canEdit)
        {
            m_nameInput.handleEvent(event, window);
        }
        m_vocationWarriorBtn.handleEvent(event, window);
        m_vocationMageBtn.handleEvent(event, window);
        m_sexMaleBtn.handleEvent(event, window);
        m_sexFemaleBtn.handleEvent(event, window);
        m_confirmCreateButton.handleEvent(event, window);
        m_cancelCreateButton.handleEvent(event, window);
    }
    else
    {
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left)
        {
            const sf::Vector2f pos = window.mapPixelToCoords(
                {event.mouseButton.x, event.mouseButton.y});
            const int row = rowAtPosition(pos);
            if (row >= 0)
            {
                m_selectedIndex = row;
                refreshButtons();
            }
        }
    }

    m_enterGameButton.handleEvent(event, window);
    m_createCharButton.handleEvent(event, window);
    m_backButton.handleEvent(event, window);
}

void CharacterSelectPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const sf::FloatRect panel = panelRect();
    m_theme->drawPanel(target, panel);
    m_theme->drawTitle(target, u8"选择角色", panel.left + panel.width / 2.f, panel.top + 16.f, 28);

    if (!m_statusMessage.empty())
    {
        const sf::Color color = m_status == Status::Error ? sf::Color(255, 120, 120)
                                                          : m_theme->accentColor();
        m_theme->drawText(target,
                          m_statusMessage,
                          panel.left + 40.f,
                          panel.top + 56.f,
                          16,
                          color);
    }

    if (m_mode == Mode::Create)
    {
        m_theme->drawText(target,
                          u8"创建新角色",
                          panel.left + 40.f,
                          panel.top + 90.f,
                          18,
                          m_theme->textColor());
        m_nameInput.draw(target);
        m_vocationWarriorBtn.draw(target);
        m_vocationMageBtn.draw(target);
        m_sexMaleBtn.draw(target);
        m_sexFemaleBtn.draw(target);
        m_confirmCreateButton.draw(target);
        m_cancelCreateButton.draw(target);
    }
    else if (m_status == Status::Loading)
    {
        m_theme->drawText(target,
                          m_statusMessage.empty() ? u8"正在获取角色列表..." : m_statusMessage,
                          panel.left + 40.f,
                          panel.top + 200.f,
                          18,
                          m_theme->accentColor());
    }
    else
    {
        const sf::FloatRect list = listAreaRect();
        m_theme->drawText(target, u8"角色列表", panel.left + 40.f, panel.top + 90.f, 18, m_theme->textColor());

        for (size_t i = 0; i < m_characters.size(); ++i)
        {
            const float rowY = list.top + static_cast<float>(i) * kRowHeight;
            const bool selected = static_cast<int>(i) == m_selectedIndex;
            if (selected)
            {
                sf::RectangleShape highlight(sf::Vector2f(list.width, kRowHeight - 2.f));
                highlight.setPosition(list.left, rowY);
                highlight.setFillColor(sf::Color(60, 100, 90, 120));
                target.draw(highlight);
            }

            const CharacterEntry& ch = m_characters[i];
            std::string line = ch.name + u8"  Lv." + std::to_string(ch.level) + u8"  " +
                               vocationLabel(ch.vocation) + u8"  " + sexLabel(ch.sex);
            if (ch.userID == m_lastUserId)
            {
                line += u8"  [上次登录]";
            }

            m_theme->drawText(target,
                              line,
                              list.left + 8.f,
                              rowY + 8.f,
                              16,
                              selected ? m_theme->accentColor() : m_theme->textColor());
        }
    }

    m_enterGameButton.draw(target);
    m_createCharButton.draw(target);
    m_backButton.draw(target);
}

/**
 * @file    CharacterSelectPanel.cpp
 * @brief   CharacterSelectPanel 实现
 */

#include "ui/CharacterSelectPanel.h"

#include "LoginCommon.h"
#include "ui/UiTheme.h"
#include "util/TextUtil.h"

namespace
{
constexpr float kPanelW     = 720.f;
constexpr float kPanelH     = 520.f;
constexpr float kListW      = 280.f;
constexpr float kListH      = 240.f;
constexpr float kRowHeight  = 36.f;
constexpr float kBtnW       = 150.f;
constexpr float kBtnH       = 40.f;
constexpr float kBottomBtnY = 420.f;

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

void drawSectionPanel(sf::RenderTarget& target,
                      const sf::FloatRect& rect,
                      const sf::Color& fill,
                      const sf::Color& border)
{
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(fill);
    box.setOutlineColor(border);
    box.setOutlineThickness(1.f);
    target.draw(box);

    sf::RectangleShape accent({3.f, rect.height - 8.f});
    accent.setPosition(rect.left + 4.f, rect.top + 4.f);
    accent.setFillColor(sf::Color(212, 175, 55, 200));
    target.draw(accent);
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
    , m_previewAnimTime(0.f)
{
}

void CharacterSelectPanel::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;

    const sf::FloatRect panel = panelRect();
    const float px = panel.left;
    const float py = panel.top;

    const float formLeft = px + 28.f;
    const float formTop  = py + 92.f;

    m_nameInput.setup(theme, u8"角色名（2–12 码点）", formLeft + 12.f, formTop + 8.f, 276.f, 36.f);
    m_nameInput.setMaxCodepoints(MAX_ROLE_NAME_LEN);

    m_vocationWarriorBtn.setup(theme, u8"战士 · 近战", formLeft + 12.f, formTop + 58.f, 276.f, 44.f);
    m_vocationMageBtn.setup(theme, u8"法师 · 法术", formLeft + 12.f, formTop + 110.f, 276.f, 44.f);
    m_sexMaleBtn.setup(theme, u8"男侠", formLeft + 12.f, formTop + 168.f, 130.f, 40.f);
    m_sexFemaleBtn.setup(theme, u8"女侠", formLeft + 158.f, formTop + 168.f, 130.f, 40.f);

    m_confirmCreateButton.setup(theme, u8"确认创建", formLeft + 12.f, formTop + 224.f, 130.f, 42.f);
    m_cancelCreateButton.setup(theme, u8"取消", formLeft + 158.f, formTop + 224.f, 130.f, 40.f);

    const float centerX = px + panel.width / 2.f;
    m_enterGameButton.setup(theme, u8"进入游戏", centerX - kBtnW / 2.f, py + kBottomBtnY, kBtnW, kBtnH);
    m_createCharButton.setup(
        theme, u8"创建角色", px + panel.width - kBtnW - 40.f, py + kBottomBtnY, kBtnW, kBtnH);
    m_backButton.setup(theme, u8"返回登录", px + 40.f, py + 470.f, panel.width - 80.f, 36.f);

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
        m_statusMessage.clear();
        m_previewAnimTime = 0.f;
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
        m_mode = Mode::Select;
        m_statusMessage.clear();
        if (m_status == Status::Error)
        {
            m_status = Status::Ready;
        }
        refreshButtons();
    });

    refreshButtons();
}

bool CharacterSelectPanel::getSelectedAppearance(uint8_t& vocation, uint8_t& sex) const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_characters.size()))
    {
        return false;
    }
    const CharacterEntry& ch = m_characters[static_cast<size_t>(m_selectedIndex)];
    vocation = ch.vocation;
    sex      = ch.sex;
    return true;
}

void CharacterSelectPanel::setCharacters(const std::vector<CharacterEntry>& chars, uint64_t lastUserId)
{
    m_characters  = chars;
    m_lastUserId  = lastUserId;
    m_mode        = Mode::Select;
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
    m_previewAnimTime  = 0.f;
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
    const bool ready   = m_status == Status::Ready;
    const bool loading = m_status == Status::Loading;
    const bool hasChar   = m_selectedIndex >= 0 &&
                         m_selectedIndex < static_cast<int>(m_characters.size());

    m_enterGameButton.setEnabled(ready && m_mode == Mode::Select && hasChar && !loading);
    m_createCharButton.setEnabled(ready && m_mode == Mode::Select && !loading);
    m_backButton.setEnabled(!loading);
    m_confirmCreateButton.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_cancelCreateButton.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_vocationWarriorBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_vocationMageBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_sexMaleBtn.setEnabled(ready && m_mode == Mode::Create && !loading);
    m_sexFemaleBtn.setEnabled(ready && m_mode == Mode::Create && !loading);

    m_vocationWarriorBtn.setSelected(m_selectedVocation == CharacterDef::kVocationWarrior);
    m_vocationMageBtn.setSelected(m_selectedVocation == CharacterDef::kVocationMage);
    m_sexMaleBtn.setSelected(m_selectedSex == CharacterDef::kSexMale);
    m_sexFemaleBtn.setSelected(m_selectedSex == CharacterDef::kSexFemale);
}

bool CharacterSelectPanel::validateCreate(std::string& err) const
{
    const std::string name = m_nameInput.text();
    if (name.empty())
    {
        err = u8"请输入角色名";
        return false;
    }
    const std::size_t charCount = TextUtil::utf8CodepointCount(name);
    if (charCount < MIN_ROLE_NAME_LEN)
    {
        err = u8"角色名过短";
        return false;
    }
    if (charCount > MAX_ROLE_NAME_LEN)
    {
        err = u8"角色名过长";
        return false;
    }
    return true;
}

sf::FloatRect CharacterSelectPanel::panelRect() const
{
    const float px = (static_cast<float>(m_viewSize.x) - kPanelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - kPanelH) / 2.f;
    return sf::FloatRect(px, py, kPanelW, kPanelH);
}

sf::FloatRect CharacterSelectPanel::listAreaRect() const
{
    const sf::FloatRect panel = panelRect();
    return sf::FloatRect(panel.left + panel.width - kListW - 20.f,
                         panel.top + 80.f,
                         kListW,
                         kListH);
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
    else if (m_status == Status::Ready)
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

    if (m_mode == Mode::Select)
    {
        m_enterGameButton.handleEvent(event, window);
        m_createCharButton.handleEvent(event, window);
    }
    m_backButton.handleEvent(event, window);
}

void CharacterSelectPanel::update(float dt)
{
    if (m_mode == Mode::Create)
    {
        m_previewAnimTime += dt;
        if (m_status == Status::Ready)
        {
            m_nameInput.update(dt);
        }
    }
}

void CharacterSelectPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const sf::FloatRect panel = panelRect();
    m_theme->drawPanel(target, panel);
    const char* title = m_mode == Mode::Create ? u8"创建角色" : u8"选择角色";
    m_theme->drawTitle(target, title, panel.left + panel.width / 2.f, panel.top + 16.f, 28);

    if (!m_statusMessage.empty())
    {
        const sf::Color color = m_status == Status::Error ? sf::Color(255, 120, 120)
                                                        : m_theme->accentColor();
        m_theme->drawText(target,
                          m_statusMessage,
                          panel.left + 40.f,
                          panel.top + 52.f,
                          16,
                          color);
    }

    if (m_mode == Mode::Create)
    {
        const float px = panel.left;
        const float py = panel.top;
        const sf::FloatRect formRect(px + 20.f, py + 84.f, 320.f, 300.f);
        const sf::FloatRect previewRect(px + 360.f, py + 84.f, 320.f, 300.f);

        drawSectionPanel(target,
                         formRect,
                         sf::Color(30, 45, 48, 90),
                         sf::Color(100, 130, 125, 120));
        drawSectionPanel(target,
                         previewRect,
                         sf::Color(25, 40, 50, 100),
                         sf::Color(212, 175, 55, 140));

        m_theme->drawText(target,
                          u8"角色信息",
                          formRect.left + 16.f,
                          formRect.top + 8.f,
                          16,
                          m_theme->titleColor());
        m_theme->drawText(target,
                          u8"侠客预览",
                          previewRect.left + 16.f,
                          previewRect.top + 8.f,
                          16,
                          m_theme->titleColor());

        const float previewCenterX = previewRect.left + previewRect.width / 2.f;
        const float previewCenterY = previewRect.top + previewRect.height / 2.f + 24.f;
        sf::RectangleShape stage({previewRect.width - 40.f, 180.f});
        stage.setPosition(previewRect.left + 20.f, previewRect.top + 48.f);
        stage.setFillColor(sf::Color(15, 30, 35, 120));
        stage.setOutlineColor(sf::Color(64, 180, 170, 80));
        stage.setOutlineThickness(1.f);
        target.draw(stage);

        const CharacterSprite& sprite =
            CharacterSpriteLibrary::instance().get(m_selectedVocation, m_selectedSex);
        sprite.draw(target, previewCenterX, previewCenterY + 40.f, 0.f, true, m_previewAnimTime);

        m_theme->drawText(target,
                          u8"支持中英文、数字与下划线",
                          previewRect.left + 20.f,
                          previewRect.top + previewRect.height - 36.f,
                          13,
                          m_theme->textColor());

        m_nameInput.draw(target);
        m_vocationWarriorBtn.draw(target);
        m_vocationMageBtn.draw(target);
        m_sexMaleBtn.draw(target);
        m_sexFemaleBtn.draw(target);
        m_confirmCreateButton.draw(target);
        m_cancelCreateButton.draw(target);

        const sf::FloatRect list = listAreaRect();
        sf::RectangleShape dim({list.width, list.height});
        dim.setPosition(list.left, list.top);
        dim.setFillColor(sf::Color(0, 0, 0, 100));
        target.draw(dim);
    }

    const sf::FloatRect list = listAreaRect();
    if (m_mode == Mode::Select)
    {
        m_theme->drawText(target,
                          u8"角色列表",
                          list.left,
                          list.top - 28.f,
                          18,
                          m_theme->textColor());
    }

    if (m_status == Status::Loading)
    {
        m_theme->drawText(target,
                          m_statusMessage.empty() ? u8"正在连接..." : m_statusMessage,
                          list.left,
                          list.top + 80.f,
                          18,
                          m_theme->accentColor());
    }
    else if (m_mode == Mode::Select)
    {
        if (m_characters.empty())
        {
            m_theme->drawText(target,
                              u8"暂无角色",
                              list.left + 8.f,
                              list.top + 12.f,
                              16,
                              m_theme->textColor());
        }
        else
        {
            const size_t maxRows = static_cast<size_t>(kListH / kRowHeight);
            const size_t drawCount = std::min(m_characters.size(), maxRows);
            for (size_t i = 0; i < drawCount; ++i)
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
                    line += u8"  [上次]";
                }

                m_theme->drawText(target,
                                  line,
                                  list.left + 8.f,
                                  rowY + 8.f,
                                  15,
                                  selected ? m_theme->accentColor() : m_theme->textColor());
            }
        }
    }

    if (m_mode == Mode::Select)
    {
        m_enterGameButton.draw(target);
        m_createCharButton.draw(target);
    }
    m_backButton.draw(target);
}

/**
 * @file    CharacterSelectPanel.h
 * @brief   账号登录后的角色选择与创建面板
 *
 * 职责：
 * - 展示 S2C_USER_LIST 下发的角色列表
 * - 选择角色进入游戏，或创建新角色
 *
 * 协作：GameApp、LoginSession（selectCharacter / createCharacter）。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "net/CharacterTypes.h"
#include "ui/widgets/Button.h"
#include "ui/widgets/TextInput.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class UiTheme;

/**
 * @brief 选角/创角面板
 */
class CharacterSelectPanel
{
public:
    enum class Status
    {
        Loading,
        Ready,
        Error,
    };

    enum class Mode
    {
        Select,
        Create,
    };

    CharacterSelectPanel();

    void setup(UiTheme* theme, const sf::Vector2u& viewSize);

    void setCharacters(const std::vector<CharacterEntry>& chars, uint64_t lastUserId);
    void setStatus(Status status, const std::string& message = "");
    void reset();

    void setOnEnterGame(std::function<void(uint64_t userId)> cb);
    void setOnCreateCharacter(std::function<void(const std::string& name,
                                                 uint8_t vocation,
                                                 uint8_t sex)> cb);
    void setOnBack(std::function<void()> cb);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderTarget& target) const;

private:
    void refreshSelection();
    void refreshButtons();
    bool validateCreate(std::string& err) const;
    int rowAtPosition(const sf::Vector2f& pos) const;
    sf::FloatRect listAreaRect() const;
    sf::FloatRect panelRect() const;

    UiTheme*                    m_theme;
    sf::Vector2u                m_viewSize;
    Status                      m_status;
    Mode                        m_mode;
    std::string                 m_statusMessage;
    std::vector<CharacterEntry> m_characters;
    uint64_t                    m_lastUserId;
    int                         m_selectedIndex;

    TextInput                   m_nameInput;
    Button                      m_vocationWarriorBtn;
    Button                      m_vocationMageBtn;
    Button                      m_sexMaleBtn;
    Button                      m_sexFemaleBtn;
    Button                      m_enterGameButton;
    Button                      m_createCharButton;
    Button                      m_backButton;
    Button                      m_confirmCreateButton;
    Button                      m_cancelCreateButton;

    uint8_t                     m_selectedVocation;
    uint8_t                     m_selectedSex;

    std::function<void(uint64_t)> m_onEnterGame;
    std::function<void(const std::string&, uint8_t, uint8_t)> m_onCreateCharacter;
    std::function<void()>         m_onBack;
};

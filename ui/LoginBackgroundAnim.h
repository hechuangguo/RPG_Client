/**
 * @file    LoginBackgroundAnim.h
 * @brief   登录背景全画面序列帧动画
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <string>

/**
 * @brief 登录界面一体化动态背景（单张横排全画面 sprite sheet）
 */
class LoginBackgroundAnim
{
public:
    LoginBackgroundAnim();

    /**
     * @brief 从 exe 目录加载 assets/ui/login_bg_anim.json 与全画面序列帧
     * @param exeDir 可执行文件所在目录
     */
    bool load(const std::string& exeDir);

    bool isLoaded() const;

    /** @brief 单帧宽度（像素），用于 cover 宽高比计算 */
    unsigned frameWidth() const;

    /** @brief 单帧高度（像素） */
    unsigned frameHeight() const;

    void update(float dt);

    void draw(sf::RenderTarget& target, float scale, float offsetX, float offsetY) const;

private:
    sf::Texture m_texture;
    int         m_frames;
    float       m_fps;
    float       m_frameTimer;
    int         m_frameIndex;
    bool        m_loaded;
};

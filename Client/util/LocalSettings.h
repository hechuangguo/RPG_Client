/**
 * @file    LocalSettings.h
 * @brief   客户端本地用户偏好持久化
 *
 * 职责：
 * - 保存/加载 lastAccount、lastZoneId 到 Windows %APPDATA%/RPGClient/settings.json
 * - 支持「记住账号」登录体验
 *
 * 协作：LoginPanel 读写；GameApp 启动时 load()。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief 本地设置管理器
 */
class LocalSettings
{
public:
    LocalSettings();

    /**
     * @brief 从用户 AppData 目录加载设置
     * @return 文件不存在时返回 true 并使用默认值
     */
    bool load();

    /**
     * @brief 保存当前设置到 AppData
     * @return 写入成功返回 true
     */
    bool save() const;

    /** @brief 上次登录账号 */
    const std::string& lastAccount() const;

    /**
     * @brief 设置上次登录账号
     * @param account 账号字符串
     */
    void setLastAccount(const std::string& account);

    /** @brief 上次选择的游戏区号（0 表示未选） */
    uint32_t lastZoneId() const;

    /**
     * @brief 设置上次选择的游戏区号
     * @param zoneId 区号
     */
    void setLastZoneId(uint32_t zoneId);

    /** @brief 是否记住账号 */
    bool rememberAccount() const;

    /**
     * @brief 设置是否记住账号
     * @param remember true 则 save 时写入 lastAccount
     */
    void setRememberAccount(bool remember);

    /** @brief 设置文件绝对路径（调试用） */
    std::string settingsPath() const;

private:
    bool ensureDirectory() const;

    std::string m_lastAccount;
    uint32_t    m_lastZoneId;
    bool        m_rememberAccount;
};

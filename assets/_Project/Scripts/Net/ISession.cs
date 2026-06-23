/// <summary>
/// 网络会话统一接口。
/// 所有网络会话（LoginSession / GameSession / 未来 BattleSession 等）均实现此接口，
/// 方便 GameApp 统一管理生命周期，支持运行时动态增删。
/// </summary>
using Rpg.Client.Config;

namespace Rpg.Client.Net
{
    public interface ISession
    {
        /// <summary>每帧驱动：Poll 收包、心跳、超时检测。</summary>
        void Update();

        /// <summary>注入客户端配置（连接地址、超时、心跳间隔等）。</summary>
        void SetConfig(ClientConfigLoader config);

        /// <summary>取消当前会话，断开连接并清理内部状态。幂等，重复调用安全。</summary>
        void Cancel();

        /// <summary>清空所有对外回调委托，防止 GameApp 切换状态后残留回调触发。</summary>
        void ClearHandlers();
    }
}

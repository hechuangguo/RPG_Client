/// <summary>
/// 游戏脚本宿主。职责：接收 GameSession 消息并更新 Quest/Bag 模型；聊天/任务/背包消息回调。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.Game;
using Rpg.Client.Log;
using Rpg.Proto.Bag;
using Rpg.Proto.Chat;
using QuestProto = Rpg.Proto.Quest;

namespace Rpg.Client.Scripting
{
    public sealed class GameScriptHost
    {
        private readonly QuestModel _quests = new QuestModel();
        private readonly ItemBagModel _bag = new ItemBagModel();
        private readonly List<BagSlotEntry> _bagSyncScratch = new List<BagSlotEntry>();

        public QuestModel Quests => _quests;
        public ItemBagModel Bag => _bag;

        /// <summary>聊天消息回调 (channel, senderName, content)。HUD 面板订阅此事件显示聊天。</summary>
        public event Action<string, string, string> OnChatMessage;
        /// <summary>系统公告回调。</summary>
        public event Action<string> OnNoticeMessage;

        public void OnEnterGame(ulong userId, uint mapId)
        {
            ClientLogger.Instance.InfoFormat("GameScriptHost：进入游戏 userId={0} mapId={1}", userId, mapId);
        }

        public void OnTick(long nowMs)
        {
            // Phase 3：转发至 Lua OnTick
        }

        public void OnChat(S2CChatNotify chat)
        {
            if (chat == null)
            {
                return;
            }

            ClientLogger.Instance.InfoFormat(
                "GameScriptHost：聊天 channel={0} {1}: {2}",
                chat.Channel,
                chat.FromName ?? string.Empty,
                chat.Content ?? string.Empty);

            OnChatMessage?.Invoke(
                chat.Channel.ToString(),
                chat.FromName ?? string.Empty,
                chat.Content ?? string.Empty);
        }

        public void OnNotice(string content)
        {
            ClientLogger.Instance.InfoFormat("GameScriptHost：公告 {0}", content ?? string.Empty);
            OnNoticeMessage?.Invoke(content ?? string.Empty);
        }

        public void OnQuestInfo(QuestProto.S2CQuestInfo info)
        {
            if (info == null)
            {
                return;
            }

            if (info.Code != 0)
            {
                ClientLogger.Instance.WarnFormat("GameScriptHost：任务同步失败 code={0}", info.Code);
                OnNoticeMessage?.Invoke($"任务同步失败（code={info.Code}）");
                return;
            }

            var entries = new List<Game.QuestEntry>();
            if (info.Entries != null)
            {
                foreach (var e in info.Entries)
                {
                    entries.Add(new Game.QuestEntry
                    {
                        QuestId = e.QuestId,
                        Name = e.Name ?? string.Empty,
                        Progress = e.Progress,
                        Target = e.Target,
                        Done = e.Done
                    });
                    ClientLogger.Instance.InfoFormat(
                        "GameScriptHost：任务 {0} {1} {2}/{3} done={4}",
                        e.QuestId,
                        e.Name ?? string.Empty,
                        e.Progress,
                        e.Target,
                        e.Done);
                }
            }

            _quests.ReplaceAll(entries);
        }

        public void OnBagInfo(S2CBagInfoRsp rsp)
        {
            if (rsp == null)
            {
                return;
            }

            if (rsp.Code != 0)
            {
                ClientLogger.Instance.WarnFormat("GameScriptHost：背包同步失败 code={0}", rsp.Code);
                OnNoticeMessage?.Invoke($"背包同步失败（code={rsp.Code}）");
                return;
            }

            _bagSyncScratch.Clear();
            if (rsp.Slots != null)
            {
                foreach (var s in rsp.Slots)
                {
                    _bagSyncScratch.Add(new BagSlotEntry { ItemId = s.ItemId, Count = s.Count });
                }
            }

            _bag.SetSlots(_bagSyncScratch);
            ClientLogger.Instance.InfoFormat("GameScriptHost：背包同步 {0} 格", _bagSyncScratch.Count);
        }
    }
}

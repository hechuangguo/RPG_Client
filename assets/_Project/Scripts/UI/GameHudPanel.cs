/// <summary>
/// 游戏内 HUD 面板。职责：聊天消息展示/输入、状态栏（坐标 / RTT / FPS）、任务追踪、背包摘要。
/// 布局全部程序化生成，无需 Prefab。
/// </summary>
using System.Collections.Generic;
using System.Text;
using Rpg.Client.Game;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class GameHudPanel : MonoBehaviour
    {
        /// <summary>聊天记录最大保留条数。</summary>
        private const int MaxChatLines = 50;

        /// <summary>布局参数。</summary>
        private const float ChatAreaWidth = 420f;
        private const float ChatAreaHeight = 280f;
        private const float InputHeight = 40f;
        private const float StatusBarHeight = 28f;
        private const float MinimapSize = 140f;
        private const float QuestPanelWidth = 260f;
        private const float QuestPanelHeight = 120f;
        private const float BagBarHeight = 36f;
        private const float Padding = 8f;
        private const int MaxBagPreviewSlots = 8;

        [Header("Chat")]
        [SerializeField] private ScrollRect _chatScroll;
        [SerializeField] private Text _chatLogText;
        [SerializeField] private InputField _chatInput;
        [SerializeField] private Button _sendBtn;

        [Header("Status")]
        [SerializeField] private Text _statusText;

        [Header("Minimap (Phase 2)")]
        [SerializeField] private RawImage _minimapPlaceholder;

        [Header("Quest / Bag")]
        [SerializeField] private Text _questTrackerText;
        [SerializeField] private Text _bagBarText;

        private readonly List<string> _chatLines = new List<string>();
        private readonly StringBuilder _chatBuilder = new StringBuilder(1024);
        private int _chatDisplayedCount;
        private System.Func<string, bool> _onChatSend;
        private Canvas _canvas;
        private QuestModel _quests;
        private ItemBagModel _bag;

        /// <summary>注册聊天发送回调。返回 true 表示发送成功（HUD 清空输入框）。</summary>
        public void SetChatSendCallback(System.Func<string, bool> callback) => _onChatSend = callback;

        /// <summary>绑定任务/背包模型并订阅变更。</summary>
        public void BindModels(QuestModel quests, ItemBagModel bag)
        {
            UnbindModels();
            _quests = quests;
            _bag = bag;
            if (_quests != null)
            {
                _quests.OnChanged += RefreshQuestTracker;
            }

            if (_bag != null)
            {
                _bag.OnChanged += RefreshBagBar;
            }

            RefreshQuestTracker();
            RefreshBagBar();
        }

        private void Awake()
        {
            _canvas = GetComponentInParent<Canvas>();
            EnsureLayout();
            WireChatInput();
        }

        /// <summary>追加一条聊天消息到日志区域。</summary>
        public void AppendChat(string channel, string sender, string content)
        {
            var prefix = string.IsNullOrEmpty(channel) ? "" : $"[{channel}]";
            var from = string.IsNullOrEmpty(sender) ? "" : $"<b>{sender}</b>";
            var line = $"{prefix} {from}: {content}";
            _chatLines.Add(line);
            while (_chatLines.Count > MaxChatLines)
            {
                _chatLines.RemoveAt(0);
                _chatDisplayedCount = Mathf.Max(0, _chatDisplayedCount - 1);
            }

            RefreshChatLog();
            ScrollToBottom();
        }

        /// <summary>追加纯提示文本（系统消息、公告）。</summary>
        public void AppendNotice(string text)
        {
            _chatLines.Add($"<color=#FFD700>[系统]</color> {text}");
            while (_chatLines.Count > MaxChatLines)
            {
                _chatLines.RemoveAt(0);
                _chatDisplayedCount = Mathf.Max(0, _chatDisplayedCount - 1);
            }

            RefreshChatLog();
            ScrollToBottom();
        }

        /// <summary>更新状态栏（坐标 / RTT / FPS）。</summary>
        public void SetStatus(string status)
        {
            if (_statusText != null)
            {
                _statusText.text = status ?? string.Empty;
            }
        }

        private void RefreshQuestTracker()
        {
            if (_questTrackerText == null)
            {
                return;
            }

            if (_quests == null || _quests.Entries.Count == 0)
            {
                _questTrackerText.text = "任务：暂无";
                return;
            }

            _chatBuilder.Clear();
            _chatBuilder.AppendLine("任务追踪");
            foreach (var entry in _quests.Entries.Values)
            {
                if (entry == null)
                {
                    continue;
                }

                var status = entry.Done ? "已完成" : $"{entry.Progress}/{entry.Target}";
                _chatBuilder.Append("• ");
                _chatBuilder.Append(string.IsNullOrEmpty(entry.Name) ? $"#{entry.QuestId}" : entry.Name);
                _chatBuilder.Append(" (");
                _chatBuilder.Append(status);
                _chatBuilder.AppendLine(")");
            }

            _questTrackerText.text = _chatBuilder.ToString().TrimEnd();
        }

        private void RefreshBagBar()
        {
            if (_bagBarText == null)
            {
                return;
            }

            if (_bag == null || _bag.Slots.Count == 0)
            {
                _bagBarText.text = "背包：空";
                return;
            }

            _chatBuilder.Clear();
            _chatBuilder.Append("背包：");
            var shown = 0;
            foreach (var slot in _bag.Slots)
            {
                if (slot == null || slot.ItemId == 0)
                {
                    continue;
                }

                if (shown > 0)
                {
                    _chatBuilder.Append(" | ");
                }

                _chatBuilder.Append('#');
                _chatBuilder.Append(slot.ItemId);
                _chatBuilder.Append('×');
                _chatBuilder.Append(slot.Count);
                shown++;
                if (shown >= MaxBagPreviewSlots)
                {
                    break;
                }
            }

            if (_bag.Slots.Count > MaxBagPreviewSlots)
            {
                _chatBuilder.Append(" …");
            }

            _bagBarText.text = _chatBuilder.ToString();
        }

        private void RefreshChatLog()
        {
            if (_chatLogText == null)
            {
                return;
            }

            if (_chatLines.Count < _chatDisplayedCount)
            {
                _chatLogText.text = string.Join("\n", _chatLines);
                _chatDisplayedCount = _chatLines.Count;
                return;
            }

            if (_chatDisplayedCount == 0)
            {
                _chatLogText.text = string.Join("\n", _chatLines);
            }
            else if (_chatLines.Count > _chatDisplayedCount)
            {
                _chatBuilder.Clear();
                if (!string.IsNullOrEmpty(_chatLogText.text))
                {
                    _chatBuilder.Append(_chatLogText.text);
                }

                for (var i = _chatDisplayedCount; i < _chatLines.Count; i++)
                {
                    if (_chatBuilder.Length > 0)
                    {
                        _chatBuilder.Append('\n');
                    }

                    _chatBuilder.Append(_chatLines[i]);
                }

                _chatLogText.text = _chatBuilder.ToString();
            }

            _chatDisplayedCount = _chatLines.Count;
        }

        private void ScrollToBottom()
        {
            if (_chatScroll == null)
            {
                return;
            }

            Canvas.ForceUpdateCanvases();
            _chatScroll.verticalNormalizedPosition = 0f;
        }

        private void WireChatInput()
        {
            _chatInput?.onEndEdit.AddListener(text =>
            {
                if (!Input.GetKeyDown(KeyCode.Return) && !Input.GetKeyDown(KeyCode.KeypadEnter))
                {
                    return;
                }

                SendChatMessage();
            });
            _sendBtn?.onClick.AddListener(SendChatMessage);
        }

        private void SendChatMessage()
        {
            var text = _chatInput?.text?.Trim();
            if (string.IsNullOrEmpty(text))
            {
                return;
            }

            if (_onChatSend?.Invoke(text) == true && _chatInput != null)
            {
                _chatInput.text = string.Empty;
                _chatInput.ActivateInputField();
            }
        }

        /// <summary>确保 UI 布局存在。优先使用序列化字段，缺失时程序化生成。</summary>
        private void EnsureLayout()
        {
            // ---- 状态栏（顶部全宽） ----
            if (_statusText == null)
            {
                var go = new GameObject("StatusBar", typeof(RectTransform), typeof(Text));
                go.transform.SetParent(transform, false);
                var srt = go.GetComponent<RectTransform>();
                srt.anchorMin = new Vector2(0, 1);
                srt.anchorMax = new Vector2(1, 1);
                srt.pivot = new Vector2(0.5f, 1);
                srt.sizeDelta = new Vector2(0, StatusBarHeight);
                srt.anchoredPosition = Vector2.zero;
                _statusText = go.GetComponent<Text>();
                _statusText.font = Font.CreateDynamicFontFromOSFont("Arial", 14);
                _statusText.fontSize = 14;
                _statusText.color = new Color(0.9f, 0.9f, 0.9f, 1f);
                _statusText.alignment = TextAnchor.MiddleLeft;
                _statusText.horizontalOverflow = HorizontalWrapMode.Overflow;
                srt.SetAsFirstSibling();
            }

            // ---- 小地图占位（右上角） ----
            if (_minimapPlaceholder == null)
            {
                var go = new GameObject("Minimap", typeof(RectTransform), typeof(RawImage));
                go.transform.SetParent(transform, false);
                var mrt = go.GetComponent<RectTransform>();
                mrt.anchorMin = new Vector2(1, 1);
                mrt.anchorMax = new Vector2(1, 1);
                mrt.pivot = new Vector2(1, 1);
                mrt.sizeDelta = new Vector2(MinimapSize, MinimapSize);
                mrt.anchoredPosition = new Vector2(-Padding, -StatusBarHeight - Padding);
                _minimapPlaceholder = go.GetComponent<RawImage>();
                _minimapPlaceholder.color = new Color(0.15f, 0.15f, 0.15f, 0.8f);
                var label = new GameObject("Label", typeof(RectTransform), typeof(Text));
                label.transform.SetParent(mrt, false);
                var lrt = label.GetComponent<RectTransform>();
                lrt.anchorMin = Vector2.zero;
                lrt.anchorMax = Vector2.one;
                lrt.sizeDelta = Vector2.zero;
                var lt = label.GetComponent<Text>();
                lt.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                lt.fontSize = 12;
                lt.color = new Color(0.5f, 0.5f, 0.5f, 1f);
                lt.alignment = TextAnchor.MiddleCenter;
                lt.text = "小地图\n(Phase 2)";
            }

            // ---- 任务追踪（小地图下方） ----
            if (_questTrackerText == null)
            {
                var go = new GameObject("QuestTracker", typeof(RectTransform), typeof(Image));
                go.transform.SetParent(transform, false);
                var qrt = go.GetComponent<RectTransform>();
                qrt.anchorMin = new Vector2(1, 1);
                qrt.anchorMax = new Vector2(1, 1);
                qrt.pivot = new Vector2(1, 1);
                qrt.sizeDelta = new Vector2(QuestPanelWidth, QuestPanelHeight);
                qrt.anchoredPosition = new Vector2(
                    -Padding,
                    -StatusBarHeight - Padding - MinimapSize - Padding);
                go.GetComponent<Image>().color = new Color(0, 0, 0, 0.35f);

                var textGo = new GameObject("Text", typeof(RectTransform), typeof(Text));
                textGo.transform.SetParent(qrt, false);
                var trt = textGo.GetComponent<RectTransform>();
                trt.anchorMin = Vector2.zero;
                trt.anchorMax = Vector2.one;
                trt.offsetMin = new Vector2(6f, 4f);
                trt.offsetMax = new Vector2(-6f, -4f);
                _questTrackerText = textGo.GetComponent<Text>();
                _questTrackerText.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                _questTrackerText.fontSize = 12;
                _questTrackerText.color = new Color(0.92f, 0.92f, 0.92f, 1f);
                _questTrackerText.alignment = TextAnchor.UpperLeft;
                _questTrackerText.horizontalOverflow = HorizontalWrapMode.Wrap;
                _questTrackerText.verticalOverflow = VerticalWrapMode.Overflow;
                _questTrackerText.text = "任务：暂无";
            }

            // ---- 背包摘要（聊天区上方） ----
            if (_bagBarText == null)
            {
                var go = new GameObject("BagBar", typeof(RectTransform), typeof(Image));
                go.transform.SetParent(transform, false);
                var brt = go.GetComponent<RectTransform>();
                brt.anchorMin = new Vector2(0, 0);
                brt.anchorMax = new Vector2(0, 0);
                brt.pivot = new Vector2(0, 0);
                brt.sizeDelta = new Vector2(ChatAreaWidth, BagBarHeight);
                brt.anchoredPosition = new Vector2(
                    Padding,
                    Padding + InputHeight + Padding + ChatAreaHeight + Padding);
                go.GetComponent<Image>().color = new Color(0, 0, 0, 0.3f);

                var textGo = new GameObject("Text", typeof(RectTransform), typeof(Text));
                textGo.transform.SetParent(brt, false);
                var trt = textGo.GetComponent<RectTransform>();
                trt.anchorMin = Vector2.zero;
                trt.anchorMax = Vector2.one;
                trt.offsetMin = new Vector2(6f, 0f);
                trt.offsetMax = new Vector2(-6f, 0f);
                _bagBarText = textGo.GetComponent<Text>();
                _bagBarText.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                _bagBarText.fontSize = 12;
                _bagBarText.color = new Color(0.88f, 0.88f, 0.88f, 1f);
                _bagBarText.alignment = TextAnchor.MiddleLeft;
                _bagBarText.horizontalOverflow = HorizontalWrapMode.Overflow;
                _bagBarText.text = "背包：空";
            }

            // ---- 聊天区域（左下） ----
            if (_chatScroll == null)
            {
                var scrollGo = new GameObject("ChatArea", typeof(RectTransform), typeof(ScrollRect), typeof(Image));
                scrollGo.transform.SetParent(transform, false);
                var srt2 = scrollGo.GetComponent<RectTransform>();
                srt2.anchorMin = new Vector2(0, 0);
                srt2.anchorMax = new Vector2(0, 0);
                srt2.pivot = new Vector2(0, 0);
                srt2.sizeDelta = new Vector2(ChatAreaWidth, ChatAreaHeight);
                srt2.anchoredPosition = new Vector2(Padding, Padding + InputHeight + Padding);
                scrollGo.GetComponent<Image>().color = new Color(0, 0, 0, 0.35f);

                _chatScroll = scrollGo.GetComponent<ScrollRect>();

                var vp = new GameObject("Viewport", typeof(RectTransform), typeof(Image), typeof(Mask));
                vp.transform.SetParent(srt2, false);
                var vprt = vp.GetComponent<RectTransform>();
                vprt.anchorMin = Vector2.zero;
                vprt.anchorMax = Vector2.one;
                vprt.sizeDelta = Vector2.zero;
                vp.GetComponent<Image>().color = Color.clear;
                _chatScroll.viewport = vprt;

                var content = new GameObject("Content", typeof(RectTransform));
                content.transform.SetParent(vprt, false);
                var crt = content.GetComponent<RectTransform>();
                crt.anchorMin = new Vector2(0, 1);
                crt.anchorMax = new Vector2(1, 1);
                crt.pivot = new Vector2(0.5f, 1);
                crt.sizeDelta = new Vector2(0, 0);
                _chatScroll.content = crt;

                var textGo = new GameObject("ChatLog", typeof(RectTransform), typeof(Text));
                textGo.transform.SetParent(crt, false);
                var trt = textGo.GetComponent<RectTransform>();
                trt.anchorMin = new Vector2(0, 1);
                trt.anchorMax = new Vector2(1, 1);
                trt.pivot = new Vector2(0.5f, 1);
                trt.sizeDelta = new Vector2(0, 0);
                _chatLogText = textGo.GetComponent<Text>();
                _chatLogText.font = Font.CreateDynamicFontFromOSFont("Arial", 13);
                _chatLogText.fontSize = 13;
                _chatLogText.color = Color.white;
                _chatLogText.alignment = TextAnchor.LowerLeft;
                _chatLogText.horizontalOverflow = HorizontalWrapMode.Wrap;
                _chatLogText.verticalOverflow = VerticalWrapMode.Overflow;
                _chatLogText.supportRichText = true;

                var fitter = textGo.AddComponent<ContentSizeFitter>();
                fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

                _chatScroll.horizontal = false;
                _chatScroll.vertical = true;
                _chatScroll.movementType = ScrollRect.MovementType.Clamped;
            }

            // ---- 聊天输入框（底部） ----
            if (_chatInput == null)
            {
                var inputGo = new GameObject("ChatInput", typeof(RectTransform), typeof(Image), typeof(InputField));
                inputGo.transform.SetParent(transform, false);
                var irt = inputGo.GetComponent<RectTransform>();
                irt.anchorMin = new Vector2(0, 0);
                irt.anchorMax = new Vector2(0, 0);
                irt.pivot = new Vector2(0, 0);
                irt.sizeDelta = new Vector2(ChatAreaWidth - 60f, InputHeight);
                irt.anchoredPosition = new Vector2(Padding, Padding);
                inputGo.GetComponent<Image>().color = new Color(0.1f, 0.1f, 0.1f, 0.6f);

                _chatInput = inputGo.GetComponent<InputField>();

                var placeGo = new GameObject("Placeholder", typeof(RectTransform), typeof(Text));
                placeGo.transform.SetParent(irt, false);
                var prt = placeGo.GetComponent<RectTransform>();
                prt.anchorMin = Vector2.zero;
                prt.anchorMax = Vector2.one;
                prt.sizeDelta = Vector2.zero;
                var pt = placeGo.GetComponent<Text>();
                pt.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                pt.fontSize = 12;
                pt.color = new Color(0.5f, 0.5f, 0.5f, 1f);
                pt.alignment = TextAnchor.MiddleLeft;
                pt.text = "按 Enter 发送聊天...";
                _chatInput.placeholder = pt;

                var textGo2 = new GameObject("Text", typeof(RectTransform), typeof(Text));
                textGo2.transform.SetParent(irt, false);
                var trt2 = textGo2.GetComponent<RectTransform>();
                trt2.anchorMin = Vector2.zero;
                trt2.anchorMax = Vector2.one;
                trt2.sizeDelta = Vector2.zero;
                var ct = textGo2.GetComponent<Text>();
                ct.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                ct.fontSize = 12;
                ct.color = Color.white;
                ct.alignment = TextAnchor.MiddleLeft;
                ct.supportRichText = false;
                _chatInput.textComponent = ct;
            }

            // ---- 发送按钮（输入框右侧） ----
            if (_sendBtn == null)
            {
                var btnGo = new GameObject("SendBtn", typeof(RectTransform), typeof(Image), typeof(Button));
                btnGo.transform.SetParent(transform, false);
                var brt = btnGo.GetComponent<RectTransform>();
                brt.anchorMin = new Vector2(0, 0);
                brt.anchorMax = new Vector2(0, 0);
                brt.pivot = new Vector2(0, 0);
                brt.sizeDelta = new Vector2(54f, InputHeight);
                brt.anchoredPosition = new Vector2(Padding + ChatAreaWidth - 60f + 4f, Padding);
                btnGo.GetComponent<Image>().color = new Color(0.25f, 0.45f, 0.75f, 0.9f);

                _sendBtn = btnGo.GetComponent<Button>();
                var btnTextGo = new GameObject("Text", typeof(RectTransform), typeof(Text));
                btnTextGo.transform.SetParent(brt, false);
                var btrt = btnTextGo.GetComponent<RectTransform>();
                btrt.anchorMin = Vector2.zero;
                btrt.anchorMax = Vector2.one;
                btrt.sizeDelta = Vector2.zero;
                var bt = btnTextGo.GetComponent<Text>();
                bt.font = Font.CreateDynamicFontFromOSFont("Arial", 12);
                bt.fontSize = 12;
                bt.color = Color.white;
                bt.alignment = TextAnchor.MiddleCenter;
                bt.text = "发送";
            }
        }

        private void UnbindModels()
        {
            if (_quests != null)
            {
                _quests.OnChanged -= RefreshQuestTracker;
            }

            if (_bag != null)
            {
                _bag.OnChanged -= RefreshBagBar;
            }

            _quests = null;
            _bag = null;
        }

        private void OnDestroy()
        {
            UnbindModels();
            _chatInput?.onEndEdit.RemoveAllListeners();
            _sendBtn?.onClick.RemoveAllListeners();
        }
    }
}

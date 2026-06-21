/// <summary>
/// 区服列表单行视图。
/// </summary>
using Rpg.Client.Net;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class ZoneListItemView : MonoBehaviour
    {
        [SerializeField] private Text _nameText;
        [SerializeField] private Text _statusText;
        [SerializeField] private Text _onlineText;
        [SerializeField] private Image _background;
        [SerializeField] private Button _button;

        private static readonly Color NormalColor = new Color(0.14f, 0.16f, 0.2f, 0.95f);
        private static readonly Color SelectedColor = new Color(0.22f, 0.38f, 0.58f, 0.95f);
        private static readonly Color DisabledColor = new Color(0.1f, 0.1f, 0.12f, 0.85f);

        public GameZoneEntry Entry { get; private set; }
        public bool Selectable { get; private set; }

        /// <summary>检查子控件引用是否已有效绑定（用于检测 Prefab 绑定丢失）。</summary>
        public bool HasValidVisuals() => _nameText != null && _statusText != null && _background != null && _button != null;

        /// <summary>供运行时代码创建列表项后绑定子控件引用。</summary>
        public void InitVisuals(Text nameText, Text statusText, Text onlineText, Image background, Button button)
        {
            _nameText = nameText;
            _statusText = statusText;
            _onlineText = onlineText;
            _background = background;
            _button = button;
        }

        public void Bind(GameZoneEntry entry, bool selected, System.Action<ZoneListItemView> onClick)
        {
            Entry = entry;
            Selectable = entry.Enabled
                         && entry.LoadStatus != ZoneLoadStatus.Maintenance
                         && entry.GatewayCount > 0;

            if (_nameText != null)
            {
                _nameText.text = entry.Name;
            }

            if (_statusText != null)
            {
                _statusText.text = FormatLoadStatus(entry);
            }

            if (_onlineText != null)
            {
                _onlineText.text = entry.OnlineCount > 0 ? $"在线 {entry.OnlineCount}" : string.Empty;
            }

            if (_background != null)
            {
                _background.color = !Selectable
                    ? DisabledColor
                    : selected ? SelectedColor : NormalColor;
            }

            if (_button != null)
            {
                _button.interactable = Selectable;
                _button.onClick.RemoveAllListeners();
                if (Selectable)
                {
                    _button.onClick.AddListener(() => onClick?.Invoke(this));
                }
            }
        }

        private static string FormatLoadStatus(GameZoneEntry entry)
        {
            if (!entry.Enabled || entry.LoadStatus == ZoneLoadStatus.Maintenance)
            {
                return "维护中";
            }

            if (entry.GatewayCount == 0)
            {
                return "无网关";
            }

            return entry.LoadStatus switch
            {
                ZoneLoadStatus.Smooth => "流畅",
                ZoneLoadStatus.Busy => "繁忙",
                ZoneLoadStatus.Full => "爆满",
                _ => string.Empty
            };
        }
    }
}

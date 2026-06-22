/// <summary>
/// 区服列表面板（ScrollView + 确认/取消）。
/// </summary>
using System;
using System.Collections;
using System.Collections.Generic;
using Rpg.Client.Log;
using Rpg.Client.Net;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class ServerListPanel : MonoBehaviour
    {
        private const float ItemHeight = 48f;
        private const float ItemSpacing = 4f;
        private const float ContentPadding = 4f;

        [SerializeField] private Text _hintText;
        [SerializeField] private Transform _listContent;
        [SerializeField] private ZoneListItemView _itemPrefab;
        [SerializeField] private Button _confirmBtn;
        [SerializeField] private Button _cancelBtn;

        private readonly List<ZoneListItemView> _items = new List<ZoneListItemView>();
        private UiViewPool<ZoneListItemView> _itemPool;
        private GameZoneEntry _selectedEntry;
        private Action<uint, byte, string> _onConfirmed;
        private Action _onCancel;
        private ScrollRect _scrollRect;
        private bool _viewportMaskFixed;

        private static Font _cachedFont;
        private static Sprite _whiteSprite;

        private void Awake()
        {
            ResolveListContent();
            PrepareListContent();
            _confirmBtn?.onClick.AddListener(ConfirmSelection);
            _cancelBtn?.onClick.AddListener(() => _onCancel?.Invoke());
        }

        public void SetCallbacks(Action<uint, byte, string> onConfirmed, Action onCancel)
        {
            _onConfirmed = onConfirmed;
            _onCancel = onCancel;
        }

        public void ShowZones(List<GameZoneEntry> zones, uint selectedZoneId)
        {
            if (!gameObject.activeInHierarchy)
            {
                gameObject.SetActive(true);
            }

            ResolveListContent();
            PrepareListContent();
            _itemPool ??= new UiViewPool<ZoneListItemView>(_itemPrefab, _listContent);
            ClearItems();
            _selectedEntry = null;

            if (zones == null || zones.Count == 0)
            {
                SetHint("没有可用区服");
                SetConfirmEnabled(false);
                LayoutListItems();
                return;
            }

            SetHint("请选择区服");
            GameZoneEntry firstSelectable = null;

            foreach (var zone in zones)
            {
                var item = CreateListItem();
                if (item == null)
                {
                    continue;
                }

                var selected = zone.ZoneId == selectedZoneId;
                item.Bind(zone, selected, OnItemClicked);
                _items.Add(item);

                if (selected && item.Selectable)
                {
                    _selectedEntry = zone;
                }

                if (firstSelectable == null && item.Selectable)
                {
                    firstSelectable = zone;
                }
            }

            if (_selectedEntry == null && firstSelectable != null)
            {
                _selectedEntry = firstSelectable;
                RefreshSelectionHighlight();
            }

            LayoutListItems();
            if (NeedsDeferredLayout())
            {
                StartCoroutine(DeferredLayoutCoroutine());
            }

            SetConfirmEnabled(_selectedEntry != null);

            if (_items.Count == 0)
            {
                ClientLogger.Instance.WarnFormat("ServerListPanel：收到 {0} 条区服数据但未创建列表项", zones.Count);
            }
            else
            {
                ClientLogger.Instance.InfoFormat("ServerListPanel：已创建 {0} 条区服列表项", _items.Count);
            }
        }

        public void SetHint(string message)
        {
            if (_hintText != null)
            {
                _hintText.text = message ?? string.Empty;
            }
        }

        private void ResolveListContent()
        {
            if (_listContent != null)
            {
                _scrollRect = _listContent.GetComponentInParent<ScrollRect>();
                return;
            }

            _scrollRect = GetComponentInChildren<ScrollRect>(true);
            if (_scrollRect != null)
            {
                _listContent = _scrollRect.content;
                ClientLogger.Instance.Warn("ServerListPanel：_listContent 未绑定，已从 ScrollRect 自动解析");
            }
        }

        private void PrepareListContent()
        {
            if (_listContent == null)
            {
                return;
            }

            EnsureViewportMask();

            if (_listContent is RectTransform contentRt)
            {
                contentRt.anchorMin = new Vector2(0f, 1f);
                contentRt.anchorMax = new Vector2(1f, 1f);
                contentRt.pivot = new Vector2(0.5f, 1f);
                contentRt.anchoredPosition = Vector2.zero;
            }

            var vlg = _listContent.GetComponent<VerticalLayoutGroup>();
            if (vlg != null)
            {
                vlg.enabled = true;
                vlg.spacing = ItemSpacing;
                vlg.padding = new RectOffset(
                    (int)ContentPadding, (int)ContentPadding,
                    (int)ContentPadding, (int)ContentPadding);
            }

            var fitter = _listContent.GetComponent<ContentSizeFitter>();
            if (fitter != null)
            {
                fitter.enabled = true;
                fitter.horizontalFit = ContentSizeFitter.FitMode.Unconstrained;
                fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;
            }
        }

        /// <summary>
        /// 旧 Boot 场景 Viewport 使用无 Sprite 的 Mask，会把子项全部裁掉；运行时改为 RectMask2D。
        /// </summary>
        private void EnsureViewportMask()
        {
            if (_viewportMaskFixed || _scrollRect?.viewport == null)
            {
                return;
            }

            var viewportGo = _scrollRect.viewport.gameObject;
            if (viewportGo.GetComponent<RectMask2D>() == null)
            {
                var legacyMask = viewportGo.GetComponent<Mask>();
                if (legacyMask != null)
                {
                    legacyMask.enabled = false;
                    Destroy(legacyMask);
                }

                viewportGo.AddComponent<RectMask2D>();
                ClientLogger.Instance.Info("ServerListPanel：已将 Viewport Mask 替换为 RectMask2D");
            }

            _viewportMaskFixed = true;
        }

        private void LayoutListItems()
        {
            if (_listContent is not RectTransform contentRt)
            {
                return;
            }

            foreach (var item in _items)
            {
                if (item != null)
                {
                    EnsureListItemLayoutElement(item);
                }
            }

            LayoutRebuilder.ForceRebuildLayoutImmediate(contentRt);
            Canvas.ForceUpdateCanvases();

            if (_scrollRect != null)
            {
                _scrollRect.verticalNormalizedPosition = 1f;
            }
        }

        private static void EnsureListItemLayoutElement(ZoneListItemView item)
        {
            var rt = item.GetComponent<RectTransform>();
            var layout = item.GetComponent<LayoutElement>() ?? item.gameObject.AddComponent<LayoutElement>();
            layout.minHeight = ItemHeight;
            layout.preferredHeight = ItemHeight;
            layout.flexibleWidth = 1f;

            rt.anchorMin = new Vector2(0f, 0.5f);
            rt.anchorMax = new Vector2(1f, 0.5f);
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = new Vector2(0f, ItemHeight);
        }

        private bool NeedsDeferredLayout()
        {
            return _scrollRect?.viewport != null && _scrollRect.viewport.rect.width <= 1f;
        }

        private IEnumerator DeferredLayoutCoroutine()
        {
            yield return null;
            LayoutListItems();

            if (NeedsDeferredLayout())
            {
                yield return null;
                LayoutListItems();
            }
        }

        private void OnItemClicked(ZoneListItemView item)
        {
            _selectedEntry = item.Entry;
            RefreshSelectionHighlight();
            SetConfirmEnabled(true);
        }

        private void RefreshSelectionHighlight()
        {
            foreach (var item in _items)
            {
                item.Bind(item.Entry, item.Entry == _selectedEntry, OnItemClicked);
            }
        }

        private void ConfirmSelection()
        {
            if (_selectedEntry == null)
            {
                return;
            }

            _onConfirmed?.Invoke(_selectedEntry.ZoneId, _selectedEntry.GameType, _selectedEntry.Name);
        }

        private void SetConfirmEnabled(bool enabled)
        {
            if (_confirmBtn != null)
            {
                _confirmBtn.interactable = enabled;
            }
        }

        private ZoneListItemView CreateListItem()
        {
            if (_listContent == null)
            {
                ClientLogger.Instance.Err("ServerListPanel：_listContent 为 null，无法创建列表项");
                return null;
            }

            if (_itemPrefab != null)
            {
                var instance = _itemPool?.Rent();
                if (instance != null && instance.HasValidVisuals())
                {
                    EnsureListItemBackground(instance.GetComponent<Image>());
                    EnsureListItemLayoutElement(instance);
                    return instance;
                }

                if (instance != null)
                {
                    instance.gameObject.SetActive(false);
                }
            }

            var fallback = CreateFallbackListItem();
            _itemPool?.Track(fallback);
            return fallback;
        }

        private ZoneListItemView CreateFallbackListItem()
        {
            var font = ResolveUIFont();

            var go = new GameObject("ZoneListItem", typeof(RectTransform),
                typeof(CanvasRenderer), typeof(Image), typeof(ZoneListItemView), typeof(Button));
            go.transform.SetParent(_listContent, false);

            var bg = go.GetComponent<Image>();
            EnsureListItemBackground(bg);

            var nameText = CreateRowText(go.transform, "NameText", font, 18, TextAnchor.MiddleLeft,
                new Vector2(12f, 0f), new Vector2(-280f, 0f));
            var statusText = CreateRowText(go.transform, "StatusText", font, 16, TextAnchor.MiddleCenter,
                Vector2.zero, Vector2.zero);
            var statusRt = statusText.rectTransform;
            statusRt.anchorMin = new Vector2(0.72f, 0.5f);
            statusRt.anchorMax = new Vector2(0.72f, 0.5f);
            statusRt.pivot = new Vector2(0.5f, 0.5f);
            statusRt.sizeDelta = new Vector2(80f, 28f);
            statusRt.anchoredPosition = Vector2.zero;

            var onlineText = CreateRowText(go.transform, "OnlineText", font, 14, TextAnchor.MiddleRight,
                new Vector2(12f, 0f), new Vector2(-12f, 0f));

            var item = go.GetComponent<ZoneListItemView>();
            item.InitVisuals(nameText, statusText, onlineText, bg, go.GetComponent<Button>());

            var btn = go.GetComponent<Button>();
            btn.targetGraphic = bg;
            EnsureListItemLayoutElement(item);
            return item;
        }

        private static Text CreateRowText(Transform parent, string name, Font font, int size,
            TextAnchor anchor, Vector2 offsetMin, Vector2 offsetMax)
        {
            var textGo = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            textGo.transform.SetParent(parent, false);
            var rt = textGo.GetComponent<RectTransform>();
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = offsetMin;
            rt.offsetMax = offsetMax;

            var text = textGo.GetComponent<Text>();
            text.font = font;
            text.fontSize = size;
            text.alignment = anchor;
            text.color = Color.white;
            text.supportRichText = false;
            text.horizontalOverflow = HorizontalWrapMode.Overflow;
            text.verticalOverflow = VerticalWrapMode.Overflow;
            return text;
        }

        private static void EnsureListItemBackground(Image image)
        {
            if (image == null)
            {
                return;
            }

            image.sprite = GetWhiteSprite();
            image.type = Image.Type.Simple;
            image.color = new Color(0.14f, 0.16f, 0.2f, 0.95f);
        }

        private static Sprite GetWhiteSprite()
        {
            if (_whiteSprite != null)
            {
                return _whiteSprite;
            }

            var tex = new Texture2D(1, 1, TextureFormat.RGBA32, false);
            tex.SetPixel(0, 0, Color.white);
            tex.Apply(false, true);
            _whiteSprite = Sprite.Create(tex, new Rect(0f, 0f, 1f, 1f), new Vector2(0.5f, 0.5f), 100f);
            return _whiteSprite;
        }

        private Font ResolveUIFont()
        {
            if (_hintText != null && _hintText.font != null)
            {
                return _hintText.font;
            }

            if (_cachedFont != null)
            {
                return _cachedFont;
            }

            _cachedFont = Font.CreateDynamicFontFromOSFont(new[] { "Microsoft YaHei UI", "SimHei", "Arial" }, 16);
            if (_cachedFont == null)
            {
                _cachedFont = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf")
                              ?? Resources.GetBuiltinResource<Font>("Arial.ttf");
            }

            return _cachedFont;
        }

        private void ClearItems()
        {
            _itemPool?.ReleaseAll();
            _items.Clear();
        }
    }
}

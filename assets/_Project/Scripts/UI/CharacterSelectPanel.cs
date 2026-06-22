/// <summary>
/// 选角面板（ScrollView + 创角区 + 进入世界）。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.Log;
using Rpg.Client.Net;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class CharacterSelectPanel : MonoBehaviour
    {
        private const float ItemHeight = 44f;
        private const float ItemSpacing = 4f;
        private const float ContentPadding = 4f;

        [SerializeField] private Text _hintText;
        [SerializeField] private Transform _listContent;
        [SerializeField] private CharacterListItemView _itemPrefab;
        [SerializeField] private InputField _createNameInput;
        [SerializeField] private Dropdown _vocationDropdown;
        [SerializeField] private Dropdown _sexDropdown;
        [SerializeField] private Button _createCharBtn;
        [SerializeField] private Button _enterWorldBtn;

        private readonly List<CharacterListItemView> _items = new List<CharacterListItemView>();
        private UiViewPool<CharacterListItemView> _itemPool;
        private ulong _selectedUserId;
        private Action<ulong> _onEnterWorld;
        private Action<string, byte, byte> _onCreateCharacter;
        private ScrollRect _scrollRect;
        private bool _listContentPrepared;

        private void Awake()
        {
            TryEnsureRuntimeLayout();
            ResolveListContent();
            _enterWorldBtn?.onClick.AddListener(() =>
            {
                if (_selectedUserId != 0)
                {
                    _onEnterWorld?.Invoke(_selectedUserId);
                }
            });
            _createCharBtn?.onClick.AddListener(() =>
            {
                var vocation = (byte)(_vocationDropdown != null ? _vocationDropdown.value : CharacterDef.VocationWarrior);
                var sex = (byte)(_sexDropdown != null ? _sexDropdown.value : CharacterDef.SexMale);
                var name = _createNameInput != null ? _createNameInput.text : string.Empty;
                _onCreateCharacter?.Invoke(name, vocation, sex);
            });
            InitDropdowns();
        }

        public void SetCallbacks(Action<ulong> onEnterWorld, Action<string, byte, byte> onCreateCharacter)
        {
            _onEnterWorld = onEnterWorld;
            _onCreateCharacter = onCreateCharacter;
        }

        public void ShowCharacters(List<CharacterEntry> chars, ulong highlightUserId)
        {
            ResolveListContent();
            PrepareListContent();
            _itemPool ??= new UiViewPool<CharacterListItemView>(_itemPrefab, _listContent);
            ClearItems();

            _selectedUserId = 0;
            if (chars == null || chars.Count == 0)
            {
                SetHint("暂无角色，请创建");
                SetEnterWorldEnabled(false);
                UpdateContentHeight();
                return;
            }

            SetHint("请选择角色");
            CharacterEntry first = null;

            foreach (var entry in chars)
            {
                var item = CreateListItem();
                if (item == null)
                {
                    continue;
                }

                var selected = highlightUserId != 0
                    ? entry.UserId == highlightUserId
                    : first == null;
                item.Bind(entry, selected, OnItemClicked);
                _items.Add(item);

                if (selected)
                {
                    _selectedUserId = entry.UserId;
                }

                if (first == null)
                {
                    first = entry;
                }
            }

            if (_selectedUserId == 0 && first != null)
            {
                _selectedUserId = first.UserId;
                RefreshSelectionHighlight();
            }

            LayoutListItems();
            SetEnterWorldEnabled(_selectedUserId != 0);
        }

        public void SetBusy(bool busy)
        {
            if (_enterWorldBtn != null)
            {
                _enterWorldBtn.interactable = !busy && _selectedUserId != 0;
            }

            if (_createCharBtn != null)
            {
                _createCharBtn.interactable = !busy;
            }

            if (_createNameInput != null)
            {
                _createNameInput.interactable = !busy;
            }

            if (_vocationDropdown != null)
            {
                _vocationDropdown.interactable = !busy;
            }

            if (_sexDropdown != null)
            {
                _sexDropdown.interactable = !busy;
            }
        }

        public void SetHint(string message)
        {
            if (_hintText != null)
            {
                _hintText.text = message ?? string.Empty;
            }
        }

        /// <summary>兼容旧 Boot 场景：按名称绑定控件并在缺少 ScrollView 时运行时创建。</summary>
        public void TryEnsureRuntimeLayout()
        {
            WireLegacyControl(ref _createNameInput, "CreateNameInput");
            WireLegacyControl(ref _createCharBtn, "CreateCharBtn");
            WireLegacyControl(ref _enterWorldBtn, "EnterWorldBtn");

            if (_hintText == null)
            {
                var hint = transform.Find("HintText");
                if (hint != null)
                {
                    _hintText = hint.GetComponent<Text>();
                }
            }

            if (_vocationDropdown == null)
            {
                _vocationDropdown = transform.Find("VocationDropdown")?.GetComponent<Dropdown>();
            }

            if (_sexDropdown == null)
            {
                _sexDropdown = transform.Find("SexDropdown")?.GetComponent<Dropdown>();
            }

            var legacyList = transform.Find("CharListText");
            if (legacyList != null)
            {
                legacyList.gameObject.SetActive(false);
            }

            if (_listContent == null)
            {
                EnsureCharacterScrollView();
            }
        }

        private void WireLegacyControl<T>(ref T field, string childName) where T : Component
        {
            if (field != null)
            {
                return;
            }

            var child = transform.Find(childName);
            if (child != null)
            {
                field = child.GetComponent<T>();
            }
        }

        private void EnsureCharacterScrollView()
        {
            var existing = transform.Find("CharacterScrollView");
            if (existing != null)
            {
                var existingScroll = existing.GetComponent<ScrollRect>();
                if (existingScroll?.content != null)
                {
                    _listContent = existingScroll.content;
                    _scrollRect = existingScroll;
                    return;
                }
            }

            var font = _hintText != null ? _hintText.font : Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

            var scrollGo = new GameObject("CharacterScrollView", typeof(RectTransform), typeof(CanvasRenderer),
                typeof(Image), typeof(ScrollRect));
            scrollGo.transform.SetParent(transform, false);
            var scrollRt = scrollGo.GetComponent<RectTransform>();
            scrollRt.anchorMin = new Vector2(0.5f, 0.62f);
            scrollRt.anchorMax = new Vector2(0.5f, 0.62f);
            scrollRt.pivot = new Vector2(0.5f, 0.5f);
            scrollRt.sizeDelta = new Vector2(420f, 180f);
            scrollGo.GetComponent<Image>().color = new Color(0.06f, 0.08f, 0.1f, 0.9f);

            var viewportGo = new GameObject("Viewport", typeof(RectTransform), typeof(CanvasRenderer),
                typeof(Image), typeof(Mask));
            viewportGo.transform.SetParent(scrollGo.transform, false);
            var viewportRt = viewportGo.GetComponent<RectTransform>();
            viewportRt.anchorMin = Vector2.zero;
            viewportRt.anchorMax = Vector2.one;
            viewportRt.offsetMin = Vector2.zero;
            viewportRt.offsetMax = Vector2.zero;
            viewportGo.GetComponent<Image>().color = new Color(0f, 0f, 0f, 0.01f);
            viewportGo.GetComponent<Mask>().showMaskGraphic = false;

            var contentGo = new GameObject("Content", typeof(RectTransform));
            contentGo.transform.SetParent(viewportGo.transform, false);
            var contentRt = contentGo.GetComponent<RectTransform>();
            contentRt.anchorMin = new Vector2(0f, 1f);
            contentRt.anchorMax = new Vector2(1f, 1f);
            contentRt.pivot = new Vector2(0.5f, 1f);
            contentRt.anchoredPosition = Vector2.zero;
            contentRt.sizeDelta = new Vector2(0f, 0f);

            if (_hintText == null)
            {
                var hintGo = new GameObject("HintText", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
                hintGo.transform.SetParent(transform, false);
                var hintRt = hintGo.GetComponent<RectTransform>();
                hintRt.anchorMin = new Vector2(0.5f, 0.88f);
                hintRt.anchorMax = new Vector2(0.5f, 0.88f);
                hintRt.sizeDelta = new Vector2(420f, 32f);
                _hintText = hintGo.GetComponent<Text>();
                _hintText.font = font;
                _hintText.fontSize = 18;
                _hintText.alignment = TextAnchor.MiddleCenter;
                _hintText.color = Color.white;
                _hintText.text = "请选择角色";
            }

            var createdScroll = scrollGo.GetComponent<ScrollRect>();
            createdScroll.viewport = viewportRt;
            createdScroll.content = contentRt;
            createdScroll.horizontal = false;
            createdScroll.vertical = true;
            createdScroll.movementType = ScrollRect.MovementType.Clamped;
            _scrollRect = createdScroll;
            _listContent = contentRt;
        }

        private void InitDropdowns()
        {
            if (_vocationDropdown != null && _vocationDropdown.options.Count == 0)
            {
                _vocationDropdown.ClearOptions();
                _vocationDropdown.AddOptions(new List<string> { "战士", "法师" });
                _vocationDropdown.value = CharacterDef.VocationWarrior;
            }

            if (_sexDropdown != null && _sexDropdown.options.Count == 0)
            {
                _sexDropdown.ClearOptions();
                _sexDropdown.AddOptions(new List<string> { "男", "女" });
                _sexDropdown.value = CharacterDef.SexMale;
            }
        }

        private void OnItemClicked(CharacterListItemView item)
        {
            _selectedUserId = item.Entry.UserId;
            RefreshSelectionHighlight();
            SetEnterWorldEnabled(true);
        }

        private void RefreshSelectionHighlight()
        {
            foreach (var item in _items)
            {
                item.Bind(item.Entry, item.Entry.UserId == _selectedUserId, OnItemClicked);
            }
        }

        private void SetEnterWorldEnabled(bool enabled)
        {
            if (_enterWorldBtn != null)
            {
                _enterWorldBtn.interactable = enabled;
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
            }
        }

        private void PrepareListContent()
        {
            if (_listContentPrepared || _listContent == null)
            {
                return;
            }

            var vlg = _listContent.GetComponent<VerticalLayoutGroup>();
            if (vlg != null)
            {
                vlg.enabled = false;
            }

            var fitter = _listContent.GetComponent<ContentSizeFitter>();
            if (fitter != null)
            {
                fitter.enabled = false;
            }

            _listContentPrepared = true;
        }

        private void LayoutListItems()
        {
            if (_listContent is not RectTransform contentRt)
            {
                return;
            }

            for (var i = 0; i < _items.Count; i++)
            {
                var item = _items[i];
                if (item == null)
                {
                    continue;
                }

                var rt = item.GetComponent<RectTransform>();
                rt.anchorMin = new Vector2(0f, 1f);
                rt.anchorMax = new Vector2(1f, 1f);
                rt.pivot = new Vector2(0.5f, 1f);
                rt.sizeDelta = new Vector2(0f, ItemHeight);
                rt.anchoredPosition = new Vector2(0f, -(ContentPadding + i * (ItemHeight + ItemSpacing)));
            }

            UpdateContentHeight();

            if (_scrollRect != null)
            {
                _scrollRect.verticalNormalizedPosition = 1f;
            }

            Canvas.ForceUpdateCanvases();
        }

        private void UpdateContentHeight()
        {
            if (_listContent is not RectTransform contentRt)
            {
                return;
            }

            if (_items.Count == 0)
            {
                contentRt.sizeDelta = new Vector2(0f, 0f);
                return;
            }

            var totalHeight = ContentPadding * 2f
                              + _items.Count * ItemHeight
                              + (_items.Count - 1) * ItemSpacing;
            contentRt.sizeDelta = new Vector2(0f, totalHeight);
        }

        private CharacterListItemView CreateListItem()
        {
            if (_listContent == null)
            {
                ClientLogger.Instance.Err("CharacterSelectPanel：_listContent 为 null，无法创建列表项");
                return null;
            }

            if (_itemPrefab != null)
            {
                var instance = _itemPool?.Rent();
                if (instance != null && instance.HasValidVisuals())
                {
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

        private CharacterListItemView CreateFallbackListItem()
        {
            var font = _hintText != null ? _hintText.font : null;

            var go = new GameObject("CharacterListItem", typeof(RectTransform),
                typeof(CanvasRenderer), typeof(Image), typeof(CharacterListItemView), typeof(Button));
            go.transform.SetParent(_listContent, false);

            var bg = go.GetComponent<Image>();
            bg.color = new Color(0.14f, 0.16f, 0.2f, 0.95f);

            var nameText = CreateRowText(go.transform, "NameText", font, 18, TextAnchor.MiddleLeft,
                new Vector2(12f, 0f), new Vector2(-80f, 0f));
            var levelText = CreateRowText(go.transform, "LevelText", font, 16, TextAnchor.MiddleRight,
                new Vector2(12f, 0f), new Vector2(-12f, 0f));

            var item = go.GetComponent<CharacterListItemView>();
            var btn = go.GetComponent<Button>();
            btn.targetGraphic = bg;
            item.InitVisuals(nameText, levelText, bg, btn);
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
            return text;
        }

        private void ClearItems()
        {
            _itemPool?.ReleaseAll();
            _items.Clear();
        }
    }
}

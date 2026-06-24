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
        private uint _selectedModelId = CharacterDef.ModelDefault;
        private Action<ulong> _onEnterWorld;
        private Action<string, byte, byte, uint> _onCreateCharacter;
        private ScrollRect _scrollRect;
        private readonly List<Image> _modelButtons = new List<Image>();
        private static readonly Color ModelSelectedColor = new Color(0.85f, 0.7f, 0.25f, 0.9f);
        private static readonly Color ModelNormalColor = new Color(0.14f, 0.16f, 0.2f, 0.9f);
        private static readonly string[] ModelNames = { "男·大人", "男·小人", "女·大人", "女·小人" };
        private static readonly uint[] ModelIds = {
            CharacterDef.ModelMaleAdult,
            CharacterDef.ModelMaleChild,
            CharacterDef.ModelFemaleAdult,
            CharacterDef.ModelFemaleChild
        };

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
                _onCreateCharacter?.Invoke(name, vocation, sex, _selectedModelId);
            });
            InitDropdowns();
            EnsureModelSelector();
        }

        public void SetCallbacks(Action<ulong> onEnterWorld, Action<string, byte, byte, uint> onCreateCharacter)
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

            foreach (var btn in _modelButtons)
            {
                var button = btn.GetComponent<Button>();
                if (button != null)
                {
                    button.interactable = !busy;
                }
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

        /// <summary>创建模型选择按钮组（男大/男小/女大/女小），默认选中男大。</summary>
        private void EnsureModelSelector()
        {
            // 已存在则跳过
            if (transform.Find("ModelSelector") != null)
            {
                return;
            }

            var font = _hintText != null ? _hintText.font : Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

            // 容器
            var container = new GameObject("ModelSelector", typeof(RectTransform));
            container.transform.SetParent(transform, false);
            var containerRt = container.GetComponent<RectTransform>();
            containerRt.anchorMin = new Vector2(0.5f, 0.46f);
            containerRt.anchorMax = new Vector2(0.5f, 0.46f);
            containerRt.pivot = new Vector2(0.5f, 0.5f);
            containerRt.sizeDelta = new Vector2(420f, 36f);

            // 标签
            var labelGo = new GameObject("ModelLabel", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            labelGo.transform.SetParent(container.transform, false);
            var labelRt = labelGo.GetComponent<RectTransform>();
            labelRt.anchorMin = new Vector2(0, 0.5f);
            labelRt.anchorMax = new Vector2(0, 0.5f);
            labelRt.pivot = new Vector2(0, 0.5f);
            labelRt.sizeDelta = new Vector2(60f, 30f);
            var labelText = labelGo.GetComponent<Text>();
            labelText.font = font;
            labelText.fontSize = 14;
            labelText.alignment = TextAnchor.MiddleRight;
            labelText.color = new Color(0.7f, 0.7f, 0.7f, 1f);
            labelText.text = "模型:";

            // 4个模型按钮
            for (var i = 0; i < 4; i++)
            {
                var btnGo = new GameObject($"ModelBtn_{i}", typeof(RectTransform), typeof(CanvasRenderer),
                    typeof(Image), typeof(Button));
                btnGo.transform.SetParent(container.transform, false);
                var btnRt = btnGo.GetComponent<RectTransform>();
                var btnWidth = 80f;
                var gap = 4f;
                var startX = 64f;
                btnRt.anchorMin = new Vector2(0, 0.5f);
                btnRt.anchorMax = new Vector2(0, 0.5f);
                btnRt.pivot = new Vector2(0, 0.5f);
                btnRt.anchoredPosition = new Vector2(startX + i * (btnWidth + gap), 0);
                btnRt.sizeDelta = new Vector2(btnWidth, 28f);

                var btnImage = btnGo.GetComponent<Image>();
                btnImage.color = i == 0 ? ModelSelectedColor : ModelNormalColor;

                var btnTextGo = new GameObject("Text", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
                btnTextGo.transform.SetParent(btnGo.transform, false);
                var btnTextRt = btnTextGo.GetComponent<RectTransform>();
                btnTextRt.anchorMin = Vector2.zero;
                btnTextRt.anchorMax = Vector2.one;
                btnTextRt.offsetMin = Vector2.zero;
                btnTextRt.offsetMax = Vector2.zero;
                var btnText = btnTextGo.GetComponent<Text>();
                btnText.font = font;
                btnText.fontSize = 12;
                btnText.alignment = TextAnchor.MiddleCenter;
                btnText.color = Color.white;
                btnText.text = ModelNames[i];

                var btn = btnGo.GetComponent<Button>();
                btn.targetGraphic = btnImage;
                var idx = i;
                btn.onClick.AddListener(() => SelectModel(idx));

                _modelButtons.Add(btnImage);
            }

            _selectedModelId = CharacterDef.ModelDefault;
        }

        private void SelectModel(int index)
        {
            if (index < 0 || index >= ModelIds.Length)
            {
                return;
            }

            _selectedModelId = ModelIds[index];
            for (var i = 0; i < _modelButtons.Count; i++)
            {
                _modelButtons[i].color = i == index ? ModelSelectedColor : ModelNormalColor;
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
            if (_listContent == null)
            {
                return;
            }

            var vlg = _listContent.GetComponent<VerticalLayoutGroup>();
            if (vlg == null)
            {
                vlg = _listContent.gameObject.AddComponent<VerticalLayoutGroup>();
            }

            vlg.enabled = true;
            vlg.spacing = ItemSpacing;
            vlg.padding = new RectOffset(
                (int)ContentPadding, (int)ContentPadding,
                (int)ContentPadding, (int)ContentPadding);
            vlg.childAlignment = TextAnchor.UpperCenter;
            vlg.childControlWidth = true;
            vlg.childControlHeight = false;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;

            var fitter = _listContent.GetComponent<ContentSizeFitter>();
            if (fitter == null)
            {
                fitter = _listContent.gameObject.AddComponent<ContentSizeFitter>();
            }

            fitter.enabled = true;
            fitter.horizontalFit = ContentSizeFitter.FitMode.Unconstrained;
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;
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

        private static void EnsureListItemLayoutElement(CharacterListItemView item)
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
            UiViewFactory.EnsureListItemBackground(bg);

            var nameText = UiViewFactory.CreateRowText(go.transform, "NameText", font, 18, TextAnchor.MiddleLeft,
                new Vector2(12f, 0f), new Vector2(-80f, 0f));
            var levelText = UiViewFactory.CreateRowText(go.transform, "LevelText", font, 16, TextAnchor.MiddleRight,
                new Vector2(12f, 0f), new Vector2(-12f, 0f));

            var item = go.GetComponent<CharacterListItemView>();
            var btn = go.GetComponent<Button>();
            btn.targetGraphic = bg;
            item.InitVisuals(nameText, levelText, bg, btn);
            return item;
        }

        private void ClearItems()
        {
            _itemPool?.ReleaseAll();
            _items.Clear();
        }
    }
}

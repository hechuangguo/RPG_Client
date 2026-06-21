/// <summary>
/// 区服列表面板（ScrollView + 确认/取消）。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.Net;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class ServerListPanel : MonoBehaviour
    {
        [SerializeField] private Text _hintText;
        [SerializeField] private Transform _listContent;
        [SerializeField] private ZoneListItemView _itemPrefab;
        [SerializeField] private Button _confirmBtn;
        [SerializeField] private Button _cancelBtn;

        private readonly List<ZoneListItemView> _items = new List<ZoneListItemView>();
        private GameZoneEntry _selectedEntry;
        private Action<uint, byte, string> _onConfirmed;
        private Action _onCancel;

        private void Awake()
        {
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
            ClearItems();
            _selectedEntry = null;

            if (zones == null || zones.Count == 0)
            {
                SetHint("没有可用区服");
                SetConfirmEnabled(false);
                return;
            }

            SetHint("请选择区服");
            GameZoneEntry firstSelectable = null;

            foreach (var zone in zones)
            {
                var item = InstantiateItem();
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

            SetConfirmEnabled(_selectedEntry != null);
        }

        public void SetHint(string message)
        {
            if (_hintText != null)
            {
                _hintText.text = message ?? string.Empty;
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

        private ZoneListItemView InstantiateItem()
        {
            if (_itemPrefab == null || _listContent == null)
            {
                var go = new GameObject("ZoneListItemFallback", typeof(RectTransform));
                go.transform.SetParent(_listContent, false);
                return go.AddComponent<ZoneListItemView>();
            }

            return Instantiate(_itemPrefab, _listContent);
        }

        private void ClearItems()
        {
            foreach (var item in _items)
            {
                if (item != null)
                {
                    Destroy(item.gameObject);
                }
            }

            _items.Clear();
        }
    }
}

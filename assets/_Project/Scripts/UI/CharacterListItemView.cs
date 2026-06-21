/// <summary>
/// 选角列表单行视图。
/// </summary>
using Rpg.Client.Net;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class CharacterListItemView : MonoBehaviour
    {
        [SerializeField] private Text _nameText;
        [SerializeField] private Text _levelText;
        [SerializeField] private Image _background;
        [SerializeField] private Button _button;

        private static readonly Color NormalColor = new Color(0.14f, 0.16f, 0.2f, 0.95f);
        private static readonly Color SelectedColor = new Color(0.22f, 0.38f, 0.58f, 0.95f);

        public CharacterEntry Entry { get; private set; }

        public bool HasValidVisuals() =>
            _nameText != null && _levelText != null && _background != null && _button != null;

        public void InitVisuals(Text nameText, Text levelText, Image background, Button button)
        {
            _nameText = nameText;
            _levelText = levelText;
            _background = background;
            _button = button;
        }

        public void Bind(CharacterEntry entry, bool selected, System.Action<CharacterListItemView> onClick)
        {
            Entry = entry;

            if (_nameText != null)
            {
                _nameText.text = entry.Name;
            }

            if (_levelText != null)
            {
                _levelText.text = $"Lv{entry.Level}";
            }

            if (_background != null)
            {
                _background.color = selected ? SelectedColor : NormalColor;
            }

            if (_button != null)
            {
                _button.onClick.RemoveAllListeners();
                _button.onClick.AddListener(() => onClick?.Invoke(this));
            }
        }
    }
}

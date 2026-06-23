/// <summary>
/// UI 构建工具类，提供列表项文本、背景等共享辅助方法。
/// </summary>
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public static class UiViewFactory
    {
        /// <summary>
        /// 在 parent 下创建 Text 子物体，返回 Text 组件。
        /// offsetMin / offsetMax 使用 Unity 的 RectTransform offset 语义（左下/右上边距）。
        /// </summary>
        public static Text CreateRowText(Transform parent, string name, Font font, int size,
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

        /// <summary>
        /// 确保 Image 有白色 Sprite 和标准背景色。
        /// </summary>
        public static void EnsureListItemBackground(Image image)
        {
            if (image == null)
            {
                return;
            }

            if (image.sprite == null)
            {
                image.sprite = GetWhiteSprite();
            }

            image.type = Image.Type.Simple;
            image.color = new Color(0.14f, 0.16f, 0.2f, 0.95f);
        }

        private static Sprite _whiteSprite;

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
    }
}

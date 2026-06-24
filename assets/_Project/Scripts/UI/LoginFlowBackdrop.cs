/// <summary>
/// 登录流程静态背景（仅底图）。
/// </summary>
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class LoginFlowBackdrop : MonoBehaviour
    {
        private static readonly string[] LegacyLayerNames =
        {
            "Water", "Waterfall", "Trees", "BirdFxRoot", "Birds", "FishingBoat", "MistNear", "MistFar"
        };

        [SerializeField] private Image _base;

        private void Awake()
        {
            TryBindFromChildren();
            EnsureBehindUiPanels();
            ApplyBaseOnlyPolicy();
        }

        public void SetVisible(bool visible)
        {
            gameObject.SetActive(visible);
            if (visible)
            {
                EnsureBehindUiPanels();
                ApplyBaseOnlyPolicy();
            }
            else
            {
                RemoveForegroundFxLayer();
            }
        }

        public void BindBase(Image baseLayer, Sprite baseSprite)
        {
            _base = baseLayer;
            if (_base != null)
            {
                if (baseSprite != null)
                {
                    _base.sprite = baseSprite;
                }

                _base.raycastTarget = false;
            }

            ApplyBaseOnlyPolicy();
        }

        private void ApplyBaseOnlyPolicy()
        {
            if (_base != null)
            {
                _base.gameObject.SetActive(true);
                _base.raycastTarget = false;
            }

            DisableLegacyLayers();
            DisableAllSubtreeRaycasts();
            RemoveForegroundFxLayer();
        }

        private void EnsureBehindUiPanels()
        {
            transform.SetAsFirstSibling();
        }

        private void DisableAllSubtreeRaycasts()
        {
            foreach (var graphic in GetComponentsInChildren<Graphic>(true))
            {
                graphic.raycastTarget = false;
            }
        }

        private void DisableLegacyLayers()
        {
            foreach (var layerName in LegacyLayerNames)
            {
                var layer = transform.Find(layerName);
                if (layer != null)
                {
                    layer.gameObject.SetActive(false);
                }
            }
        }

        private void RemoveForegroundFxLayer()
        {
            var canvas = GetComponentInParent<Canvas>();
            if (canvas == null)
            {
                return;
            }

            var fxLayer = canvas.transform.Find("LoginFlowForegroundFx");
            if (fxLayer == null)
            {
                return;
            }

            if (Application.isPlaying)
            {
                Destroy(fxLayer.gameObject);
            }
            else
            {
                DestroyImmediate(fxLayer.gameObject);
            }
        }

        private void TryBindFromChildren()
        {
            if (_base == null)
            {
                _base = transform.Find("Base")?.GetComponent<Image>();
            }
        }
    }
}

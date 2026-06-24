/// <summary>
/// 登录背景湖面渔船漂移动效（水平往返 + 起伏）。
/// </summary>
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class LoginFlowBoatFx : MonoBehaviour
    {
        [SerializeField] private RectTransform _boatRoot;
        [SerializeField] private Image _boatImage;
        [SerializeField] private float _driftMinX = 260f;
        [SerializeField] private float _driftMaxX = 940f;
        [SerializeField] private float _baseY = 110f;
        [SerializeField] private float _bobAmplitude = 8f;
        [SerializeField] private float _bobSpeed = 1.4f;
        [SerializeField] private float _driftSpeed = 26f;
        [SerializeField] private float _rollAmplitude = 1.5f;
        [SerializeField] private float _rollSpeed = 1.1f;

        private bool _playing;
        private float _driftDirection = 1f;
        private float _bobPhase;

        public void Configure(RectTransform boatRoot, Image boatImage, Sprite boatSprite)
        {
            _boatRoot = boatRoot;
            _boatImage = boatImage;
            if (_boatImage != null)
            {
                _boatImage.sprite = boatSprite;
                _boatImage.preserveAspect = true;
                _boatImage.SetNativeSize();
                var size = _boatImage.rectTransform.sizeDelta;
                if (size.x > 220f)
                {
                    var scale = 220f / size.x;
                    _boatImage.rectTransform.sizeDelta = size * scale;
                }

                _boatImage.color = Color.white;
            }
        }

        public void Play()
        {
            _playing = true;
            _bobPhase = 0f;
            if (_boatRoot != null)
            {
                var pos = _boatRoot.anchoredPosition;
                pos.x = (_driftMinX + _driftMaxX) * 0.5f;
                pos.y = _baseY;
                _boatRoot.anchoredPosition = pos;
            }
        }

        public void Stop()
        {
            _playing = false;
        }

        private void Update()
        {
            if (!_playing || _boatRoot == null)
            {
                return;
            }

            var dt = Time.unscaledDeltaTime;
            var pos = _boatRoot.anchoredPosition;
            pos.x += _driftSpeed * _driftDirection * dt;
            if (pos.x >= _driftMaxX)
            {
                pos.x = _driftMaxX;
                _driftDirection = -1f;
                _boatRoot.localScale = new Vector3(-1f, 1f, 1f);
            }
            else if (pos.x <= _driftMinX)
            {
                pos.x = _driftMinX;
                _driftDirection = 1f;
                _boatRoot.localScale = Vector3.one;
            }

            _bobPhase += _bobSpeed * dt;
            pos.y = _baseY + Mathf.Sin(_bobPhase) * _bobAmplitude;
            _boatRoot.anchoredPosition = pos;

            if (_boatImage != null)
            {
                var roll = Mathf.Sin(_bobPhase * _rollSpeed) * _rollAmplitude;
                _boatImage.rectTransform.localEulerAngles = new Vector3(0f, 0f, roll);
            }
        }
    }
}

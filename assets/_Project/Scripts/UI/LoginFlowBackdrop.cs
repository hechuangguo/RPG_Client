/// <summary>
/// 登录流程共用仙侠分层动效背景（山、水、瀑布、树、雾、飞鸟）。
/// </summary>
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class LoginFlowBackdrop : MonoBehaviour
    {
        private const string ScrollShaderName = "UI/ScrollUV";
        private const int MaxBirds = 5;

        [Header("Layers")]
        [SerializeField] private Image _base;
        [SerializeField] private Image _water;
        [SerializeField] private Image _waterfall;
        [SerializeField] private Image _trees;
        [SerializeField] private Image _mistNear;
        [SerializeField] private Image _mistFar;
        [SerializeField] private RectTransform _birdRoot;
        [SerializeField] private Sprite _birdSprite;

        [Header("Motion")]
        [SerializeField] private bool _hideRedundantLayersOnCompositeBase = true;
        [SerializeField] private Vector2 _waterScrollSpeed = new Vector2(0.04f, 0f);
        [SerializeField] private Vector2 _waterfallScrollSpeed = new Vector2(0f, -0.12f);
        [SerializeField] private float _mistNearDriftSpeed = 18f;
        [SerializeField] private float _mistFarDriftSpeed = 10f;
        [SerializeField] private float _mistPulseSpeed = 0.35f;
        [SerializeField] private float _birdSpawnMinInterval = 2.5f;
        [SerializeField] private float _birdSpawnMaxInterval = 5.5f;
        [SerializeField] private float _birdMinSpeed = 80f;
        [SerializeField] private float _birdMaxSpeed = 140f;

        private Material _waterMaterial;
        private Material _waterfallMaterial;
        private readonly List<BirdAgent> _birds = new List<BirdAgent>();
        private readonly Queue<Image> _birdPool = new Queue<Image>();
        private Coroutine _birdRoutine;
        private bool _playing;
        private float _mistPulsePhase;

        private struct BirdAgent
        {
            public Image Image;
            public float Speed;
            public float Y;
        }

        private void Awake()
        {
            TryBindFromChildren();
            ApplyCompositeBaseLayerPolicy();
            EnsureScrollMaterials();
            HideAllBirds();
        }

        /// <summary>
        /// 当前 base 图为完整场景时，底部 water/trees/waterfall 会与底图重复叠出「上下两张图」。
        /// 待美术拆成独立分层后再关闭此选项。
        /// </summary>
        private void ApplyCompositeBaseLayerPolicy()
        {
            if (!_hideRedundantLayersOnCompositeBase)
            {
                return;
            }

            SetLayerActive(_water, false);
            SetLayerActive(_waterfall, false);
            SetLayerActive(_trees, false);
        }

        private static void SetLayerActive(Image layer, bool active)
        {
            if (layer != null)
            {
                layer.gameObject.SetActive(active);
            }
        }

        private void TryBindFromChildren()
        {
            if (_base == null)
            {
                _base = transform.Find("Base")?.GetComponent<Image>();
            }

            if (_water == null)
            {
                _water = transform.Find("Water")?.GetComponent<Image>();
            }

            if (_waterfall == null)
            {
                _waterfall = transform.Find("Waterfall")?.GetComponent<Image>();
            }

            if (_trees == null)
            {
                _trees = transform.Find("Trees")?.GetComponent<Image>();
            }

            if (_mistNear == null)
            {
                _mistNear = transform.Find("MistNear")?.GetComponent<Image>();
            }

            if (_mistFar == null)
            {
                _mistFar = transform.Find("MistFar")?.GetComponent<Image>();
            }

            if (_birdRoot == null)
            {
                _birdRoot = transform.Find("Birds") as RectTransform;
            }
        }

        private void OnDestroy()
        {
            if (_waterMaterial != null)
            {
                Destroy(_waterMaterial);
            }

            if (_waterfallMaterial != null)
            {
                Destroy(_waterfallMaterial);
            }
        }

        private void Update()
        {
            if (!_playing)
            {
                return;
            }

            var dt = Time.unscaledDeltaTime;
            AnimateMist(_mistNear, _mistNearDriftSpeed, 0.22f, dt);
            AnimateMist(_mistFar, _mistFarDriftSpeed, 0.14f, dt);
            AnimateBirds(dt);
        }

        public void SetVisible(bool visible)
        {
            gameObject.SetActive(visible);
            if (visible)
            {
                Play();
            }
            else
            {
                Stop();
            }
        }

        public void Play()
        {
            if (_playing)
            {
                return;
            }

            _playing = true;
            _mistPulsePhase = 0f;
            if (_birdRoutine == null)
            {
                _birdRoutine = StartCoroutine(SpawnBirdsLoop());
            }
        }

        public void Stop()
        {
            _playing = false;
            if (_birdRoutine != null)
            {
                StopCoroutine(_birdRoutine);
                _birdRoutine = null;
            }

            HideAllBirds();
        }

        /// <summary>运行时或编辑器创建层级后注入引用。</summary>
        public void BindLayers(
            Image baseLayer,
            Image water,
            Image waterfall,
            Image trees,
            Image mistNear,
            Image mistFar,
            RectTransform birdRoot,
            Sprite birdSprite)
        {
            _base = baseLayer;
            _water = water;
            _waterfall = waterfall;
            _trees = trees;
            _mistNear = mistNear;
            _mistFar = mistFar;
            _birdRoot = birdRoot;
            _birdSprite = birdSprite;
            EnsureScrollMaterials();
        }

        private void EnsureScrollMaterials()
        {
            var scrollShader = Shader.Find(ScrollShaderName);
            if (scrollShader == null)
            {
                return;
            }

            if (_water != null)
            {
                _waterMaterial = new Material(scrollShader) { hideFlags = HideFlags.HideAndDontSave };
                _water.material = _waterMaterial;
                _waterMaterial.SetVector("_ScrollSpeed", _waterScrollSpeed);
            }

            if (_waterfall != null)
            {
                _waterfallMaterial = new Material(scrollShader) { hideFlags = HideFlags.HideAndDontSave };
                _waterfall.material = _waterfallMaterial;
                _waterfallMaterial.SetVector("_ScrollSpeed", _waterfallScrollSpeed);
            }
        }

        private void AnimateMist(Image mist, float driftSpeed, float baseAlpha, float dt)
        {
            if (mist == null)
            {
                return;
            }

            var rt = mist.rectTransform;
            var pos = rt.anchoredPosition;
            pos.x += driftSpeed * dt;
            if (pos.x > 120f)
            {
                pos.x = -120f;
            }

            rt.anchoredPosition = pos;

            _mistPulsePhase += _mistPulseSpeed * dt;
            var c = mist.color;
            c.a = baseAlpha + Mathf.Sin(_mistPulsePhase) * 0.06f;
            mist.color = c;
        }

        private IEnumerator SpawnBirdsLoop()
        {
            while (_playing)
            {
                var wait = Random.Range(_birdSpawnMinInterval, _birdSpawnMaxInterval);
                yield return new WaitForSecondsRealtime(wait);
                if (!_playing || _birdSprite == null || _birdRoot == null)
                {
                    continue;
                }

                if (CountActiveBirds() >= MaxBirds)
                {
                    continue;
                }

                SpawnBird();
            }
        }

        private void SpawnBird()
        {
            var img = RentBirdImage();
            if (img == null)
            {
                return;
            }

            var rt = img.rectTransform;
            var speed = Random.Range(_birdMinSpeed, _birdMaxSpeed);
            var fromLeft = Random.value > 0.5f;
            var y = Random.Range(80f, 280f);
            var startX = fromLeft ? -80f : 1360f;
            var endX = fromLeft ? 1360f : -80f;
            rt.anchoredPosition = new Vector2(startX, y);
            rt.localScale = fromLeft ? Vector3.one : new Vector3(-1f, 1f, 1f);

            img.gameObject.SetActive(true);
            _birds.Add(new BirdAgent
            {
                Image = img,
                Speed = speed,
                Y = y
            });
        }

        private void AnimateBirds(float dt)
        {
            for (var i = _birds.Count - 1; i >= 0; i--)
            {
                var bird = _birds[i];
                if (bird.Image == null)
                {
                    _birds.RemoveAt(i);
                    continue;
                }

                var rt = bird.Image.rectTransform;
                var pos = rt.anchoredPosition;
                var dir = rt.localScale.x >= 0f ? 1f : -1f;
                pos.x += bird.Speed * dir * dt;
                rt.anchoredPosition = pos;

                if (pos.x < -120f || pos.x > 1400f)
                {
                    ReturnBirdImage(bird.Image);
                    _birds.RemoveAt(i);
                }
            }
        }

        private int CountActiveBirds()
        {
            var count = 0;
            foreach (var bird in _birds)
            {
                if (bird.Image != null && bird.Image.gameObject.activeSelf)
                {
                    count++;
                }
            }

            return count;
        }

        private Image RentBirdImage()
        {
            if (_birdPool.Count > 0)
            {
                return _birdPool.Dequeue();
            }

            if (_birdRoot == null)
            {
                return null;
            }

            var go = new GameObject("Bird", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            go.transform.SetParent(_birdRoot, false);
            var img = go.GetComponent<Image>();
            img.sprite = _birdSprite;
            img.raycastTarget = false;
            var rt = img.rectTransform;
            rt.sizeDelta = new Vector2(48f, 24f);
            return img;
        }

        private void ReturnBirdImage(Image img)
        {
            if (img == null)
            {
                return;
            }

            img.gameObject.SetActive(false);
            _birdPool.Enqueue(img);
        }

        private void HideAllBirds()
        {
            _birds.Clear();
            if (_birdRoot == null)
            {
                return;
            }

            for (var i = 0; i < _birdRoot.childCount; i++)
            {
                _birdRoot.GetChild(i).gameObject.SetActive(false);
            }
        }
    }
}

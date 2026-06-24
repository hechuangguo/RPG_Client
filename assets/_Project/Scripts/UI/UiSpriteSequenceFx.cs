/// <summary>
/// UGUI 序列帧播放与横向飞行动效生成器。
/// </summary>
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class UiSpriteSequenceFx : MonoBehaviour
    {
        private const int DefaultFrameCount = 4;
        private const int DefaultFrameWidth = 64;

        [Header("Frames")]
        [SerializeField] private Sprite[] _frames;
        [SerializeField] private float _frameRate = 10f;
        [SerializeField] private bool _randomStartFrame = true;

        [Header("Fly Spawner")]
        [SerializeField] private RectTransform _spawnRoot;
        [SerializeField] private int _maxInstances = 4;
        [SerializeField] private float _spawnMinInterval = 2.5f;
        [SerializeField] private float _spawnMaxInterval = 5.5f;
        [SerializeField] private float _minSpeed = 80f;
        [SerializeField] private float _maxSpeed = 140f;
        [SerializeField] private Vector2 _yRange = new Vector2(80f, 280f);
        [SerializeField] private float _targetWidth = 36f;
        [SerializeField] private float _screenMinX = -120f;
        [SerializeField] private float _screenMaxX = 1400f;

        private readonly List<FlyAgent> _flyAgents = new List<FlyAgent>();
        private readonly Queue<FlyAgent> _pool = new Queue<FlyAgent>();
        private Coroutine _spawnRoutine;
        private bool _playing;

        private sealed class FlyAgent
        {
            public Image Image;
            public UiSpriteSequencePlayer Player;
            public float Speed;
            public bool FromLeft;
        }

        public void Configure(
            RectTransform spawnRoot,
            Sprite[] frames,
            int maxInstances,
            float spawnMinInterval,
            float spawnMaxInterval,
            float minSpeed,
            float maxSpeed,
            Vector2 yRange,
            float targetWidth)
        {
            _spawnRoot = spawnRoot;
            _frames = frames;
            _maxInstances = maxInstances;
            _spawnMinInterval = spawnMinInterval;
            _spawnMaxInterval = spawnMaxInterval;
            _minSpeed = minSpeed;
            _maxSpeed = maxSpeed;
            _yRange = yRange;
            _targetWidth = targetWidth;
        }

        public void Play()
        {
            _playing = true;
            if (_spawnRoutine == null)
            {
                _spawnRoutine = StartCoroutine(SpawnLoop());
            }
        }

        public void Stop()
        {
            _playing = false;
            if (_spawnRoutine != null)
            {
                StopCoroutine(_spawnRoutine);
                _spawnRoutine = null;
            }

            HideAll();
        }

        private void Update()
        {
            if (!_playing)
            {
                return;
            }

            AnimateFlyers(Time.unscaledDeltaTime);
        }

        private IEnumerator SpawnLoop()
        {
            yield return new WaitForSecondsRealtime(0.35f);
            if (_playing && _frames != null && _frames.Length > 0 && _spawnRoot != null)
            {
                SpawnFlyer();
                if (CountActive() < _maxInstances)
                {
                    SpawnFlyer();
                }
            }

            while (_playing)
            {
                var wait = Random.Range(_spawnMinInterval, _spawnMaxInterval);
                yield return new WaitForSecondsRealtime(wait);
                if (!_playing || _frames == null || _frames.Length == 0 || _spawnRoot == null)
                {
                    continue;
                }

                if (CountActive() >= _maxInstances)
                {
                    continue;
                }

                SpawnFlyer();
            }
        }

        private void SpawnFlyer()
        {
            var agent = RentAgent();
            if (agent == null)
            {
                return;
            }

            var fromLeft = Random.value > 0.5f;
            var y = Random.Range(_yRange.x, _yRange.y);
            var rt = agent.Image.rectTransform;
            rt.anchoredPosition = new Vector2(fromLeft ? _screenMinX : _screenMaxX, y);
            rt.localScale = fromLeft ? Vector3.one : new Vector3(-1f, 1f, 1f);
            agent.Speed = Random.Range(_minSpeed, _maxSpeed);
            agent.FromLeft = fromLeft;
            agent.Player.SetFrames(_frames);
            agent.Player.Play(_randomStartFrame);
            agent.Image.gameObject.SetActive(true);
            _flyAgents.Add(agent);
        }

        private void AnimateFlyers(float dt)
        {
            for (var i = _flyAgents.Count - 1; i >= 0; i--)
            {
                var agent = _flyAgents[i];
                if (agent.Image == null)
                {
                    _flyAgents.RemoveAt(i);
                    continue;
                }

                var rt = agent.Image.rectTransform;
                var pos = rt.anchoredPosition;
                var dir = agent.FromLeft ? 1f : -1f;
                pos.x += agent.Speed * dir * dt;
                rt.anchoredPosition = pos;

                if (pos.x < _screenMinX - 40f || pos.x > _screenMaxX + 40f)
                {
                    ReturnAgent(agent);
                    _flyAgents.RemoveAt(i);
                }
            }
        }

        private FlyAgent RentAgent()
        {
            if (_pool.Count > 0)
            {
                return _pool.Dequeue();
            }

            if (_spawnRoot == null)
            {
                return null;
            }

            var go = new GameObject("BirdFx", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(UiSpriteSequencePlayer));
            go.transform.SetParent(_spawnRoot, false);
            var image = go.GetComponent<Image>();
            image.raycastTarget = false;
            image.preserveAspect = true;
            var player = go.GetComponent<UiSpriteSequencePlayer>();
            player.Bind(image, _frameRate, true);
            ConfigureFlyerSize(image.rectTransform);
            return new FlyAgent { Image = image, Player = player };
        }

        private void ConfigureFlyerSize(RectTransform rt)
        {
            if (_frames == null || _frames.Length == 0 || _frames[0] == null)
            {
                rt.sizeDelta = new Vector2(_targetWidth, _targetWidth * 0.5f);
                return;
            }

            var size = _frames[0].rect.size;
            if (size.x <= 0f)
            {
                return;
            }

            var scale = _targetWidth / size.x;
            rt.sizeDelta = size * scale;
        }

        private void ReturnAgent(FlyAgent agent)
        {
            if (agent == null)
            {
                return;
            }

            agent.Player.Stop();
            agent.Image.gameObject.SetActive(false);
            _pool.Enqueue(agent);
        }

        private int CountActive()
        {
            var count = 0;
            foreach (var agent in _flyAgents)
            {
                if (agent.Image != null && agent.Image.gameObject.activeSelf)
                {
                    count++;
                }
            }

            return count;
        }

        private void HideAll()
        {
            _flyAgents.Clear();
            if (_spawnRoot == null)
            {
                return;
            }

            for (var i = 0; i < _spawnRoot.childCount; i++)
            {
                _spawnRoot.GetChild(i).gameObject.SetActive(false);
            }
        }

        /// <summary>从横版序列图按等宽切帧（运行时/编辑器共用）。</summary>
        public static Sprite[] SliceEqualWidthSheet(Texture2D texture, int frameCount, float pixelsPerUnit = 100f)
        {
            if (texture == null || frameCount <= 0)
            {
                return System.Array.Empty<Sprite>();
            }

            var frameWidth = texture.width / frameCount;
            if (frameWidth <= 0)
            {
                return System.Array.Empty<Sprite>();
            }

            var frames = new Sprite[frameCount];
            for (var i = 0; i < frameCount; i++)
            {
                var rect = new Rect(i * frameWidth, 0f, frameWidth, texture.height);
                frames[i] = Sprite.Create(texture, rect, new Vector2(0.5f, 0.5f), pixelsPerUnit);
            }

            return frames;
        }
    }

    /// <summary>单 Image 序列帧播放器。</summary>
    public sealed class UiSpriteSequencePlayer : MonoBehaviour
    {
        [SerializeField] private Image _image;
        [SerializeField] private Sprite[] _frames;
        [SerializeField] private float _frameRate = 10f;
        [SerializeField] private bool _loop = true;

        private int _frameIndex;
        private float _timer;
        private bool _playing;

        public void Bind(Image image, float frameRate, bool loop)
        {
            _image = image;
            _frameRate = frameRate;
            _loop = loop;
        }

        public void SetFrames(Sprite[] frames)
        {
            _frames = frames;
            _frameIndex = 0;
            _timer = 0f;
            ApplyFrame();
        }

        public void Play(bool randomStartFrame)
        {
            _playing = true;
            if (_frames == null || _frames.Length == 0)
            {
                return;
            }

            _frameIndex = randomStartFrame ? Random.Range(0, _frames.Length) : 0;
            _timer = 0f;
            ApplyFrame();
        }

        public void Stop()
        {
            _playing = false;
        }

        private void Update()
        {
            if (!_playing || _frames == null || _frames.Length == 0 || _frameRate <= 0f)
            {
                return;
            }

            _timer += Time.unscaledDeltaTime;
            var frameDuration = 1f / _frameRate;
            while (_timer >= frameDuration)
            {
                _timer -= frameDuration;
                _frameIndex++;
                if (_frameIndex >= _frames.Length)
                {
                    _frameIndex = _loop ? 0 : _frames.Length - 1;
                    if (!_loop)
                    {
                        _playing = false;
                        break;
                    }
                }

                ApplyFrame();
            }
        }

        private void ApplyFrame()
        {
            if (_image == null || _frames == null || _frames.Length == 0)
            {
                return;
            }

            var index = Mathf.Clamp(_frameIndex, 0, _frames.Length - 1);
            _image.sprite = _frames[index];
        }
    }
}

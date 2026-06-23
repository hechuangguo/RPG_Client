/// <summary>
/// UI 列表项轻量对象池：隐藏复用，避免反复 Instantiate/Destroy。
/// Rent() 使用 Queue 实现 O(1) 出租。
/// </summary>
using System.Collections.Generic;
using UnityEngine;

namespace Rpg.Client.UI
{
    public sealed class UiViewPool<T> where T : Component
    {
        private readonly T _prefab;
        private readonly Transform _parent;
        private readonly Queue<T> _available = new Queue<T>();
        private readonly List<T> _allInstances = new List<T>();

        public UiViewPool(T prefab, Transform parent)
        {
            _prefab = prefab;
            _parent = parent;
        }

        /// <summary>O(1) 获取一个已隐藏的实例，若池空则 Instantiate 新实例。</summary>
        public T Rent()
        {
            while (_available.Count > 0)
            {
                var item = _available.Dequeue();
                if (item != null)
                {
                    item.gameObject.SetActive(true);
                    return item;
                }
            }

            if (_prefab == null || _parent == null)
            {
                return null;
            }

            var instance = Object.Instantiate(_prefab, _parent);
            _allInstances.Add(instance);
            return instance;
        }

        /// <summary>隐藏所有实例并归还到可用队列。</summary>
        public void ReleaseAll()
        {
            _available.Clear();
            foreach (var item in _allInstances)
            {
                if (item != null)
                {
                    item.gameObject.SetActive(false);
                    _available.Enqueue(item);
                }
            }
        }

        /// <summary>将手动创建的 fallback 实例加入池管理。</summary>
        public void Track(T instance)
        {
            if (instance != null && !_allInstances.Contains(instance))
            {
                _allInstances.Add(instance);
                _available.Enqueue(instance);
            }
        }
    }
}

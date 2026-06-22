/// <summary>
/// UI 列表项轻量对象池：隐藏复用，避免反复 Instantiate/Destroy。
/// </summary>
using System.Collections.Generic;
using UnityEngine;

namespace Rpg.Client.UI
{
    public sealed class UiViewPool<T> where T : Component
    {
        private readonly T _prefab;
        private readonly Transform _parent;
        private readonly List<T> _instances = new List<T>();

        public UiViewPool(T prefab, Transform parent)
        {
            _prefab = prefab;
            _parent = parent;
        }

        public T Rent()
        {
            foreach (var item in _instances)
            {
                if (item != null && !item.gameObject.activeSelf)
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
            _instances.Add(instance);
            return instance;
        }

        public void ReleaseAll()
        {
            foreach (var item in _instances)
            {
                if (item != null)
                {
                    item.gameObject.SetActive(false);
                }
            }
        }

        public void Track(T instance)
        {
            if (instance != null && !_instances.Contains(instance))
            {
                _instances.Add(instance);
            }
        }
    }
}

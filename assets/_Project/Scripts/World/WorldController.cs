/// <summary>
/// 世界场景控制器（对标 game/GameScene）。
/// 职责：加载 mapId 场景、驱动 EntityManager、本地玩家移动输入。
/// 协作：GameSession、EntityManager。
/// </summary>
using Rpg.Client.Net;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using UnityEngine;

namespace Rpg.Client.World
{
    public sealed class WorldController : MonoBehaviour
    {
        [SerializeField] private EntityManager _entities;
        [SerializeField] private GameSession _gameSession;
        [SerializeField] private float _moveSpeed = 5f;

        private ulong _localUserId;
        private bool _active;

        public void LoadMap(S2CEnterGame enter)
        {
            _localUserId = enter.UserId;
            _active = true;
            _entities?.Clear();
            if (enter.Pos != null)
            {
                _entities?.SpawnLocalPlayer(enter.UserId, enter.Name, enter.Pos.X, enter.Pos.Y, enter.Pos.Z);
            }
        }

        public void BindSession(GameSession session)
        {
            _gameSession = session;
            if (session == null)
            {
                return;
            }

            session.OnSpawn = spawn => _entities?.SpawnRemote(spawn);
            session.OnMove = move => _entities?.ApplyMove(move);
            session.OnDespawn = despawn => _entities?.Despawn(despawn.EntityId);
        }

        public void Leave()
        {
            _active = false;
            _localUserId = 0;
            _entities?.Clear();
        }

        private void Update()
        {
            if (!_active || _gameSession == null)
            {
                return;
            }

            var h = Input.GetAxisRaw("Horizontal");
            var v = Input.GetAxisRaw("Vertical");
            if (Mathf.Abs(h) < 0.01f && Mathf.Abs(v) < 0.01f)
            {
                return;
            }

            var pos = _entities?.LocalPosition ?? Vector3.zero;
            pos += new Vector3(h, 0, v).normalized * (_moveSpeed * Time.deltaTime);
            _entities?.SetLocalPosition(pos);
            _gameSession.SendMove(_localUserId, pos.x, pos.y, pos.z, 0f, MoveType.Walk);
        }
    }
}

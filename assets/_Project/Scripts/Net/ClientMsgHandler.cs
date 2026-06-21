/// <summary>
/// 客户端 Protobuf 消息构造与解析（对标 sdk/net/ClientMsgHandler）。
/// 协作：PacketCodec、LoginSession、GameSession、ZoneListSession。
/// </summary>
using System.Collections.Generic;
using Google.Protobuf;
using Rpg.Client.Util;
using Rpg.Proto.Bag;
using Rpg.Proto.Chat;
using Rpg.Proto.Client;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using Rpg.Proto.Quest;
using Rpg.Proto.System;
using Rpg.Proto.Zone;

namespace Rpg.Client.Net
{
    public static class ClientMsgHandler
    {
        private static readonly byte LoginModule = (byte)ClientModule.Login;
        private static readonly byte SceneModule = (byte)ClientModule.Scene;
        private static readonly byte SystemModule = (byte)ClientModule.System;
        private static readonly byte ChatModule = (byte)ClientModule.Chat;
        private static readonly byte QuestModule = (byte)ClientModule.Quest;
        private static readonly byte BagModule = (byte)ClientModule.Bag;

        public static byte[] BuildLoginReq(string account, string password, uint zoneId, byte gameType)
        {
            var req = new C2SLoginReq
            {
                Account = account,
                PasswordDigest = ByteString.CopyFrom(PasswordDigest.Sha256Utf8Password(password)),
                ZoneId = zoneId,
                GameType = gameType,
                ProtocolVersion = WireConstants.CurrentProtocolVersion
            };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SLoginReq, req);
        }

        public static byte[] BuildRegisterReq(string account, string password, string confirm, uint zoneId, byte gameType)
        {
            var req = new C2SRegisterReq
            {
                Account = account,
                PasswordDigest = ByteString.CopyFrom(PasswordDigest.Sha256Utf8Password(password)),
                ConfirmPasswordDigest = ByteString.CopyFrom(PasswordDigest.Sha256Utf8Password(confirm)),
                ZoneId = zoneId,
                GameType = gameType,
                ProtocolVersion = WireConstants.CurrentProtocolVersion
            };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SRegisterReq, req);
        }

        public static byte[] BuildZoneListReq(byte gameType = WireConstants.ZoneListAllGameTypes)
        {
            var req = new C2SZoneListReq { GameType = gameType };
            return PacketCodec.Encode(LoginModule, (byte)ZoneMsgSub.C2SZoneListReq, req);
        }

        public static byte[] BuildGatewayAuthReq(string account, string loginToken, uint zoneId, byte gameType)
        {
            var req = new C2SGatewayAuthReq
            {
                Account = account,
                LoginToken = loginToken,
                ZoneId = zoneId,
                GameType = gameType,
                ProtocolVersion = WireConstants.CurrentProtocolVersion
            };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SGatewayAuthReq, req);
        }

        public static byte[] BuildSelectUserReq(ulong userId, ulong loginTxnId)
        {
            var req = new C2SSelectUserReq { UserId = userId, LoginTxnId = loginTxnId };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SSelectUserReq, req);
        }

        public static byte[] BuildCreateUserReq(string name, byte vocation, byte sex)
        {
            var req = new C2SCreateUserReq
            {
                Name = name,
                Vocation = vocation,
                Sex = sex
            };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SCreateUserReq, req);
        }

        public static byte[] BuildLogoutReq(LogoutAction action)
        {
            var req = new C2SLogoutReq { Action = action };
            return PacketCodec.Encode(LoginModule, (byte)LoginMsgSub.C2SLogoutReq, req);
        }

        public static byte[] BuildHeartbeat(uint seq)
        {
            var req = new C2SHeartbeat { Seq = seq };
            return PacketCodec.Encode(SystemModule, (byte)SystemMsgSub.C2SHeartbeat, req);
        }

        public static byte[] BuildMoveReq(ulong userId, float x, float y, float z, float dir, MoveType moveType)
        {
            var req = new C2SMoveReq
            {
                UserId = userId,
                Pos = new Vec3 { X = x, Y = y, Z = z },
                Dir = dir,
                MoveType = moveType
            };
            return PacketCodec.Encode(SceneModule, (byte)MapDataMsgSub.C2SMoveReq, req);
        }

        public static byte[] BuildChatReq(ChatChannel channel, string content)
        {
            var req = new C2SChatReq
            {
                Channel = channel,
                Content = content ?? string.Empty
            };
            return PacketCodec.Encode(ChatModule, (byte)ChatMsgSub.C2SChatReq, req);
        }

        public static byte[] BuildQuestAcceptReq(uint questId)
        {
            var req = new C2SQuestAcceptReq { QuestId = questId };
            return PacketCodec.Encode(QuestModule, (byte)QuestMsgSub.C2SQuestAccept, req);
        }

        public static byte[] BuildQuestSubmitReq(uint questId)
        {
            var req = new C2SQuestSubmitReq { QuestId = questId };
            return PacketCodec.Encode(QuestModule, (byte)QuestMsgSub.C2SQuestSubmit, req);
        }

        public static byte[] BuildBagInfoReq(ulong userId)
        {
            var req = new C2SBagInfoReq { UserId = userId };
            return PacketCodec.Encode(BagModule, (byte)BagMsgSub.C2SBagInfoReq, req);
        }

        public static bool TryParseZoneListRsp(byte[] body, out S2CZoneListRsp rsp)
        {
            rsp = S2CZoneListRsp.Parser.ParseFrom(body);
            return rsp != null;
        }

        public static bool TryParseLoginRsp(byte[] body, out S2CLoginRsp rsp) { rsp = null; try { rsp = S2CLoginRsp.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseRegisterRsp(byte[] body, out S2CRegisterRsp rsp) { rsp = null; try { rsp = S2CRegisterRsp.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseGatewayInfo(byte[] body, out S2CGatewayInfo info) { info = null; try { info = S2CGatewayInfo.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseEnterGame(byte[] body, out S2CEnterGame enter) { enter = null; try { enter = S2CEnterGame.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseUserList(byte[] body, out S2CUserList list) { list = null; try { list = S2CUserList.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseCreateUserRsp(byte[] body, out S2CCreateUserRsp rsp) { rsp = null; try { rsp = S2CCreateUserRsp.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseLogoutRsp(byte[] body, out S2CLogoutRsp rsp) { rsp = null; try { rsp = S2CLogoutRsp.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseSpawnEntity(byte[] body, out S2CSpawnEntity spawn) { spawn = null; try { spawn = S2CSpawnEntity.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseMoveNotify(byte[] body, out S2CMoveNotify notify) { notify = null; try { notify = S2CMoveNotify.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseDespawnEntity(byte[] body, out S2CDespawnEntity despawn) { despawn = null; try { despawn = S2CDespawnEntity.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseGatewayError(byte[] body, out S2CError err) { err = null; try { err = S2CError.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseHeartbeat(byte[] body, out S2CHeartbeat hb) { hb = null; try { hb = S2CHeartbeat.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseNotice(byte[] body, out S2CNotice notice) { notice = null; try { notice = S2CNotice.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseKick(byte[] body, out S2CKick kick) { kick = null; try { kick = S2CKick.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseChatNotify(byte[] body, out S2CChatNotify chat) { chat = null; try { chat = S2CChatNotify.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseQuestInfo(byte[] body, out S2CQuestInfo info) { info = null; try { info = S2CQuestInfo.Parser.ParseFrom(body); return true; } catch { return false; } }
        public static bool TryParseBagInfoRsp(byte[] body, out S2CBagInfoRsp rsp) { rsp = null; try { rsp = S2CBagInfoRsp.Parser.ParseFrom(body); return true; } catch { return false; } }

        public static List<GameZoneEntry> ToZoneEntries(S2CZoneListRsp rsp)
        {
            var list = new List<GameZoneEntry>();
            if (rsp?.Entries == null)
            {
                return list;
            }

            foreach (var e in rsp.Entries)
            {
                list.Add(new GameZoneEntry
                {
                    ZoneId = e.ZoneId,
                    GameType = (byte)e.GameType,
                    Enabled = e.Enabled != 0,
                    Name = e.Name ?? string.Empty,
                    Ip = e.Ip ?? string.Empty,
                    SuperPort = (ushort)e.SuperPort,
                    OnlineCount = e.OnlineCount,
                    LoadStatus = e.Enabled == 0
                        ? ZoneLoadStatus.Maintenance
                        : (ZoneLoadStatus)(int)e.LoadLevel,
                    GatewayCount = e.GatewayCount
                });
            }

            return list;
        }

        public static List<CharacterEntry> ToCharacterEntries(S2CUserList list)
        {
            var result = new List<CharacterEntry>();
            if (list?.Entries == null)
            {
                return result;
            }

            foreach (var e in list.Entries)
            {
                result.Add(new CharacterEntry
                {
                    UserId = e.UserId,
                    Name = e.Name ?? string.Empty,
                    Level = e.Level,
                    Vocation = (byte)e.Vocation,
                    Sex = (byte)e.Sex
                });
            }

            return result;
        }
    }
}

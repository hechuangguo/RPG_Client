/// <summary>
/// 初始化 Addressables Remote Profile 与 Map_1002 分组。
/// 菜单：RPG → Setup Map Addressables
/// </summary>
using System.IO;
using UnityEditor;
using UnityEditor.AddressableAssets;
using UnityEditor.AddressableAssets.Settings;
using UnityEditor.AddressableAssets.Settings.GroupSchemas;
using UnityEngine;

namespace Rpg.Client.EditorTools
{
    public static class MapAddressablesSetup
    {
        private const string ScenePath = "assets/_Project/Scenes/World_1002.unity";
        private const string GroupName = "Map_1002";
        private const string SceneAddress = "Addressables/Map_1002/scene";
        private const string RemoteLoadPath = "http://127.0.0.1:8765/rpg/map/addressables/[BuildTarget]";

        [MenuItem("RPG/Setup Map Addressables")]
        public static void SetupMapAddressables()
        {
            if (EditorApplication.isPlaying)
            {
                Debug.LogWarning("MapAddressablesSetup：Play 模式下不可用");
                return;
            }

            var settings = AddressableAssetSettingsDefaultObject.Settings;
            if (settings == null)
            {
                settings = AddressableAssetSettings.Create(
                    AddressableAssetSettingsDefaultObject.kDefaultConfigFolder,
                    AddressableAssetSettingsDefaultObject.kDefaultConfigAssetName,
                    true,
                    true);
                AssetDatabase.SaveAssets();
            }

            EnsureRemoteProfile(settings);
            var group = EnsureGroup(settings, GroupName);
            AddSceneEntry(settings, group, ScenePath, SceneAddress);

            EditorUtility.SetDirty(settings);
            AssetDatabase.SaveAssets();
            Debug.Log("MapAddressablesSetup：Addressables 已配置 Map_1002 Remote 组");
        }

        /// <summary>batchmode：-executeMethod Rpg.Client.EditorTools.MapAddressablesSetup.SetupMapAddressablesBatch</summary>
        public static void SetupMapAddressablesBatch() => SetupMapAddressables();

        private static void EnsureRemoteProfile(AddressableAssetSettings settings)
        {
            var profileId = settings.profileSettings.GetProfileId("Default");
            if (string.IsNullOrEmpty(profileId))
            {
                return;
            }

            settings.profileSettings.SetValue(profileId, AddressableAssetSettings.kRemoteBuildPath, "ServerData/[BuildTarget]");
            settings.profileSettings.SetValue(profileId, AddressableAssetSettings.kRemoteLoadPath, RemoteLoadPath);
            settings.activeProfileId = profileId;
        }

        private static AddressableAssetGroup EnsureGroup(AddressableAssetSettings settings, string groupName)
        {
            var group = settings.FindGroup(groupName);
            if (group == null)
            {
                group = settings.CreateGroup(
                    groupName,
                    false,
                    false,
                    true,
                    null,
                    typeof(BundledAssetGroupSchema),
                    typeof(ContentUpdateGroupSchema));
            }

            var bundled = group.GetSchema<BundledAssetGroupSchema>();
            if (bundled != null)
            {
                bundled.BuildPath.SetVariableByName(settings, AddressableAssetSettings.kRemoteBuildPath);
                bundled.LoadPath.SetVariableByName(settings, AddressableAssetSettings.kRemoteLoadPath);
                bundled.BundleMode = BundledAssetGroupSchema.BundlePackingMode.PackTogether;
            }

            return group;
        }

        private static void AddSceneEntry(
            AddressableAssetSettings settings,
            AddressableAssetGroup group,
            string assetPath,
            string address)
        {
            if (!File.Exists(assetPath))
            {
                Debug.LogWarningFormat("MapAddressablesSetup：场景不存在 {0}，请先执行 Setup World Scene 1002", assetPath);
                return;
            }

            var guid = AssetDatabase.AssetPathToGUID(assetPath);
            if (string.IsNullOrEmpty(guid))
            {
                return;
            }

            var entry = settings.FindAssetEntry(guid);
            if (entry == null)
            {
                entry = settings.CreateOrMoveEntry(guid, group);
            }
            else if (entry.parentGroup != group)
            {
                settings.MoveEntry(entry, group);
            }

            entry.address = address;
            entry.SetLabel("Map_1002", true, true);
        }
    }
}

/// <summary>
/// Addressables 内容构建（batchmode 入口）。
/// </summary>
using UnityEditor;
using UnityEditor.AddressableAssets.Settings;

namespace Rpg.Client.EditorTools
{
    public static class MapAddressablesBuild
    {
        /// <summary>batchmode：-executeMethod Rpg.Client.EditorTools.MapAddressablesBuild.BuildMapAddressablesBatch</summary>
        public static void BuildMapAddressablesBatch()
        {
            MapAddressablesSetup.SetupMapAddressablesBatch();

            var settings = UnityEditor.AddressableAssets.AddressableAssetSettingsDefaultObject.Settings;
            if (settings == null)
            {
                EditorApplication.Exit(1);
                return;
            }

            AddressableAssetSettings.BuildPlayerContent();
            EditorApplication.Exit(0);
        }
    }
}

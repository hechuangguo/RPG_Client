/// <summary>
/// Boot 场景一键生成（菜单 RPG → Setup Boot Scene）。
/// 职责：创建 Canvas 层级、绑定 GameApp/GameUiController SerializeField、注册 Build Settings。
/// </summary>
using System.Collections.Generic;
using System.IO;
using Rpg.Client.App;
using Rpg.Client.UI;
using Rpg.Client.World;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace Rpg.Client.EditorTools
{
    public static class BootSceneSetup
    {
        private const string ScenePath = "assets/_Project/Scenes/Boot.unity";
        private const string ZoneListItemPrefabPath = "assets/_Project/Prefabs/UI/ZoneListItem.prefab";
        private const string CharacterListItemPrefabPath = "assets/_Project/Prefabs/UI/CharacterListItem.prefab";
        private const string BackdropArtRoot = "assets/_Project/Art/UI/LoginFlowBackdrop";
        private static readonly Color PanelOverlayColor = new Color(0.08f, 0.1f, 0.14f, 0.5f);
        private const float RefWidth = 1280f;
        private const float RefHeight = 720f;

        /// <summary>batchmode：Unity -executeMethod Rpg.Client.EditorTools.BootSceneSetup.SetupBootSceneBatch</summary>
        public static void SetupBootSceneBatch() => SetupBootSceneInternal(forceOverwrite: true);

        /// <summary>batchmode：Unity -executeMethod Rpg.Client.EditorTools.BootSceneSetup.AddLoginFlowBackdropBatch</summary>
        public static void AddLoginFlowBackdropBatch() => AddLoginFlowBackdrop();

        private static bool GuardNotPlaying(string action)
        {
            if (!EditorApplication.isPlaying && !EditorApplication.isPlayingOrWillChangePlaymode)
            {
                return true;
            }

            Debug.LogWarningFormat("BootSceneSetup：Play 模式下无法{0}，请先停止运行", action);
            return false;
        }

        [MenuItem("RPG/Setup Boot Scene")]
        public static void SetupBootScene()
        {
            if (!GuardNotPlaying("重建 Boot 场景"))
            {
                return;
            }
            var force = !File.Exists(ScenePath) ||
                        EditorUtility.DisplayDialog("Setup Boot Scene", "Boot.unity 已存在，是否覆盖？", "覆盖", "取消");
            if (!force)
            {
                return;
            }

            SetupBootSceneInternal(forceOverwrite: true);
        }

        /// <summary>修复 Boot 场景中 Missing Script（如 VocationDropdown / SexDropdown 的 UGUI Dropdown）。</summary>
        [MenuItem("RPG/Add Login Flow Backdrop")]
        public static void AddLoginFlowBackdrop()
        {
            if (!GuardNotPlaying("添加登录流程背景"))
            {
                return;
            }

            if (!File.Exists(ScenePath))
            {
                Debug.LogWarning("BootSceneSetup：未找到 Boot.unity，请先执行 Setup Boot Scene");
                return;
            }

            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            var canvas = Object.FindObjectOfType<Canvas>();
            var ui = Object.FindObjectOfType<GameUiController>();
            if (canvas == null || ui == null)
            {
                Debug.LogWarning("BootSceneSetup：场景中缺少 Canvas 或 GameUiController");
                return;
            }

            var existing = canvas.GetComponentInChildren<LoginFlowBackdrop>(true);
            if (existing != null)
            {
                existing.transform.SetAsFirstSibling();
                RefreshLoginFlowBackdropAssets(existing);
                WireLoginFlowBackdrop(ui, existing);
                SoftenLoginFlowPanels(canvas.transform);
                EditorSceneManager.MarkSceneDirty(scene);
                EditorSceneManager.SaveScene(scene);
                Debug.Log("BootSceneSetup：已重新绑定并刷新 LoginFlowBackdrop 资源");
                return;
            }

            var backdrop = CreateLoginFlowBackdrop(canvas.transform);
            backdrop.transform.SetAsFirstSibling();
            WireLoginFlowBackdrop(ui, backdrop);
            SoftenLoginFlowPanels(canvas.transform);
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene);
            Debug.Log("BootSceneSetup：已添加 LoginFlowBackdrop");
        }

        [MenuItem("RPG/Fix Boot Scene Missing Scripts")]
        public static void FixBootSceneMissingScripts()
        {
            if (!GuardNotPlaying("修复 Boot 场景"))
            {
                return;
            }

            if (!File.Exists(ScenePath))
            {
                Debug.LogWarning("BootSceneSetup：未找到 Boot.unity，请先执行 Setup Boot Scene");
                return;
            }

            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            var removed = 0;
            foreach (var root in scene.GetRootGameObjects())
            {
                removed += RemoveMissingScriptsRecursive(root.transform);
            }

            if (RebuildCharacterSelectDropdowns())
            {
                EditorSceneManager.MarkSceneDirty(scene);
                EditorSceneManager.SaveScene(scene);
                Debug.LogFormat("BootSceneSetup：已修复 Missing Script（清理 {0} 处）并重建职业/性别 Dropdown", removed);
            }
            else
            {
                Debug.LogWarning("BootSceneSetup：未找到 CharacterSelectPanel，仅清理 Missing Script");
            }
        }

        private static int RemoveMissingScriptsRecursive(Transform node)
        {
            var removed = GameObjectUtility.RemoveMonoBehavioursWithMissingScript(node.gameObject);
            for (var i = 0; i < node.childCount; i++)
            {
                removed += RemoveMissingScriptsRecursive(node.GetChild(i));
            }

            return removed;
        }

        private static bool RebuildCharacterSelectDropdowns()
        {
            var panel = Object.FindObjectOfType<CharacterSelectPanel>(true);
            if (panel == null)
            {
                return false;
            }

            var font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf")
                       ?? Resources.GetBuiltinResource<Font>("Arial.ttf");
            var parent = panel.transform;

            DestroyIfExists(parent, "VocationDropdown");
            DestroyIfExists(parent, "SexDropdown");

            var vocationDropdown = CreateDropdown(parent, "VocationDropdown", font,
                new List<string> { "战士", "法师" });
            SetAnchored(vocationDropdown.GetComponent<RectTransform>(), new Vector2(0.35f, 0.3f), new Vector2(140, 36));

            var sexDropdown = CreateDropdown(parent, "SexDropdown", font,
                new List<string> { "男", "女" });
            SetAnchored(sexDropdown.GetComponent<RectTransform>(), new Vector2(0.65f, 0.3f), new Vector2(140, 36));

            var so = new SerializedObject(panel);
            so.FindProperty("_vocationDropdown").objectReferenceValue = vocationDropdown;
            so.FindProperty("_sexDropdown").objectReferenceValue = sexDropdown;
            so.ApplyModifiedPropertiesWithoutUndo();
            return true;
        }

        private static void DestroyIfExists(Transform parent, string childName)
        {
            var child = parent.Find(childName);
            if (child != null)
            {
                Object.DestroyImmediate(child.gameObject);
            }
        }

        private static void SetupBootSceneInternal(bool forceOverwrite)
        {
            if (File.Exists(ScenePath) && !forceOverwrite)
            {
                return;
            }

            var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);

            // 移除默认 Main Camera / Directional Light（稍后自建）
            Object.DestroyImmediate(GameObject.Find("Main Camera"));
            var light = GameObject.Find("Directional Light");
            if (light != null)
            {
                Object.DestroyImmediate(light);
            }

            var font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (font == null)
            {
                font = Resources.GetBuiltinResource<Font>("Arial.ttf");
            }

            var gameRoot = new GameObject("GameRoot");
            var gameApp = gameRoot.AddComponent<GameApp>();

            var worldGo = new GameObject("World");
            worldGo.transform.SetParent(gameRoot.transform, false);
            var world = worldGo.AddComponent<WorldController>();

            var entitiesGo = new GameObject("Entities");
            entitiesGo.transform.SetParent(worldGo.transform, false);
            var entityManager = entitiesGo.AddComponent<EntityManager>();

            var entityRoot = new GameObject("EntityRoot");
            entityRoot.transform.SetParent(entitiesGo.transform, false);

            var cameraGo = new GameObject("Main Camera");
            cameraGo.transform.SetParent(gameRoot.transform, false);
            cameraGo.tag = "MainCamera";
            var cam = cameraGo.AddComponent<Camera>();
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = new Color(0.15f, 0.18f, 0.22f);
            cameraGo.AddComponent<AudioListener>();

            var eventSystemGo = new GameObject("EventSystem");
            eventSystemGo.AddComponent<EventSystem>();
            eventSystemGo.AddComponent<StandaloneInputModule>();

            var canvasGo = CreateCanvasRoot();
            var loginFlowBackdrop = CreateLoginFlowBackdrop(canvasGo.transform);
            loginFlowBackdrop.transform.SetAsFirstSibling();
            var ui = canvasGo.AddComponent<GameUiController>();

            var statusBar = CreateStretchPanel(canvasGo.transform, "StatusBar", new Color(0, 0, 0, 0.35f));
            var statusRect = statusBar.GetComponent<RectTransform>();
            statusRect.anchorMin = new Vector2(0, 1);
            statusRect.anchorMax = new Vector2(1, 1);
            statusRect.pivot = new Vector2(0.5f, 1);
            statusRect.sizeDelta = new Vector2(0, 48);
            statusRect.anchoredPosition = Vector2.zero;

            var statusText = CreateText(statusBar.transform, "StatusText", string.Empty, font, 16,
                TextAnchor.MiddleLeft, new Vector2(12, 0), new Vector2(-12, 0));
            var errorText = CreateText(statusBar.transform, "ErrorText", string.Empty, font, 16,
                TextAnchor.MiddleRight, new Vector2(12, 0), new Vector2(-12, 0));
            errorText.color = new Color(1f, 0.45f, 0.45f);

            var zoneHomePanel = CreatePanel(canvasGo.transform, "ZoneHomePanel", true);
            var zoneNameText = CreateText(zoneHomePanel.transform, "ZoneNameText", "未选择", font, 24,
                TextAnchor.MiddleCenter, Vector2.zero, Vector2.zero);
            SetAnchored(zoneNameText.rectTransform, new Vector2(0.5f, 0.65f), new Vector2(400, 40));
            var selectServerBtn = CreateButton(zoneHomePanel.transform, "SelectServerBtn", "选择区服", font);
            SetAnchored(selectServerBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.45f), new Vector2(200, 44));
            var enterGameBtn = CreateButton(zoneHomePanel.transform, "EnterGameBtn", "进入游戏", font);
            SetAnchored(enterGameBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.35f), new Vector2(200, 44));

            var serverListPanelGo = CreatePanel(canvasGo.transform, "ServerListPanel", false);
            var zoneListItemPrefab = EnsureZoneListItemPrefab(font);
            var serverList = serverListPanelGo.AddComponent<ServerListPanel>();
            var serverListHint = CreateText(serverListPanelGo.transform, "HintText", "正在拉取区列表…", font, 20,
                TextAnchor.MiddleCenter, Vector2.zero, new Vector2(500, 40));
            SetAnchored(serverListHint.rectTransform, new Vector2(0.5f, 0.82f), new Vector2(520, 40));
            var scroll = CreateScrollView(serverListPanelGo.transform, "ZoneScrollView", out var listContent);
            SetAnchored(scroll.GetComponent<RectTransform>(), new Vector2(0.5f, 0.52f), new Vector2(520, 320));
            var confirmServerListBtn = CreateButton(serverListPanelGo.transform, "ConfirmServerListBtn", "确认", font);
            SetAnchored(confirmServerListBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.22f), new Vector2(160, 44));
            var cancelServerListBtn = CreateButton(serverListPanelGo.transform, "CancelServerListBtn", "取消", font);
            SetAnchored(cancelServerListBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.14f), new Vector2(160, 44));
            WireServerListPanel(serverList, serverListHint, listContent, zoneListItemPrefab,
                confirmServerListBtn, cancelServerListBtn);

            var authPanel = CreatePanel(canvasGo.transform, "AuthPanel", false);
            var accountInput = CreateInputField(authPanel.transform, "AccountInput", "账号", font);
            SetAnchored(accountInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.60f), new Vector2(320, 40));
            var passwordInput = CreateInputField(authPanel.transform, "PasswordInput", "密码", font, true);
            SetAnchored(passwordInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.52f), new Vector2(320, 40));
            var showLoginPasswordToggle = CreateToggle(authPanel.transform, "ShowLoginPasswordToggle", "显示密码", font);
            SetAnchored(showLoginPasswordToggle.GetComponent<RectTransform>(), new Vector2(0.5f, 0.45f), new Vector2(200, 30));
            var rememberToggle = CreateToggle(authPanel.transform, "RememberToggle", "记住账号", font);
            SetAnchored(rememberToggle.GetComponent<RectTransform>(), new Vector2(0.5f, 0.38f), new Vector2(200, 30));
            var loginBtn = CreateButton(authPanel.transform, "LoginBtn", "登录", font);
            SetAnchored(loginBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.30f), new Vector2(200, 44));
            var gotoRegisterBtn = CreateButton(authPanel.transform, "GotoRegisterBtn", "注册账号", font);
            SetAnchored(gotoRegisterBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.22f), new Vector2(200, 44));
            var authSelectServerBtn = CreateButton(authPanel.transform, "AuthSelectServerBtn", "选择服务器", font);
            SetAnchored(authSelectServerBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.14f), new Vector2(200, 44));

            var registerPanel = CreatePanel(canvasGo.transform, "RegisterPanel", false);
            var regAccount = CreateInputField(registerPanel.transform, "RegAccount", "账号", font);
            SetAnchored(regAccount.GetComponent<RectTransform>(), new Vector2(0.5f, 0.60f), new Vector2(320, 40));
            var regPassword = CreateInputField(registerPanel.transform, "RegPassword", "密码", font, true);
            SetAnchored(regPassword.GetComponent<RectTransform>(), new Vector2(0.5f, 0.52f), new Vector2(320, 40));
            var regConfirm = CreateInputField(registerPanel.transform, "RegConfirm", "确认密码", font, true);
            SetAnchored(regConfirm.GetComponent<RectTransform>(), new Vector2(0.5f, 0.44f), new Vector2(320, 40));
            var showRegisterPasswordToggle = CreateToggle(registerPanel.transform, "ShowRegisterPasswordToggle", "显示密码", font);
            SetAnchored(showRegisterPasswordToggle.GetComponent<RectTransform>(), new Vector2(0.5f, 0.37f), new Vector2(200, 30));
            var registerBtn = CreateButton(registerPanel.transform, "RegisterBtn", "提交注册", font);
            SetAnchored(registerBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.28f), new Vector2(200, 44));
            var backToLoginBtn = CreateButton(registerPanel.transform, "BackToLoginBtn", "返回登录", font);
            SetAnchored(backToLoginBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.20f), new Vector2(200, 44));

            var characterPanel = CreatePanel(canvasGo.transform, "CharacterPanel", false);
            var characterSelect = characterPanel.AddComponent<CharacterSelectPanel>();
            var charHint = CreateText(characterPanel.transform, "HintText", "请选择角色", font, 18,
                TextAnchor.MiddleCenter, Vector2.zero, new Vector2(400, 32));
            SetAnchored(charHint.rectTransform, new Vector2(0.5f, 0.88f), new Vector2(420, 32));
            var charScroll = CreateScrollView(characterPanel.transform, "CharacterScrollView", out var charListContent);
            SetAnchored(charScroll.GetComponent<RectTransform>(), new Vector2(0.5f, 0.62f), new Vector2(420, 180));
            var charListItemPrefab = EnsureCharacterListItemPrefab(font);
            var createNameInput = CreateInputField(characterPanel.transform, "CreateNameInput", "角色名", font);
            SetAnchored(createNameInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.38f), new Vector2(280, 40));
            var vocationDropdown = CreateDropdown(characterPanel.transform, "VocationDropdown", font,
                new List<string> { "战士", "法师" });
            SetAnchored(vocationDropdown.GetComponent<RectTransform>(), new Vector2(0.35f, 0.3f), new Vector2(140, 36));
            var sexDropdown = CreateDropdown(characterPanel.transform, "SexDropdown", font,
                new List<string> { "男", "女" });
            SetAnchored(sexDropdown.GetComponent<RectTransform>(), new Vector2(0.65f, 0.3f), new Vector2(140, 36));
            var createCharBtn = CreateButton(characterPanel.transform, "CreateCharBtn", "创建角色", font);
            SetAnchored(createCharBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.2f), new Vector2(200, 44));
            var enterWorldBtn = CreateButton(characterPanel.transform, "EnterWorldBtn", "进入世界", font);
            SetAnchored(enterWorldBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.1f), new Vector2(200, 44));
            WireCharacterSelectPanel(characterSelect, charHint, charListContent, charListItemPrefab,
                createNameInput, vocationDropdown, sexDropdown, createCharBtn, enterWorldBtn);

            var gameHudPanel = CreatePanel(canvasGo.transform, "GameHudPanel", false);
            CreateText(gameHudPanel.transform, "HudHint", "游戏中 — WASD 移动，ESC 退出", font, 18,
                TextAnchor.LowerCenter, new Vector2(0, 24), new Vector2(600, 36));

            var exitDialog = CreateStretchPanel(canvasGo.transform, "ExitDialog", new Color(0, 0, 0, 0.65f));
            exitDialog.SetActive(false);
            var exitBox = CreateStretchPanel(exitDialog.transform, "Box", new Color(0.12f, 0.14f, 0.18f, 0.95f));
            var exitBoxRect = exitBox.GetComponent<RectTransform>();
            exitBoxRect.anchorMin = new Vector2(0.5f, 0.5f);
            exitBoxRect.anchorMax = new Vector2(0.5f, 0.5f);
            exitBoxRect.sizeDelta = new Vector2(360, 220);
            CreateText(exitBox.transform, "Title", "退出游戏", font, 22,
                TextAnchor.UpperCenter, new Vector2(0, -16), new Vector2(-20, 40));
            var exitReturnCharBtn = CreateButton(exitBox.transform, "BtnReturnCharSelect", "返回选角", font);
            SetAnchored(exitReturnCharBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.55f), new Vector2(260, 40));
            var exitReturnLoginBtn = CreateButton(exitBox.transform, "BtnReturnLogin", "返回登录", font);
            SetAnchored(exitReturnLoginBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.38f), new Vector2(260, 40));
            var exitQuitBtn = CreateButton(exitBox.transform, "BtnQuit", "退出客户端", font);
            SetAnchored(exitQuitBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.21f), new Vector2(260, 40));

            WireUiController(ui, zoneHomePanel, serverListPanelGo, serverList, authPanel, registerPanel, characterPanel,
                characterSelect, gameHudPanel, exitDialog, zoneNameText, selectServerBtn, enterGameBtn, accountInput,
                passwordInput, showLoginPasswordToggle, rememberToggle, loginBtn, gotoRegisterBtn, authSelectServerBtn,
                regAccount, regPassword, regConfirm, showRegisterPasswordToggle, registerBtn, backToLoginBtn,
                statusText, errorText, exitReturnCharBtn, exitReturnLoginBtn, exitQuitBtn);
            WireLoginFlowBackdrop(ui, loginFlowBackdrop);

            WireGameApp(gameApp, ui, world);
            WireWorld(world, entityManager);
            WireEntityManager(entityManager, entityRoot.transform);

            var dir = Path.GetDirectoryName(ScenePath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            EditorSceneManager.SaveScene(scene, ScenePath);
            RegisterBuildScene(ScenePath);

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Debug.Log("BootSceneSetup：已生成 " + ScenePath);

            if (!Application.isBatchMode)
            {
                EditorUtility.DisplayDialog("Setup Boot Scene",
                    "Boot 场景已生成并加入 Build Settings。\n" + ScenePath, "确定");
            }
        }

        private static void RegisterBuildScene(string scenePath)
        {
            var sceneAsset = AssetDatabase.LoadAssetAtPath<SceneAsset>(scenePath);
            if (sceneAsset == null)
            {
                return;
            }

            var scenes = EditorBuildSettings.scenes;
            foreach (var entry in scenes)
            {
                if (entry.path == scenePath)
                {
                    return;
                }
            }

            var list = new EditorBuildSettingsScene[scenes.Length + 1];
            for (var i = 0; i < scenes.Length; i++)
            {
                list[i] = scenes[i];
            }

            list[list.Length - 1] = new EditorBuildSettingsScene(scenePath, true);
            EditorBuildSettings.scenes = list;
        }

        private static GameObject CreateCanvasRoot()
        {
            var go = new GameObject("Canvas");
            var canvas = go.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            go.AddComponent<CanvasScaler>().uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            var scaler = go.GetComponent<CanvasScaler>();
            scaler.referenceResolution = new Vector2(RefWidth, RefHeight);
            scaler.matchWidthOrHeight = 0.5f;
            go.AddComponent<GraphicRaycaster>();
            return go;
        }

        private static GameObject CreateStretchPanel(Transform parent, string name, Color color)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            go.transform.SetParent(parent, false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
            var image = go.GetComponent<Image>();
            image.color = color;
            image.raycastTarget = false;
            return go;
        }

        private static GameObject CreatePanel(Transform parent, string name, bool active)
        {
            var panel = CreateStretchPanel(parent, name, PanelOverlayColor);
            panel.SetActive(active);
            return panel;
        }

        private static LoginFlowBackdrop CreateLoginFlowBackdrop(Transform canvasRoot)
        {
            var rootGo = new GameObject("LoginFlowBackdrop", typeof(RectTransform), typeof(LoginFlowBackdrop));
            rootGo.transform.SetParent(canvasRoot, false);
            StretchFull(rootGo.GetComponent<RectTransform>());

            var baseLayer = CreateBackdropLayer(rootGo.transform, "Base",
                LoadBackdropSprite("backdrop_base.png"), Color.white);
            baseLayer.raycastTarget = false;

            var backdrop = rootGo.GetComponent<LoginFlowBackdrop>();
            backdrop.BindBase(baseLayer, baseLayer.sprite);
            return backdrop;
        }

        private static Image CreateBackdropLayer(Transform parent, string name, Sprite sprite, Color color)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            go.transform.SetParent(parent, false);
            StretchFull(go.GetComponent<RectTransform>());
            var image = go.GetComponent<Image>();
            image.sprite = sprite;
            image.color = color;
            image.type = Image.Type.Simple;
            image.preserveAspect = false;
            return image;
        }

        private static void StretchFull(RectTransform rt)
        {
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
        }

        private static Sprite LoadBackdropSprite(string fileName)
        {
            var path = $"{BackdropArtRoot}/{fileName}";
            var sprite = AssetDatabase.LoadAssetAtPath<Sprite>(path);
            if (sprite != null)
            {
                return sprite;
            }

            var tex = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if (tex == null)
            {
                Debug.LogWarningFormat("BootSceneSetup：未找到背景图 {0}", path);
                return null;
            }

            return Sprite.Create(tex, new Rect(0f, 0f, tex.width, tex.height), new Vector2(0.5f, 0.5f), 100f);
        }

        private static void RefreshLoginFlowBackdropAssets(LoginFlowBackdrop backdrop)
        {
            if (backdrop == null)
            {
                return;
            }

            var root = backdrop.transform;
            var baseLayer = FindBackdropLayerImage(root, "Base");
            AssignBackdropSprite(baseLayer, "backdrop_base.png");
            backdrop.BindBase(baseLayer, baseLayer != null ? baseLayer.sprite : null);
            EditorUtility.SetDirty(backdrop);
        }

        private static Image FindBackdropLayerImage(Transform root, string layerName)
        {
            return root.Find(layerName)?.GetComponent<Image>();
        }

        private static void AssignBackdropSprite(Image image, string fileName)
        {
            if (image == null)
            {
                return;
            }

            var sprite = LoadBackdropSprite(fileName);
            if (sprite == null)
            {
                return;
            }

            image.sprite = sprite;
            EditorUtility.SetDirty(image);
        }

        private static void WireLoginFlowBackdrop(GameUiController ui, LoginFlowBackdrop backdrop)
        {
            var so = new SerializedObject(ui);
            so.FindProperty("_loginFlowBackdrop").objectReferenceValue = backdrop;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static void SoftenLoginFlowPanels(Transform canvasRoot)
        {
            var names = new[]
            {
                "ZoneHomePanel", "ServerListPanel", "AuthPanel", "RegisterPanel", "CharacterPanel"
            };
            foreach (var panelName in names)
            {
                var panel = canvasRoot.Find(panelName);
                if (panel == null)
                {
                    continue;
                }

                var image = panel.GetComponent<Image>();
                if (image != null)
                {
                    image.color = PanelOverlayColor;
                    image.raycastTarget = false;
                }
            }
        }

        private static Text CreateText(Transform parent, string name, string content, Font font, int size,
            TextAnchor anchor, Vector2 offsetMin, Vector2 offsetMax)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            go.transform.SetParent(parent, false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = offsetMin;
            rt.offsetMax = offsetMax;
            var text = go.GetComponent<Text>();
            text.font = font;
            text.fontSize = size;
            text.alignment = anchor;
            text.color = Color.white;
            text.text = content;
            text.horizontalOverflow = HorizontalWrapMode.Wrap;
            text.verticalOverflow = VerticalWrapMode.Overflow;
            return text;
        }

        private static void SetAnchored(RectTransform rt, Vector2 anchorCenter, Vector2 size)
        {
            rt.anchorMin = anchorCenter;
            rt.anchorMax = anchorCenter;
            rt.pivot = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = size;
            rt.anchoredPosition = Vector2.zero;
        }

        private static Button CreateButton(Transform parent, string name, string label, Font font)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(Button));
            go.transform.SetParent(parent, false);
            go.GetComponent<Image>().color = new Color(0.22f, 0.38f, 0.58f);
            var text = CreateText(go.transform, "Text", label, font, 18, TextAnchor.MiddleCenter,
                Vector2.zero, Vector2.zero);
            var btn = go.GetComponent<Button>();
            btn.targetGraphic = go.GetComponent<Image>();
            return btn;
        }

        private static InputField CreateInputField(Transform parent, string name, string placeholder, Font font,
            bool password = false)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(InputField));
            go.transform.SetParent(parent, false);
            go.GetComponent<Image>().color = new Color(0.1f, 0.12f, 0.16f);

            var textGo = new GameObject("Text", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            textGo.transform.SetParent(go.transform, false);
            var textRt = textGo.GetComponent<RectTransform>();
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = new Vector2(8, 4);
            textRt.offsetMax = new Vector2(-8, -4);
            var text = textGo.GetComponent<Text>();
            text.font = font;
            text.fontSize = 16;
            text.color = Color.white;
            text.supportRichText = false;

            var phGo = new GameObject("Placeholder", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            phGo.transform.SetParent(go.transform, false);
            var phRt = phGo.GetComponent<RectTransform>();
            phRt.anchorMin = Vector2.zero;
            phRt.anchorMax = Vector2.one;
            phRt.offsetMin = new Vector2(8, 4);
            phRt.offsetMax = new Vector2(-8, -4);
            var ph = phGo.GetComponent<Text>();
            ph.font = font;
            ph.fontSize = 16;
            ph.fontStyle = FontStyle.Italic;
            ph.color = new Color(1, 1, 1, 0.45f);
            ph.text = placeholder;

            var field = go.GetComponent<InputField>();
            field.textComponent = text;
            field.placeholder = ph;
            if (password)
            {
                field.contentType = InputField.ContentType.Password;
            }

            return field;
        }

        private static Toggle CreateToggle(Transform parent, string name, string label, Font font)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Toggle));
            go.transform.SetParent(parent, false);

            var bg = new GameObject("Background", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            bg.transform.SetParent(go.transform, false);
            var bgRt = bg.GetComponent<RectTransform>();
            bgRt.anchorMin = new Vector2(0, 0.5f);
            bgRt.anchorMax = new Vector2(0, 0.5f);
            bgRt.sizeDelta = new Vector2(20, 20);
            bgRt.anchoredPosition = new Vector2(10, 0);
            bg.GetComponent<Image>().color = new Color(0.15f, 0.17f, 0.22f);

            var check = new GameObject("Checkmark", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            check.transform.SetParent(bg.transform, false);
            var checkRt = check.GetComponent<RectTransform>();
            checkRt.anchorMin = Vector2.zero;
            checkRt.anchorMax = Vector2.one;
            checkRt.offsetMin = new Vector2(4, 4);
            checkRt.offsetMax = new Vector2(-4, -4);
            check.GetComponent<Image>().color = new Color(0.35f, 0.75f, 0.45f);

            var labelText = CreateText(go.transform, "Label", label, font, 16, TextAnchor.MiddleLeft,
                new Vector2(36, 0), new Vector2(-8, 0));

            var toggle = go.GetComponent<Toggle>();
            toggle.targetGraphic = bg.GetComponent<Image>();
            toggle.graphic = check.GetComponent<Image>();
            return toggle;
        }

        private static CharacterListItemView EnsureCharacterListItemPrefab(Font font)
        {
            var existing = AssetDatabase.LoadAssetAtPath<CharacterListItemView>(CharacterListItemPrefabPath);
            if (existing != null)
            {
                return existing;
            }

            var dir = Path.GetDirectoryName(CharacterListItemPrefabPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            var go = new GameObject("CharacterListItem", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image),
                typeof(CharacterListItemView), typeof(Button), typeof(LayoutElement));
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = new Vector2(0f, 1f);
            rt.anchorMax = new Vector2(1f, 1f);
            rt.pivot = new Vector2(0.5f, 1f);
            rt.sizeDelta = new Vector2(0f, 44f);
            var layout = go.GetComponent<LayoutElement>();
            layout.minHeight = 44f;
            layout.preferredHeight = 44f;
            layout.flexibleWidth = 1f;
            go.GetComponent<Image>().color = new Color(0.14f, 0.16f, 0.2f, 0.95f);

            var nameText = CreateText(go.transform, "NameText", "角色名", font, 18,
                TextAnchor.MiddleLeft, new Vector2(12, 0), new Vector2(-80, 0));
            var levelText = CreateText(go.transform, "LevelText", "Lv1", font, 16,
                TextAnchor.MiddleRight, new Vector2(12, 0), new Vector2(-12, 0));

            var item = go.GetComponent<CharacterListItemView>();
            item.InitVisuals(nameText, levelText, go.GetComponent<Image>(), go.GetComponent<Button>());

            var btn = go.GetComponent<Button>();
            btn.targetGraphic = go.GetComponent<Image>();

            PrefabUtility.SaveAsPrefabAsset(go, CharacterListItemPrefabPath);
            Object.DestroyImmediate(go);
            return AssetDatabase.LoadAssetAtPath<CharacterListItemView>(CharacterListItemPrefabPath);
        }

        private static Dropdown CreateDropdown(Transform parent, string name, Font font, List<string> options)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(Dropdown));
            go.transform.SetParent(parent, false);
            go.GetComponent<Image>().color = new Color(0.1f, 0.12f, 0.16f);

            var labelGo = new GameObject("Label", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            labelGo.transform.SetParent(go.transform, false);
            var labelRt = labelGo.GetComponent<RectTransform>();
            labelRt.anchorMin = Vector2.zero;
            labelRt.anchorMax = Vector2.one;
            labelRt.offsetMin = new Vector2(8, 2);
            labelRt.offsetMax = new Vector2(-24, -2);
            var label = labelGo.GetComponent<Text>();
            label.font = font;
            label.fontSize = 16;
            label.color = Color.white;
            label.alignment = TextAnchor.MiddleLeft;

            var arrowGo = new GameObject("Arrow", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            arrowGo.transform.SetParent(go.transform, false);
            var arrowRt = arrowGo.GetComponent<RectTransform>();
            arrowRt.anchorMin = new Vector2(1, 0.5f);
            arrowRt.anchorMax = new Vector2(1, 0.5f);
            arrowRt.sizeDelta = new Vector2(20, 20);
            arrowRt.anchoredPosition = new Vector2(-12, 0);
            var arrow = arrowGo.GetComponent<Text>();
            arrow.font = font;
            arrow.text = "▼";
            arrow.fontSize = 12;
            arrow.alignment = TextAnchor.MiddleCenter;

            var template = new GameObject("Template", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image),
                typeof(ScrollRect));
            template.transform.SetParent(go.transform, false);
            template.SetActive(false);
            var templateRt = template.GetComponent<RectTransform>();
            templateRt.anchorMin = new Vector2(0, 0);
            templateRt.anchorMax = new Vector2(1, 0);
            templateRt.pivot = new Vector2(0.5f, 1);
            templateRt.anchoredPosition = new Vector2(0, 2);
            templateRt.sizeDelta = new Vector2(0, 120);
            template.GetComponent<Image>().color = new Color(0.08f, 0.1f, 0.14f);

            var viewport = CreateStretchPanel(template.transform, "Viewport", new Color(0, 0, 0, 0));
            viewport.AddComponent<RectMask2D>();

            var contentGo = new GameObject("Content", typeof(RectTransform));
            contentGo.transform.SetParent(viewport.transform, false);
            var contentRt = contentGo.GetComponent<RectTransform>();
            contentRt.anchorMin = new Vector2(0, 1);
            contentRt.anchorMax = new Vector2(1, 1);
            contentRt.pivot = new Vector2(0.5f, 1);
            contentRt.sizeDelta = new Vector2(0, 28);

            var itemGo = new GameObject("Item", typeof(RectTransform), typeof(Toggle));
            itemGo.transform.SetParent(contentGo.transform, false);
            var itemRt = itemGo.GetComponent<RectTransform>();
            itemRt.anchorMin = new Vector2(0, 0.5f);
            itemRt.anchorMax = new Vector2(1, 0.5f);
            itemRt.sizeDelta = new Vector2(0, 28);
            var itemBg = new GameObject("Item Background", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            itemBg.transform.SetParent(itemGo.transform, false);
            var itemBgRt = itemBg.GetComponent<RectTransform>();
            itemBgRt.anchorMin = Vector2.zero;
            itemBgRt.anchorMax = Vector2.one;
            itemBgRt.offsetMin = Vector2.zero;
            itemBgRt.offsetMax = Vector2.zero;
            itemBg.GetComponent<Image>().color = new Color(0.12f, 0.14f, 0.18f);
            var itemLabel = CreateText(itemGo.transform, "Item Label", string.Empty, font, 14,
                TextAnchor.MiddleLeft, new Vector2(8, 0), new Vector2(-8, 0));
            var toggle = itemGo.GetComponent<Toggle>();
            toggle.targetGraphic = itemBg.GetComponent<Image>();
            toggle.graphic = null;

            var scroll = template.GetComponent<ScrollRect>();
            scroll.viewport = viewport.GetComponent<RectTransform>();
            scroll.content = contentRt;
            scroll.horizontal = false;

            var dropdown = go.GetComponent<Dropdown>();
            dropdown.targetGraphic = go.GetComponent<Image>();
            dropdown.template = templateRt;
            dropdown.captionText = label;
            dropdown.itemText = itemLabel;
            dropdown.options = options.ConvertAll(o => new Dropdown.OptionData(o));
            return dropdown;
        }

        private static void WireCharacterSelectPanel(CharacterSelectPanel panel, Text hint, Transform content,
            CharacterListItemView itemPrefab, InputField createName, Dropdown vocation, Dropdown sex,
            Button createBtn, Button enterBtn)
        {
            var so = new SerializedObject(panel);
            so.FindProperty("_hintText").objectReferenceValue = hint;
            so.FindProperty("_listContent").objectReferenceValue = content;
            so.FindProperty("_itemPrefab").objectReferenceValue = itemPrefab;
            so.FindProperty("_createNameInput").objectReferenceValue = createName;
            so.FindProperty("_vocationDropdown").objectReferenceValue = vocation;
            so.FindProperty("_sexDropdown").objectReferenceValue = sex;
            so.FindProperty("_createCharBtn").objectReferenceValue = createBtn;
            so.FindProperty("_enterWorldBtn").objectReferenceValue = enterBtn;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static ZoneListItemView EnsureZoneListItemPrefab(Font font)
        {
            var existing = AssetDatabase.LoadAssetAtPath<ZoneListItemView>(ZoneListItemPrefabPath);
            if (existing != null)
            {
                return existing;
            }

            var dir = Path.GetDirectoryName(ZoneListItemPrefabPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            var go = new GameObject("ZoneListItem", typeof(RectTransform), typeof(CanvasRenderer), typeof(Image),
                typeof(ZoneListItemView), typeof(Button), typeof(LayoutElement));
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = new Vector2(0f, 1f);
            rt.anchorMax = new Vector2(1f, 1f);
            rt.pivot = new Vector2(0.5f, 1f);
            rt.sizeDelta = new Vector2(0f, 48f);
            var layout = go.GetComponent<LayoutElement>();
            layout.minHeight = 48f;
            layout.preferredHeight = 48f;
            layout.flexibleWidth = 1f;
            go.GetComponent<Image>().color = new Color(0.14f, 0.16f, 0.2f, 0.95f);

            var nameText = CreateText(go.transform, "NameText", "区服名", font, 18,
                TextAnchor.MiddleLeft, new Vector2(12, 0), new Vector2(-280, 0));
            var statusText = CreateText(go.transform, "StatusText", "流畅", font, 16,
                TextAnchor.MiddleCenter, Vector2.zero, new Vector2(80, 0));
            SetAnchored(statusText.rectTransform, new Vector2(0.72f, 0.5f), new Vector2(80, 28));
            var onlineText = CreateText(go.transform, "OnlineText", string.Empty, font, 14,
                TextAnchor.MiddleRight, new Vector2(12, 0), new Vector2(-12, 0));

            var item = go.GetComponent<ZoneListItemView>();
            item.InitVisuals(nameText, statusText, onlineText,
                go.GetComponent<Image>(), go.GetComponent<Button>());

            var btn = go.GetComponent<Button>();
            btn.targetGraphic = go.GetComponent<Image>();

            PrefabUtility.SaveAsPrefabAsset(go, ZoneListItemPrefabPath);
            Object.DestroyImmediate(go);
            return AssetDatabase.LoadAssetAtPath<ZoneListItemView>(ZoneListItemPrefabPath);
        }

        private static GameObject CreateScrollView(Transform parent, string name, out Transform content)
        {
            var scrollGo = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image),
                typeof(ScrollRect));
            scrollGo.transform.SetParent(parent, false);
            scrollGo.GetComponent<Image>().color = new Color(0.06f, 0.08f, 0.1f, 0.9f);

            var viewport = CreateStretchPanel(scrollGo.transform, "Viewport", new Color(0, 0, 0, 0));
            viewport.AddComponent<RectMask2D>();

            var contentGo = new GameObject("Content", typeof(RectTransform), typeof(VerticalLayoutGroup),
                typeof(ContentSizeFitter));
            contentGo.transform.SetParent(viewport.transform, false);
            var contentRt = contentGo.GetComponent<RectTransform>();
            contentRt.anchorMin = new Vector2(0, 1);
            contentRt.anchorMax = new Vector2(1, 1);
            contentRt.pivot = new Vector2(0.5f, 1);
            contentRt.anchoredPosition = Vector2.zero;
            contentRt.sizeDelta = new Vector2(0, 0);

            var layout = contentGo.GetComponent<VerticalLayoutGroup>();
            layout.childControlHeight = true;
            layout.childControlWidth = true;
            layout.childForceExpandHeight = false;
            layout.childForceExpandWidth = true;
            layout.spacing = 4;
            layout.padding = new RectOffset(4, 4, 4, 4);

            var fitter = contentGo.GetComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            var scroll = scrollGo.GetComponent<ScrollRect>();
            scroll.viewport = viewport.GetComponent<RectTransform>();
            scroll.content = contentRt;
            scroll.horizontal = false;
            scroll.vertical = true;
            scroll.movementType = ScrollRect.MovementType.Clamped;

            content = contentGo.transform;
            return scrollGo;
        }

        private static void WireServerListPanel(ServerListPanel panel, Text hint, Transform content,
            ZoneListItemView itemPrefab, Button confirmBtn, Button cancelBtn)
        {
            var so = new SerializedObject(panel);
            so.FindProperty("_hintText").objectReferenceValue = hint;
            so.FindProperty("_listContent").objectReferenceValue = content;
            so.FindProperty("_itemPrefab").objectReferenceValue = itemPrefab;
            so.FindProperty("_confirmBtn").objectReferenceValue = confirmBtn;
            so.FindProperty("_cancelBtn").objectReferenceValue = cancelBtn;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static void WireUiController(GameUiController ui, GameObject zoneHomePanel,
            GameObject serverListPanel, ServerListPanel serverList, GameObject authPanel, GameObject registerPanel,
            GameObject characterPanel, CharacterSelectPanel characterSelect, GameObject gameHudPanel,
            GameObject exitDialog,
            Text zoneNameText, Button selectServerBtn, Button enterGameBtn, InputField accountInput,
            InputField passwordInput, Toggle showLoginPasswordToggle, Toggle rememberToggle, Button loginBtn,
            Button gotoRegisterBtn, Button authSelectServerBtn, InputField regAccount, InputField regPassword,
            InputField regConfirm, Toggle showRegisterPasswordToggle, Button registerBtn, Button backToLoginBtn,
            Text statusText, Text errorText, Button exitReturnCharBtn, Button exitReturnLoginBtn, Button exitQuitBtn)
        {
            var so = new SerializedObject(ui);
            so.FindProperty("_zoneHomePanel").objectReferenceValue = zoneHomePanel;
            so.FindProperty("_serverListPanel").objectReferenceValue = serverListPanel;
            so.FindProperty("_serverList").objectReferenceValue = serverList;
            so.FindProperty("_authPanel").objectReferenceValue = authPanel;
            so.FindProperty("_registerPanel").objectReferenceValue = registerPanel;
            so.FindProperty("_characterPanel").objectReferenceValue = characterPanel;
            so.FindProperty("_characterSelect").objectReferenceValue = characterSelect;
            so.FindProperty("_gameHudPanel").objectReferenceValue = gameHudPanel;
            so.FindProperty("_exitDialog").objectReferenceValue = exitDialog;
            so.FindProperty("_zoneNameText").objectReferenceValue = zoneNameText;
            so.FindProperty("_selectServerBtn").objectReferenceValue = selectServerBtn;
            so.FindProperty("_enterGameBtn").objectReferenceValue = enterGameBtn;
            so.FindProperty("_accountInput").objectReferenceValue = accountInput;
            so.FindProperty("_passwordInput").objectReferenceValue = passwordInput;
            so.FindProperty("_showLoginPasswordToggle").objectReferenceValue = showLoginPasswordToggle;
            so.FindProperty("_rememberToggle").objectReferenceValue = rememberToggle;
            so.FindProperty("_loginBtn").objectReferenceValue = loginBtn;
            so.FindProperty("_gotoRegisterBtn").objectReferenceValue = gotoRegisterBtn;
            so.FindProperty("_authSelectServerBtn").objectReferenceValue = authSelectServerBtn;
            so.FindProperty("_regAccount").objectReferenceValue = regAccount;
            so.FindProperty("_regPassword").objectReferenceValue = regPassword;
            so.FindProperty("_regConfirm").objectReferenceValue = regConfirm;
            so.FindProperty("_showRegisterPasswordToggle").objectReferenceValue = showRegisterPasswordToggle;
            so.FindProperty("_registerBtn").objectReferenceValue = registerBtn;
            so.FindProperty("_backToLoginBtn").objectReferenceValue = backToLoginBtn;
            so.FindProperty("_statusText").objectReferenceValue = statusText;
            so.FindProperty("_errorText").objectReferenceValue = errorText;
            so.FindProperty("_exitReturnCharBtn").objectReferenceValue = exitReturnCharBtn;
            so.FindProperty("_exitReturnLoginBtn").objectReferenceValue = exitReturnLoginBtn;
            so.FindProperty("_exitQuitBtn").objectReferenceValue = exitQuitBtn;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static void WireGameApp(GameApp gameApp, GameUiController ui, WorldController world)
        {
            var so = new SerializedObject(gameApp);
            so.FindProperty("_ui").objectReferenceValue = ui;
            so.FindProperty("_world").objectReferenceValue = world;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static void WireWorld(WorldController world, EntityManager entities)
        {
            var so = new SerializedObject(world);
            so.FindProperty("_entities").objectReferenceValue = entities;
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static void WireEntityManager(EntityManager entityManager, Transform entityRoot)
        {
            var so = new SerializedObject(entityManager);
            so.FindProperty("_entityRoot").objectReferenceValue = entityRoot;
            so.FindProperty("_playerPrefab").objectReferenceValue = CreateEditorPlayerPrefab(entityManager.transform);
            so.ApplyModifiedPropertiesWithoutUndo();
        }

        private static GameObject CreateEditorPlayerPrefab(Transform parent)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            go.name = "PlayerPrefab";
            go.SetActive(false);
            go.transform.SetParent(parent, false);
            var renderer = go.GetComponent<Renderer>();
            if (renderer != null)
            {
                renderer.sharedMaterial = new Material(Shader.Find("Standard"))
                {
                    color = new Color(0.25f, 0.55f, 1f)
                };
            }

            return go;
        }
    }
}

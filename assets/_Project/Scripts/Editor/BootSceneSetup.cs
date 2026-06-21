/// <summary>
/// Boot 场景一键生成（菜单 RPG → Setup Boot Scene）。
/// 职责：创建 Canvas 层级、绑定 GameApp/GameUiController SerializeField、注册 Build Settings。
/// </summary>
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
        private const float RefWidth = 1280f;
        private const float RefHeight = 720f;

        /// <summary>batchmode：Unity -executeMethod Rpg.Client.EditorTools.BootSceneSetup.SetupBootSceneBatch</summary>
        public static void SetupBootSceneBatch() => SetupBootSceneInternal(forceOverwrite: true);

        [MenuItem("RPG/Setup Boot Scene")]
        public static void SetupBootScene()
        {
            var force = !File.Exists(ScenePath) ||
                        EditorUtility.DisplayDialog("Setup Boot Scene", "Boot.unity 已存在，是否覆盖？", "覆盖", "取消");
            if (!force)
            {
                return;
            }

            SetupBootSceneInternal(forceOverwrite: true);
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
            SetAnchored(accountInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.58f), new Vector2(320, 40));
            var passwordInput = CreateInputField(authPanel.transform, "PasswordInput", "密码", font, true);
            SetAnchored(passwordInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.5f), new Vector2(320, 40));
            var rememberToggle = CreateToggle(authPanel.transform, "RememberToggle", "记住账号", font);
            SetAnchored(rememberToggle.GetComponent<RectTransform>(), new Vector2(0.5f, 0.42f), new Vector2(200, 30));
            var loginBtn = CreateButton(authPanel.transform, "LoginBtn", "登录", font);
            SetAnchored(loginBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.32f), new Vector2(200, 44));
            var gotoRegisterBtn = CreateButton(authPanel.transform, "GotoRegisterBtn", "注册账号", font);
            SetAnchored(gotoRegisterBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.24f), new Vector2(200, 44));

            var registerPanel = CreatePanel(canvasGo.transform, "RegisterPanel", false);
            var regAccount = CreateInputField(registerPanel.transform, "RegAccount", "账号", font);
            SetAnchored(regAccount.GetComponent<RectTransform>(), new Vector2(0.5f, 0.58f), new Vector2(320, 40));
            var regPassword = CreateInputField(registerPanel.transform, "RegPassword", "密码", font, true);
            SetAnchored(regPassword.GetComponent<RectTransform>(), new Vector2(0.5f, 0.5f), new Vector2(320, 40));
            var regConfirm = CreateInputField(registerPanel.transform, "RegConfirm", "确认密码", font, true);
            SetAnchored(regConfirm.GetComponent<RectTransform>(), new Vector2(0.5f, 0.42f), new Vector2(320, 40));
            var registerBtn = CreateButton(registerPanel.transform, "RegisterBtn", "提交注册", font);
            SetAnchored(registerBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.32f), new Vector2(200, 44));
            var backToLoginBtn = CreateButton(registerPanel.transform, "BackToLoginBtn", "返回登录", font);
            SetAnchored(backToLoginBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.24f), new Vector2(200, 44));

            var characterPanel = CreatePanel(canvasGo.transform, "CharacterPanel", false);
            var charListText = CreateText(characterPanel.transform, "CharListText", "暂无角色", font, 18,
                TextAnchor.UpperLeft, new Vector2(20, -20), new Vector2(-40, -120));
            SetAnchored(charListText.rectTransform, new Vector2(0.5f, 0.65f), new Vector2(400, 160));
            var createNameInput = CreateInputField(characterPanel.transform, "CreateNameInput", "角色名", font);
            SetAnchored(createNameInput.GetComponent<RectTransform>(), new Vector2(0.5f, 0.38f), new Vector2(280, 40));
            var enterWorldBtn = CreateButton(characterPanel.transform, "EnterWorldBtn", "进入世界", font);
            SetAnchored(enterWorldBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.28f), new Vector2(200, 44));
            var createCharBtn = CreateButton(characterPanel.transform, "CreateCharBtn", "创建角色", font);
            SetAnchored(createCharBtn.GetComponent<RectTransform>(), new Vector2(0.5f, 0.2f), new Vector2(200, 44));

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
                gameHudPanel, exitDialog, zoneNameText, selectServerBtn, enterGameBtn, accountInput,
                passwordInput, rememberToggle, loginBtn, gotoRegisterBtn, regAccount, regPassword, regConfirm,
                registerBtn, backToLoginBtn, charListText, createNameInput, enterWorldBtn,
                createCharBtn, statusText, errorText, exitReturnCharBtn, exitReturnLoginBtn, exitQuitBtn);

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
            go.GetComponent<Image>().color = color;
            return go;
        }

        private static GameObject CreatePanel(Transform parent, string name, bool active)
        {
            var panel = CreateStretchPanel(parent, name, new Color(0.08f, 0.1f, 0.14f, 0.85f));
            panel.SetActive(active);
            return panel;
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
                typeof(ZoneListItemView), typeof(Button));
            var rt = go.GetComponent<RectTransform>();
            rt.sizeDelta = new Vector2(500, 48);
            go.GetComponent<Image>().color = new Color(0.14f, 0.16f, 0.2f, 0.95f);

            var nameText = CreateText(go.transform, "NameText", "区服名", font, 18,
                TextAnchor.MiddleLeft, new Vector2(12, 0), new Vector2(-280, 0));
            var statusText = CreateText(go.transform, "StatusText", "流畅", font, 16,
                TextAnchor.MiddleCenter, Vector2.zero, new Vector2(80, 0));
            SetAnchored(statusText.rectTransform, new Vector2(0.72f, 0.5f), new Vector2(80, 28));
            var onlineText = CreateText(go.transform, "OnlineText", string.Empty, font, 14,
                TextAnchor.MiddleRight, new Vector2(12, 0), new Vector2(-12, 0));

            var item = go.GetComponent<ZoneListItemView>();
            var so = new SerializedObject(item);
            so.FindProperty("_nameText").objectReferenceValue = nameText;
            so.FindProperty("_statusText").objectReferenceValue = statusText;
            so.FindProperty("_onlineText").objectReferenceValue = onlineText;
            so.FindProperty("_background").objectReferenceValue = go.GetComponent<Image>();
            so.FindProperty("_button").objectReferenceValue = go.GetComponent<Button>();
            so.ApplyModifiedPropertiesWithoutUndo();

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
            viewport.AddComponent<Mask>().showMaskGraphic = false;

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
            GameObject characterPanel, GameObject gameHudPanel, GameObject exitDialog,
            Text zoneNameText, Button selectServerBtn, Button enterGameBtn, InputField accountInput,
            InputField passwordInput, Toggle rememberToggle, Button loginBtn, Button gotoRegisterBtn,
            InputField regAccount, InputField regPassword, InputField regConfirm, Button registerBtn,
            Button backToLoginBtn, Text charListText, InputField createNameInput,
            Button enterWorldBtn, Button createCharBtn, Text statusText, Text errorText,
            Button exitReturnCharBtn, Button exitReturnLoginBtn, Button exitQuitBtn)
        {
            var so = new SerializedObject(ui);
            so.FindProperty("_zoneHomePanel").objectReferenceValue = zoneHomePanel;
            so.FindProperty("_serverListPanel").objectReferenceValue = serverListPanel;
            so.FindProperty("_serverList").objectReferenceValue = serverList;
            so.FindProperty("_authPanel").objectReferenceValue = authPanel;
            so.FindProperty("_registerPanel").objectReferenceValue = registerPanel;
            so.FindProperty("_characterPanel").objectReferenceValue = characterPanel;
            so.FindProperty("_gameHudPanel").objectReferenceValue = gameHudPanel;
            so.FindProperty("_exitDialog").objectReferenceValue = exitDialog;
            so.FindProperty("_zoneNameText").objectReferenceValue = zoneNameText;
            so.FindProperty("_selectServerBtn").objectReferenceValue = selectServerBtn;
            so.FindProperty("_enterGameBtn").objectReferenceValue = enterGameBtn;
            so.FindProperty("_accountInput").objectReferenceValue = accountInput;
            so.FindProperty("_passwordInput").objectReferenceValue = passwordInput;
            so.FindProperty("_rememberToggle").objectReferenceValue = rememberToggle;
            so.FindProperty("_loginBtn").objectReferenceValue = loginBtn;
            so.FindProperty("_gotoRegisterBtn").objectReferenceValue = gotoRegisterBtn;
            so.FindProperty("_regAccount").objectReferenceValue = regAccount;
            so.FindProperty("_regPassword").objectReferenceValue = regPassword;
            so.FindProperty("_regConfirm").objectReferenceValue = regConfirm;
            so.FindProperty("_registerBtn").objectReferenceValue = registerBtn;
            so.FindProperty("_backToLoginBtn").objectReferenceValue = backToLoginBtn;
            so.FindProperty("_charListText").objectReferenceValue = charListText;
            so.FindProperty("_createNameInput").objectReferenceValue = createNameInput;
            so.FindProperty("_enterWorldBtn").objectReferenceValue = enterWorldBtn;
            so.FindProperty("_createCharBtn").objectReferenceValue = createCharBtn;
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
            so.ApplyModifiedPropertiesWithoutUndo();
        }
    }
}

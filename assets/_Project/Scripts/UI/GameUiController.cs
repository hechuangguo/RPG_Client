/// <summary>
/// UI 流程控制器。
/// 职责：根据 AppState 显示/隐藏 Canvas 面板；绑定按钮事件。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.App;
using Rpg.Client.Log;
using Rpg.Client.Net;
using Rpg.Proto.Login;
using UnityEngine;
using UnityEngine.UI;

namespace Rpg.Client.UI
{
    public sealed class GameUiController : MonoBehaviour
    {
        [Header("Panels")]
        [SerializeField] private GameObject _zoneHomePanel;
        [SerializeField] private GameObject _serverListPanel;
        [SerializeField] private ServerListPanel _serverList;
        [SerializeField] private GameObject _authPanel;
        [SerializeField] private GameObject _registerPanel;
        [SerializeField] private GameObject _characterPanel;
        [SerializeField] private CharacterSelectPanel _characterSelect;
        [SerializeField] private GameObject _gameHudPanel;
        [SerializeField] private GameObject _exitDialog;

        [Header("Zone Home")]
        [SerializeField] private Text _zoneNameText;
        [SerializeField] private Button _selectServerBtn;
        [SerializeField] private Button _enterGameBtn;

        [Header("Auth")]
        [SerializeField] private InputField _accountInput;
        [SerializeField] private InputField _passwordInput;
        [SerializeField] private Toggle _showLoginPasswordToggle;
        [SerializeField] private Toggle _rememberToggle;
        [SerializeField] private Button _loginBtn;
        [SerializeField] private Button _gotoRegisterBtn;
        [SerializeField] private Button _authSelectServerBtn;

        [Header("Register")]
        [SerializeField] private InputField _regAccount;
        [SerializeField] private InputField _regPassword;
        [SerializeField] private InputField _regConfirm;
        [SerializeField] private Toggle _showRegisterPasswordToggle;
        [SerializeField] private Button _registerBtn;
        [SerializeField] private Button _backToLoginBtn;

        [Header("Exit Dialog")]
        [SerializeField] private Button _exitReturnCharBtn;
        [SerializeField] private Button _exitReturnLoginBtn;
        [SerializeField] private Button _exitQuitBtn;

        [Header("Common")]
        [SerializeField] private Text _statusText;
        [SerializeField] private Text _errorText;

        public Action OnSelectServerClicked;
        public Action OnCancelServerList;
        public Action OnGoToRegister;
        public Action OnBackToLogin;
        public Action<uint, byte, string> OnZoneConfirmed;
        public Action OnEnterGameFromHome;
        public Action<string, string, bool> OnLoginClicked;
        public Action<string, string, string> OnRegisterClicked;
        public Action<ulong> OnSelectCharacter;
        public Action<string, byte, byte> OnCreateCharacter;
        public Action<LogoutAction> OnExitGameAction;

        private bool _registerBusy;

        private void Awake()
        {
            ResolveServerList();
            ResolveCharacterSelect();
            EnsureAuthRegisterUiRefs();
            _selectServerBtn?.onClick.AddListener(() => OnSelectServerClicked?.Invoke());
            _enterGameBtn?.onClick.AddListener(() => OnEnterGameFromHome?.Invoke());
            _loginBtn?.onClick.AddListener(() =>
                OnLoginClicked?.Invoke(_accountInput.text, _passwordInput.text, _rememberToggle.isOn));
            _gotoRegisterBtn?.onClick.AddListener(() =>
            {
                ShowRegister(true);
                OnGoToRegister?.Invoke();
            });
            _authSelectServerBtn?.onClick.AddListener(() => OnSelectServerClicked?.Invoke());
            _showLoginPasswordToggle?.onValueChanged.AddListener(OnLoginPasswordVisibilityChanged);
            _showRegisterPasswordToggle?.onValueChanged.AddListener(OnRegisterPasswordVisibilityChanged);
            _registerBtn?.onClick.AddListener(() =>
                OnRegisterClicked?.Invoke(_regAccount.text, _regPassword.text, _regConfirm.text));
            _backToLoginBtn?.onClick.AddListener(() =>
            {
                ShowRegister(false);
                OnBackToLogin?.Invoke();
            });
            _serverList?.SetCallbacks(
                (zoneId, gameType, name) => OnZoneConfirmed?.Invoke(zoneId, gameType, name),
                () => OnCancelServerList?.Invoke());
            _characterSelect?.SetCallbacks(
                userId => OnSelectCharacter?.Invoke(userId),
                (name, voc, sex) => OnCreateCharacter?.Invoke(name, voc, sex));
            _exitReturnCharBtn?.onClick.AddListener(() =>
            {
                ShowExitDialog(false);
                OnExitGameAction?.Invoke(LogoutAction.ReturnCharSelect);
            });
            _exitReturnLoginBtn?.onClick.AddListener(() =>
            {
                ShowExitDialog(false);
                OnExitGameAction?.Invoke(LogoutAction.ReturnLogin);
            });
            _exitQuitBtn?.onClick.AddListener(() =>
            {
                ShowExitDialog(false);
                OnExitGameAction?.Invoke(LogoutAction.Unspecified);
            });
        }

        private void OnDestroy()
        {
            OnSelectServerClicked = null;
            OnCancelServerList = null;
            OnGoToRegister = null;
            OnBackToLogin = null;
            OnZoneConfirmed = null;
            OnEnterGameFromHome = null;
            OnLoginClicked = null;
            OnRegisterClicked = null;
            OnSelectCharacter = null;
            OnCreateCharacter = null;
            OnExitGameAction = null;
            _selectServerBtn?.onClick.RemoveAllListeners();
            _enterGameBtn?.onClick.RemoveAllListeners();
            _loginBtn?.onClick.RemoveAllListeners();
            _gotoRegisterBtn?.onClick.RemoveAllListeners();
            _authSelectServerBtn?.onClick.RemoveAllListeners();
            _showLoginPasswordToggle?.onValueChanged.RemoveAllListeners();
            _showRegisterPasswordToggle?.onValueChanged.RemoveAllListeners();
            _registerBtn?.onClick.RemoveAllListeners();
            _backToLoginBtn?.onClick.RemoveAllListeners();
            _exitReturnCharBtn?.onClick.RemoveAllListeners();
            _exitReturnLoginBtn?.onClick.RemoveAllListeners();
            _exitQuitBtn?.onClick.RemoveAllListeners();
        }

        public void SetAppState(AppState state, string zoneName, string lastAccount, bool remember)
        {
            _zoneHomePanel?.SetActive(state == AppState.ZoneHome);
            _serverListPanel?.SetActive(state == AppState.ServerList);
            _authPanel?.SetActive(state == AppState.AuthLogin || state == AppState.Connecting);
            _registerPanel?.SetActive(state == AppState.Register);
            _characterPanel?.SetActive(state == AppState.CharacterSelect);
            _gameHudPanel?.SetActive(state == AppState.Game);
            _exitDialog?.SetActive(false);

            if (_zoneNameText != null)
            {
                _zoneNameText.text = string.IsNullOrEmpty(zoneName) ? "未选择" : zoneName;
            }

            if (_enterGameBtn != null)
            {
                _enterGameBtn.interactable = !string.IsNullOrEmpty(zoneName) && zoneName != "未选择";
            }

            if (_accountInput != null && !string.IsNullOrEmpty(lastAccount))
            {
                _accountInput.text = lastAccount;
            }

            ApplyConnectingLock(state == AppState.Connecting);

            if (_rememberToggle != null)
            {
                _rememberToggle.isOn = remember;
            }
        }

        public void ShowServerList(List<GameZoneEntry> zones, uint selectedZoneId)
        {
            ResolveServerList();
            _serverListPanel?.SetActive(true);

            if (_serverList == null)
            {
                ClientLogger.Instance.Err("GameUiController：ServerListPanel 未绑定，无法显示区列表");
                ShowError("区列表 UI 未就绪，请执行 RPG → Setup Boot Scene");
                return;
            }

            ShowError(string.Empty);
            _serverList.ShowZones(zones, selectedZoneId);

            var hasSelectable = false;
            if (zones != null)
            {
                foreach (var z in zones)
                {
                    if (z.Enabled && z.LoadStatus != ZoneLoadStatus.Maintenance && z.GatewayCount > 0)
                    {
                        hasSelectable = true;
                        break;
                    }
                }
            }

            if (!hasSelectable)
            {
                ShowError("没有可用区服");
            }
        }

        public void ShowCharacterSelect(List<CharacterEntry> chars, ulong highlightUserId)
        {
            ResolveCharacterSelect();
            _characterPanel?.SetActive(true);
            if (_characterSelect == null)
            {
                ClientLogger.Instance.Err("GameUiController：CharacterSelectPanel 未绑定，无法显示角色列表");
                ShowError("选角 UI 未就绪，请执行 RPG → Setup Boot Scene");
                return;
            }

            _characterSelect.SetBusy(false);
            _characterSelect.ShowCharacters(chars, highlightUserId);
        }

        public void SetRegisterBusy(bool busy)
        {
            _registerBusy = busy;
            if (_registerBtn != null)
            {
                _registerBtn.interactable = !busy;
            }
        }

        public void SetCharacterBusy(bool busy)
        {
            _characterSelect?.SetBusy(busy);
        }

        public void ShowRegister(bool show)
        {
            _authPanel?.SetActive(!show);
            _registerPanel?.SetActive(show);
        }

        public void SetStatus(string msg)
        {
            if (_statusText != null)
            {
                _statusText.text = msg ?? string.Empty;
            }
        }

        public void ShowError(string msg)
        {
            if (_errorText != null)
            {
                _errorText.text = msg ?? string.Empty;
            }
        }

        public void ShowMessage(string msg) => SetStatus(msg);

        public void ShowExitDialog(bool show) => _exitDialog?.SetActive(show);

        public void SetServerListHint(string message) => _serverList?.SetHint(message ?? string.Empty);

        private void ResolveServerList()
        {
            if (_serverList != null)
            {
                return;
            }

            if (_serverListPanel != null)
            {
                _serverList = _serverListPanel.GetComponent<ServerListPanel>();
                if (_serverList == null)
                {
                    _serverList = _serverListPanel.GetComponentInChildren<ServerListPanel>(true);
                }
            }

            if (_serverList == null)
            {
                _serverList = GetComponentInChildren<ServerListPanel>(true);
            }
        }

        private void ResolveCharacterSelect()
        {
            if (_characterSelect != null)
            {
                _characterSelect.TryEnsureRuntimeLayout();
                return;
            }

            if (_characterPanel != null)
            {
                _characterSelect = _characterPanel.GetComponent<CharacterSelectPanel>();
                if (_characterSelect == null)
                {
                    _characterSelect = _characterPanel.AddComponent<CharacterSelectPanel>();
                }
            }

            if (_characterSelect == null)
            {
                _characterSelect = GetComponentInChildren<CharacterSelectPanel>(true);
            }

            _characterSelect?.TryEnsureRuntimeLayout();
        }

        /// <summary>场景未重跑 Setup Boot Scene 时，按名称查找或克隆已有控件补齐引用。</summary>
        private void EnsureAuthRegisterUiRefs()
        {
            if (_authPanel != null)
            {
                _showLoginPasswordToggle ??= FindDeepComponent<Toggle>(_authPanel.transform, "ShowLoginPasswordToggle");
                if (_showLoginPasswordToggle == null && _rememberToggle != null)
                {
                    _showLoginPasswordToggle = CloneToggle(_rememberToggle, "ShowLoginPasswordToggle", "显示密码", 0.45f);
                }

                _authSelectServerBtn ??= FindDeepComponent<Button>(_authPanel.transform, "AuthSelectServerBtn");
                if (_authSelectServerBtn == null && _gotoRegisterBtn != null)
                {
                    _authSelectServerBtn = CloneButton(_gotoRegisterBtn, "AuthSelectServerBtn", "选择服务器", 0.14f);
                }

                if (_rememberToggle != null)
                {
                    SetAnchoredY(_rememberToggle.GetComponent<RectTransform>(), 0.38f);
                }

                if (_loginBtn != null)
                {
                    SetAnchoredY(_loginBtn.GetComponent<RectTransform>(), 0.30f);
                }

                if (_gotoRegisterBtn != null)
                {
                    SetAnchoredY(_gotoRegisterBtn.GetComponent<RectTransform>(), 0.22f);
                }
            }

            if (_registerPanel != null)
            {
                _showRegisterPasswordToggle ??=
                    FindDeepComponent<Toggle>(_registerPanel.transform, "ShowRegisterPasswordToggle");
                if (_showRegisterPasswordToggle == null && _rememberToggle != null)
                {
                    _showRegisterPasswordToggle = CloneToggle(
                        _rememberToggle, _registerPanel.transform, "ShowRegisterPasswordToggle", "显示密码", 0.37f);
                }

                if (_registerBtn != null)
                {
                    SetAnchoredY(_registerBtn.GetComponent<RectTransform>(), 0.28f);
                }

                if (_backToLoginBtn != null)
                {
                    SetAnchoredY(_backToLoginBtn.GetComponent<RectTransform>(), 0.20f);
                }
            }
        }

        private static T FindDeepComponent<T>(Transform root, string name) where T : Component
        {
            if (root == null)
            {
                return null;
            }

            foreach (var child in root.GetComponentsInChildren<Transform>(true))
            {
                if (child.name != name)
                {
                    continue;
                }

                var component = child.GetComponent<T>();
                if (component != null)
                {
                    return component;
                }
            }

            return null;
        }

        private static Toggle CloneToggle(Toggle template, string objectName, string label, float anchorY) =>
            CloneToggle(template, template.transform.parent, objectName, label, anchorY);

        private static Toggle CloneToggle(
            Toggle template, Transform parent, string objectName, string label, float anchorY)
        {
            var clone = Instantiate(template, parent);
            clone.name = objectName;
            clone.isOn = false;
            SetToggleLabel(clone, label);
            SetAnchoredY(clone.GetComponent<RectTransform>(), anchorY);
            return clone;
        }

        private static Button CloneButton(Button template, string objectName, string label, float anchorY)
        {
            var clone = Instantiate(template, template.transform.parent);
            clone.name = objectName;
            SetButtonLabel(clone, label);
            SetAnchoredY(clone.GetComponent<RectTransform>(), anchorY);
            return clone;
        }

        private static void SetToggleLabel(Toggle toggle, string label)
        {
            var text = toggle.GetComponentInChildren<Text>(true);
            if (text != null)
            {
                text.text = label;
            }
        }

        private static void SetButtonLabel(Button button, string label)
        {
            var text = button.GetComponentInChildren<Text>(true);
            if (text != null)
            {
                text.text = label;
            }
        }

        private static void SetAnchoredY(RectTransform rect, float anchorY)
        {
            if (rect == null)
            {
                return;
            }

            rect.anchorMin = new Vector2(0.5f, anchorY);
            rect.anchorMax = new Vector2(0.5f, anchorY);
            rect.anchoredPosition = Vector2.zero;
        }

        private void ApplyConnectingLock(bool connecting)
        {
            if (_loginBtn != null)
            {
                _loginBtn.interactable = !connecting;
            }

            if (_authSelectServerBtn != null)
            {
                _authSelectServerBtn.interactable = !connecting;
            }

            if (_registerBtn != null)
            {
                _registerBtn.interactable = !connecting && !_registerBusy;
            }
        }

        private void OnLoginPasswordVisibilityChanged(bool visible) =>
            ApplyPasswordVisibility(_passwordInput, visible);

        private void OnRegisterPasswordVisibilityChanged(bool visible)
        {
            ApplyPasswordVisibility(_regPassword, visible);
            ApplyPasswordVisibility(_regConfirm, visible);
        }

        private static void ApplyPasswordVisibility(InputField field, bool visible)
        {
            if (field == null)
            {
                return;
            }

            field.contentType = visible
                ? InputField.ContentType.Standard
                : InputField.ContentType.Password;
            field.ForceLabelUpdate();
        }
    }
}

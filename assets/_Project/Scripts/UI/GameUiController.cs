/// <summary>
/// UI 流程控制器（对标 ui/* Panel 聚合）。
/// 职责：根据 AppState 显示/隐藏 Canvas 面板；绑定按钮事件。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.App;
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
        [SerializeField] private GameObject _authPanel;
        [SerializeField] private GameObject _registerPanel;
        [SerializeField] private GameObject _characterPanel;
        [SerializeField] private GameObject _gameHudPanel;
        [SerializeField] private GameObject _exitDialog;

        [Header("Zone Home")]
        [SerializeField] private Text _zoneNameText;
        [SerializeField] private Button _selectServerBtn;
        [SerializeField] private Button _enterGameBtn;

        [Header("Auth")]
        [SerializeField] private InputField _accountInput;
        [SerializeField] private InputField _passwordInput;
        [SerializeField] private Toggle _rememberToggle;
        [SerializeField] private Button _loginBtn;
        [SerializeField] private Button _gotoRegisterBtn;

        [Header("Register")]
        [SerializeField] private InputField _regAccount;
        [SerializeField] private InputField _regPassword;
        [SerializeField] private InputField _regConfirm;
        [SerializeField] private Button _registerBtn;
        [SerializeField] private Button _backToLoginBtn;

        [Header("Server List")]
        [SerializeField] private Button _cancelServerListBtn;

        [Header("Exit Dialog")]
        [SerializeField] private Button _exitReturnCharBtn;
        [SerializeField] private Button _exitReturnLoginBtn;
        [SerializeField] private Button _exitQuitBtn;

        [Header("Character")]
        [SerializeField] private Text _charListText;
        [SerializeField] private InputField _createNameInput;
        [SerializeField] private Button _enterWorldBtn;
        [SerializeField] private Button _createCharBtn;

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

        private ulong _selectedUserId;

        private void Awake()
        {
            _selectServerBtn?.onClick.AddListener(() => OnSelectServerClicked?.Invoke());
            _enterGameBtn?.onClick.AddListener(() => OnEnterGameFromHome?.Invoke());
            _loginBtn?.onClick.AddListener(() =>
                OnLoginClicked?.Invoke(_accountInput.text, _passwordInput.text, _rememberToggle.isOn));
            _gotoRegisterBtn?.onClick.AddListener(() =>
            {
                ShowRegister(true);
                OnGoToRegister?.Invoke();
            });
            _registerBtn?.onClick.AddListener(() =>
                OnRegisterClicked?.Invoke(_regAccount.text, _regPassword.text, _regConfirm.text));
            _backToLoginBtn?.onClick.AddListener(() =>
            {
                ShowRegister(false);
                OnBackToLogin?.Invoke();
            });
            _cancelServerListBtn?.onClick.AddListener(() => OnCancelServerList?.Invoke());
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
            _enterWorldBtn?.onClick.AddListener(() => OnSelectCharacter?.Invoke(_selectedUserId));
            _createCharBtn?.onClick.AddListener(() =>
                OnCreateCharacter?.Invoke(_createNameInput.text, CharacterDef.VocationWarrior, CharacterDef.SexMale));
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

            if (_loginBtn != null)
            {
                _loginBtn.interactable = state != AppState.Connecting;
            }

            if (_rememberToggle != null)
            {
                _rememberToggle.isOn = remember;
            }
        }

        public void ShowServerList(List<GameZoneEntry> zones, uint selectedZoneId)
        {
            // 简化：确认第一个可用区；完整 UI 可扩展为列表项 Prefab
            foreach (var z in zones)
            {
                if (z.Enabled)
                {
                    OnZoneConfirmed?.Invoke(z.ZoneId, z.GameType, z.Name);
                    return;
                }
            }

            ShowError("没有可用区服");
        }

        public void ShowCharacterSelect(List<CharacterEntry> chars, ulong highlightUserId)
        {
            _selectedUserId = highlightUserId != 0 ? highlightUserId : (chars.Count > 0 ? chars[0].UserId : 0);
            if (_charListText != null)
            {
                if (chars.Count == 0)
                {
                    _charListText.text = "暂无角色";
                }
                else
                {
                    _charListText.text = string.Join("\n", chars.ConvertAll(c => $"{c.Name} Lv{c.Level}"));
                }
            }
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
    }
}

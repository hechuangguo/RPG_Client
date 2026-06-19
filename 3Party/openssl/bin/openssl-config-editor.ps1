param(
    [string]$OpensslConf,
    [string]$FipsModule
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# === Path resolution ===
function Resolve-AnyPath {
    param([string]$path)
    
    if ([string]::IsNullOrWhiteSpace($path)) { return $path }
    try {
        if ([System.IO.Path]::IsPathRooted($path)) {
            return [System.IO.Path]::GetFullPath($path)
        }
        return [System.IO.Path]::GetFullPath((Join-Path (Get-Location).Path $path))
    } catch {
        return $path
    }
}

# Depends on:
# - $script:scriptDir
# - $script:opensslExe
function Find-DefaultOpensslConf {
    # 1. OPENSSL_CONF environment variable (respects user/system overrides)
    if ($env:OPENSSL_CONF) {
        $candidate = Resolve-AnyPath $env:OPENSSL_CONF
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }

    # 2. Relative: two levels up from script dir, then ssl\openssl.cnf
    #    (matches the original assumption about script placement)
    $candidate = Resolve-AnyPath (Join-Path $script:scriptDir '..\..\ssl\openssl.cnf')
    if (Test-Path -LiteralPath $candidate) { return $candidate }

    # 3. Fallback to baked in `OPENSSL_DIR`
    if (Test-Path -LiteralPath $script:opensslExe) {
        try {
            $output = & $script:opensslExe version -d 2>&1
            if ($LASTEXITCODE -eq 0) {
                $candidate = output.split('"')[1]
                if (Test-Path -LiteralPath $candidate) { return $candidate }
            }
        } catch {
        }
    }

    # 4. Known FireDaemon SSL 4 location under Common Files
    $commonFiles = $env:CommonProgramFiles   # Handles both 32/64-bit correctly
    if ($commonFiles) {
        return Resolve-AnyPath (Join-Path $commonFiles 'FireDaemon SSL 4\openssl.cnf')
    }

    # 5. Absolute last resort: return the relative path (will not exist)
    return Resolve-AnyPath (Join-Path $scriptDir '..\..\ssl\openssl.cnf')
}

$scriptDir = if ($PSCommandPath) { Split-Path -Parent $PSCommandPath } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$script:opensslExe = Resolve-AnyPath (Join-Path $scriptDir 'openssl.exe')

# unused, just for completeness. Path is determined by Find-DefaultOpensslConf
$defaultOpensslConf = Resolve-AnyPath (Join-Path $scriptDir '..\..\ssl\openssl.cnf')
$defaultFipsModule  = Resolve-AnyPath (Join-Path $scriptDir '..\lib\ossl-modules\fips.dll')

$script:opensslConfFromCli = -not [string]::IsNullOrWhiteSpace($OpensslConf)
$script:fipsModuleFromCli  = -not [string]::IsNullOrWhiteSpace($FipsModule)

if ($script:opensslConfFromCli) {
    $initialOpensslConfPath = Resolve-AnyPath $OpensslConf
} else {
    $initialOpensslConfPath = Find-DefaultOpensslConf
}

$fipsModuleAutoFound = Test-Path -LiteralPath $defaultFipsModule
if ($script:fipsModuleFromCli) {
    $initialFipsDllPath = Resolve-AnyPath $FipsModule
} elseif ($fipsModuleAutoFound) {
    $initialFipsDllPath = $defaultFipsModule
} else {
    $initialFipsDllPath = ''
}

$script:opensslConfLocked = $script:opensslConfFromCli
$script:fipsDllLocked     = $script:fipsModuleFromCli -or ($script:opensslConfFromCli -and $fipsModuleAutoFound)

$script:suppressRegen = $false

# === Form ===
$form = New-Object System.Windows.Forms.Form
$form.Text = "OpenSSL Configuration Editor"
$form.Size = New-Object System.Drawing.Size(900,680)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false

# === Tab Control ===
$tabControl = New-Object System.Windows.Forms.TabControl
$tabControl.Location = New-Object System.Drawing.Point(10,10)
$tabControl.Size = New-Object System.Drawing.Size(860,590)

# ==================== TAB 1: FIPS Configuration ====================
$tabFips = New-Object System.Windows.Forms.TabPage
$tabFips.Text = "FIPS Configuration"

# --- Files group ---
$groupFiles = New-Object System.Windows.Forms.GroupBox
$groupFiles.Text = "Files"
$groupFiles.Location = New-Object System.Drawing.Point(10,10)
$groupFiles.Size = New-Object System.Drawing.Size(820,90)

$lblOpensslConf = New-Object System.Windows.Forms.Label
$lblOpensslConf.Text = "OpenSSL Config (openssl.cnf):"
$lblOpensslConf.Location = New-Object System.Drawing.Point(20,25)
$lblOpensslConf.Size = New-Object System.Drawing.Size(200,20)

$txtOpensslConfPath = New-Object System.Windows.Forms.TextBox
$txtOpensslConfPath.Location = New-Object System.Drawing.Point(220,23)
$txtOpensslConfPath.Size = New-Object System.Drawing.Size(460,20)
$txtOpensslConfPath.Text = $initialOpensslConfPath
if ($script:opensslConfLocked) {
    $txtOpensslConfPath.ReadOnly = $true
    $txtOpensslConfPath.BackColor = [System.Drawing.SystemColors]::Control
}

$btnBrowseOpensslConf = New-Object System.Windows.Forms.Button
$btnBrowseOpensslConf.Text = "Browse..."
$btnBrowseOpensslConf.Location = New-Object System.Drawing.Point(690,22)
$btnBrowseOpensslConf.Size = New-Object System.Drawing.Size(100,25)
$btnBrowseOpensslConf.Add_Click({ Browse-OpensslConf })
$btnBrowseOpensslConf.Visible = -not $script:opensslConfLocked

$lblFipsDll = New-Object System.Windows.Forms.Label
$lblFipsDll.Text = "FIPS Provider DLL (fips.dll):"
$lblFipsDll.Location = New-Object System.Drawing.Point(20,55)
$lblFipsDll.Size = New-Object System.Drawing.Size(200,20)

$txtFipsDllPath = New-Object System.Windows.Forms.TextBox
$txtFipsDllPath.Location = New-Object System.Drawing.Point(220,53)
$txtFipsDllPath.Size = New-Object System.Drawing.Size(460,20)
$txtFipsDllPath.Text = $initialFipsDllPath
if ($script:fipsDllLocked) {
    $txtFipsDllPath.ReadOnly = $true
    $txtFipsDllPath.BackColor = [System.Drawing.SystemColors]::Control
}

$btnBrowseFipsDll = New-Object System.Windows.Forms.Button
$btnBrowseFipsDll.Text = "Browse..."
$btnBrowseFipsDll.Location = New-Object System.Drawing.Point(690,52)
$btnBrowseFipsDll.Size = New-Object System.Drawing.Size(100,25)
$btnBrowseFipsDll.Add_Click({ Browse-FipsDll })
$btnBrowseFipsDll.Visible = -not $script:fipsDllLocked

$groupFiles.Controls.AddRange(@($lblOpensslConf, $txtOpensslConfPath, $btnBrowseOpensslConf, $lblFipsDll, $txtFipsDllPath, $btnBrowseFipsDll))

# --- FIPS Mode group ---
$groupFipsMode = New-Object System.Windows.Forms.GroupBox
$groupFipsMode.Text = "FIPS Mode"
$groupFipsMode.Location = New-Object System.Drawing.Point(10,110)
$groupFipsMode.Size = New-Object System.Drawing.Size(820,100)

$radioStrictFips = New-Object System.Windows.Forms.RadioButton
$radioStrictFips.Text = "Strict FIPS Mode (FIPS + Base providers only)"
$radioStrictFips.Location = New-Object System.Drawing.Point(20,25)
$radioStrictFips.Size = New-Object System.Drawing.Size(500,20)

$radioFlexible = New-Object System.Windows.Forms.RadioButton
$radioFlexible.Text = "Flexible Mode (FIPS + Base + Default providers)"
$radioFlexible.Location = New-Object System.Drawing.Point(20,50)
$radioFlexible.Size = New-Object System.Drawing.Size(500,20)

$radioNormal = New-Object System.Windows.Forms.RadioButton
$radioNormal.Text = "Normal Mode (Default provider only)"
$radioNormal.Location = New-Object System.Drawing.Point(20,75)
$radioNormal.Size = New-Object System.Drawing.Size(500,20)
$radioNormal.Checked = $true

$radioHandler = {
    Update-CheckboxDisplay
    if ($script:suppressRegen) { return }
    Regenerate-Preview
}
$radioStrictFips.Add_CheckedChanged($radioHandler)
$radioFlexible.Add_CheckedChanged($radioHandler)
$radioNormal.Add_CheckedChanged($radioHandler)

$groupFipsMode.Controls.AddRange(@($radioStrictFips, $radioFlexible, $radioNormal))

# --- Active Providers group (informational) ---
$groupProviders = New-Object System.Windows.Forms.GroupBox
$groupProviders.Text = "Active Providers (informational)"
$groupProviders.Location = New-Object System.Drawing.Point(10,220)
$groupProviders.Size = New-Object System.Drawing.Size(820,100)

$chkFipsProvider = New-Object System.Windows.Forms.CheckBox
$chkFipsProvider.Text = "FIPS Provider (FIPS-approved algorithms only)"
$chkFipsProvider.Location = New-Object System.Drawing.Point(20,25)
$chkFipsProvider.Size = New-Object System.Drawing.Size(400,20)
$chkFipsProvider.Enabled = $false
$chkFipsProvider.AutoCheck = $false

$chkBaseProvider = New-Object System.Windows.Forms.CheckBox
$chkBaseProvider.Text = "Base Provider (Encoding/decoding operations)"
$chkBaseProvider.Location = New-Object System.Drawing.Point(20,50)
$chkBaseProvider.Size = New-Object System.Drawing.Size(400,20)
$chkBaseProvider.Enabled = $false
$chkBaseProvider.AutoCheck = $false

$chkDefaultProvider = New-Object System.Windows.Forms.CheckBox
$chkDefaultProvider.Text = "Default Provider (All standard algorithms)"
$chkDefaultProvider.Location = New-Object System.Drawing.Point(20,75)
$chkDefaultProvider.Size = New-Object System.Drawing.Size(400,20)
$chkDefaultProvider.Checked = $true
$chkDefaultProvider.Enabled = $false
$chkDefaultProvider.AutoCheck = $false

$groupProviders.Controls.AddRange(@($chkFipsProvider, $chkBaseProvider, $chkDefaultProvider))

# --- Status ---
$lblStatus = New-Object System.Windows.Forms.Label
$lblStatus.Text = "Status: No configuration loaded"
$lblStatus.Location = New-Object System.Drawing.Point(10,335)
$lblStatus.Size = New-Object System.Drawing.Size(820,150)
$lblStatus.BorderStyle = "FixedSingle"
$lblStatus.BackColor = [System.Drawing.Color]::LightYellow
$lblStatus.Padding = New-Object System.Windows.Forms.Padding(5)

$tabFips.Controls.AddRange(@($groupFiles, $groupFipsMode, $groupProviders, $lblStatus))

# ==================== TAB 2: Advanced Editor ====================
$tabAdvanced = New-Object System.Windows.Forms.TabPage
$tabAdvanced.Text = "Advanced Editor"

$lblAdvanced = New-Object System.Windows.Forms.Label
$lblAdvanced.Text = "Direct configuration file editing (saved as-is on Save):"
$lblAdvanced.Location = New-Object System.Drawing.Point(10,10)
$lblAdvanced.Size = New-Object System.Drawing.Size(500,20)

$txtConfigEditor = New-Object System.Windows.Forms.TextBox
$txtConfigEditor.Multiline = $true
$txtConfigEditor.ScrollBars = "Both"
$txtConfigEditor.WordWrap = $false
$txtConfigEditor.AcceptsReturn = $true
$txtConfigEditor.AcceptsTab = $true
$txtConfigEditor.Location = New-Object System.Drawing.Point(10,35)
$txtConfigEditor.Size = New-Object System.Drawing.Size(830,520)
$txtConfigEditor.Font = New-Object System.Drawing.Font("Consolas",10)

$tabAdvanced.Controls.AddRange(@($lblAdvanced, $txtConfigEditor))

# ==================== TAB 3: Help ====================
$tabHelp = New-Object System.Windows.Forms.TabPage
$tabHelp.Text = "Help"

$txtHelp = New-Object System.Windows.Forms.RichTextBox
$txtHelp.Location = New-Object System.Drawing.Point(10,10)
$txtHelp.Size = New-Object System.Drawing.Size(830,545)
$txtHelp.ReadOnly = $true
$txtHelp.Font = New-Object System.Drawing.Font("Segoe UI",10)
$txtHelp.Text = @"
OpenSSL Configuration Editor - Help

USAGE:

1. The OpenSSL Config and FIPS Provider DLL paths are pre-filled
   from the script's installation. Adjust if needed (when running
   the script standalone, outside a deployed configuration).

2. Choose a FIPS mode.

3. Click Save. This will:
   - For Strict / Flexible modes: regenerate fipsmodule.cnf alongside
     openssl.cnf using 'openssl fipsinstall'.
   - Write the updated openssl.cnf to disk.

The original openssl.cnf is preserved verbatim except for the
provider-related sections, which are stripped and replaced based on
the chosen mode. Manual edits in the Advanced Editor tab are honored
on Save (they are saved as-is).

FIPS MODES:

Strict FIPS Mode:
- Only FIPS-approved cryptographic algorithms are available
- Activates: FIPS provider + Base provider
- MD5, RC4, and other non-approved algorithms are blocked
- Use this for FIPS 140-3 compliance

Flexible Mode:
- Both FIPS and standard algorithms available
- Activates: FIPS provider + Base provider + Default provider
- Applications can choose which provider to use
- Not strictly FIPS compliant
- Note: Default provider must be explicitly activated when any other
  provider is activated, otherwise applications depending on OpenSSL
  may not work correctly.

Normal Mode:
- Standard OpenSSL operation
- Activates: Default provider only
- All algorithms available
- No FIPS restrictions

PROVIDERS:

FIPS Provider:
- Contains only FIPS 140-3 validated cryptographic algorithms
- Examples: AES, SHA-256, RSA, ECDSA, HMAC
- Excludes: MD5, RC4, DES, and other non-approved algorithms

Base Provider:
- Non-cryptographic support operations
- Examples: PEM/DER encoding, certificate parsing, Base64
- Required for FIPS mode to handle data formats

Default Provider:
- All standard OpenSSL algorithms (FIPS + legacy)
- Includes weak/deprecated algorithms for compatibility

VALIDATION:

Test your configuration with:
  openssl list -providers

For strict FIPS mode, MD5 should fail:
  openssl dgst -md5 test.txt

FIPS-approved algorithms should work:
  openssl sha256 test.txt
"@

$tabHelp.Controls.Add($txtHelp)

# Add tabs
$tabControl.TabPages.AddRange(@($tabFips, $tabAdvanced, $tabHelp))
$form.Controls.Add($tabControl)

# === Bottom buttons ===
$btnSave = New-Object System.Windows.Forms.Button
$btnSave.Text = "Save"
$btnSave.Location = New-Object System.Drawing.Point(700,610)
$btnSave.Size = New-Object System.Drawing.Size(80,30)
$btnSave.Add_Click({ Save-AllConfig })

$btnClose = New-Object System.Windows.Forms.Button
$btnClose.Text = "Close"
$btnClose.Location = New-Object System.Drawing.Point(790,610)
$btnClose.Size = New-Object System.Drawing.Size(80,30)
$btnClose.Add_Click({ $form.Close() })

$form.Controls.AddRange(@($btnSave, $btnClose))

# ============================================================
# Functions
# ============================================================

function Get-CurrentMode {
    if ($radioStrictFips.Checked) { return 'strict' }
    if ($radioFlexible.Checked)   { return 'flexible' }
    return 'normal'
}

function Get-FipsmoduleCnfPath {
    $opensslConfPath = $txtOpensslConfPath.Text
    if ([string]::IsNullOrWhiteSpace($opensslConfPath)) { return $null }
    $dir = Split-Path -Parent $opensslConfPath
    if (-not $dir) { $dir = '.' }
    return Join-Path $dir 'fipsmodule.cnf'
}

function Update-CheckboxDisplay {
    if ($radioStrictFips.Checked) {
        $chkFipsProvider.Checked    = $true
        $chkBaseProvider.Checked    = $true
        $chkDefaultProvider.Checked = $false
    } elseif ($radioFlexible.Checked) {
        $chkFipsProvider.Checked    = $true
        $chkBaseProvider.Checked    = $true
        $chkDefaultProvider.Checked = $true
    } else {
        $chkFipsProvider.Checked    = $false
        $chkBaseProvider.Checked    = $false
        $chkDefaultProvider.Checked = $true
    }
}

function Browse-OpensslConf {
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Filter = "Config files (*.cnf)|*.cnf|All files (*.*)|*.*"
    $dialog.Title = "Select openssl.cnf"
    $dialog.FileName = "openssl.cnf"
    if ($dialog.ShowDialog() -eq "OK") {
        $txtOpensslConfPath.Text = $dialog.FileName
        Load-Config $dialog.FileName
    }
}

function Browse-FipsDll {
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Filter = "DLL files (*.dll)|*.dll|All files (*.*)|*.*"
    $dialog.Title = "Select fips.dll"
    $dialog.FileName = "fips.dll"
    if ($dialog.ShowDialog() -eq "OK") {
        $txtFipsDllPath.Text = $dialog.FileName
    }
}

function Load-Config {
    param([string]$path)

    if (-not (Test-Path -LiteralPath $path)) {
        $script:suppressRegen = $true
        $txtConfigEditor.Text = ''
        $radioNormal.Checked = $true
        $script:suppressRegen = $false
        Update-CheckboxDisplay
        $lblStatus.Text = "Status: $path does not exist (will be created on Save)"
        $lblStatus.BackColor = [System.Drawing.Color]::LightYellow
        return
    }

    try {
        $content = [System.IO.File]::ReadAllText($path)
        $content = $content -replace "`r`n", "`n" -replace "`n", "`r`n"

        $script:suppressRegen = $true
        $txtConfigEditor.Text = $content
        $mode = Parse-Mode $content
        switch ($mode) {
            'strict'   { $radioStrictFips.Checked = $true }
            'flexible' { $radioFlexible.Checked   = $true }
            default    { $radioNormal.Checked     = $true }
        }
        $script:suppressRegen = $false
        Update-CheckboxDisplay

        $lblStatus.Text = "Status: Loaded $path (mode detected: $mode)"
        $lblStatus.BackColor = [System.Drawing.Color]::LightGreen
    } catch {
        $lblStatus.Text = "Status: Error loading file: $_"
        $lblStatus.BackColor = [System.Drawing.Color]::LightPink
    }
}

function Regenerate-Preview {
    $current = $txtConfigEditor.Text
    $mode = Get-CurrentMode
    $fipsmoduleCnfPath = Get-FipsmoduleCnfPath
    $newContent = Regenerate-FipsBlock $current $mode $fipsmoduleCnfPath
    $txtConfigEditor.Text = $newContent
}

function Parse-Mode {
    param([string]$content)

    $lines = $content -split "`r?`n"

    # FIPS is active when openssl_conf points to an init section that lists fips
    # in its providers section. The .include directive is optional (the FIPS
    # sections may be inlined in openssl.cnf rather than pulled from fipsmodule.cnf).

    # Find init section name
    $initSection = $null
    $currentSection = $null
    foreach ($line in $lines) {
        if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
        if ($null -eq $currentSection -and $line -match '^\s*openssl_conf\s*=\s*(\S+)') {
            $initSection = $matches[1]
        }
    }
    if (-not $initSection) { return 'normal' }

    # Find providers section name
    $providersSection = $null
    $currentSection = $null
    foreach ($line in $lines) {
        if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
        if ($currentSection -eq $initSection -and $line -match '^\s*providers\s*=\s*(\S+)') {
            $providersSection = $matches[1]
        }
    }
    if (-not $providersSection) { return 'normal' }

    # Inspect [providersSection] for fips and default activation
    $hasFips = $false
    $hasDefault = $false
    $currentSection = $null
    foreach ($line in $lines) {
        if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
        if ($currentSection -eq $providersSection) {
            if ($line -match '^\s*fips\s*=')    { $hasFips = $true }
            if ($line -match '^\s*default\s*=') { $hasDefault = $true }
        }
    }

    if ($hasFips -and $hasDefault) { return 'flexible' }
    if ($hasFips)                  { return 'strict' }
    return 'normal'
}

function Strip-FipsBlock {
    param([string]$content)

    $lines = $content -split "`r?`n"

    # Phase 1: discover section names
    $initSection = $null
    $providersSection = $null
    $providerSubSections = @()

    $currentSection = $null
    foreach ($line in $lines) {
        if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
        if ($null -eq $currentSection -and $line -match '^\s*openssl_conf\s*=\s*(\S+)') {
            $initSection = $matches[1]
        }
    }

    if ($initSection) {
        $currentSection = $null
        foreach ($line in $lines) {
            if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
            if ($currentSection -eq $initSection -and $line -match '^\s*providers\s*=\s*(\S+)') {
                $providersSection = $matches[1]
            }
        }
    }

    if ($providersSection) {
        $currentSection = $null
        foreach ($line in $lines) {
            if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $currentSection = $matches[1]; continue }
            if ($currentSection -eq $providersSection) {
                if ($line -match '^\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*(\S+)') {
                    $sub = $matches[1]
                    if ($sub -and ($providerSubSections -notcontains $sub)) {
                        $providerSubSections += $sub
                    }
                }
            }
        }
    }

    $sectionsToStrip = @()
    if ($initSection)      { $sectionsToStrip += $initSection }
    if ($providersSection) { $sectionsToStrip += $providersSection }
    foreach ($sub in $providerSubSections) { $sectionsToStrip += $sub }

    # Phase 2: emit, dropping FIPS lines
    $out = @()
    $currentSection = $null
    $skipping = $false
    foreach ($line in $lines) {
        if ($line -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') {
            $currentSection = $matches[1]
            $skipping = $sectionsToStrip -contains $currentSection
            if (-not $skipping) { $out += $line }
            continue
        }
        if ($skipping) { continue }
        # Drop fipsmodule.cnf .include lines wherever they appear (uncommented or stock-commented).
        # Path uses .* (not \S*) so paths with spaces like "C:\Program Files\..." are matched.
        if ($line -match '^\s*#?\s*\.include\s+.*[Ff]ipsmodule\.cnf\b') { continue }
        # openssl_conf is only meaningful in the default section, so only strip there.
        if ($null -eq $currentSection -and $line -match '^\s*#?\s*openssl_conf\s*=') { continue }
        $out += $line
    }

    # Trim trailing empty lines (find first non-blank from end, then slice once)
    $lastNonBlank = $out.Count
    while ($lastNonBlank -gt 0 -and [string]::IsNullOrWhiteSpace($out[$lastNonBlank - 1])) {
        $lastNonBlank--
    }
    if ($lastNonBlank -eq 0) {
        $out = @()
    } elseif ($lastNonBlank -lt $out.Count) {
        $out = @($out[0..($lastNonBlank - 1)])
    }

    return ($out -join "`r`n")
}

function Regenerate-FipsBlock {
    param(
        [string]$content,
        [string]$mode,
        [string]$fipsmoduleCnfPath
    )

    $stripped = Strip-FipsBlock $content
    $lines = @($stripped -split "`r?`n")

    # Build top-level directives (go into default section, before first [name])
    $topDirectives = @("openssl_conf = openssl_init")
    if ($mode -eq 'strict' -or $mode -eq 'flexible') {
        if (-not [string]::IsNullOrWhiteSpace($fipsmoduleCnfPath)) {
            $fipsCnfFwd = $fipsmoduleCnfPath -replace '\\', '/'
            $topDirectives += ".include $fipsCnfFwd"
        }
    }

    # Find first [section] header
    $firstSectionIdx = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*\[\s*([^\]]+?)\s*\]\s*$') { $firstSectionIdx = $i; break }
    }

    if ($firstSectionIdx -eq -1) {
        # No named sections in source; directives followed by named sections
        if ($lines.Count -gt 0 -and -not [string]::IsNullOrWhiteSpace($lines[$lines.Count - 1])) {
            $lines = $lines + @('')
        }
        $lines = $lines + $topDirectives
    } elseif ($firstSectionIdx -eq 0) {
        # File starts with a section; prepend directives
        $lines = $topDirectives + @('') + $lines
    } else {
        $before = @($lines[0..($firstSectionIdx - 1)])
        $after  = @($lines[$firstSectionIdx..($lines.Count - 1)])
        # Strip trailing blanks from $before so we don't double-up
        $lastNonBlank = $before.Count
        while ($lastNonBlank -gt 0 -and [string]::IsNullOrWhiteSpace($before[$lastNonBlank - 1])) {
            $lastNonBlank--
        }
        if ($lastNonBlank -eq 0) {
            $before = @()
        } elseif ($lastNonBlank -lt $before.Count) {
            $before = @($before[0..($lastNonBlank - 1)])
        }
        $lines = $before + @('') + $topDirectives + @('') + $after
    }

    # Build named sections at EOF
    $named = @()
    $named += ''
    $named += '[openssl_init]'
    $named += 'providers = provider_sect'
    $named += ''
    if ($mode -eq 'strict') {
        $named += '[provider_sect]'
        $named += 'fips = fips_sect'
        $named += 'base = base_sect'
        $named += ''
        $named += '[fips_sect]'
        $named += 'activate = 1'
        $named += ''
        $named += '[base_sect]'
        $named += 'activate = 1'
    } elseif ($mode -eq 'flexible') {
        $named += '[provider_sect]'
        $named += 'fips = fips_sect'
        $named += 'base = base_sect'
        $named += 'default = default_sect'
        $named += ''
        $named += '[fips_sect]'
        $named += 'activate = 1'
        $named += ''
        $named += '[base_sect]'
        $named += 'activate = 1'
        $named += ''
        $named += '[default_sect]'
        $named += 'activate = 1'
    } else {
        $named += '[provider_sect]'
        $named += 'default = default_sect'
        $named += ''
        $named += '[default_sect]'
        $named += 'activate = 1'
    }
    $named += ''

    return ($lines + $named) -join "`r`n"
}

function Save-AllConfig {
    $opensslConfPath = $txtOpensslConfPath.Text
    if ([string]::IsNullOrWhiteSpace($opensslConfPath)) {
        [System.Windows.Forms.MessageBox]::Show("Please specify the openssl.cnf path", "Error", "OK", "Error") | Out-Null
        return
    }

    $mode = Get-CurrentMode
    $needsFips = ($mode -ne 'normal')
    $fipsmoduleCnfPath = $null

    if ($needsFips) {
        $fipsDllPath = $txtFipsDllPath.Text
        if ([string]::IsNullOrWhiteSpace($fipsDllPath)) {
            [System.Windows.Forms.MessageBox]::Show("Please specify the fips.dll path for FIPS modes", "Error", "OK", "Error") | Out-Null
            return
        }
        if (-not (Test-Path -LiteralPath $fipsDllPath)) {
            [System.Windows.Forms.MessageBox]::Show("FIPS DLL not found:`r`n$fipsDllPath", "Error", "OK", "Error") | Out-Null
            return
        }
        if (-not (Test-Path -LiteralPath $script:opensslExe)) {
            [System.Windows.Forms.MessageBox]::Show("openssl.exe not found at:`r`n$($script:opensslExe)`r`n`r`nNeeded to generate fipsmodule.cnf.", "Error", "OK", "Error") | Out-Null
            return
        }

        $fipsmoduleCnfPath = Get-FipsmoduleCnfPath
        try {
            $output = & $script:opensslExe fipsinstall -out $fipsmoduleCnfPath -module $fipsDllPath 2>&1
            if ($LASTEXITCODE -ne 0) {
                [System.Windows.Forms.MessageBox]::Show("openssl fipsinstall failed (exit $LASTEXITCODE):`r`n`r`n$output", "Error", "OK", "Error") | Out-Null
                return
            }
        } catch {
            [System.Windows.Forms.MessageBox]::Show("Error running openssl fipsinstall:`r`n$_", "Error", "OK", "Error") | Out-Null
            return
        }
    }

    try {
        $content = $txtConfigEditor.Text -replace "`r`n", "`n" -replace "`n", "`r`n"
        $utf8NoBom = New-Object System.Text.UTF8Encoding $false
        [System.IO.File]::WriteAllText($opensslConfPath, $content, $utf8NoBom)

        if ($needsFips) {
            $lblStatus.Text = "Status: Saved $opensslConfPath`r`n        and $fipsmoduleCnfPath"
        } else {
            $lblStatus.Text = "Status: Saved $opensslConfPath"
        }
        $lblStatus.BackColor = [System.Drawing.Color]::LightGreen
    } catch {
        [System.Windows.Forms.MessageBox]::Show("Error saving openssl.cnf:`r`n$_", "Error", "OK", "Error") | Out-Null
    }
}

# === Initial load ===
if (Test-Path -LiteralPath $initialOpensslConfPath) {
    Load-Config $initialOpensslConfPath
} else {
    $script:suppressRegen = $true
    $radioNormal.Checked = $true
    $script:suppressRegen = $false
    Update-CheckboxDisplay
    $lblStatus.Text = "Status: $initialOpensslConfPath does not exist (will be created on Save)"
    $lblStatus.BackColor = [System.Drawing.Color]::LightYellow
}

[void]$form.ShowDialog()

# SIG # Begin signature block
# MIIojQYJKoZIhvcNAQcCoIIofjCCKHoCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCC/vr1wNz/1k14P
# F1Kt1s3CV9fOXRQewJ5k+SHIsXuQxKCCDPYwggYcMIIEBKADAgECAhAz1wiokUBT
# GeKlu9M5ua1uMA0GCSqGSIb3DQEBDAUAMFYxCzAJBgNVBAYTAkdCMRgwFgYDVQQK
# Ew9TZWN0aWdvIExpbWl0ZWQxLTArBgNVBAMTJFNlY3RpZ28gUHVibGljIENvZGUg
# U2lnbmluZyBSb290IFI0NjAeFw0yMTAzMjIwMDAwMDBaFw0zNjAzMjEyMzU5NTla
# MFcxCzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxLjAsBgNV
# BAMTJVNlY3RpZ28gUHVibGljIENvZGUgU2lnbmluZyBDQSBFViBSMzYwggGiMA0G
# CSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQC70f4et0JbePWQp64sg/GNIdMwhoV7
# 39PN2RZLrIXFuwHP4owoEXIEdiyBxasSekBKxRDogRQ5G19PB/YwMDB/NSXlwHM9
# QAmU6Kj46zkLVdW2DIseJ/jePiLBv+9l7nPuZd0o3bsffZsyf7eZVReqskmoPBBq
# OsMhspmoQ9c7gqgZYbU+alpduLyeE9AKnvVbj2k4aOqlH1vKI+4L7bzQHkNDbrBT
# jMJzKkQxbr6PuMYC9ruCBBV5DFIg6JgncWHvL+T4AvszWbX0w1Xn3/YIIq620QlZ
# 7AGfc4m3Q0/V8tm9VlkJ3bcX9sR0gLqHRqwG29sEDdVOuu6MCTQZlRvmcBMEJd+P
# uNeEM4xspgzraLqVT3xE6NRpjSV5wyHxNXf4T7YSVZXQVugYAtXueciGoWnxG06U
# E2oHYvDQa5mll1CeHDOhHu5hiwVoHI717iaQg9b+cYWnmvINFD42tRKtd3V6zOdG
# NmqQU8vGlHHeBzoh+dYyZ+CcblSGoGSgg8sCAwEAAaOCAWMwggFfMB8GA1UdIwQY
# MBaAFDLrkpr/NZZILyhAQnAgNpFcF4XmMB0GA1UdDgQWBBSBMpJBKyjNRsjEosYq
# ORLsSKk/FDAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADATBgNV
# HSUEDDAKBggrBgEFBQcDAzAaBgNVHSAEEzARMAYGBFUdIAAwBwYFZ4EMAQMwSwYD
# VR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybC5zZWN0aWdvLmNvbS9TZWN0aWdvUHVi
# bGljQ29kZVNpZ25pbmdSb290UjQ2LmNybDB7BggrBgEFBQcBAQRvMG0wRgYIKwYB
# BQUHMAKGOmh0dHA6Ly9jcnQuc2VjdGlnby5jb20vU2VjdGlnb1B1YmxpY0NvZGVT
# aWduaW5nUm9vdFI0Ni5wN2MwIwYIKwYBBQUHMAGGF2h0dHA6Ly9vY3NwLnNlY3Rp
# Z28uY29tMA0GCSqGSIb3DQEBDAUAA4ICAQBfNqz7+fZyWhS38Asd3tj9lwHS/QHu
# mS2G6Pa38Dn/1oFKWqdCSgotFZ3mlP3FaUqy10vxFhJM9r6QZmWLLXTUqwj3ahED
# CHd8vmnhsNufJIkD1t5cpOCy1rTP4zjVuW3MJ9bOZBHoEHJ20/ng6SyJ6UnTs5eW
# Bgrh9grIQZqRXYHYNneYyoBBl6j4kT9jn6rNVFRLgOr1F2bTlHH9nv1HMePpGoYd
# 074g0j+xUl+yk72MlQmYco+VAfSYQ6VK+xQmqp02v3Kw/Ny9hA3s7TSoXpUrOBZj
# BXXZ9jEuFWvilLIq0nQ1tZiao/74Ky+2F0snbFrmuXZe2obdq2TWauqDGIgbMYL1
# iLOUJcAhLwhpAuNMu0wqETDrgXkG4UGVKtQg9guT5Hx2DJ0dJmtfhAH2KpnNr97H
# 8OQYok6bLyoMZqaSdSa+2UA1E2+upjcaeuitHFFjBypWBmztfhj24+xkc6ZtCDaL
# rw+ZrnVrFyvCTWrDUUZBVumPwo3/E3Gb2u2e05+r5UWmEsUUWlJBl6MGAAjF5hzq
# J4I8O9vmRsTvLQA1E802fZ3lqicIBczOwDYOSxlP0GOabb/FKVMxItt1UHeG0PL4
# au5rBhs+hSMrl8h+eplBDN1Yfw6owxI9OjWb4J0sjBeBVESoeh2YnZZ/WVimVGX/
# UUIL+Efrz/jlvzCCBtIwggU6oAMCAQICEHKqHTZpwQphfhLjhuxi9/cwDQYJKoZI
# hvcNAQELBQAwVzELMAkGA1UEBhMCR0IxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRl
# ZDEuMCwGA1UEAxMlU2VjdGlnbyBQdWJsaWMgQ29kZSBTaWduaW5nIENBIEVWIFIz
# NjAeFw0yNDA0MDMwMDAwMDBaFw0yNzA0MDMyMzU5NTlaMIG6MREwDwYDVQQFEwgw
# NTkwMTYwMTETMBEGCysGAQQBgjc8AgEDEwJHQjEdMBsGA1UEDxMUUHJpdmF0ZSBP
# cmdhbml6YXRpb24xCzAJBgNVBAYTAkdCMRAwDgYDVQQIDAdTdWZmb2xrMSgwJgYD
# VQQKDB9GaXJlRGFlbW9uIFRlY2hub2xvZ2llcyBMaW1pdGVkMSgwJgYDVQQDDB9G
# aXJlRGFlbW9uIFRlY2hub2xvZ2llcyBMaW1pdGVkMIICIjANBgkqhkiG9w0BAQEF
# AAOCAg8AMIICCgKCAgEAqMRnePfZYnbLBX1NbRCB8woVKAD3tDyz4OVuaRBr2Y01
# YrC2XojPEB+rsuMpSEvR+JcwP6IVxo0wmXsZLAHeGiDSH5K5H1q6MJx0aY5xpbE+
# r2CLzpxV/05lSzsRE2iv+kYT7HoPPzfOpgI/lQGXMq7TEujtRjZDa6U7GXlq+OwQ
# qN7iBTMLH/puN11PyibF9glOqgCHIIqEsciDkXe00iLAC1PjtbhPoRj7xDlzc4uP
# Zy7X82PMveKE9c+jY8akigxDtLZuqNxeoKgXwGgYUGNOSuqBqY2hxQpjRKpCss80
# 071fhCqLY/BQSF5yVLv0GN9BQjtSmKdiPHhkJsfzdv+zmJJoZcGDd3OH8KJQovhK
# Vh8PxTzN5Sze+VWHBdTj6EfNypplDUdRouyTAYctUmg3yUDQROV46EwujrFufv05
# 8KCU4HWmLSLm9sEX3EebYwacgvfFCvwc/kK+orh4NCM/SsoHjTtnjO20Osl0G4HE
# jDmxCr5A9oIEN6vo2dPp8E4RPT7PivuNY9Toj6UTLp4NG1vh0FIAsCABIEuQZXkk
# 9DoEPaXbIOqVLMafjPHnQVsUHBMMleLCrZaCcDKS/gpRPVVspLdMJgepdvm6YIxo
# TUSJNnNvqIBnOCjJpWKn513ji/ees2e3KF8WzvbAY3GbpRj20qBTeaPPuGPa2r0C
# AwEAAaOCAbQwggGwMB8GA1UdIwQYMBaAFIEykkErKM1GyMSixio5EuxIqT8UMB0G
# A1UdDgQWBBRe0XMjM9cLqOkvXjT68LrtR06xrTAOBgNVHQ8BAf8EBAMCB4AwDAYD
# VR0TAQH/BAIwADATBgNVHSUEDDAKBggrBgEFBQcDAzBJBgNVHSAEQjBAMDUGDCsG
# AQQBsjEBAgEGATAlMCMGCCsGAQUFBwIBFhdodHRwczovL3NlY3RpZ28uY29tL0NQ
# UzAHBgVngQwBAzBLBgNVHR8ERDBCMECgPqA8hjpodHRwOi8vY3JsLnNlY3RpZ28u
# Y29tL1NlY3RpZ29QdWJsaWNDb2RlU2lnbmluZ0NBRVZSMzYuY3JsMHsGCCsGAQUF
# BwEBBG8wbTBGBggrBgEFBQcwAoY6aHR0cDovL2NydC5zZWN0aWdvLmNvbS9TZWN0
# aWdvUHVibGljQ29kZVNpZ25pbmdDQUVWUjM2LmNydDAjBggrBgEFBQcwAYYXaHR0
# cDovL29jc3Auc2VjdGlnby5jb20wJgYDVR0RBB8wHaAbBggrBgEFBQcIA6APMA0M
# C0dCLTA1OTAxNjAxMA0GCSqGSIb3DQEBCwUAA4IBgQA0iWNo97cqJr9VJJWfSwoK
# j5HNswfTK1IjgYWrczHvvJ9VP59LXQc+ccCpw5qpLQaf9AujV1/sW/BLz9YDw7cb
# iCSWqEvJRKZiD4UCPujT0lOfO/buWXyC2btj78DiG5aS4a3k/mrWbz/E2okRhNnY
# md0jvOR4jiyBtlOsp5uIkDXGIt/xYhqHVjqVPgBoQQGO2QBmf5udnousXpJGyRu7
# ZbWccGhy9uVZyHNWiEz73pyeAkocv8e8dy/117ySruhThnmIJqUoDNzs6JrgNvj6
# MYA/buUDEdRou3h+jvjvJCTguMWBvDRx4IXlyE+GYa6yrJGjUTcN8JxIPhZ6l5/4
# IXVoTOP9258suNFKwepgXVFMCYhc6Y00jUy0UFnA3LUiM8JiF7FbwHw+OUylXakh
# p3xQV9oCOeBSN5qUaxaRfWN25PQmJFFatpUOLi4ebq5Hm43Ok0+MkifLaEfsp3sh
# 050oMkj7Z3xqU5psHRvG6Jl6PEY6wAEcckxuhbTK6SMxghrtMIIa6QIBATBrMFcx
# CzAJBgNVBAYTAkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxLjAsBgNVBAMT
# JVNlY3RpZ28gUHVibGljIENvZGUgU2lnbmluZyBDQSBFViBSMzYCEHKqHTZpwQph
# fhLjhuxi9/cwDQYJYIZIAWUDBAIBBQCggdowGQYJKoZIhvcNAQkDMQwGCisGAQQB
# gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcCARUwLwYJKoZIhvcNAQkE
# MSIEIAl+1pIybSlRXK0MSXWtkne+22qk24QYx/cZXZnTsfU1MG4GCisGAQQBgjcC
# AQwxYDBeoCqAKABGAGkAcgBlAEQAYQBlAG0AbwBuACAATwBwAGUAbgBTAFMATAAg
# ADShMIAuaHR0cHM6Ly93d3cuZmlyZWRhZW1vbi5jb20vZmlyZWRhZW1vbi1vcGVu
# c3NsIDANBgkqhkiG9w0BAQEFAASCAgA47qsOqsVQ5b1oOrKAi2XYh0vdqx2DOg9l
# verGh2dgF7KXMKTgFmQmrbJMiKmq705rZGvl6H5pnIMw8IGbtHDyUByfuSykHR/4
# 47O+KDQVBu3tb0/2twnLnz43/pgxjj76UvqacWR9jUQ4Eg61z+uSJcZ2suBvxdiF
# sY1URnH9EOvA4wDcEko57rPx3SCMYVvYItYc3rkc/bm/seXcly5tBVSPCjJhu0DH
# 6cmcf3CD7h/ZjOzqv4W3K2oFkHvSS+6jH/06StJpHP53YGFV0BmyVr6/KB6oQNnf
# Z5kdfAO60MsI2Lvw2gezoCXXGSSfIH+jGAvgEdo7dfh67fqt7Gv6KIlyoBregCJM
# aobg1QLBUeEfmb5NN73KJK2EZ+QlXQU6U2xMNeVnOPhf7kYacJdXc+gmVsSPsdaw
# A/h2h2dWJupyocGz7aE1bOgYNEddwqNEdhMrjF10GaPJY+6TmK5lzQmCVaQI9J4V
# z1kzGgx0tr9eEFF3zMsyIr1wEKC87sFkQxTRajG9kqaQfFkWLzcuiwF5I++op5C0
# nbAdDHuNYVk2nn6ZY2tAfXFdRu4K2P21o2bfwkwhQ/C36l6kYdRUvaIEZ1M2a52U
# UxSg7qRMTyIdlOdjNSCLgrK9MsMwRwj/IExq/QEgxyRTYVw818LYrhSvQ/U9IEFG
# F5lfV/+OtqGCF3YwghdyBgorBgEEAYI3AwMBMYIXYjCCF14GCSqGSIb3DQEHAqCC
# F08wghdLAgEDMQ8wDQYJYIZIAWUDBAIBBQAwdwYLKoZIhvcNAQkQAQSgaARmMGQC
# AQEGCWCGSAGG/WwHATAxMA0GCWCGSAFlAwQCAQUABCAV3cIXsx52zIC5GJ6IXERT
# msDNNnriPWjcZPMy8fnJpQIQUMpDxn6CJHExLh/QKoEFKBgPMjAyNjA2MTAyMDU1
# NDlaoIITOjCCBu0wggTVoAMCAQICEAqA7xhLjfEFgtHEdqeVdGgwDQYJKoZIhvcN
# AQELBQAwaTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDkRpZ2lDZXJ0LCBJbmMuMUEw
# PwYDVQQDEzhEaWdpQ2VydCBUcnVzdGVkIEc0IFRpbWVTdGFtcGluZyBSU0E0MDk2
# IFNIQTI1NiAyMDI1IENBMTAeFw0yNTA2MDQwMDAwMDBaFw0zNjA5MDMyMzU5NTla
# MGMxCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5EaWdpQ2VydCwgSW5jLjE7MDkGA1UE
# AxMyRGlnaUNlcnQgU0hBMjU2IFJTQTQwOTYgVGltZXN0YW1wIFJlc3BvbmRlciAy
# MDI1IDEwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDQRqwtEsae0Oqu
# YFazK1e6b1H/hnAKAd/KN8wZQjBjMqiZ3xTWcfsLwOvRxUwXcGx8AUjni6bz52fG
# Tfr6PHRNv6T7zsf1Y/E3IU8kgNkeECqVQ+3bzWYesFtkepErvUSbf+EIYLkrLKd6
# qJnuzK8Vcn0DvbDMemQFoxQ2Dsw4vEjoT1FpS54dNApZfKY61HAldytxNM89PZXU
# P/5wWWURK+IfxiOg8W9lKMqzdIo7VA1R0V3Zp3DjjANwqAf4lEkTlCDQ0/fKJLKL
# kzGBTpx6EYevvOi7XOc4zyh1uSqgr6UnbksIcFJqLbkIXIPbcNmA98Oskkkrvt6l
# PAw/p4oDSRZreiwB7x9ykrjS6GS3NR39iTTFS+ENTqW8m6THuOmHHjQNC3zbJ6nJ
# 6SXiLSvw4Smz8U07hqF+8CTXaETkVWz0dVVZw7knh1WZXOLHgDvundrAtuvz0D3T
# +dYaNcwafsVCGZKUhQPL1naFKBy1p6llN3QgshRta6Eq4B40h5avMcpi54wm0i2e
# PZD5pPIssoszQyF4//3DoK2O65Uck5Wggn8O2klETsJ7u8xEehGifgJYi+6I03Uu
# T1j7FnrqVrOzaQoVJOeeStPeldYRNMmSF3voIgMFtNGh86w3ISHNm0IaadCKCkUe
# 2LnwJKa8TIlwCUNVwppwn4D3/Pt5pwIDAQABo4IBlTCCAZEwDAYDVR0TAQH/BAIw
# ADAdBgNVHQ4EFgQU5Dv88jHt/f3X85FxYxlQQ89hjOgwHwYDVR0jBBgwFoAU729T
# SunkBnx6yuKQVvYv1Ensy04wDgYDVR0PAQH/BAQDAgeAMBYGA1UdJQEB/wQMMAoG
# CCsGAQUFBwMIMIGVBggrBgEFBQcBAQSBiDCBhTAkBggrBgEFBQcwAYYYaHR0cDov
# L29jc3AuZGlnaWNlcnQuY29tMF0GCCsGAQUFBzAChlFodHRwOi8vY2FjZXJ0cy5k
# aWdpY2VydC5jb20vRGlnaUNlcnRUcnVzdGVkRzRUaW1lU3RhbXBpbmdSU0E0MDk2
# U0hBMjU2MjAyNUNBMS5jcnQwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDovL2NybDMu
# ZGlnaWNlcnQuY29tL0RpZ2lDZXJ0VHJ1c3RlZEc0VGltZVN0YW1waW5nUlNBNDA5
# NlNIQTI1NjIwMjVDQTEuY3JsMCAGA1UdIAQZMBcwCAYGZ4EMAQQCMAsGCWCGSAGG
# /WwHATANBgkqhkiG9w0BAQsFAAOCAgEAZSqt8RwnBLmuYEHs0QhEnmNAciH45PYi
# T9s1i6UKtW+FERp8FgXRGQ/YAavXzWjZhY+hIfP2JkQ38U+wtJPBVBajYfrbIYG+
# Dui4I4PCvHpQuPqFgqp1PzC/ZRX4pvP/ciZmUnthfAEP1HShTrY+2DE5qjzvZs7J
# IIgt0GCFD9ktx0LxxtRQ7vllKluHWiKk6FxRPyUPxAAYH2Vy1lNM4kzekd8oEARz
# FAWgeW3az2xejEWLNN4eKGxDJ8WDl/FQUSntbjZ80FU3i54tpx5F/0Kr15zW/mJA
# xZMVBrTE2oi0fcI8VMbtoRAmaaslNXdCG1+lqvP4FbrQ6IwSBXkZagHLhFU9HCrG
# /syTRLLhAezu/3Lr00GrJzPQFnCEH1Y58678IgmfORBPC1JKkYaEt2OdDh4GmO0/
# 5cHelAK2/gTlQJINqDr6JfwyYHXSd+V08X1JUPvB4ILfJdmL+66Gp3CSBXG6IwXM
# ZUXBhtCyIaehr0XkBoDIGMUG1dUtwq1qmcwbdUfcSYCn+OwncVUXf53VJUNOaMWM
# ts0VlRYxe5nK+At+DI96HAlXHAL5SlfYxJ7La54i71McVWRP66bW+yERNpbJCjyC
# YG2j+bdpxo/1Cy4uPcU3AWVPGrbn5PhDBf3Froguzzhk++ami+r3Qrx5bIbY3TVz
# giFI7Gq3zWcwgga0MIIEnKADAgECAhANx6xXBf8hmS5AQyIMOkmGMA0GCSqGSIb3
# DQEBCwUAMGIxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAX
# BgNVBAsTEHd3dy5kaWdpY2VydC5jb20xITAfBgNVBAMTGERpZ2lDZXJ0IFRydXN0
# ZWQgUm9vdCBHNDAeFw0yNTA1MDcwMDAwMDBaFw0zODAxMTQyMzU5NTlaMGkxCzAJ
# BgNVBAYTAlVTMRcwFQYDVQQKEw5EaWdpQ2VydCwgSW5jLjFBMD8GA1UEAxM4RGln
# aUNlcnQgVHJ1c3RlZCBHNCBUaW1lU3RhbXBpbmcgUlNBNDA5NiBTSEEyNTYgMjAy
# NSBDQTEwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC0eDHTCphBcr48
# RsAcrHXbo0ZodLRRF51NrY0NlLWZloMsVO1DahGPNRcybEKq+RuwOnPhof6pvF4u
# GjwjqNjfEvUi6wuim5bap+0lgloM2zX4kftn5B1IpYzTqpyFQ/4Bt0mAxAHeHYNn
# QxqXmRinvuNgxVBdJkf77S2uPoCj7GH8BLuxBG5AvftBdsOECS1UkxBvMgEdgkFi
# DNYiOTx4OtiFcMSkqTtF2hfQz3zQSku2Ws3IfDReb6e3mmdglTcaarps0wjUjsZv
# kgFkriK9tUKJm/s80FiocSk1VYLZlDwFt+cVFBURJg6zMUjZa/zbCclF83bRVFLe
# GkuAhHiGPMvSGmhgaTzVyhYn4p0+8y9oHRaQT/aofEnS5xLrfxnGpTXiUOeSLsJy
# goLPp66bkDX1ZlAeSpQl92QOMeRxykvq6gbylsXQskBBBnGy3tW/AMOMCZIVNSaz
# 7BX8VtYGqLt9MmeOreGPRdtBx3yGOP+rx3rKWDEJlIqLXvJWnY0v5ydPpOjL6s36
# czwzsucuoKs7Yk/ehb//Wx+5kMqIMRvUBDx6z1ev+7psNOdgJMoiwOrUG2ZdSoQb
# U2rMkpLiQ6bGRinZbI4OLu9BMIFm1UUl9VnePs6BaaeEWvjJSjNm2qA+sdFUeEY0
# qVjPKOWug/G6X5uAiynM7Bu2ayBjUwIDAQABo4IBXTCCAVkwEgYDVR0TAQH/BAgw
# BgEB/wIBADAdBgNVHQ4EFgQU729TSunkBnx6yuKQVvYv1Ensy04wHwYDVR0jBBgw
# FoAU7NfjgtJxXWRM3y5nP+e6mK4cD08wDgYDVR0PAQH/BAQDAgGGMBMGA1UdJQQM
# MAoGCCsGAQUFBwMIMHcGCCsGAQUFBwEBBGswaTAkBggrBgEFBQcwAYYYaHR0cDov
# L29jc3AuZGlnaWNlcnQuY29tMEEGCCsGAQUFBzAChjVodHRwOi8vY2FjZXJ0cy5k
# aWdpY2VydC5jb20vRGlnaUNlcnRUcnVzdGVkUm9vdEc0LmNydDBDBgNVHR8EPDA6
# MDigNqA0hjJodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRUcnVzdGVk
# Um9vdEc0LmNybDAgBgNVHSAEGTAXMAgGBmeBDAEEAjALBglghkgBhv1sBwEwDQYJ
# KoZIhvcNAQELBQADggIBABfO+xaAHP4HPRF2cTC9vgvItTSmf83Qh8WIGjB/T8Ob
# XAZz8OjuhUxjaaFdleMM0lBryPTQM2qEJPe36zwbSI/mS83afsl3YTj+IQhQE7jU
# /kXjjytJgnn0hvrV6hqWGd3rLAUt6vJy9lMDPjTLxLgXf9r5nWMQwr8Myb9rEVKC
# hHyfpzee5kH0F8HABBgr0UdqirZ7bowe9Vj2AIMD8liyrukZ2iA/wdG2th9y1IsA
# 0QF8dTXqvcnTmpfeQh35k5zOCPmSNq1UH410ANVko43+Cdmu4y81hjajV/gxdEkM
# x1NKU4uHQcKfZxAvBAKqMVuqte69M9J6A47OvgRaPs+2ykgcGV00TYr2Lr3ty9qI
# ijanrUR3anzEwlvzZiiyfTPjLbnFRsjsYg39OlV8cipDoq7+qNNjqFzeGxcytL5T
# TLL4ZaoBdqbhOhZ3ZRDUphPvSRmMThi0vw9vODRzW6AxnJll38F0cuJG7uEBYTpt
# MSbhdhGQDpOXgpIUsWTjd6xpR6oaQf/DJbg3s6KCLPAlZ66RzIg9sC+NJpud/v4+
# 7RWsWCiKi9EOLLHfMR2ZyJ/+xhCx9yHbxtl5TPau1j/1MIDpMPx0LckTetiSuEtQ
# vLsNz3Qbp7wGWqbIiOWCnb5WqxL3/BAPvIXKUjPSxyZsq8WhbaM2tszWkPZPubdc
# MIIFjTCCBHWgAwIBAgIQDpsYjvnQLefv21DiCEAYWjANBgkqhkiG9w0BAQwFADBl
# MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
# d3cuZGlnaWNlcnQuY29tMSQwIgYDVQQDExtEaWdpQ2VydCBBc3N1cmVkIElEIFJv
# b3QgQ0EwHhcNMjIwODAxMDAwMDAwWhcNMzExMTA5MjM1OTU5WjBiMQswCQYDVQQG
# EwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNl
# cnQuY29tMSEwHwYDVQQDExhEaWdpQ2VydCBUcnVzdGVkIFJvb3QgRzQwggIiMA0G
# CSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC/5pBzaN675F1KPDAiMGkz7MKnJS7J
# IT3yithZwuEppz1Yq3aaza57G4QNxDAf8xukOBbrVsaXbR2rsnnyyhHS5F/WBTxS
# D1Ifxp4VpX6+n6lXFllVcq9ok3DCsrp1mWpzMpTREEQQLt+C8weE5nQ7bXHiLQwb
# 7iDVySAdYyktzuxeTsiT+CFhmzTrBcZe7FsavOvJz82sNEBfsXpm7nfISKhmV1ef
# VFiODCu3T6cw2Vbuyntd463JT17lNecxy9qTXtyOj4DatpGYQJB5w3jHtrHEtWoY
# OAMQjdjUN6QuBX2I9YI+EJFwq1WCQTLX2wRzKm6RAXwhTNS8rhsDdV14Ztk6MUSa
# M0C/CNdaSaTC5qmgZ92kJ7yhTzm1EVgX9yRcRo9k98FpiHaYdj1ZXUJ2h4mXaXpI
# 8OCiEhtmmnTK3kse5w5jrubU75KSOp493ADkRSWJtppEGSt+wJS00mFt6zPZxd9L
# BADMfRyVw4/3IbKyEbe7f/LVjHAsQWCqsWMYRJUadmJ+9oCw++hkpjPRiQfhvbfm
# Q6QYuKZ3AeEPlAwhHbJUKSWJbOUOUlFHdL4mrLZBdd56rF+NP8m800ERElvlEFDr
# McXKchYiCd98THU/Y+whX8QgUWtvsauGi0/C1kVfnSD8oR7FwI+isX4KJpn15Gkv
# mB0t9dmpsh3lGwIDAQABo4IBOjCCATYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
# FgQU7NfjgtJxXWRM3y5nP+e6mK4cD08wHwYDVR0jBBgwFoAUReuir/SSy4IxLVGL
# p6chnfNtyA8wDgYDVR0PAQH/BAQDAgGGMHkGCCsGAQUFBwEBBG0wazAkBggrBgEF
# BQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEMGCCsGAQUFBzAChjdodHRw
# Oi8vY2FjZXJ0cy5kaWdpY2VydC5jb20vRGlnaUNlcnRBc3N1cmVkSURSb290Q0Eu
# Y3J0MEUGA1UdHwQ+MDwwOqA4oDaGNGh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9E
# aWdpQ2VydEFzc3VyZWRJRFJvb3RDQS5jcmwwEQYDVR0gBAowCDAGBgRVHSAAMA0G
# CSqGSIb3DQEBDAUAA4IBAQBwoL9DXFXnOF+go3QbPbYW1/e/Vwe9mqyhhyzshV6p
# Grsi+IcaaVQi7aSId229GhT0E0p6Ly23OO/0/4C5+KH38nLeJLxSA8hO0Cre+i1W
# z/n096wwepqLsl7Uz9FDRJtDIeuWcqFItJnLnU+nBgMTdydE1Od/6Fmo8L8vC6bp
# 8jQ87PcDx4eo0kxAGTVGamlUsLihVo7spNU96LHc/RzY9HdaXFSMb++hUD38dglo
# hJ9vytsgjTVgHAIDyyCwrFigDkBjxZgiwbJZ9VVrzyerbHbObyMt9H5xaiNrIv8S
# uFQtJ37YOtnwtoeW/VvRXKwYw02fc7cBqZ9Xql4o4rmUMYIDfDCCA3gCAQEwfTBp
# MQswCQYDVQQGEwJVUzEXMBUGA1UEChMORGlnaUNlcnQsIEluYy4xQTA/BgNVBAMT
# OERpZ2lDZXJ0IFRydXN0ZWQgRzQgVGltZVN0YW1waW5nIFJTQTQwOTYgU0hBMjU2
# IDIwMjUgQ0ExAhAKgO8YS43xBYLRxHanlXRoMA0GCWCGSAFlAwQCAQUAoIHRMBoG
# CSqGSIb3DQEJAzENBgsqhkiG9w0BCRABBDAcBgkqhkiG9w0BCQUxDxcNMjYwNjEw
# MjA1NTQ5WjArBgsqhkiG9w0BCRACDDEcMBowGDAWBBTdYjCshgotMGvaOLFoeVIw
# B/tBfjAvBgkqhkiG9w0BCQQxIgQgi74Ue/EGI7uFNw42fkGOV0Mn9Uy5um4p2EsI
# zz3jDbMwNwYLKoZIhvcNAQkQAi8xKDAmMCQwIgQgSqA/oizXXITFXJOPgo5na5yu
# yrM/420mmqM08UYRCjMwDQYJKoZIhvcNAQEBBQAEggIAHU9nvnVxpZ3QtXjQHr21
# 4C92qflYamcmheT/IaJjiSQ9UOvplEm1CgIPky1VIrlGMPL+z4qV/Yz+rIreElAD
# Ynp9ZE4tL3xuBPyDZ05obFs0oi+3tro71Bk0cUvwCVbuM+hsMU8TP6EvzBRUaZLb
# 1t8lR0VcbH8DOJwFFzABwxy9j69e+zeRe5kTI9J4vv8/wgZL+5S6YHvgUWvvjSQv
# /vhkbOhssDmn/jBZLSFsnPRuf2cAhBW0IfiHvcDIhupsvEt11aZ9+d7BFWkHDnUe
# jF3SU+vUFLQxcjChZ7idfgos/lNd9OUBiSJsAMTvTQeMObu2aAOA9rki2BFRG4C6
# Xz/TLj5FfgtaWsZg1YsrWZNN5Qb/umFRt4UBzdSHc3Md1d8Y+u91b6NnTaJ5CmJi
# +rIaeAUY6iSZw4VC4nvM3u6MGR9BGzfs473ApQAEIr6TXeAZsD4AQty1qd1jzzcR
# pXIqDXRUd353vpwgoHCOuYj4SbDK0N8bGqSwz5KNxSexh3XCk5hg2XEzsIvVwBpI
# L0tABmI1b8MbGjqKty/dGV0O+yoqA19Z6paffXDYYyFj6DU1KROcUpHQRdeaRFA6
# fhZA1f5h7q0PS/siAmGew8cZNQO0hg0BZngpTdH9BPdMZNDB86evIB5QxQ0N7K46
# w6eiS+IX3P9aP7UQ7hxaUsM=
# SIG # End signature block

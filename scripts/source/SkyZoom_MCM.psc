ScriptName SkyZoom_MCM extends SKI_ConfigBase

int _oidZoomHotkey
int _oidGamepadButton
int _oidToggleMode
int _oidZoomFOV
int _oidSmoothSpeed
int _oidEnableScrollZoomAdjust
int _oidMinZoomFOV
int _oidScrollUsesSmoothSpeed
int _oidLiveZoomBoostButton
int _oidViewMode
int _oidRequireWeaponSheathed
int _oidAllowZoomDuringDialogue
int _oidScaleSensitivity
int _oidSensitivityExponent

string[] _viewModeOptions

Event OnConfigInit()
    ModName = "SkyZoom"

    _viewModeOptions = new string[3]
    _viewModeOptions[0] = "First Person Only"
    _viewModeOptions[1] = "Third Person Only"
    _viewModeOptions[2] = "Both"
EndEvent

Event OnPageReset(string a_page)
    SetCursorFillMode(TOP_TO_BOTTOM) ; default alternates left/right per row, splitting header groups across columns

    int scrollZoomFlags = OPTION_FLAG_NONE
    if !SkyZoom_Native.GetEnableScrollZoomAdjust()
        scrollZoomFlags = OPTION_FLAG_DISABLED
    endif

    AddHeaderOption("Hotkeys")
    _oidZoomHotkey = AddKeyMapOption("Zoom Hotkey", SkyZoom_Native.GetHotkey())
    _oidGamepadButton = AddKeyMapOption("Zoom Gamepad Button", SkyZoom_Native.GetGamepadButton())
    _oidLiveZoomBoostButton = AddKeyMapOption("Live Zoom Gamepad Button", SkyZoom_Native.GetLiveZoomBoostButton(), scrollZoomFlags)
    _oidToggleMode = AddToggleOption("Toggle Mode", SkyZoom_Native.GetToggleMode())

    AddHeaderOption("Zoom")
    _oidZoomFOV = AddSliderOption("Zoom FOV", SkyZoom_Native.GetZoomFOV(), "{0} deg")
    _oidSmoothSpeed = AddSliderOption("Smooth Speed", SkyZoom_Native.GetSmoothSpeed(), "{2}")

    AddHeaderOption("Live Zoom Adjust")
    _oidEnableScrollZoomAdjust = AddToggleOption("Live Zoom Adjust", SkyZoom_Native.GetEnableScrollZoomAdjust())
    _oidMinZoomFOV = AddSliderOption("Min Live Zoom FOV", SkyZoom_Native.GetMinZoomFOV(), "{0} deg", scrollZoomFlags)
    _oidScrollUsesSmoothSpeed = AddToggleOption("Live Zoom Uses Smooth Speed", SkyZoom_Native.GetScrollUsesSmoothSpeed(), scrollZoomFlags)

    SetCursorPosition(1) ; right column: Zoom Conditions, Look Sensitivity, then About

    AddHeaderOption("Zoom Conditions")
    _oidViewMode = AddMenuOption("Active View", _viewModeOptions[SkyZoom_Native.GetViewMode()])
    _oidRequireWeaponSheathed = AddToggleOption("Require Weapon Sheathed", SkyZoom_Native.GetRequireWeaponSheathed())
    _oidAllowZoomDuringDialogue = AddToggleOption("Allow Zoom During Dialogue", SkyZoom_Native.GetAllowZoomDuringDialogue())

    AddHeaderOption("Look Sensitivity")
    _oidScaleSensitivity = AddToggleOption("Scale Sensitivity While Zoomed", SkyZoom_Native.GetScaleMouseSensitivity())
    int sensitivityExponentFlags = OPTION_FLAG_NONE
    if !SkyZoom_Native.GetScaleMouseSensitivity()
        sensitivityExponentFlags = OPTION_FLAG_DISABLED
    endif
    _oidSensitivityExponent = AddSliderOption("Sensitivity Curve", SkyZoom_Native.GetSensitivityExponent(), "{2}", sensitivityExponentFlags)

    AddHeaderOption("About")
    AddTextOption("Plugin Version", SkyZoom_Native.GetPluginVersion(), OPTION_FLAG_DISABLED)
EndEvent

Event OnOptionHighlight(int a_option)
    if a_option == _oidZoomHotkey
        if SkyZoom_Native.GetToggleMode()
            SetInfoText("Press to toggle zoom on/off.")
        else
            SetInfoText("Hold to zoom in.")
        endif
    elseif a_option == _oidGamepadButton
        if SkyZoom_Native.GetToggleMode()
            SetInfoText("Gamepad button to toggle zoom on/off.")
        else
            SetInfoText("Gamepad button to zoom in.")
        endif
    elseif a_option == _oidLiveZoomBoostButton
        SetInfoText("Gamepad button for Live Zoom Adjust.")
    elseif a_option == _oidToggleMode
        SetInfoText("Press the hotkey to toggle zoom on/off, instead of holding it down.")
    elseif a_option == _oidZoomFOV
        SetInfoText("FOV while zoomed. Lower = tighter zoom.")
    elseif a_option == _oidSmoothSpeed
        SetInfoText("Zoom transition speed. Higher = snappier.")
    elseif a_option == _oidEnableScrollZoomAdjust
        SetInfoText("While the zoom hotkey is held, scroll the wheel or hold your Live Zoom Gamepad Button to zoom in temporarily.")
    elseif a_option == _oidMinZoomFOV
        SetInfoText("Tightest zoom reachable with Live Zoom Adjust.")
    elseif a_option == _oidScrollUsesSmoothSpeed
        SetInfoText("Use Smooth Speed's timing for live zoom adjustments, instead of a faster default.")
    elseif a_option == _oidViewMode
        SetInfoText("Which view(s) the hotkey zooms in.")
    elseif a_option == _oidRequireWeaponSheathed
        SetInfoText("Only zoom outside a fighting stance. Off = zoom anytime, even mid-combat.")
    elseif a_option == _oidAllowZoomDuringDialogue
        SetInfoText("Also zoom during conversations.")
    elseif a_option == _oidScaleSensitivity
        SetInfoText("Lower look sensitivity while zoomed, so aim feels less twitchy.")
    elseif a_option == _oidSensitivityExponent
        SetInfoText("How hard sensitivity drops as you zoom in.")
    endif
EndEvent

Event OnOptionKeyMapChange(int a_option, int a_keyCode, string a_conflictControl, string a_conflictName)
    if a_keyCode == 1 ; Escape cancels the capture, don't rebind
        return
    endif

    if a_option == _oidZoomHotkey
        if SkyZoom_Native.IsKeyboardOrMouseKeycode(a_keyCode)
            SkyZoom_Native.SetHotkey(a_keyCode)
            SetKeyMapOptionValue(a_option, a_keyCode)
        else
            SetKeyMapOptionValue(a_option, SkyZoom_Native.GetHotkey())
        endif
    elseif a_option == _oidGamepadButton
        if SkyZoom_Native.IsGamepadKeycode(a_keyCode) || a_keyCode == -1
            SkyZoom_Native.SetGamepadButton(a_keyCode)
            SetKeyMapOptionValue(a_option, a_keyCode)
        else
            SetKeyMapOptionValue(a_option, SkyZoom_Native.GetGamepadButton())
        endif
    elseif a_option == _oidLiveZoomBoostButton
        if SkyZoom_Native.IsGamepadKeycode(a_keyCode) || a_keyCode == -1
            SkyZoom_Native.SetLiveZoomBoostButton(a_keyCode)
            SetKeyMapOptionValue(a_option, a_keyCode)
        else
            SetKeyMapOptionValue(a_option, SkyZoom_Native.GetLiveZoomBoostButton())
        endif
    endif
EndEvent

Event OnOptionSliderOpen(int a_option)
    if a_option == _oidZoomFOV
        SetSliderDialogStartValue(SkyZoom_Native.GetZoomFOV())
        SetSliderDialogDefaultValue(60.0)
        SetSliderDialogRange(1.0, 170.0)
        SetSliderDialogInterval(1.0)
    elseif a_option == _oidSmoothSpeed
        SetSliderDialogStartValue(SkyZoom_Native.GetSmoothSpeed())
        SetSliderDialogDefaultValue(8.0)
        SetSliderDialogRange(0.1, 60.0)
        SetSliderDialogInterval(0.1)
    elseif a_option == _oidSensitivityExponent
        SetSliderDialogStartValue(SkyZoom_Native.GetSensitivityExponent())
        SetSliderDialogDefaultValue(2.5)
        SetSliderDialogRange(0.1, 10.0)
        SetSliderDialogInterval(0.1)
    elseif a_option == _oidMinZoomFOV
        SetSliderDialogStartValue(SkyZoom_Native.GetMinZoomFOV())
        SetSliderDialogDefaultValue(20.0)
        SetSliderDialogRange(1.0, 170.0)
        SetSliderDialogInterval(1.0)
    endif
EndEvent

Event OnOptionSliderAccept(int a_option, float a_value)
    if a_option == _oidZoomFOV
        SkyZoom_Native.SetZoomFOV(a_value)
        SetSliderOptionValue(a_option, a_value, "{0} deg")
    elseif a_option == _oidSmoothSpeed
        SkyZoom_Native.SetSmoothSpeed(a_value)
        SetSliderOptionValue(a_option, a_value, "{2}")
    elseif a_option == _oidSensitivityExponent
        SkyZoom_Native.SetSensitivityExponent(a_value)
        SetSliderOptionValue(a_option, a_value, "{2}")
    elseif a_option == _oidMinZoomFOV
        SkyZoom_Native.SetMinZoomFOV(a_value)
        SetSliderOptionValue(a_option, a_value, "{0} deg")
    endif
EndEvent

Event OnOptionSelect(int a_option)
    if a_option == _oidRequireWeaponSheathed
        bool requireSheathed = !SkyZoom_Native.GetRequireWeaponSheathed()
        SkyZoom_Native.SetRequireWeaponSheathed(requireSheathed)
        SetToggleOptionValue(a_option, requireSheathed)
    elseif a_option == _oidAllowZoomDuringDialogue
        bool allowDuringDialogue = !SkyZoom_Native.GetAllowZoomDuringDialogue()
        SkyZoom_Native.SetAllowZoomDuringDialogue(allowDuringDialogue)
        SetToggleOptionValue(a_option, allowDuringDialogue)
    elseif a_option == _oidScaleSensitivity
        bool scaleSensitivity = !SkyZoom_Native.GetScaleMouseSensitivity()
        SkyZoom_Native.SetScaleMouseSensitivity(scaleSensitivity)
        SetToggleOptionValue(a_option, scaleSensitivity)
        if scaleSensitivity
            SetOptionFlags(_oidSensitivityExponent, OPTION_FLAG_NONE)
        else
            SetOptionFlags(_oidSensitivityExponent, OPTION_FLAG_DISABLED)
        endif
    elseif a_option == _oidEnableScrollZoomAdjust
        bool enableScrollZoomAdjust = !SkyZoom_Native.GetEnableScrollZoomAdjust()
        SkyZoom_Native.SetEnableScrollZoomAdjust(enableScrollZoomAdjust)
        SetToggleOptionValue(a_option, enableScrollZoomAdjust)
        int scrollZoomFlags = OPTION_FLAG_DISABLED
        if enableScrollZoomAdjust
            scrollZoomFlags = OPTION_FLAG_NONE
        endif
        SetOptionFlags(_oidMinZoomFOV, scrollZoomFlags)
        SetOptionFlags(_oidScrollUsesSmoothSpeed, scrollZoomFlags)
        SetOptionFlags(_oidLiveZoomBoostButton, scrollZoomFlags)
    elseif a_option == _oidScrollUsesSmoothSpeed
        bool scrollUsesSmoothSpeed = !SkyZoom_Native.GetScrollUsesSmoothSpeed()
        SkyZoom_Native.SetScrollUsesSmoothSpeed(scrollUsesSmoothSpeed)
        SetToggleOptionValue(a_option, scrollUsesSmoothSpeed)
    elseif a_option == _oidToggleMode
        bool toggleMode = !SkyZoom_Native.GetToggleMode()
        SkyZoom_Native.SetToggleMode(toggleMode)
        SetToggleOptionValue(a_option, toggleMode)
    endif
EndEvent

; SkyUI's per-option "reset to default" gesture.
Event OnOptionDefault(int a_option)
    if a_option == _oidZoomHotkey
        int defaultHotkey = SkyZoom_Native.GetDefaultHotkey()
        SkyZoom_Native.SetHotkey(defaultHotkey)
        SetKeyMapOptionValue(a_option, defaultHotkey)
    elseif a_option == _oidGamepadButton
        int defaultGamepadButton = SkyZoom_Native.GetDefaultGamepadButton()
        SkyZoom_Native.SetGamepadButton(defaultGamepadButton)
        SetKeyMapOptionValue(a_option, defaultGamepadButton)
    elseif a_option == _oidLiveZoomBoostButton
        int defaultLiveZoomBoostButton = SkyZoom_Native.GetDefaultLiveZoomBoostButton()
        SkyZoom_Native.SetLiveZoomBoostButton(defaultLiveZoomBoostButton)
        SetKeyMapOptionValue(a_option, defaultLiveZoomBoostButton)
    elseif a_option == _oidToggleMode
        SkyZoom_Native.SetToggleMode(false)
        SetToggleOptionValue(a_option, false)
    elseif a_option == _oidZoomFOV
        SkyZoom_Native.SetZoomFOV(60.0)
        SetSliderOptionValue(a_option, 60.0, "{0} deg")
    elseif a_option == _oidSmoothSpeed
        SkyZoom_Native.SetSmoothSpeed(8.0)
        SetSliderOptionValue(a_option, 8.0, "{2}")
    elseif a_option == _oidEnableScrollZoomAdjust
        SkyZoom_Native.SetEnableScrollZoomAdjust(true)
        SetToggleOptionValue(a_option, true)
        SetOptionFlags(_oidMinZoomFOV, OPTION_FLAG_NONE)
        SetOptionFlags(_oidScrollUsesSmoothSpeed, OPTION_FLAG_NONE)
        SetOptionFlags(_oidLiveZoomBoostButton, OPTION_FLAG_NONE)
    elseif a_option == _oidMinZoomFOV
        SkyZoom_Native.SetMinZoomFOV(20.0)
        SetSliderOptionValue(a_option, 20.0, "{0} deg")
    elseif a_option == _oidScrollUsesSmoothSpeed
        SkyZoom_Native.SetScrollUsesSmoothSpeed(false)
        SetToggleOptionValue(a_option, false)
    elseif a_option == _oidViewMode
        SkyZoom_Native.SetViewMode(2)
        SetMenuOptionValue(a_option, _viewModeOptions[2])
    elseif a_option == _oidRequireWeaponSheathed
        SkyZoom_Native.SetRequireWeaponSheathed(true)
        SetToggleOptionValue(a_option, true)
    elseif a_option == _oidAllowZoomDuringDialogue
        SkyZoom_Native.SetAllowZoomDuringDialogue(false)
        SetToggleOptionValue(a_option, false)
    elseif a_option == _oidScaleSensitivity
        SkyZoom_Native.SetScaleMouseSensitivity(true)
        SetToggleOptionValue(a_option, true)
        SetOptionFlags(_oidSensitivityExponent, OPTION_FLAG_NONE)
    elseif a_option == _oidSensitivityExponent
        SkyZoom_Native.SetSensitivityExponent(2.5)
        SetSliderOptionValue(a_option, 2.5, "{2}")
    endif
EndEvent

Event OnOptionMenuOpen(int a_option)
    if a_option == _oidViewMode
        SetMenuDialogOptions(_viewModeOptions)
        SetMenuDialogStartIndex(SkyZoom_Native.GetViewMode())
        SetMenuDialogDefaultIndex(2)
    endif
EndEvent

Event OnOptionMenuAccept(int a_option, int a_index)
    if a_option == _oidViewMode
        SkyZoom_Native.SetViewMode(a_index)
        SetMenuOptionValue(a_option, _viewModeOptions[a_index])
    endif
EndEvent

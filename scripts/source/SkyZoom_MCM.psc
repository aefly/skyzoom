ScriptName SkyZoom_MCM extends SKI_ConfigBase

int _oidZoomHotkey
int _oidGamepadButton
int _oidZoomFOV
int _oidSmoothSpeed
int _oidEnableScrollZoomAdjust
int _oidMinZoomFOV
int _oidScrollUsesSmoothSpeed
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

    AddHeaderOption("Hotkeys")
    _oidZoomHotkey = AddKeyMapOption("Zoom Hotkey", SkyZoom_Native.GetHotkey())
    _oidGamepadButton = AddKeyMapOption("Zoom Gamepad Button", SkyZoom_Native.GetGamepadButton())

    AddHeaderOption("Zoom Settings")
    _oidZoomFOV = AddSliderOption("Zoom FOV", SkyZoom_Native.GetZoomFOV(), "{0} deg")
    _oidSmoothSpeed = AddSliderOption("Smooth Speed", SkyZoom_Native.GetSmoothSpeed(), "{2}")
    _oidEnableScrollZoomAdjust = AddToggleOption("Scroll to Adjust Zoom", SkyZoom_Native.GetEnableScrollZoomAdjust())
    int scrollZoomFlags = OPTION_FLAG_NONE
    if !SkyZoom_Native.GetEnableScrollZoomAdjust()
        scrollZoomFlags = OPTION_FLAG_DISABLED
    endif
    _oidMinZoomFOV = AddSliderOption("Min Scroll Zoom FOV", SkyZoom_Native.GetMinZoomFOV(), "{0} deg", scrollZoomFlags)
    _oidScrollUsesSmoothSpeed = AddToggleOption("Scroll Zoom Uses Smooth Speed", SkyZoom_Native.GetScrollUsesSmoothSpeed(), scrollZoomFlags)
    _oidViewMode = AddMenuOption("Active View", _viewModeOptions[SkyZoom_Native.GetViewMode()])
    _oidRequireWeaponSheathed = AddToggleOption("Require Weapon Sheathed", SkyZoom_Native.GetRequireWeaponSheathed())
    _oidAllowZoomDuringDialogue = AddToggleOption("Allow Zoom During Dialogue", SkyZoom_Native.GetAllowZoomDuringDialogue())

    SetCursorPosition(1) ; right column: Mouse Sensitivity, then About

    AddHeaderOption("Mouse Sensitivity")
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
        SetInfoText("Hold this keyboard key or mouse button in first or third person to zoom in.")
    elseif a_option == _oidGamepadButton
        SetInfoText("Gamepad button that also holds-to-zoom, independently of the keyboard/mouse hotkey above.")
    elseif a_option == _oidZoomFOV
        SetInfoText("Field of view, in degrees, while the hotkey is held.")
    elseif a_option == _oidSmoothSpeed
        SetInfoText("How quickly the zoom eases in and out. Higher is snappier, lower is more gradual.")
    elseif a_option == _oidEnableScrollZoomAdjust
        SetInfoText("Lets the mouse wheel (or gamepad LB/RB) adjust the zoom while the hotkey is held, between Min Scroll Zoom FOV and Zoom FOV. Zoom FOV is the loose end - releasing the hotkey is still the only way back to unzoomed.")
    elseif a_option == _oidMinZoomFOV
        SetInfoText("Tightest FOV reachable by scrolling in, while Scroll to Adjust Zoom is on.")
    elseif a_option == _oidScrollUsesSmoothSpeed
        SetInfoText("Makes the scroll-zoom ease use the same speed as Smooth Speed above, instead of its own fixed, faster timing. Off by default - a scroll notch is a much smaller hop than the full zoom-in sweep Smooth Speed is tuned for.")
    elseif a_option == _oidViewMode
        SetInfoText("Which view(s) the hotkey zooms in.")
    elseif a_option == _oidRequireWeaponSheathed
        SetInfoText("Only zoom while not in a ready/fighting stance - includes empty-handed H2H, not just a drawn weapon or spell. Turn off to zoom from any stance.")
    elseif a_option == _oidAllowZoomDuringDialogue
        SetInfoText("Let the hotkey zoom in while talking to an NPC, even from a ready stance. Off by default, since conversations don't pause the game.")
    elseif a_option == _oidScaleSensitivity
        SetInfoText("Scales mouse sensitivity down while zoomed, so aim doesn't feel twitchy at a narrower FOV.")
    elseif a_option == _oidSensitivityExponent
        SetInfoText("How aggressively sensitivity is cut while zoomed. 1.0 scales directly with FOV; higher values cut it more at moderate zoom.")
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
    elseif a_option == _oidScrollUsesSmoothSpeed
        bool scrollUsesSmoothSpeed = !SkyZoom_Native.GetScrollUsesSmoothSpeed()
        SkyZoom_Native.SetScrollUsesSmoothSpeed(scrollUsesSmoothSpeed)
        SetToggleOptionValue(a_option, scrollUsesSmoothSpeed)
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

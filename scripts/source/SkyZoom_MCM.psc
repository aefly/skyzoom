ScriptName SkyZoom_MCM extends SKI_ConfigBase

int _oidZoomHotkey
int _oidGamepadButton
int _oidZoomFOV
int _oidSmoothSpeed
int _oidViewMode
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
    _oidViewMode = AddMenuOption("Active View", _viewModeOptions[SkyZoom_Native.GetViewMode()])

    AddHeaderOption("Mouse Sensitivity")
    _oidScaleSensitivity = AddToggleOption("Scale Sensitivity While Zoomed", SkyZoom_Native.GetScaleMouseSensitivity())
    _oidSensitivityExponent = AddSliderOption("Sensitivity Curve", SkyZoom_Native.GetSensitivityExponent(), "{2}")

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
    elseif a_option == _oidViewMode
        SetInfoText("Which view(s) the hotkey zooms in.")
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
    endif
EndEvent

Event OnOptionSelect(int a_option)
    if a_option == _oidScaleSensitivity
        bool newValue = !SkyZoom_Native.GetScaleMouseSensitivity()
        SkyZoom_Native.SetScaleMouseSensitivity(newValue)
        SetToggleOptionValue(a_option, newValue)
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

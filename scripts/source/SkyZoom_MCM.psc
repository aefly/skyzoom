ScriptName SkyZoom_MCM extends SKI_ConfigBase

int _oidZoomHotkey
int _oidGamepadButton
int _oidZoomFOV
int _oidSmoothSpeed

Event OnConfigInit()
    ModName = "SkyZoom"
EndEvent

Event OnPageReset(string a_page)
    SetCursorFillMode(TOP_TO_BOTTOM) ; default alternates left/right per row, splitting header groups across columns

    AddHeaderOption("Hotkeys")
    _oidZoomHotkey = AddKeyMapOption("Zoom Hotkey", SkyZoom_Native.GetHotkey())
    _oidGamepadButton = AddKeyMapOption("Zoom Gamepad Button", SkyZoom_Native.GetGamepadButton())

    AddHeaderOption("Zoom Settings")
    _oidZoomFOV = AddSliderOption("Zoom FOV", SkyZoom_Native.GetZoomFOV(), "{0} deg")
    _oidSmoothSpeed = AddSliderOption("Smooth Speed", SkyZoom_Native.GetSmoothSpeed(), "{2}")

    AddHeaderOption("About")
    AddTextOption("Plugin Version", SkyZoom_Native.GetPluginVersion(), OPTION_FLAG_DISABLED)
EndEvent

Event OnOptionHighlight(int a_option)
    if a_option == _oidZoomHotkey
        SetInfoText("Hold this keyboard key or mouse button in first person to zoom in.")
    elseif a_option == _oidGamepadButton
        SetInfoText("Gamepad button that also holds-to-zoom, independently of the keyboard/mouse hotkey above.")
    elseif a_option == _oidZoomFOV
        SetInfoText("Field of view, in degrees, while the hotkey is held.")
    elseif a_option == _oidSmoothSpeed
        SetInfoText("How quickly the zoom eases in and out. Higher is snappier, lower is more gradual.")
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
    endif
EndEvent

Event OnOptionSliderAccept(int a_option, float a_value)
    if a_option == _oidZoomFOV
        SkyZoom_Native.SetZoomFOV(a_value)
        SetSliderOptionValue(a_option, a_value, "{0} deg")
    elseif a_option == _oidSmoothSpeed
        SkyZoom_Native.SetSmoothSpeed(a_value)
        SetSliderOptionValue(a_option, a_value, "{2}")
    endif
EndEvent

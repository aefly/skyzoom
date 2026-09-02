ScriptName SkyZoom_Native Hidden

; GetHotkey/SetHotkey use DirectX scan codes (matching AddKeyMapOption), not
; the Windows virtual-key codes SkyZoom.ini stores - the DLL converts both.
int Function GetHotkey() Global Native
Function SetHotkey(int akeyCode) Global Native

int Function GetGamepadButton() Global Native
Function SetGamepadButton(int akeyCode) Global Native

bool Function GetToggleMode() Global Native
Function SetToggleMode(bool atoggle) Global Native

; Default hotkey/gamepad button as MCM codes, for the MCM's per-option
; "reset to default" gesture.
int Function GetDefaultHotkey() Global Native
int Function GetDefaultGamepadButton() Global Native

; Rejects a keycode from the wrong device (e.g. keyboard while mapping the
; gamepad option), since AddKeyMapOption's capture accepts any device.
bool Function IsKeyboardOrMouseKeycode(int akeyCode) Global Native
bool Function IsGamepadKeycode(int akeyCode) Global Native

float Function GetZoomFOV() Global Native
Function SetZoomFOV(float afov) Global Native

float Function GetSmoothSpeed() Global Native
Function SetSmoothSpeed(float aspeed) Global Native

bool Function GetEnableScrollZoomAdjust() Global Native
Function SetEnableScrollZoomAdjust(bool aenable) Global Native

float Function GetMinZoomFOV() Global Native
Function SetMinZoomFOV(float afov) Global Native

bool Function GetScrollUsesSmoothSpeed() Global Native
Function SetScrollUsesSmoothSpeed(bool ause) Global Native

; Gamepad-only, like GamepadButton above - the mouse wheel already covers
; keyboard/mouse users for live zoom adjust.
int Function GetLiveZoomBoostButton() Global Native
Function SetLiveZoomBoostButton(int akeyCode) Global Native
int Function GetDefaultLiveZoomBoostButton() Global Native

; 0 = first person only, 1 = third person only, 2 = both.
int Function GetViewMode() Global Native
Function SetViewMode(int aviewMode) Global Native

bool Function GetRequireWeaponSheathed() Global Native
Function SetRequireWeaponSheathed(bool arequire) Global Native

; When GamepadButton above is LT/RT, also disables that trigger's normal
; vanilla function (left/right attack-block) while the weapon is sheathed.
bool Function GetDisableTriggerWhenSheathed() Global Native
Function SetDisableTriggerWhenSheathed(bool adisable) Global Native

bool Function GetAllowZoomDuringDialogue() Global Native
Function SetAllowZoomDuringDialogue(bool aallow) Global Native

bool Function GetScaleMouseSensitivity() Global Native
Function SetScaleMouseSensitivity(bool ascale) Global Native

float Function GetSensitivityExponent() Global Native
Function SetSensitivityExponent(float aexponent) Global Native

string Function GetPluginVersion() Global Native

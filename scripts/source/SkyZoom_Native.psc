ScriptName SkyZoom_Native Hidden

; GetHotkey/SetHotkey use DirectX scan codes (matching AddKeyMapOption), not
; the Windows virtual-key codes SkyZoom.ini stores - the DLL converts both.
int Function GetHotkey() Global Native
Function SetHotkey(int akeyCode) Global Native

int Function GetGamepadButton() Global Native
Function SetGamepadButton(int akeyCode) Global Native

; Rejects a keycode from the wrong device (e.g. keyboard while mapping the
; gamepad option), since AddKeyMapOption's capture accepts any device.
bool Function IsKeyboardOrMouseKeycode(int akeyCode) Global Native
bool Function IsGamepadKeycode(int akeyCode) Global Native

float Function GetZoomFOV() Global Native
Function SetZoomFOV(float afov) Global Native

float Function GetSmoothSpeed() Global Native
Function SetSmoothSpeed(float aspeed) Global Native

; 0 = first person only, 1 = third person only, 2 = both.
int Function GetViewMode() Global Native
Function SetViewMode(int aviewMode) Global Native

bool Function GetScaleMouseSensitivity() Global Native
Function SetScaleMouseSensitivity(bool ascale) Global Native

float Function GetSensitivityExponent() Global Native
Function SetSensitivityExponent(float aexponent) Global Native

string Function GetPluginVersion() Global Native

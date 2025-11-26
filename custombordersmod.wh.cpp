// ==WindhawkMod==
// @id              custom-window-border-color
// @name            Custom Window Border Color
// @description     Set custom colors for window borders without affecting the titlebar
// @version         1.0.0
// @author          Kailuss
// @github          https://github.com/kailuss
// @include         *
// @exclude         devenv.exe
// @compilerOptions -ldwmapi -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Custom Window Border Color

This mod allows you to set custom colors for window borders in Windows 11
without affecting the titlebar color or using the accent color.

## Settings

You can configure the following colors:
- **Active Border Color**: Color for active/focused windows (default: #0078D4 - blue)
- **Inactive Border Color**: Color for inactive windows (default: #808080 - gray)

Colors should be in hexadecimal RGB format: **#RRGGBB**
- RR = Red component
- GG = Green component
- BB = Blue component

Examples:
- `#0078D4` - Blue (Windows 11 accent)
- `#FF0000` - Red
- `#00FF00` - Green
- `#FFFFFF` - White
- `#888888` - Grey
- `#000000` - Black
- `#FFA500` - Orange

## Important Note

**Transparency is NOT supported** by Windows DWM for border colors. The DWMWA_BORDER_COLOR
attribute only accepts RGB values without alpha channel. Any alpha values will be ignored
by the system.

*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- activeBorderColor: "#0078D4"
  $name: Active Border Color
  $description: Color for active window borders in hex format (#RRGGBB)
- inactiveBorderColor: "#808080"
  $name: Inactive Border Color
  $description: Color for inactive window borders in hex format (#RRGGBB)
*/
// ==/WindhawkModSettings==

#include <dwmapi.h>
#include <windhawk_api.h>
#include <string>

struct
{
    WCHAR activeBorderColor[16];
    WCHAR inactiveBorderColor[16];
} settings;

COLORREF BorderActive;
COLORREF BorderInactive;
const COLORREF ColorDefault = DWMWA_COLOR_DEFAULT;

// Convierte color hex (#RRGGBB) a COLORREF (formato 0x00BBGGRR)
COLORREF HexToColorRef(LPCWSTR hexColor)
{
    if (!hexColor || wcslen(hexColor) < 6)
    {
        Wh_Log(L"Invalid hex color format: %s", hexColor ? hexColor : L"NULL");
        return 0x00D47800; // Color por defecto (azul)
    }

    // Eliminar el # si está presente
    const WCHAR *colorStr = hexColor;
    if (colorStr[0] == L'#')
    {
        colorStr++;
    }

    // Tomar solo los primeros 6 caracteres (ignorar alpha si existe)
    WCHAR rgbStr[7] = {0};
    wcsncpy_s(rgbStr, 7, colorStr, 6);

    DWORD color = 0;
    swscanf_s(rgbStr, L"%X", &color);

    BYTE red = (color >> 16) & 0xFF;
    BYTE green = (color >> 8) & 0xFF;
    BYTE blue = color & 0xFF;

    Wh_Log(L"Parsed RGB color: R=%02X G=%02X B=%02X", red, green, blue);

    // DWM usa formato 0x00BBGGRR (little-endian, sin alpha)
    return (blue << 16) | (green << 8) | red;
}

void LoadSettings()
{
    PCWSTR activeBorderColorValue = Wh_GetStringSetting(L"activeBorderColor");
    PCWSTR inactiveBorderColorValue = Wh_GetStringSetting(L"inactiveBorderColor");

    wcscpy_s(settings.activeBorderColor, activeBorderColorValue ? activeBorderColorValue : L"#0078D4");
    wcscpy_s(settings.inactiveBorderColor, inactiveBorderColorValue ? inactiveBorderColorValue : L"#808080");

    Wh_FreeStringSetting(activeBorderColorValue);
    Wh_FreeStringSetting(inactiveBorderColorValue);

    BorderActive = HexToColorRef(settings.activeBorderColor);
    BorderInactive = HexToColorRef(settings.inactiveBorderColor);

    Wh_Log(L"Settings loaded - Active: %s (0x%08X), Inactive: %s (0x%08X)",
           settings.activeBorderColor, BorderActive,
           settings.inactiveBorderColor, BorderInactive);
}

BOOL IsValidWindow(HWND hWnd)
{
    DWORD dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    // Excluir menús contextuales
    return (dwStyle & WS_THICKFRAME) == WS_THICKFRAME || (dwStyle & WS_CAPTION) == WS_CAPTION;
}

using DwmSetWindowAttribute_t = decltype(&DwmSetWindowAttribute);
DwmSetWindowAttribute_t DwmSetWindowAttribute_orig;
HRESULT WINAPI DwmSetWindowAttribute_hook(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute)
{
    // Interceptar intentos de cambiar el color del borde para mantener nuestro color personalizado
    if (dwAttribute == DWMWA_BORDER_COLOR && IsValidWindow(hwnd))
    {
        Wh_Log(L"Blocked DWMWA_BORDER_COLOR change");
        return S_OK;
    }

    return DwmSetWindowAttribute_orig(hwnd, dwAttribute, pvAttribute, cbAttribute);
}

void SetBorderColor(HWND hWnd, BOOL activate)
{
    if (!IsValidWindow(hWnd))
        return;

    COLORREF color = activate ? BorderActive : BorderInactive;

    // Establecer el color del borde (sin alpha, solo RGB)
    HRESULT hr = DwmSetWindowAttribute_orig(hWnd, DWMWA_BORDER_COLOR, &color, sizeof(color));

    if (SUCCEEDED(hr))
    {
        Wh_Log(L"Set border color for window %p - Active: %d, Color: 0x%08X", hWnd, activate, color);
    }
    else
    {
        Wh_Log(L"Failed to set border color for window %p - HRESULT: 0x%08X", hWnd, hr);
    }
}

using DefWindowProcA_t = decltype(&DefWindowProcA);
DefWindowProcA_t DefWindowProcA_orig;
LRESULT WINAPI DefWindowProcA_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = DefWindowProcA_orig(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
        SetBorderColor(hWnd, wParam);
        break;
    }

    return result;
}

using DefWindowProcW_t = decltype(&DefWindowProcW);
DefWindowProcW_t DefWindowProcW_orig;
LRESULT WINAPI DefWindowProcW_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = DefWindowProcW_orig(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
        SetBorderColor(hWnd, wParam);
        break;
    }

    return result;
}

using DefDlgProcA_t = decltype(&DefDlgProcA);
DefDlgProcA_t DefDlgProcA_orig;
LRESULT WINAPI DefDlgProcA_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = DefDlgProcA_orig(hWnd, uMsg, wParam, lParam);

    if (uMsg == WM_NCACTIVATE)
    {
        SetBorderColor(hWnd, wParam);
    }

    return result;
}

using DefDlgProcW_t = decltype(&DefDlgProcW);
DefDlgProcW_t DefDlgProcW_orig;
LRESULT WINAPI DefDlgProcW_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = DefDlgProcW_orig(hWnd, uMsg, wParam, lParam);

    if (uMsg == WM_NCACTIVATE)
    {
        SetBorderColor(hWnd, wParam);
    }

    return result;
}

BOOL CALLBACK EnableEnumWindowsCallback(HWND hWnd, LPARAM lParam)
{
    DWORD pid = lParam;
    DWORD wPid = 0;
    GetWindowThreadProcessId(hWnd, &wPid);

    if (pid == wPid)
    {
        SetBorderColor(hWnd, GetForegroundWindow() == hWnd);
    }

    return TRUE;
}

BOOL CALLBACK DisableEnumWindowsCallback(HWND hWnd, LPARAM lParam)
{
    DWORD pid = lParam;
    DWORD wPid = 0;
    GetWindowThreadProcessId(hWnd, &wPid);

    if (pid == wPid && IsValidWindow(hWnd))
    {
        DwmSetWindowAttribute_orig(hWnd, DWMWA_BORDER_COLOR, &ColorDefault, sizeof(ColorDefault));
    }

    return TRUE;
}

void Wh_ModSettingsChanged()
{
    Wh_Log(L"Settings changed, reloading...");
    LoadSettings();

    // Aplicar nuevos colores a todas las ventanas del proceso
    EnumWindows(EnableEnumWindowsCallback, GetCurrentProcessId());
}

BOOL Wh_ModInit()
{
    Wh_Log(L"Init");

    LoadSettings();

    Wh_SetFunctionHook(
        (void *)DwmSetWindowAttribute,
        (void *)DwmSetWindowAttribute_hook,
        (void **)&DwmSetWindowAttribute_orig);

    Wh_SetFunctionHook(
        (void *)DefWindowProcW,
        (void *)DefWindowProcW_hook,
        (void **)&DefWindowProcW_orig);

    Wh_SetFunctionHook(
        (void *)DefWindowProcA,
        (void *)DefWindowProcA_hook,
        (void **)&DefWindowProcA_orig);

    Wh_SetFunctionHook(
        (void *)DefDlgProcW,
        (void *)DefDlgProcW_hook,
        (void **)&DefDlgProcW_orig);

    Wh_SetFunctionHook(
        (void *)DefDlgProcA,
        (void *)DefDlgProcA_hook,
        (void **)&DefDlgProcA_orig);

    return TRUE;
}

void Wh_ModAfterInit()
{
    Wh_Log(L"AfterInit");
    EnumWindows(EnableEnumWindowsCallback, GetCurrentProcessId());
}

void Wh_ModBeforeUninit()
{
    Wh_Log(L"BeforeUninit");
    EnumWindows(DisableEnumWindowsCallback, GetCurrentProcessId());
}
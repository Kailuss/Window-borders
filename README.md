# Custom Window Border Color

This mod for Windhawk allows you to set custom colors for window borders in Windows 11
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

# PowerToolbar

A modern Windows toolbar application that provides quick access to window management functions. PowerToolbar appears above the active window and offers a sleek, animated interface for common window operations.

## Features

- **Window Management**: Quick access to essential window controls
  - ✖ Close Window
  - ⛶ Toggle Fullscreen
  - — Minimize Window
  - ✯ Toggle Always On Top
  - ◎ Center Window
  - ☠ Kill All Processes (by process name)

- **Modern UI**: 
  - Rounded corners with smooth animations
  - Dark theme with customizable button colors
  - Tooltips with hover effects
  - Layered window with transparency

- **System Tray Integration**:
  - Minimize to system tray
  - Right-click context menu
  - Double-click to restore

- **Smart Positioning**:
  - Automatically positions above the active window
  - Minimized toolbar mode for quick access
  - Smooth animations when showing/hiding

## Requirements

- Windows 10 or later
- Visual Studio 2019 or later (for building from source)
- Windows SDK 10.0

## Building

1. Clone the repository:
   ```bash
   git clone https://github.com/Axinomyus/windows-toolbar.git
   cd windows-toolbar
   ```

2. Open `windows-toolbar/windows-toolbar.sln` in Visual Studio

3. Build the solution (Debug or Release configuration)

4. The executable will be in `windows-toolbar/x64/Debug/` or `windows-toolbar/x64/Release/`

## Usage

1. Run `windows-toolbar.exe`
2. The toolbar will automatically appear above the active window
3. Click any button to perform the corresponding action
4. Right-click the toolbar or use the minimize button to send it to the system tray
5. Double-click the system tray icon to restore the toolbar

## Project Structure

```
windows-toolbar/
├── windows-toolbar/
│   ├── main.cpp              # Application entry point
│   ├── Button.cpp/h          # Button window procedure
│   ├── Constants.h           # UI constants and colors
│   ├── Drawing.cpp/h         # Drawing and rendering functions
│   ├── Globals.cpp/h         # Global variables and button definitions
│   ├── SystemTray.cpp/h      # System tray icon management
│   ├── Toolbar.cpp/h         # Toolbar positioning and animation
│   ├── Tooltip.cpp/h         # Custom tooltip implementation
│   ├── Types.h               # Type definitions
│   ├── WindowOperations.cpp/h # Window manipulation functions
│   ├── resource.h            # Resource definitions
│   ├── windows-toolbar.rc    # Resource file
│   └── toolbar.ico           # Application icon
└── LICENSE                   # MIT License
```

## Configuration

The toolbar appearance and behavior can be customized by modifying constants in `Constants.h`:

- Layout: Gap, toolbar height, button dimensions, corner radius
- Colors: Background, card, primary, and button colors
- Animation: Duration and easing functions
- Tooltips: Delay timing

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Author

**Axinomyus**

---

**Note**: This application requires administrator privileges for some operations (like killing processes). Make sure to run with appropriate permissions if needed.


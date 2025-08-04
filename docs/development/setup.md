# Development Setup Guide

[← Back to Documentation](../README.md)

## Prerequisites

- **Daisy Toolchain**: Install the Daisy toolchain from [Daisy Toolchain](https://daisy.audio/tutorials/cpp-dev-env/)
- **Git**: Make sure you have Git installed
- **Make**: Ensure you have Make installed (usually comes with development tools)

## Repository Setup

### 1. Clone the Repository

```bash
git clone https://github.com/your-username/OpenChord.git
cd OpenChord
```

### 2. Initialize Submodules

The project uses Git submodules for the Daisy libraries. You need to initialize and update them:

```bash
# Initialize submodules
git submodule init

# Update submodules to get the actual library files
git submodule update
```

**Note**: If you cloned with `--recursive`, you can skip this step as submodules will be automatically initialized.

### 3. Verify Setup

Check that the libraries are properly downloaded:

```bash
ls -la lib/
# Should show:
# libDaisy/  DaisySP/
```

## Building the Project

### First Build (Build Libraries)

The first build will take longer as it needs to compile the Daisy libraries:

```bash
# Clean any previous builds
make clean

# Build the project
make
```

### Subsequent Builds

After the first build, subsequent builds will be faster:

```bash
make
```

## VS Code Integration

The project includes VS Code tasks that match the Daisy examples structure. These tasks are available in the Command Palette (`Ctrl+P` / `Cmd+P`):

### Available Tasks

- **`build_all`**: Build the project (default build task)
- **`build_and_program_dfu`**: Build and flash via USB (DFU mode)
- **`build_and_program`**: Build and flash via debug probe
- **`clean`**: Clean build artifacts
- **`program-dfu`**: Flash via USB (requires Daisy in bootloader mode)
- **`program`**: Flash via debug probe

### Using VS Code Tasks

1. **Open the project** in VS Code
2. **Connect your Daisy Seed** via USB or debug probe
3. **Put Daisy in bootloader mode** (if using USB):
   - Hold BOOT button, press RESET, release RESET, then release BOOT
4. **Run tasks** via Command Palette:
   - Press `Ctrl+P` (Windows/Linux) or `Cmd+P` (Mac)
   - Type the task name (e.g., `task build_and_program_dfu`)
   - Press Enter

### Windows Terminal Setup

If you're on Windows, make sure VS Code is using Git Bash as the default terminal:

1. Open VS Code Terminal (`View > Terminal`)
2. Click the `+` dropdown in the terminal
3. Select `Select Default Profile`
4. Choose `Git Bash`

This ensures proper command execution for the build tasks.

## Project Structure

```
OpenChord/
├── docs/                 # Documentation
│   ├── development/     # Development guides
│   ├── hardware/        # Hardware documentation
│   ├── overview/        # Project overview
│   └── architecture/    # System architecture
├── include/              # Header files
│   ├── io.h             # Input/Output interface
│   ├── plugin_interface.h # Plugin system interface
│   ├── system_interface.h # Core system interface
│   ├── track_interface.h # Track management interface
│   └── midi_types.h     # MIDI type definitions
├── src/                  # Source code
│   ├── main.cpp         # Application entry point
│   ├── common/          # Shared utilities
│   ├── core/            # Core system files
│   │   ├── audio/       # Audio processing
│   │   ├── io.cpp       # Input/Output implementation
│   │   ├── midi/        # MIDI handling
│   │   ├── storage/     # Data storage
│   │   ├── system.cpp   # Core system implementation
│   │   ├── tracks/      # Track management
│   │   └── ui/          # User interface
│   └── plugins/         # Plugin implementations
│       ├── fx/          # Audio effects
│       ├── input/       # Input plugins
│       ├── instruments/ # Instrument plugins
│       └── playmodes/   # Playback modes
├── lib/                  # External libraries (submodules)
│   ├── libDaisy/        # Daisy hardware library
│   └── DaisySP/         # Daisy audio processing library
├── hardware/             # Hardware design files
├── images/               # Project images and diagrams
├── tests/                # Test files
├── build/               # Build artifacts (ignored by git)
├── .vscode/             # VS Code configuration
│   └── tasks.json       # Build and flash tasks
├── Makefile             # Build configuration
├── .gitmodules          # Submodule configuration
├── .gitignore           # Git ignore rules
├── LICENSE              # Project license
└── README.md            # Project overview
```

## Development Workflow

1. **Make changes** to source files in `src/`
2. **Build** with `make` or VS Code task `build_all`
3. **Test** on hardware using `build_and_program_dfu` or `build_and_program`
4. **Commit** your changes

## Troubleshooting

### Submodule Issues

If you encounter submodule-related issues:

```bash
# Reset submodules to their proper state
git submodule deinit -f .
git submodule update --init --recursive
```

### Build Issues

If the build fails:

```bash
# Clean everything and rebuild
make clean
cd lib/libDaisy && make clean && make
cd ../DaisySP && make clean && make
cd ../.. && make
```

### Windows Build Issues

If you get errors like:
```
mkdir build
process_begin: CreateProcess(NULL, mkdir build, ...) failed.
```

Make sure you're using Git Bash as your terminal in VS Code (see Windows Terminal Setup above).

### Large Repository Size

The repository is designed to be lightweight. The actual library files are downloaded via submodules, so:

- **Repository size**: ~1-2MB (just source code and docs)
- **After submodule init**: ~50-100MB (includes all library source)
- **After first build**: ~200-300MB (includes compiled libraries)

## IDE Setup

### VS Code (Recommended)

The project includes VS Code configuration files with tasks that match the Daisy examples structure. Open the project in VS Code and:

1. Install the **Cortex Debug** extension for debugging
2. Use the provided tasks for building and flashing
3. Make sure to use Git Bash on Windows

### Other IDEs

You can use any C++ IDE. Make sure to:
- Set the include paths to include `include/`, `lib/libDaisy/src/`, and `lib/DaisySP/Source/`
- Configure the build system to use the Makefile

## Contributing

When contributing:

1. **Fork** the repository
2. **Create** a feature branch
3. **Make** your changes
4. **Test** thoroughly using the VS Code tasks
5. **Submit** a pull request
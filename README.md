# OpenChord

A portable, open-source music creation device built on the Daisy Seed platform.

## 🎵 Overview

OpenChord is a modular, portable music creation device that combines the power of the Daisy Seed with a custom hardware design. It features a multi-track recording system, real-time audio processing, and an intuitive interface for music creation on the go.

## ✨ Features

- **Multi-track Recording**: Record and layer multiple audio tracks
- **Real-time Effects**: Built-in audio effects and processing
- **MIDI Support**: Full MIDI input/output capabilities
- **Modular Design**: Plugin-based architecture for easy extension
- **Portable**: Compact design for music creation anywhere
- **Open Source**: Complete open-source firmware and hardware

## 🚀 Quick Start

### Prerequisites

- [Daisy Toolchain](https://daisy.audio/tutorials/cpp-dev-env/)
- Git
- Make

### Setup

1. **Clone the repository**:
   ```bash
   git clone https://github.com/your-username/OpenChord.git
   cd OpenChord
   ```

2. **Initialize submodules**:
   ```bash
   git submodule init
   git submodule update
   ```

3. **Build the project**:
   ```bash
   make
   ```

For detailed setup instructions, see [Development Setup Guide](docs/development/setup.md).

## 📁 Project Structure

```
OpenChord/
├── docs/                 # Documentation
│   ├── overview/        # Project overview and goals
│   ├── architecture/    # System architecture docs
│   ├── hardware/        # Hardware documentation
│   └── development/     # Development guides
├── include/             # Header files
├── src/                 # Source code
│   ├── core/           # Core system files
│   └── plugins/        # Plugin implementations
├── lib/                 # External libraries (submodules)
│   ├── libDaisy/       # Daisy hardware library
│   └── DaisySP/        # Daisy audio processing library
├── images/              # Project images and diagrams
├── build/              # Build artifacts (ignored by git)
├── Makefile            # Build configuration
└── .gitmodules         # Submodule configuration
```

## 🏗️ Architecture

OpenChord uses a modular plugin architecture:

- **Core System**: Handles audio I/O, MIDI, and basic functionality
- **Plugin System**: Extensible architecture for effects, instruments, and features
- **Track System**: Multi-track recording and playback
- **UI System**: User interface and controls

For detailed architecture information, see [Firmware Architecture](docs/architecture/firmware_architecture.md).

## 🛠️ Development

### Building

```bash
make clean    # Clean build artifacts
make          # Build the project
```

### Development Workflow

1. Make changes to source files in `src/`
2. Build with `make`
3. Test on hardware
4. Commit your changes

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📖 Documentation

- [Project Overview](docs/overview/project_overview.md) - Complete project description
- [Development Setup](docs/development/setup.md) - Detailed setup instructions
- [Firmware Architecture](docs/architecture/firmware_architecture.md) - System design
- [Hardware Documentation](docs/hardware/pinout.md) - Hardware specifications

## 🎯 Status

### Current Progress

- ✅ **Project Structure**: Complete
- ✅ **Basic Firmware**: Core system implemented
- ✅ **IO System**: Hardware interface working
- 🔄 **Audio Processing**: In development
- ⏳ **UI System**: Planned
- ⏳ **Plugin System**: Planned

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Electro-Smith](https://electro-smith.com/) for the Daisy Seed platform
- [libDaisy](https://github.com/electro-smith/libDaisy) for the hardware abstraction layer
- [DaisySP](https://github.com/electro-smith/DaisySP) for audio processing utilities

## 📞 Support

- **Issues**: Use GitHub Issues for bug reports and feature requests
- **Discussions**: Use GitHub Discussions for questions and general discussion
- **Documentation**: Check the [docs/](docs/) directory for detailed guides

---

**OpenChord** - Making music creation accessible to everyone. 
# CanBus-tard

**CAN Bus Analyzer — Qt/C++ GUI Application**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-f34b7d.svg)](https://isocpp.org/)
[![Framework: Qt](https://img.shields.io/badge/Framework-Qt-41CD52.svg)](https://www.qt.io/)
[![Build: QMake](https://img.shields.io/badge/Build-QMake-green.svg)]()


## About

**CanBus-tard** is an open-source tool for analyzing and visualizing CAN *(Controller Area Network)* frames. Built with Qt in C++, it provides an intuitive graphical interface to interact with a CAN bus — receiving, decoding, displaying, and interpreting frames in real time.

Perfect for embedded developers, automotive engineers, and electronics enthusiasts.


## Features

- 📡Real-time CAN frame reception and sending (repeatable, bindable to a keyboard key)
- 🔍Decoding and display of identifiers, data bytes, and flags
- 🖥️Clean and ergonomic Qt GUI with dockable and floatable windows for a fully customizable layout
- 🔌Support for all CAN interfaces provided by the Qt Serial Bus module:
  - SocketCAN (Linux)
  - PEAK PCAN (Windows/Linux)
  - Vector (Windows)
  - Kvaser (Windows/Linux)
  - TinyCAN (Windows)
  - Virtual CAN (Linux)
## Prerequisites

- **Qt** ≥ 5.x (or Qt 6.x)
- **C++ compiler** with C++11 support (GCC, Clang, MSVC)
- **QMake** (bundled with Qt)
- A CAN interface (e.g. PEAK PCAN, SocketCAN on Linux, etc.)


## Build & Run

### With Qt Creator

1. Open `CanBus-tard.pro` in **Qt Creator**
2. Select your build kit
3. Hit **Build** then **Run** ▶️

### Command Line

```bash
# Clone the repository
git clone https://github.com/K-Motors/CanBus_tard.git
cd CanBus_tard

# Generate Makefile and build
qmake CanBus-tard.pro
make -j$(nproc)

# Run the application
./CanBus-tard
```

## Screenshots

> *(Coming soon — feel free to contribute some!)*


## Contributing

Contributions are welcome!

1. Fork the repository
2. Create a branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m 'feat: add ...'`
4. Push: `git push origin feature/my-feature`
5. Open a **Pull Request**

> Please follow the code style defined in `.clang-format`.


## License

This project is licensed under the **GNU GPL v3**.
See the [LICENSE](LICENSE) file for details.


*Made with ❤️ by [K-Motors](https://github.com/K-Motors)*

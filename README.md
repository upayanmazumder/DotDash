# 📡 DotDash  

Design and Implementation of an FPGA + ESP32 Based Morse Communication System  

---

## 📖 Overview  

DotDash is a hybrid communication system that leverages an **FPGA** (for real-time signal encoding/decoding) and an **ESP32** (for wireless networking and user interfacing).  
The project demonstrates a modern implementation of **Morse code transmission and reception**, combining the parallelism of hardware logic with the flexibility of embedded software.  

---

## 📂 Project Structure  

```text
DotDash/
├── esp32/                     # ESP32 (Arduino IDE) firmware
│   ├── src/                   # Main code (.ino, .cpp, .h)
│   ├── include/               # Header files
│   └── lib/                   # Optional Arduino libraries
│
├── fpga/                      # FPGA (Quartus) design
│   ├── src/                   # Verilog / VHDL sources
│   ├── constraints/           # Pin assignments (.qsf, .sdc)
│   ├── sim/                   # Simulation testbenches
│   └── project/               # Quartus project files (.qpf, .qsf)
│
├── docs/                      # Documentation & diagrams
│   ├── architecture.md
│   ├── block-diagram.png
│   └── protocol.md
│
├── tools/                     # Helper scripts
│   └── flash_fpga.sh
│
├── .gitignore
├── LICENSE
└── README.md

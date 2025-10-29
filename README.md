# DotDash  

**ESP32-Based Real-Time Morse Code Communication System with Digital Logic Simulation**  

---

## Overview  

DotDash is a **real-time Morse code communication system** implemented entirely on an **ESP32**, combining **touch input**, **OLED display**, **live web portal**, and **digital logic gate simulation** for educational purposes.  

Users input Morse code through a **touch-sensitive pin**, and the system provides **instant feedback**:  

- **Line 1 (OLED):** Scrolling dots and dashes representing raw Morse input  
- **Line 2 (OLED):** Decoded letters as they are interpreted  
- **Progress bar (OLED):** Visual indicator of touch duration, showing dot vs dash  

The **local web interface** mirrors the OLED display, updating live:  

- **Raw Morse** in one section  
- **Decoded letters** in another  

DotDash automatically detects:  

- **Letter gaps (1s):** Converts current Morse token to a letter  
- **End of input (4s):** Finalizes the message display on OLED and in the web portal  

Additionally, live **Serial Monitor output** mirrors the OLED and web portal, enabling debugging or monitoring via USB.

---

## Key Features  

1. **Touch-Based Morse Input:** Single pin differentiates dots (`.`) and dashes (`-`) based on press duration.  
2. **OLED Feedback:** Live scrolling display of Morse symbols and decoded letters.  
3. **Progress Visualization:** Horizontal bar indicates touch duration; fills progressively as touch is held.  
4. **Digital Logic Simulation:** Implements combinational logic (AND, OR, NOT gates) to encode letters A-Z to Morse code using 5-bit binary representation.
5. **Web Interface:** Access via ESP32 Wi-Fi AP with terminal-styled interface; live updates of Morse and decoded letters.  
6. **Automatic Decoding:** Detects letter gaps and message completion automatically.  
7. **Configurable Timings:** Adjust `DOT_TIME`, `CHAR_GAP`, `END_GAP`, and other parameters to suit user preference.  
8. **Serial Monitor Integration:** Real-time output of Morse and decoded letters for development and monitoring.  

---

## Digital Logic Architecture

DotDash includes a **5-bit to Morse encoder** that simulates combinational logic circuits:

### Circuit Design
- **5 Input Bits** (b4, b3, b2, b1, b0) represent letters A-Z (values 0-25)
- **5 NOT Gates** invert each input bit
- **26 AND Gates** detect unique 5-bit patterns for each letter
- **1 Output Multiplexer** selects the corresponding Morse code

### Boolean Expressions

Each letter is encoded using Boolean algebra in **Sum-of-Products (SOP)** form:

| Letter | Binary | Boolean Expression | Morse |
|--------|--------|-------------------|-------|
| A | 00000 | ¬b4 · ¬b3 · ¬b2 · ¬b1 · ¬b0 | `.-` |
| B | 00001 | ¬b4 · ¬b3 · ¬b2 · ¬b1 · b0 | `-...` |
| C | 00010 | ¬b4 · ¬b3 · ¬b2 · b1 · ¬b0 | `-.-.` |
| ... | ... | ... | ... |
| Z | 11001 | b4 · b3 · ¬b2 · ¬b1 · b0 | `--..` |

**Note:** `·` represents AND, `¬` represents NOT, `+` represents OR

### Gate Count
- **NOT Gates:** 5 (one per input bit)
- **5-input AND Gates:** 26 (one per letter)
- **Total:** 32 gates for complete A-Z decoder

This implementation demonstrates how **combinational logic circuits** can be used in embedded systems for character encoding, making DotDash both a functional communication tool and an educational platform for digital design concepts.

---

## Implementation Notes  

- **ESP32 Responsibilities:** Wi-Fi Access Point, DNS redirect, web server, OLED control, touch input reading, scrolling logic, progress bar animation, digital logic gate simulation, and live Serial output.  
- **Touch Input:** Uses `touchRead()` with dynamic threshold calibration to detect touch events and differentiate dots vs dashes.  
- **OLED Display (128x64 SSD1306):** Scrolling Morse line, decoded letter line, and touch progress bar with real-time fill animation.  
- **Web Portal:** Terminal-styled interface with two separate divs for live Morse symbols and decoded letters; updates every 200ms.  
- **Logic Gate Simulation:** AND, OR, NOT, and XOR functions simulate hardware-level combinational logic for character encoding.
- **Timing Parameters:**  

| Parameter     | Default | Description                  |
|---------------|---------|------------------------------|
| `DOT_TIME`    | 200 ms  | Max duration for a dot       |
| `DASH_TIME`   | 600 ms  | Max duration for a dash      |
| `CHAR_GAP`    | 1000 ms | Gap to detect end of letter  |
| `END_GAP`     | 4000 ms | Gap to detect end of message |
| `DEBOUNCE`    | 30 ms   | Touch debounce delay         |

- **Morse Table:** Supports A–Z; easily extendable to 0–9 and punctuation.  

---

## Hardware Requirements

- **ESP32 Development Board** (any variant with touch pins)
- **128x64 OLED Display** (SSD1306, I2C interface)
- **Touch-sensitive surface** or conductive material connected to GPIO 4
- **Jumper wires** for connections

### Pin Configuration

| Component | Pin |
|-----------|-----|
| Touch Input | GPIO 4 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |

---

## Software Dependencies

- **Arduino IDE** or **PlatformIO**
- **ESP32 Board Support** (via Board Manager)
- **Libraries:**
  - `WiFi.h` (built-in)
  - `DNSServer.h` (built-in)
  - `WebServer.h` (built-in)
  - `Wire.h` (built-in)
  - `U8g2lib.h` (install via Library Manager)

---

## Installation & Setup

1. **Install Arduino IDE** and add ESP32 board support
2. **Install U8g2 library** via Library Manager
3. **Connect hardware** according to pin configuration
4. **Upload the code** to your ESP32
5. **Connect to WiFi AP:**
   - SSID: `DotDash-DSD`
   - Password: (none - open network)
6. **Access web interface** at `http://192.168.4.1`

---

## Serial Monitor Example  
```
DSD Morse Encoder Started (A-Z Complete)
Access at: 192.168.4.1

Pressed!
Released! Duration: 150 ms | Symbol: .
Current token: .
Decoded char: E
Live Morse: . | Letters: E

Pressed!
Released! Duration: 650 ms | Symbol: -
Current token: -
Decoded char: T
Live Morse: - | Letters: ET

Message complete: HELLO
```

---

## Web Portal Features  

- **Terminal aesthetic** with green on black background
- **Real-time updates** every 200ms
- **Responsive design** for mobile and desktop
- **Visual sections:**
  - Raw Morse code display
  - Decoded letters display
- **High contrast** for enhanced visibility

Portal automatically opens when connecting to `DotDash-DSD` network via captive portal redirect.

---

## Usage  

1. **Power on** the ESP32 with the DotDash firmware.  
2. **Connect** to the Wi-Fi network `DotDash-DSD` (open, no password).  
3. **Open browser** and navigate to `http://192.168.4.1/` (or wait for captive portal).  
4. **Touch the pad** to input Morse code:  
   - **Short press (<200ms)** = dot (`.`)  
   - **Long press (>200ms)** = dash (`-`)  
5. **Watch feedback** on OLED display and web portal simultaneously.  
6. **Wait 1 second** between letters for automatic decoding.
7. **Wait 4 seconds** after last input for message finalization.  

### Tips for Best Results
- Calibrate touch sensitivity by adjusting `THRESHOLD_FACTOR` (default 0.7)
- Use consistent pressure for reliable dot/dash detection
- Practice timing: quick taps for dots, held presses for dashes
- Monitor the progress bar for visual feedback on press duration

---

## Educational Value

DotDash serves as an excellent learning tool for:

- **Embedded Systems:** ESP32 programming, peripheral interfacing
- **Digital Logic Design:** Boolean algebra, combinational circuits, gate-level implementation
- **Web Technologies:** Server-client communication, real-time updates
- **Communication Systems:** Morse code protocol, encoding/decoding
- **Human-Computer Interaction:** Touch sensing, visual feedback, progress indicators

The digital logic simulation demonstrates how software can model hardware circuits, bridging the gap between theoretical computer science and practical embedded applications.

---

## Future Enhancements

- [ ] Add support for numbers (0-9) and punctuation
- [ ] Implement message history and playback
- [ ] Add audio feedback (buzzer/speaker)
- [ ] Multi-user communication mode
- [ ] FPGA implementation of logic gates
- [ ] Mobile app integration via Bluetooth
- [ ] Machine learning for adaptive timing calibration

---

## License

This project is open-source. Feel free to modify and distribute with attribution.

---

## Contributors  
<table>
	<tr align="center">
		<td>
			Upayan Mazumder
			<p align="center">
				<img src="https://upayan.dev/upayan.webp" width="150" height="150" alt="Upayan Mazumder">
			</p>
			<p align="center">
				<a href="https://github.com/upayanmazumder">
					<img src="http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height="36" alt="GitHub"/>
				</a>
				<a href="https://www.linkedin.com/in/upayanmazumder">
					<img src="http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
		<td>
			Atharva Sharma
			<p align="center">
				<img src="https://github.com/atharvaSharma17.png" width="150" height="150" alt="Atharva Sharma">
			</p>
			<p align="center">
				<a href="https://github.com/atharvaSharma17">
					<img src="http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height="36" alt="GitHub"/>
				</a>
				<a href="https://www.linkedin.com/in/atharva-sharma-vit/">
					<img src="http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
		<td>
			Siddharth Verma
			<p align="center">
				<img src="https://github.com/SiddharthVerma1384.png" width="150" height="150" alt="Siddharth Verma">
			</p>
			<p align="center">
				<a href="https://github.com/SiddharthVerma1384">
					<img src="http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height="36" alt="GitHub"/>
				</a>
				<a href="https://www.linkedin.com/in/siddharth-verma-9a8823368/">
					<img src="http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
    <td>
			Divyansh Saxena
			<p align="center">
				<img src="https://avatars.githubusercontent.com/u/176986720?v=4" width="150" height="150" alt="Divyansh Saxena">
			</p>
			<p align="center">
				<a href="https://github.com/DIVYANSHSAXENA23">
					<img src="http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height="36" alt="GitHub"/>
				</a>
				<a href="linkedin.com/in/divyansh-saxena-2b48a7308">
					<img src="http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
	</tr>
</table>

---

## Acknowledgments

This project was developed as part of the coursework at **VIT Vellore** under the guidance of **Dr. Naveen Mishra**.

# DotDash  

**ESP32-Based Real-Time Morse Code Communication System**  

---

## Overview  

DotDash is a **real-time Morse code communication system** implemented entirely on an **ESP32**, combining **touch input**, **OLED display**, and a **live web portal**.  

Users input Morse code through a **touch-sensitive pin**, and the system provides **instant feedback**:  

- **Line 1 (OLED):** Scrolling dots and dashes representing raw Morse input  
- **Line 2 (OLED):** Decoded letters as they are interpreted  
- **Progress bar (OLED):** Visual indicator of touch duration, showing dot vs dash  

The **local web interface** mirrors the OLED display, updating live:  

- **Raw Morse** in one section  
- **Decoded letters** in another  

DotDash automatically detects:  

- **Letter gaps (1s):** Converts current Morse token to a letter  
- **End of input (5s):** Finalizes the message display on OLED and in the web portal  

Additionally, live **Serial Monitor output** mirrors the OLED and web portal, enabling debugging or monitoring via USB.

---

## Key Features  

1. **Touch-Based Morse Input:** Single pin differentiates dots (`.`) and dashes (`-`) based on press duration.  
2. **OLED Feedback:** Live scrolling display of Morse symbols and decoded letters.  
3. **Progress Visualization:** Horizontal bar indicates touch duration; glow appears after DOT threshold is crossed.  
4. **Web Interface:** Access via ESP32 Wi-Fi AP; live updates of Morse and decoded letters.  
5. **Automatic Decoding:** Detects letter gaps and message completion automatically.  
6. **Configurable Timings:** Adjust `DOT_TIME`, `CHAR_GAP`, `END_GAP`, and other parameters to suit user preference.  
7. **Serial Monitor Integration:** Real-time output of Morse and decoded letters for development and monitoring.  

---

## Implementation Notes  

- **ESP32 Responsibilities:** Wi-Fi Access Point, DNS redirect, web server, OLED control, touch input reading, scrolling logic, progress bar animation, and live Serial output.  
- **Touch Input:** Uses `touchRead()` to detect touch events and differentiate dots vs dashes.  
- **OLED Display:** Scrolling Morse line, decoded letter line, and touch progress bar with visual DOT threshold indicator.  
- **Web Portal:** Two separate divs for live Morse symbols and decoded letters; updates every 200ms.  
- **Timing Parameters:**  

| Parameter     | Default | Description                  |
|---------------|---------|------------------------------|
| `DOT_TIME`    | 200 ms  | Max duration for a dot       |
| `DASH_TIME`   | 3 × DOT_TIME | Max duration for a dash |
| `CHAR_GAP`    | 1 s     | Gap to detect end of letter  |
| `END_GAP`     | 5 s     | Gap to detect end of message |

- **Morse Table:** Supports A–Z and 0–9; easily extendable.  

---

## Serial Monitor Example  

```

Pressed!
Released! Duration: 150 ms | Symbol: .
Current token: .
Decoded char: E
Live Morse: . | Letters: E
Message complete: HELLO

```

---

## Web Portal Example  

- **Raw Morse:** `. - ..`  
- **Decoded Letters:** `HEL`  

The portal updates every 200ms for near real-time feedback.

---

## Usage  

1. Flash the ESP32 with the DotDash firmware.  
2. Connect to the Wi-Fi network `DotDash`.  
3. Open a browser and navigate to `http://192.168.4.1/`.  
4. Touch the pad to input Morse code:  
   - Short press = dot  
   - Long press = dash  
5. Watch the OLED and web portal for live feedback.  
6. Wait 1 second for letter decoding or 5 seconds for the message to finalize.  

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
	</tr>
</table>  

---
```

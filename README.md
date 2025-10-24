# DotDash  

**Design and Implementation of an ESP32-Based Morse Communication System**  

---

## Overview  

DotDash is a **Morse code communication system** built entirely on an **ESP32**, with wireless networking, user interfacing, and display.  

Users input Morse code via a **touch-sensitive pad**, and the system provides immediate feedback on a **128x64 OLED**:  

- **Line 1:** Scrolling dots and dashes representing raw Morse input  
- **Line 2:** Decoded letters as they are interpreted  
- **Progress bar:** Shows how long a touch must be held to differentiate **dot** vs **dash**  

A **local web portal** displays live Morse input and translated text in separate, clearly formatted sections.  

The system automatically detects:  

- **Letter gaps** (1s) to update decoded letters  
- **End of input** (5s) to finalize the message display on OLED and web  

This design demonstrates **real-time Morse code input and translation** using just an ESP32, providing:  

- Immediate visual feedback for input duration  
- Real-time translation of Morse code to readable text  
- Web-based monitoring and accessibility  

---

## Key Features  

1. **Touch-Based Input:** Single touch pin differentiates dots and dashes with precise timing.  
2. **OLED Feedback:** Scrolling Morse and decoded letters for immediate user feedback.  
3. **Web Interface:** Live update of both raw Morse and translated text.  
4. **Automatic Decoding:** Letters decoded after 1s of inactivity, final message finalized after 5s.  
5. **Configurable Timing:** DOT/DASH thresholds, letter gaps, and end-of-message delays adjustable in code.  

---

## Implementation Notes  

- **ESP32:** Handles Wi-Fi Access Point, web server, OLED display, touch reading, scrolling logic, and progress bar animation  
- **Web Portal:** Separate divs for raw Morse (`.`/`-`) and decoded letters  
- **Timing Parameters:** `CHAR_GAP = 1s`, `END_GAP = 5s` for responsive letter decoding and final message display  
- **Visual Feedback:** Horizontal progress bar shows touch duration for dot vs dash  

---

## Contributors

<table>
	<tr align="center">
		<td>
		Upayan Mazumder
		<p align="center">
			<img src = "https://upayan.dev/upayan.webp" width="150" height="150" alt="Upayan Mazumder">
		</p>
			<p align="center">
				<a href = "https://github.com/upayanmazumder">
					<img src = "http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height = "36" alt="GitHub"/>
				</a>
				<a href = "https://www.linkedin.com/in/upayanmazumder">
					<img src = "http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
		<td>
		Upayan Mazumder
		<p align="center">
			<img src = "https://upayan.dev/upayan.webp" width="150" height="150" alt="Upayan Mazumder">
		</p>
			<p align="center">
				<a href = "https://github.com/upayanmazumder">
					<img src = "http://www.iconninja.com/files/241/825/211/round-collaboration-social-github-code-circle-network-icon.svg" width="36" height = "36" alt="GitHub"/>
				</a>
				<a href = "https://www.linkedin.com/in/upayanmazumder">
					<img src = "http://www.iconninja.com/files/863/607/751/network-linkedin-social-connection-circular-circle-media-icon.svg" width="36" height="36" alt="LinkedIn"/>
				</a>
			</p>
		</td>
	</tr>
</table>

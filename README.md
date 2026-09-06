# ESP32-CAM-Door-Security-System
This project is an automated door security camera built using an ESP32-CAM. It relies on a hardware logic trigger (such as an AND gate) to detect when a door is opened, captures a real-time image, and sends the photo directly to a Telegram chat.

Key Features:
Zero-Delay Capture: Automatically flushes old image frames from the camera's buffer to ensure the captured photo reflects the exact moment the alarm triggered.
Instant Telegram Alerts: Sends a converted live JPEG photo along with a "🚨 SECURITY ALERT: Door Opened!" message to a specified Telegram chat. 
Hardware Logic Trigger: Reads external logic signals via GPIO 14 and utilizes an alarmLatched boolean state to prevent duplicate triggers and spam.  
Buzzer Support: Includes pre-defined pin configurations (GPIO 15) for integrating a low-trigger buzzer. 

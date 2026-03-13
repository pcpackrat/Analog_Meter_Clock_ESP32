# Analog Meter Clock - User Manual

## 1. Initial Wi-Fi Setup

When the clock is powered on for the first time, or if it cannot connect to a previously saved Wi-Fi network, it will enter Setup Mode.

### Steps to Connect:
1. Open the Wi-Fi settings on your smartphone, tablet, or computer.
2. Look for the network named **MeterClock_Config** and connect to it. No password is required.
3. Your device should automatically open a **Captive Portal** login page. If it does not, open a web browser and navigate to `http://10.5.5.5/`.
4. Enter your home Wi-Fi **Network Name (SSID)** and **Password**.
5. Click **Save & Connect**.
6. The clock will display a success message and then restart to connect to your home network. Note the IP address displayed on screen.
7. Reconnect your smartphone or computer to your home Wi-Fi network.
8. Open your web browser and navigate to `http://meterclock.local/` (or the IP address noted in step 6) to access the clock's Dashboard.

If you ever need to change the Wi-Fi credentials, you can navigate to the **WiFi Configuration** page from the Dashboard and click **Clear WiFi Credentials**.

## 2. Web Interface Options

The Dashboard provides access to several configuration pages to customize your clock.

### Time Settings
Configure how the clock keeps and displays time.
* **Primary Timezone**: Select your local timezone from the dropdown list.
* **Secondary Timezone**: Select an alternate timezone. This will be displayed when the secondary timezone toggle switch (hardware) is flipped.
* **NTP Server**: The time server used to automatically sync the time (default: `pool.ntp.org`).
* **Time Source**: 
  * **Automatic (NTP)**: Recommended. Automatically syncs time from the Internet.
  * **Manual**: Allows you to set the date and time manually if the clock is offline. A **Sync Browser Time** button is provided for convenience.
* **Hour Format**: Choose between 12-Hour or 24-Hour display on the Hour meter.

### LED Lighting
Customize the backlight colors and brightness of the meters based on the time of day.
* **Day Color & Brightness**: Select the backlight color and brightness level used during daytime hours.
* **Night Color & Brightness**: Select the backlight color and brightness level used during night mode.
* **Night Schedule**: Set the **Night Start** and **Night End** times. The clock will automatically switch to the Night Color and Night Brightness between these hours.

### Meter Calibration
Calibrate the physical movement of the analog meters to ensure they point accurately to the markings.
* **Enable Calibration Mode**: Check this box to stop normal timekeeping and allow manual control of the needles for alignment.
* **Hour, Minute, and Second Meters**: Each meter has three calibration points:
  * **Min (0)**: Adjust the value until the needle aligns perfectly with 0.
  * **Mid (Halfway)**: Adjust the value until the needle aligns perfectly with the middle of the scale (e.g., 6/12 hours, 30 minutes, 30 seconds).
  * **Max (Full)**: Adjust the value until the needle aligns perfectly with the maximum end of the scale.
* *Note:* Moving focus from a field or changing its value will automatically update the physical meter so you can preview the alignment. Click **Save Calibration** when done.

### Firmware
Keep your clock up to date.
* **Firmware Update**: Click **Choose File** to select a new firmware `.bin` file provided by the developer, then click **Upload Firmware**. The clock will install the update and reboot automatically.

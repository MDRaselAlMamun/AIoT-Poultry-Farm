# Remote Access to Raspberry Pi OS

This file explains two beginner-friendly ways to access **Raspberry Pi OS remotely**:

1. **Raspberry Pi Connect** (remote desktop + remote shell in a browser)\
2. **RealVNC Viewer** (remote desktop)

> ✅ Tip: If you only want the simplest “works anywhere” method, use **Raspberry Pi Connect**.

## Method 1 — Remote Access with Raspberry Pi Connect (Browser-based)

### Step 1 — Power up the Raspberry Pi 5

1. Insert the prepared **microSD card** into the Raspberry Pi 5.
2. Connect **Ethernet** (recommended) or ensure **Wi-Fi** will be available (if you preconfigured it in Imager).
3. (Optional) Connect a **monitor** and **keyboard/mouse** if you want to see the desktop locally.
4. Plug in a **USB-C power supply (5V⎓5A recommended)** to the Raspberry Pi 5.
5. Wait for the Pi to boot (Green LED).

### Step 2 — Sign in on your computer (anywhere)

1. Open a browser on your laptop/PC.
2. Go to: **[connect.raspberrypi.com](https://connect.raspberrypi.com/devices)**
3. Sign in with your **Raspberry Pi ID**. [Open In **1. Operating System (OS) Install** Phase]
4. You’ll see your linked devices.

![Raspberry Pi Connect devices list](images/devices.png)

### Step 3 — Connect to your Raspberry Pi

1. Find your Raspberry Pi in the device list.
2. Click **Connect via** and choose one option:
   - **Screen sharing** (remote desktop), or
   - **Remote shell** (browser terminal)

> If using **Remote shell**, you may briefly see a “waiting/connecting” message.

![Raspberry Pi Connect remote shell waiting](images/waiting-for-remote-shell.png)

> After the Raspberry Pi finishes booting, you should arrive at the **Raspberry Pi OS desktop (Home screen)**.

![Raspberry Pi Home](images/hero.png)

## Method 2 — Remote Desktop with RealVNC Viewer on Windows

This method uses **RealVNC Viewer on Windows** to control the **Raspberry Pi OS desktop** over your **local network (LAN)**.

> **Important (Wayland vs X11):**
>
> - **Raspberry Pi Connect Screen sharing needs Wayland** and is **not compatible with X11**. If you switch to X11 to use RealVNC Server, Connect **Screen sharing will stop**, but **Remote shell will still work**.  
> - RealVNC Server itself does **not support Wayland**, so for the “classic” RealVNC experience, switch to **X11** first.  
> Sources: Raspberry Pi Remote Access docs + Raspberry Pi Connect docs + RealVNC Help Center.

### Requirements

- Raspberry Pi 5 is powered on and connected to the internet/LAN.
- You can sign in to **connect.raspberrypi.com** (Raspberry Pi ID).
- You know the Raspberry Pi OS **username + password**.
- Windows PC is on the **same network** as the Raspberry Pi (for direct IP connection).

### Step 1 — Access Raspberry Pi using Raspberry Pi Connect (Remote Shell)

1. On your Windows PC, open a browser and go to: **connect.raspberrypi.com**
2. Sign in with your **Raspberry Pi ID**.
3. Find your Raspberry Pi in the device list.
4. Click **Connect via** → **Remote shell**.

![Raspberry Pi Connect device list](images/remote-shell.png)

> Use **Remote shell** here because we may switch to **X11**, which disables Connect **Screen sharing**.

### Step 2 — Prepare Raspberry Pi for RealVNC (Recommended Path: Switch to X11)

> This is the most reliable way to use **RealVNC Viewer** with Raspberry Pi OS Bookworm.

#### 2.1 Switch from Wayland to X11

In the **Raspberry Pi Connect Remote shell**, run:

```bash
sudo raspi-config
```

Then navigate:

- **Advanced Options** → **Wayland** → select **X11**
- Confirm and **Reboot** when prompted

✅ **After reboot:**

- **Raspberry Pi Connect Remote shell** still works
- **Raspberry Pi Connect Screen sharing** will **not** work (expected)

---

### Step 3 — Enable VNC using `raspi-config` (turn on RealVNC Server in X11)

After the reboot, open **Connect Remote shell** again and run:

```bash
sudo raspi-config
```

Navigate:

- **Interface Options** → **VNC**
- Choose **Yes** to enable VNC
- Exit **raspi-config**

> **Optional (recommended for headless):** Set a VNC screen size so the desktop isn’t tiny.

In **raspi-config**:

- **Display Options** → **VNC Resolution** → choose a resolution (e.g., **1920×1080**)

### Step 4 — Find the Raspberry Pi IP address (2 easy options)

#### Option A (Fastest) — Use Raspberry Pi Connect Remote Shell

Run:

```bash
hostname -I
```

You’ll get something like:

- `192.168.1.50`

#### Option B — Use Advanced IP Scanner on Windows

1. Download **Advanced IP Scanner** (Windows).
2. Install/run it (it can run without full installation).
3. Click **Scan**.
4. Look for your Pi by:
   - the hostname you set (example: `pi5-home`), or
   - manufacturer/vendor showing **Raspberry Pi**, or
   - matching IP range (example: `192.168.1.x`)

### Step 5 — Install RealVNC Viewer on Windows

1. Download **RealVNC Viewer for Windows** from the official RealVNC download page.
2. Run the installer.
3. Click **Next** → accept the license → **Install** → **Finish**.
4. Open **RealVNC Viewer**.

### Step 6 — Connect using RealVNC Viewer (Windows → Raspberry Pi)

1. Open **RealVNC Viewer**.
2. In the address bar at the top, enter your Pi IP address:
   - Example: `192.168.1.50`
3. Press **Enter** (or click **Connect**).
4. If a security prompt appears, confirm/accept to continue.
5. When asked to log in:
   - Enter your Raspberry Pi OS **username**
   - Enter your Raspberry Pi OS **password**

✅ If successful, you will see the **Raspberry Pi OS desktop** and can control it with your mouse and keyboard.

### Troubleshooting (Quick Fixes)

#### If you can’t connect

- Confirm the Pi and Windows PC are on the **same network**.
- Re-check that VNC is enabled:
  - `sudo raspi-config` → **Interface Options** → **VNC** → **Enable**
- If headless and desktop looks wrong, set:
  - `sudo raspi-config` → **Display Options** → **VNC Resolution**

#### If you MUST keep Wayland (and keep Connect Screen sharing)

RealVNC Server won’t work on Wayland, but you can connect RealVNC Viewer to **WayVNC** with extra configuration (RSA-AES key).  
*(Use this only if you must stay on Wayland.)*

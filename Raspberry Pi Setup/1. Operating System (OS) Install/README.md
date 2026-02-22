# Raspberry Pi 5 Setup Guide

This guide will help you set up a **Raspberry Pi 5** using a **microSD card**. You’ll format the SD card first, then use **Raspberry Pi Imager** to install the operating system.

## What You Need

- Raspberry Pi 5
- microSD card (**32GB or more** recommended, Class 10 / UHS-I)
- microSD card reader (built-in slot or USB adapter)
- Laptop/Desktop (Windows/macOS/Linux)
- **Power supply for Raspberry Pi 5** (recommended **high-quality 5V⎓5A USB-C**, e.g., official **27W** PSU)
- Monitor + HDMI cable (optional; only needed for a desktop/monitor setup)
- Keyboard + mouse (optional; only needed for a desktop/monitor setup)
- Internet connection (Wi-Fi or Ethernet)

## Part A — Full Format the SD Card

> **Tip:** This step is optional. Raspberry Pi Imager can erase and repartition the card.
> Use SD Card Formatter if your card has problems, previous OS partitions, or Imager fails verification.

### 1) Download SD Card Formatter

✅ Download the official **SD Card Formatter** tool here:

- SD Association SD Card Formatter: https://www.sdcard.org/downloads/formatter/

### 2) Insert SD Card Into Your Computer

1. Take your **microSD card**
2. Insert your **microSD card** into your laptop/desktop.
3. If your computer doesn’t have a microSD slot, use a **USB card reader**.

### 3) Full Format the SD Card (Overwrite)

1. Open **SD Card Formatter**.
2. Select your **SD card** from the list.

   > ⚠️ **Warning:** Be sure you select the correct drive. Formatting erases everything.

3. Choose:
   > **Option:** *Overwrite Format* (full format)  
     *(This is the “full format” option and is more reliable than quick format.)*
4. Click **Format**.
5. Wait for it to finish, then close the formatter.

## Part B — Install Raspberry Pi OS Using Raspberry Pi Imager

### Step 1: Install and Launch Imager

Download **Raspberry Pi Imager** here:

> Raspberry Pi Imager: https://www.raspberrypi.com/software/

1. Install and Open **Raspberry Pi Imager**
2. If prompted, allow administrator/root permissions (needed to write to storage devices).

### Step 2: Configure the Fundamentals

Tell Raspberry Pi Imager what combination of hardware and operating system you want to use:

> 1. In the **Device tab**, select your Raspberry Pi model **(Raspberry Pi 5)** from the list. Select Next.

![Raspberry Pi 5 device](images/choose-device.png)

> 2. In the **OS tab**, choose from the available operating systems (**Raspberry Pi OS (64-bit)**). Select Next.

![Raspberry Pi 5 os](images/choose-os.png)

> 3. In the **Storage tab**, select the storage device to write the image to. Select Next.


![Raspberry Pi 5 stortage](images/choose-storage.png)

> ⚠️ **Warning**
>
> If you have more than one storage device connected, choose carefully (size is a good clue).
>
> If unsure, disconnect other removable drives first.


### Step 3: Customise Your OS (Recommended)

Raspberry Pi Imager can preconfigure key settings **before first boot**.

> These steps are optional, but strongly recommended—especially for headless setup.

#### You can preconfigure

- Hostname
- Time zone
- Keyboard layout
- Username and password
- Wi-Fi credentials
- Remote access (SSH)
- Raspberry Pi Connect (optional)

> 1. **Customisation → Hostname:** set a hostname (letters, numbers, hyphens only).

![Raspberry Pi 5 hostname](images/os-customisation-hostname.png)

> 2. **Customisation → Localisation:** choose your city/time zone and keyboard layout. 

![Raspberry Pi 5 local](images/os-customisation-locale.png)

> 3. **Customisation → User:** create your username and password.  

![Raspberry Pi 5 user](images/os-customisation-user.png)

> 4. **Customisation → Wi-Fi:** enter SSID and password (skip if using Ethernet). 

![Raspberry Pi 5 wifi](images/os-customisation-wifi-secure.png)

> 5. **Customisation → Remote Access (SSH):** enable SSH if you want remote login.
>  
> - **Use password authentication** (simple) or SSH keys (more secure). 

![Raspberry Pi 5 ssh](images/os-customisation-ssh.png)

> 6. Some operating systems now show the **Customisation > Raspberry Pi Connect tab**.
>
> - To link your device to your Raspberry Pi Connect account, complete the following steps:

> **Toggle the Enable Raspberry Pi Connect** switch to the active position.

![Raspberry Pi 5 connect](images/os-customisation-connect.png)

> Select **Open Raspberry Pi Connect**. The Raspberry Pi Connect website opens in your default browser.

![Raspberry Pi 5 new](images/new.png)

1. On the page, click **Create one for free**.
2. The registration form will open.

> Fill in the Raspberry Pi ID form

![Raspberry Pi 5 reg](images/reg-form.png)

- Enter your details (email, password, and any other required information).
- Double-check your email address before continuing.

> Continue and verify your email

1. Click **Continue**.
2. Raspberry Pi will send a confirmation email to your email address.
3. Open your email inbox and find the message from Raspberry Pi.
   - If you don’t see it, check **Spam/Junk/Promotions**.
4. Click the **confirmation/verification link** in the email.

![Raspberry Pi 5 confirm](images/confirm.png)

> Log in to your Raspberry Pi ID account

After confirming your email, return to this link and log in:

- **Raspberry Pi ID Login:** https://id.raspberrypi.com/sign-in

![Raspberry Pi 5 login](images/login.png)

> Sign in to your **Raspberry Pi ID account** or, if you don’t yet have an account, sign up.

> On the **New auth key** page, create your auth key.

The auth key is a single-use, temporary token. The Raspberry Pi Connect website displays how long after its creation the token expires. To use the token, ensure that you boot your Raspberry Pi and connect it to the internet before the expiry time.

- If your Raspberry Pi ID account isn’t a member of any organisations, select Create auth key and launch Raspberry Pi Imager.

- If you are a member of one or more organisations, select the organisation or account to associate the key with. Then select Create auth key and launch Raspberry Pi Imager.

![Raspberry Pi 5 auth](images/connect-auth-key.png)

Your browser might ask you whether you want to allow the site to open Raspberry Pi Imager. Confirm that you want to open Raspberry Pi Imager.

Raspberry Pi Imager opens at the Raspberry Pi Connect tab. This tab shows a message to confirm that Imager received the authentication token from the browser and a field containing the token. Select Next.

![Raspberry Pi 5 auth](images/os-customisation-connect-token.png)

### Step 4: Write and Verify

After customisation, Imager shows a summary.

> 1. Review your choices and select **Write**.   

   ![Raspberry Pi 5 summary](images/summary.png)

> 2. Confirm the warning prompt to erase and write.  

   ![Raspberry Pi 5 confirm](images/are-you-sure.png)

> 3. Wait while Imager writes the image.  

   ![Raspberry Pi 5 writing](images/writing.png)

>4. Let Imager **Verify** (recommended).  

   ![Raspberry Pi 5 verify](images/verify.png)

> 5. When finished, select **Finish** and **safely eject** the microSD card.

   ![Raspberry Pi 5 finished](images/finished.png)

## Next: First Boot (Quick Checklist)

1. Insert the microSD card into the Raspberry Pi 5.
2. Connect network (Ethernet or Wi-Fi if preconfigured).
3. Connect monitor/keyboard/mouse (optional).
4. Power on.

> **Note:** After imaging, your computer may prompt you to format the card. **Do not format it**—that’s normal because it now contains Linux partitions.

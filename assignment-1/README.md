Assignment 1 – VM, Docker, and Containerized App

Deliverables (10 marks total)

- VM setup (2.5): Ubuntu VM running, screenshot of specs + VM window.
- Docker install (2.5): Run `docker run hello-world`, screenshot of success.
- App deploy (5): Build and run a simple web app in Docker on port 5000 or 8080; screenshot of running container output or browser page.

What’s included in this folder

- `assignment-1/python` – Minimal Flask app (port 5000) + Dockerfile.
- `assignment-1/node` – Minimal Express app (port 8080) + Dockerfile.
  Use either Python or Node for the assignment.

1) VM Setup (Ubuntu on VirtualBox/VMware)

- Download Ubuntu Desktop ISO (24.04.1 LTS recommended) from the official Ubuntu site.
  - For typical PCs/Intel Macs: use `amd64` (x86_64) ISO.
  - For ARM PCs/boards: use `arm64` ISO.
  - Note: Apple Silicon Macs cannot bare‑metal boot Ubuntu Desktop from USB; use a VM (UTM/VMware) instead.
- Create a new VM in VirtualBox/VMware (details below):
  - Name: `Ubuntu-Edge`
  - vCPU: 2 (or more if available)
  - RAM: 4096 MB (or more)
  - Disk: 30 GB (dynamically allocated)
  - Attach the Ubuntu ISO and boot. At the boot menu, pick “Try or Install Ubuntu (safe graphics)” if you see display issues.

  VirtualBox (Intel/x86_64):
  - Type/Version: Linux → Ubuntu (64-bit)
  - System: Enable EFI (recommended), Chipset ICH9 (default is fine)
  - Display: Video Memory 128 MB; 3D Acceleration optional
  - Storage: Controller SATA → add virtual disk (VDI, dynamically allocated) → attach ISO to Optical Drive
  - Network: NAT (default) is fine

 VMware Fusion (Intel):
  - Create new VM → Install from disc or image… → select ISO
  - OS type: Ubuntu 64-bit
  - Processors & Memory: 2 vCPU, 4–8 GB RAM
  - Display: Enable 3D Acceleration; leave defaults otherwise

  Apple Silicon (M1/M2/M3):
  - VirtualBox desktop support is limited; VMware Fusion 13+ or UTM is recommended
  - VMware Fusion: OS type Ubuntu 64-bit (ARM), 2–4 vCPU, 4–8 GB RAM, Graphics Auto; use “safe graphics” at boot if needed
  - UTM: Use “Virtualize” → Linux, attach ARM64 ISO; defaults work

VMware Fusion 13 (Apple Silicon) – Install on macOS

- Download: Get VMware Fusion 13 (Apple Silicon/ARM build). Choose Fusion Player (free for personal use) or Fusion Pro. If using Player, register and retrieve your free Personal Use License key.
- Install: Open the downloaded `.dmg`, drag VMware Fusion to `Applications`.
- First launch permissions: When prompted, approve VMware system software and network extensions.
  - System Settings → Privacy & Security → Allow items from “VMware, Inc.” (you may need to unlock with your password) and restart if requested.
  - Grant network extension/packet filter permissions when Fusion asks.
- Licensing: On first run, enter your Player license key (or start trial/Pro if applicable).

Create an Ubuntu ARM VM in Fusion (M1/M2/M3)

- New VM → Install from disc or image… → select `ubuntu-24.04.1-desktop-arm64.iso` (or your chosen ARM64 ISO).
- OS: Linux → Ubuntu 64‑bit (ARM). Resources: 2–4 vCPU, 4–8 GB RAM, 30 GB disk. Graphics: defaults are fine.
- Boot the VM; if you see a blank/garbled screen at the Ubuntu menu, select “Try or Install Ubuntu (safe graphics)”.
- After install, improve integration inside Ubuntu:
  - `sudo apt update && sudo apt install -y open-vm-tools open-vm-tools-desktop`
  - Reboot the VM to enable clipboard/resolution features.
- Install Ubuntu with defaults. After first boot, update packages:
  `sudo apt update && sudo apt upgrade -y`
- (VirtualBox) Install Guest Additions for better display/clipboard:
  Devices > Insert Guest Additions CD image… then run the installer inside the VM.
- Show system specs in the VM for your screenshot (any 1–2 commands is fine):
  - `Settings > About` in Ubuntu GUI
  - `hostnamectl`
  - `lsb_release -a`
  - `lscpu | head` and `free -h`
- Screenshot to capture: The VM desktop plus a window or terminal with system specs.

1a) Optional: Try/Install Ubuntu from USB (Bare Metal)

- Download the Ubuntu Desktop ISO (24.04.1 LTS recommended).
- Create a bootable USB (8GB+; it will be erased):
  - Cross‑platform GUI: balenaEtcher → Select ISO → Select USB → Flash.
  - Windows: Rufus → Select ISO → Partition scheme `GPT`, Target system `UEFI` → Start.
  - Linux/macOS CLI (advanced): identify the USB device, then
    - macOS: `diskutil list` → `diskutil unmountDisk /dev/diskX` → `sudo dd if=~/Downloads/ubuntu.iso of=/dev/rdiskX bs=4m conv=sync status=progress`
    - Linux: `lsblk` to find `/dev/sdX` → `sudo dd if=~/Downloads/ubuntu.iso of=/dev/sdX bs=4M conv=fsync status=progress`
- Boot from USB:
  - PC: Reboot → open Boot Menu (F12/F10/Esc/Del varies) → pick the USB (UEFI). If you see a black screen, choose “safe graphics”. You may need to disable Secure Boot on some hardware.
  - Intel Mac: Reboot holding `Option` → choose “EFI Boot”.
  - Apple Silicon Mac: Not supported for Ubuntu Desktop via USB; use a VM (UTM/VMware) or consider Asahi Linux.
- In the Ubuntu menu, choose “Try Ubuntu” to run live without installing; when ready click “Install Ubuntu”. Back up data first if you plan to install.
- Optional integrity check: verify the ISO checksum (`shasum -a 256 <iso>` on macOS, `certutil -hashfile <iso> SHA256` on Windows) and compare with Ubuntu’s published SHA256.

2) Install Docker

- Ubuntu (recommended inside your VM):
  1. `sudo apt-get update`
  2. `sudo apt-get install -y ca-certificates curl gnupg`
  3. `sudo install -m 0755 -d /etc/apt/keyrings`
  4. `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
  5. `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
  6. `sudo apt-get update`
  7. `sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin`
  8. Add your user to the docker group, then refresh the session:
     - `sudo usermod -aG docker $USER`
     - `newgrp docker`
  9. Test: `docker run hello-world`

- macOS: Install Docker Desktop from docker.com, start it, then run `docker run hello-world` in Terminal.
- Windows: Install Docker Desktop (with WSL2) and run `docker run hello-world` in PowerShell.
- Screenshot to capture: Terminal showing successful `docker run hello-world` output.

3) Containerize and Run the Web App

Option A: Python (Flask, port 5000)

- Build:
  - `cd assignment-1/python`
  - `docker build -t edge-flask:1 .`
- Run (maps port 5000 on your machine to the container):
  - `docker run --rm -p 5000:5000 --name edge-flask edge-flask:1`
- Verify:
  - Open `http://localhost:5000` in a browser, or
  - `curl http://localhost:5000`
- Screenshot to capture: Browser or terminal showing the JSON response.

Option B: Node.js (Express, port 8080)

- Build:
  - `cd assignment-1/node`
  - `docker build -t edge-node:1 .`
- Run (maps port 8080 on your machine to the container):
  - `docker run --rm -p 8080:8080 --name edge-node edge-node:1`
- Verify:
  - Open `http://localhost:8080` in a browser, or
  - `curl http://localhost:8080`
- Screenshot to capture: Browser or terminal showing the JSON response.

Helpful Checks and Extras

- Show running containers: `docker ps`
- Show images: `docker images`
- Stop container (if not using `--rm`): `docker stop <name-or-id>`
- If ports are in use, change the left side of `-p` (e.g., `-p 5001:5000`).
- On Apple Silicon (M1/M2), official Python/Node images support arm64; if you hit image arch issues, you can add `--platform linux/amd64` to the `docker build` and `docker run` commands.

Submission Checklist

- VM: Screenshot of Ubuntu VM running with visible system specs.
- Docker: Screenshot of successful `docker run hello-world` output.
- App: Screenshot of the running container’s output (browser or curl) on port 5000 or 8080.

Files you will use

- Python app: `assignment-1/python/app.py`, `assignment-1/python/Dockerfile`.
- Node app: `assignment-1/node/server.js`, `assignment-1/node/Dockerfile`.

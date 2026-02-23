# Building a Virtual Hardware Device from Scratch using MMIO in QEMU

**Author:** Ahmed Dajani (<adajani@iastate.edu>)

**Read more:** [Linkedin Article](https://www.linkedin.com/pulse/building-virtual-hardware-device-from-scratch-using-mmio-ahmed-dajani-vhigc/)

---

## Device Concept: MMIO Calculator

### Design Choice
MMIO → registers (a, b, op, result, status)

### Register Map (Memory-Mapped)

| Offset | Register | Description                     |
| ------ | -------- | ------------------------------- |
| 0x00   | A        | Operand A                       |
| 0x04   | B        | Operand B                       |
| 0x08   | OP       | Operation (0=+, 1=-, 2=*, 3=/)  |
| 0x0C   | CTRL     | Write `1` → start               |
| 0x10   | RESULT   | Result                          |
| 0x14   | STATUS   | 0=idle, 1=busy, 2=done, 3=error |

---

## Step 1: Install QEMU Dependencies
```bash
sudo apt update
sudo apt install -y \
  git build-essential ninja-build pkg-config \
  libglib2.0-dev libpixman-1-dev \
  python3 python3-venv \
  libfdt-dev zlib1g-dev
pip install tomli
```

## Step 2: Clone and Build QEMU from Source
```bash
git clone https://gitlab.com/qemu-project/qemu.git
cd qemu
git checkout 07f97d5da04a9f97e273de85c76f5017d8135a6e
```

### Apply the MMIO Calculator Patch
The MMIO calculator device is delivered as a patch against QEMU.
```bash
# Inside the qemu/ directory
git apply qemu.diff
```

### Configure (x86_64 softmmu target):
```bash
./configure --target-list=x86_64-softmmu
```

### Build:
```bash
make -j$(nproc)
```

---

## Step 3: Build the Linux Kernel

### Install prerequisites:
```bash
sudo apt update
sudo apt install -y git build-essential libncurses-dev bison flex libssl-dev bc \
    zlib1g-dev libelf-dev
```

### Get Linux kernel source:
```bash
git clone https://github.com/torvalds/linux.git
cd linux
git checkout v6.6
```

### Configure for QEMU (x86_64):
```bash
make ARCH=x86_64 defconfig
```

### Optional — open menuconfig for custom options:
```bash
make ARCH=x86_64 menuconfig
```

### Build the kernel:
This creates `arch/x86/boot/bzImage`.
```bash
make -j$(nproc) ARCH=x86_64
```

---

## Step 4: Build BusyBox (Minimal Root Filesystem)

### Clone and apply default config:
```bash
git clone https://git.busybox.net/busybox
cd busybox
make defconfig
```

### Enable static linking:
```bash
echo 'CONFIG_STATIC=y' >> .config
```

> **menuconfig alternative:** `make menuconfig` → Settings → Build Options →
> **Build static binary (no shared libs)** → enable with spacebar → Save → Exit

### Build and install into rootfs:
This populates `../rootfs` with `/bin`, `/sbin`, `/usr`, etc.
```bash
make -j$(nproc)
make CONFIG_PREFIX=../rootfs install
cd ..
```

---

## Step 5: Build the Test Program

Create a `calc_test.c` in any directory, and copy the binary to `rootfs/bin` folder.

### Compile statically and install into rootfs:
```bash
gcc -static -o rootfs/bin/calc_test calc_test.c
```

---

## Step 6: Create the Root Filesystem Image

```bash
cd rootfs

mkdir -p proc sys dev tmp

# Create /init script — PID 1, runs before the shell
cat > init << 'EOF'
#!/bin/sh
mount -t proc     proc     /proc
mount -t sysfs    sysfs    /sys
mount -t devtmpfs devtmpfs /dev
exec /bin/sh
EOF
chmod +x init

# Pack and compress
find . | cpio -H newc -o > ../rootfs.cpio
cd ..
gzip rootfs.cpio
```

This creates `rootfs.cpio.gz` — the initramfs loaded by QEMU at boot.

---

## Step 7: Boot QEMU with the Device and RootFS

```bash
./qemu/build/qemu-system-x86_64 \
  -kernel linux/arch/x86/boot/bzImage \
  -initrd rootfs.cpio.gz \
  -append "console=ttyS0 root=/dev/ram rdinit=/init" \
  -nographic
```

Once the BusyBox shell appears, run the test program:
```bash
/bin/calc_test 10 '*' 5
```
---

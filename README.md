                           
# PiHole - LogReader

![GitHub Release](https://img.shields.io/github/v/release/pkimSec/pihole-logfile-reader?include_prereleases)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues-raw/pkimsec/pihole-logfile-reader)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues-pr/pkimsec/pihole-logfile-reader)


A simple project in C that can be used to read through PiHole-LogFiles and Search for Strings and apply different filters like "gravity blocked" or "reply" or search in specific time frames. The project uses a simple GTK GUI and has a Help Menu where every function is explained, if not self-explanatory.
 
# Quick Start Demo

![Alt text](https://github.com/user-attachments/assets/dc25e994-33dc-47de-95b6-32b5cfbb30c7)


You can either use the latest pre-compiled version from the releases or compile it yourself using gcc.
 
# Installation

Step 1: Clone the repo to your local machine
```
git clone https://github.com/pkimSec/pihole-logfile-reader
```
Step 2: Enter into the cloned directory
```
cd pihole-logfile-reader
```
Step 3: Install dependencies using package manager (Example on Arch Linux using pacman)
```
sudo pacman -S - < dependencies
```
Step 4: Make compile-script executable 
```
chmod +x compile.sh
```
Step 5: Run compile script
```
./compile.sh
```

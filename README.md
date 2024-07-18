                           
# PiHole - LogReader

[![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/navendu-pottekkat/awesome-readme?include_prereleases)](https://img.shields.io/github/v/release/navendu-pottekkat/awesome-readme?include_prereleases)
[![GitHub last commit](https://img.shields.io/github/last-commit/navendu-pottekkat/awesome-readme)](https://img.shields.io/github/last-commit/navendu-pottekkat/awesome-readme)
[![GitHub issues](https://img.shields.io/github/issues-raw/navendu-pottekkat/awesome-readme)](https://img.shields.io/github/issues-raw/navendu-pottekkat/awesome-readme)
[![GitHub pull requests](https://img.shields.io/github/issues-pr/navendu-pottekkat/awesome-readme)](https://img.shields.io/github/issues-pr/navendu-pottekkat/awesome-readme)
[![GitHub](https://img.shields.io/github/license/navendu-pottekkat/awesome-readme)](https://img.shields.io/github/license/navendu-pottekkat/awesome-readme)

A simple project in C that can be used to read through LogFiles and Search for Strings and apply different filters like "gravity blocked" or "reply" or search in specific time frames. The project uses a simple GTK GUI and has a Help Menu where every function is explained, if not self-explanatory.
 
# Quick Start Demo

![Alt text](https://github.com/user-attachments/assets/dc25e994-33dc-47de-95b6-32b5cfbb30c7)


You can either use the latest pre-compiled version from the releases or compile it yourself using gcc.
 
# Installation

1. Clone the repo to your local machine:
git clone https://github.com/pkimSec/pihole-logfile-reader

2. Go into the cloned directory
cd pihole-logfile-reader

3. Install dependencies (Example on Arch Linux using pacman)
sudo pacman -S - < dependencies

4. Make compile-script executable 
chmod +x compile.sh

5. Run compile script
./compile.sh

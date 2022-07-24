# Installer GUI

## Dependencies
- Python
- [Tk Interface](https://pkgs.org/search/?q=python3-tk) 

### Fedora
```bash
sudo dnf install python3-pillow-tk python3-tkinter
```

### Ubuntu
```bash
sudo apt-get install python3-tk
```

## Running
First things first, make sure you are in [`Editor/Linux/Installer`](Editor/Linux/Installer)

Next, give the `Installer.py` script permission to run with `chmod +x`
```bash
chmod +x Installer.py
```

Now, you can run the sript with Python
```bash
python3 Installer.py
```
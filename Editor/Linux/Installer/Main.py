#!/usr/bin/python3

import Application
from Application import *
import sys


def main():
    try:
        app = InstallerApplication()
        app.mainloop()
    except KeyboardInterrupt:
        app.destroy()
        sys.exit(0)


if __name__ == "__main__":
    main()

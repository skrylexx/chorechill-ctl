# Chore Chill CTL

hand made app just to learn C and save my old MSI from dying

## Architecture

```
fan-control/
├── install.sh                 # Global deployment script
├── Makefile                   # C automation
├── README.md                  # you hare here
├── config/
│   ├── fan-control.service    # systemd to run daemon
│   └── default_config.ini     # profils
├── backend/                   # daemon
│   ├── include/               # head files
│   │   ├── hardware.h         # sig for /sys/class/hwmon/
│   │   ├── controller.h       # sig for calculs
│   │   └── ipc.h              # sig for UNIX Socket
│   ├── src/                   # source files (.c)
│   │   ├── main.c             # Entrypoint (SIGINT)
│   │   ├── hardware.c         # read ° and write RPM
│   │   ├── controller.c       # ome math things
│   │   └── ipc.c              # Unix Socket server to communicate with GUI
│   └── lib/                   # not sure if we will need it but optional libraries
└── frontend/                  # GUI
    ├── requirements.txt       # Dependancies
    └── src/
        ├── main.py            # Entrypoint
        ├── gui/               # visual components
        │   ├── app_window.py  # main window
        │   └── graph.py       # math things
        └── ipc_client.py      # UNIX Socket client to communicate with C code
```
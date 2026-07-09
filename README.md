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
│   └── default_config.ini     # profils, change name for smth like "profils.ini" ?
├── backend/                   # daemon
│   ├── include/               # head files
│   │   ├── hardware.h         # sig for /sys/class/hwmon/
│   │   ├── controller.h       # sig for calculs
│   │   └── ipc.h              # sig for UNIX Socket
│   ├── src/                   # source files (.c)
│   │   ├── main.c             # Entrypoint (SIGINT)
│   │   ├── hardware.c         # read ° and write RPM
│   │   ├── controller.c       # math things
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

## Note
v0 will run for MSI GF63 thin only, or computer with same MO-BO

## Note v2

to find good chore temp :
```sh
ls -l /sys/class/hwmon/
cat /sys/class/hwmon/hwmonX/name #must return smth like coretemp, acpitz)

cat /sys/class/hwmon/hwmonX/temp1_input #can have many "temp_input" but 1 is mainly the good one
```

find fan monitor :


## URIs 
https://docs.kernel.org/wmi/devices/msi-wmi-platform.html

## Addresses
CPU temp (read) : 0x68
CPU fan speed (read) : 0x71
Force fan speed (write) : 0x71

## run
gcc main.c hardware.c ipc.c -o ../compiled/v0
sudo modprobe ec_sys write_support=1
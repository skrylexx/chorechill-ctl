import tkinter as tk
from tkinter import ttk

# local imports
from gui.frontend import FanControllerUI
from ipc.main import IPCClient

def main():
    root = tk.Tk()
    
    # set a cleaner theme
    style = ttk.Style()
    if "clam" in style.theme_names():
        style.theme_use("clam")

    # init the IPC client
    ipc = IPCClient()

    # define what happens when user clicks "Apply"
    def handle_apply(speed):
        print(f"[MAIN] Forcing fan speed to {speed}%")
        # send command to backend
        ipc.send_command(f"SET:{speed}")

    # define what happens when user changes mode
    def handle_mode(mode):
        print(f"[MAIN] Switching to {mode} mode")
        if mode == "auto":
            ipc.send_command("SET:AUTO")

    # init the UI, passing our logic functions
    app = FanControllerUI(root, apply_callback=handle_apply, mode_callback=handle_mode)

    # the polling loop
    def poll_telemetry():
        # ask the C backend for data
        response = ipc.send_command("GET")
        
        if not response.startswith("ERR"):
            # assuming the C backend replies with a simple string like "55,43,2254"
            try:
                data = response.split(",")
                if len(data) == 3:
                    app.update_telemetry(data[0], data[1], data[2])
            except Exception:
                pass
        else:
            # backend is offline or not responding
            app.update_telemetry("--", "--", "--")
            
        # schedule the next execution in 1000 ms (1 second)
        root.after(1000, poll_telemetry)

    # start the infinite loop
    poll_telemetry()
    
    print("[SYSTEM] Starting Frontend with Polling...")
    root.mainloop()

if __name__ == "__main__":
    main()
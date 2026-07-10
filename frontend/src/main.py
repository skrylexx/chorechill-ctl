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

    # define what happens when user clicks "Apply Speed"
    def handle_apply(speed):
        print(f"[MAIN] Forcing fan speed to {speed}%")
        ipc.send_command(f"SET:{speed}")

    # define what happens when user clicks a mode or curve button
    def handle_mode(mode_str):
        print(f"[MAIN] Action requested: {mode_str}")
        
        if mode_str == "restore_default" or mode_str == "auto":
            response = ipc.send_command("SET_DEFAULT")
            #print(f"[MAIN] Backend response: {response}")
            
            if not response.startswith("ERR"):
                app.show_success("✅ Default parameters applied!")
            else:
                app.show_error("❌ Failed to contact backend")

        elif mode_str == "silent_curve":
            response = ipc.send_command("SET_SILENT")
            #print(f"[MAIN] Backend response: {response}")

            if not response.startswith("ERR"):
                app.show_success("✅ Silent curve applied !") # make it dynamic in the future
            else:
                app.show_error("❌ Failed to contact backend")
        
        elif mode_str == "gaming_curve" or mode_str == "auto":
            response = ipc.send_command("SET_GAMING")
            #print(f"[MAIN] Backend response: {response}")
            
            if not response.startswith("ERR"):
                app.show_success("✅ Gaming parameters applied!")
            else:
                app.show_error("❌ Failed to contact backend")

        elif mode_str == "custom_curve":
            response = ipc.send_command("SET_CUSTOM")
            #print(f"[MAIN] Backend response: {response}")

            if not response.startswith("ERR"):
                app.show_success("✅ Custom curve applied !") # make it dynamic in the future
            else:
                app.show_error("❌ Failed to contact backend")

    # init the UI, passing our logic functions
    app = FanControllerUI(root, apply_callback=handle_apply, mode_callback=handle_mode)

    # the polling loop
    def poll_telemetry():
        response = ipc.send_command("GET")
        
        if not response.startswith("ERR"):
            try:
                data = response.split(",")
                if len(data) == 3:
                    app.update_telemetry(data[0], data[1], data[2])
            except Exception:
                pass
        else:
            app.update_telemetry("--", "--", "--")
            
        root.after(1000, poll_telemetry)

    # start the infinite loop
    poll_telemetry()
    
    print("[SYSTEM] Starting Frontend with Polling...")
    root.mainloop()

if __name__ == "__main__":
    main()
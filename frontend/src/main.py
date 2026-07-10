import tkinter as tk
from tkinter import ttk
import json
import os

# local imports
from gui.frontend import FanControllerUI
from ipc.main import IPCClient

def main():
    root = tk.Tk()
    style = ttk.Style()
    if "clam" in style.theme_names():
        style.theme_use("clam")

    ipc = IPCClient()

    def apply_profile(profile_key):
        config_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "config", "profiles.json"))
        
        try:
            with open(config_path, "r") as f:
                data = json.load(f)
                
            if profile_key in data["profiles"]:
                profile = data["profiles"][profile_key]
                
                temps_str = ",".join(map(str, profile["temperatures_c"]))
                speeds_str = ",".join(map(str, profile["speeds_percent"]))
                
                payload = f"SET_CURVE:{temps_str};{speeds_str}"
                
                print(f"[MAIN] Sending to backend : {payload}")
                return ipc.send_command(payload)
            else:
                print(f"[MAIN] Error: Profile '{profile_key}' not found in JSON.")
                return "ERR: Profile not found"
                
        except Exception as e:
            print(f"[MAIN] Error reading JSON: {e}")
            return "ERR: JSON read error"


    def handle_mode(mode_str):
        print(f"[MAIN] Action requested: {mode_str}")
        
        # Dictionnaire qui traduit l'action de l'UI vers la clé exacte du fichier JSON
        json_key_map = {
            "restore_default": "default",
            "auto": "default",
            "silent_curve": "silent",
            "gaming_curve": "gaming",
            "custom_curve": "custom"
        }
        
        if mode_str in json_key_map:
            profile_name = json_key_map[mode_str]
            
            response = apply_profile(profile_name)
            
            if not response.startswith("ERR"):
                app.show_success(f"Profile '{profile_name}' applied !")
            else:
                app.show_error("Backend error")
        else:
            print(f"[MAIN] Error: Unknown mode requested '{mode_str}'")

    def handle_apply(speed):
        print(f"[MAIN] Forcing fan speed to {speed}%")
        ipc.send_command(f"SET:{speed}")

    app = FanControllerUI(root, apply_callback=handle_apply, mode_callback=handle_mode)

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

    poll_telemetry()
    print("[SYSTEM] Starting Frontend with Polling...")
    root.mainloop()

if __name__ == "__main__":
    main()
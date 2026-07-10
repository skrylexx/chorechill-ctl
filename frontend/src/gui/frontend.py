import tkinter as tk
from tkinter import ttk

class FanControllerUI:
    def __init__(self, root, apply_callback=None, mode_callback=None):
        self.root = root
        self.root.title("Chore Chill")
        # Augmentation de la hauteur (520) et largeur (420) pour que le texte rentre largement
        self.root.geometry("420x520") 
        self.root.resizable(False, False)

        # store the functions passed from main.py
        self.apply_callback = apply_callback
        self.mode_callback = mode_callback

        # main container with padding
        main_frame = ttk.Frame(self.root, padding="20 20 20 20")
        main_frame.pack(fill="both", expand=True)

        # telemetry section
        telemetry_group = ttk.LabelFrame(main_frame, text=" Sensors Telemetry ", padding="10 10 10 10")
        telemetry_group.pack(fill="x", pady=(0, 20))

        self.temp_var = tk.StringVar(value="CPU Temp : -- °C")
        self.fan_pct_var = tk.StringVar(value="Fan Speed : -- %")
        self.rpm_var = tk.StringVar(value="RPM : --")

        ttk.Label(telemetry_group, textvariable=self.temp_var, font=("Helvetica", 11, "bold")).pack(anchor="w", pady=2)
        ttk.Label(telemetry_group, textvariable=self.fan_pct_var, font=("Helvetica", 11)).pack(anchor="w", pady=2)
        ttk.Label(telemetry_group, textvariable=self.rpm_var, font=("Helvetica", 11)).pack(anchor="w", pady=2)

        # controls section
        control_group = ttk.LabelFrame(main_frame, text=" Fan Control ", padding="10 10 10 10")
        control_group.pack(fill="x")

        self.mode_var = tk.StringVar(value="auto")
        
        radio_frame = ttk.Frame(control_group)
        radio_frame.pack(fill="x", pady=(0, 15))
        
        ttk.Radiobutton(radio_frame, text="Auto (Curve)", variable=self.mode_var, value="auto", command=self.on_mode_change).pack(side="left", padx=(0, 20))
        ttk.Radiobutton(radio_frame, text="Manual", variable=self.mode_var, value="manual", command=self.on_mode_change).pack(side="left")

        self.slider_var = tk.IntVar(value=50)
        self.slider = ttk.Scale(control_group, from_=0, to=100, orient="horizontal", variable=self.slider_var, command=self.on_slider_move)
        self.slider.pack(fill="x", pady=(0, 5))
        self.slider.state(['disabled'])

        self.slider_val_label = ttk.Label(control_group, text="Target: 50%")
        self.slider_val_label.pack(anchor="center")

        self.apply_btn = ttk.Button(control_group, text="Apply Speed", command=self.on_apply_click)
        self.apply_btn.pack(pady=(10, 0))
        self.apply_btn.state(['disabled'])

        # divider for custom curve buttons
        ttk.Separator(control_group, orient='horizontal').pack(fill='x', pady=15)

        default_fan_curve = ttk.Button(control_group, text="MSI Default Fan Curve", command=self.msi_default_curve)
        default_fan_curve.pack(pady=(0, 30))

        silent_fan_curve = ttk.Button(control_group, text="Silent Fan Curve", command=self.silent_fan_curve)
        silent_fan_curve.pack(pady=(0, 20))

        gaming_fan_curve = ttk.Button(control_group, text="Gaming Fan Curve", command=self.gaming_fan_curve)
        gaming_fan_curve.pack(pady=(0, 10))

        custom_fan_curve = ttk.Button(control_group, text="Custom Fan Curve", command=self.custom_fan_curve)
        custom_fan_curve.pack(pady=(0, 0))

        # status label for feedback messages (Now packed safely in the main frame)
        self.status_label = ttk.Label(main_frame, text="", font=("Helvetica", 10, "bold"))
        self.status_label.pack(pady=(15, 0))

    # event handlers for UI interactions
    def on_mode_change(self):
        current_mode = self.mode_var.get()
        
        # UI logic
        if current_mode == "manual":
            self.slider.state(['!disabled'])
            self.apply_btn.state(['!disabled'])
        else:
            self.slider.state(['disabled'])
            self.apply_btn.state(['disabled'])
            
        # trigger external logic if provided
        if self.mode_callback:
            self.mode_callback(current_mode)

    def on_slider_move(self, event=None):
        current_val = self.slider_var.get()
        self.slider_val_label.config(text=f"Target: {current_val}%")

    def on_apply_click(self):
        current_val = self.slider_var.get()
        # trigger external logic if provided
        if self.apply_callback:
            self.apply_callback(current_val)

    # update telemetry values in the UI
    def update_telemetry(self, temp, fan_pct, rpm):
        self.temp_var.set(f"CPU Temp : {temp} °C")
        self.fan_pct_var.set(f"Fan Speed : {fan_pct} %")
        self.rpm_var.set(f"RPM : {rpm}")

    # buttons for fan curve management
    def msi_default_curve(self):
        print("[UI] Restoring default fan curve...")
        # CORRECTION : On met à jour l'UI visuellement sans déclencher de double commande réseau
        self.mode_var.set("auto")
        self.slider.state(['disabled'])
        self.apply_btn.state(['disabled'])
        
        if self.mode_callback:
            self.mode_callback("restore_default")
    
    def silent_fan_curve(self):
        print("[UI] Setting silent fan curve...")
        if self.mode_callback:
            self.mode_callback("silent_curve")
    
    def gaming_fan_curve(self):
        print("[UI] Setting gaming fan curve...")
        if self.mode_callback:
            self.mode_callback("gaming_curve")

    def custom_fan_curve(self):
        print("[UI] Setting custom fan curve...")
        if self.mode_callback:
            self.mode_callback("custom_curve")

    # methods to show temporary success or error messages
    def show_success(self, message="✅ Done !"):
        self.status_label.config(text=message, foreground="green")
        # delete the message after 2 seconds
        self.root.after(2000, lambda: self.status_label.config(text=""))
        
    def show_error(self, message="❌ Error"):
        self.status_label.config(text=message, foreground="red")
        self.root.after(2000, lambda: self.status_label.config(text=""))
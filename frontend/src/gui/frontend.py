import tkinter as tk
from tkinter import ttk

class FanControllerUI:
    def __init__(self, root, apply_callback=None, mode_callback=None):
        self.root = root
        self.root.title("EC Fan Controller")
        self.root.geometry("400x350")
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
import customtkinter as ctk
import json
import os

# ==========================================
# DRACULA & GRAPHITE THEME
# ==========================================
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

BG       = "#282a36"
SURFACE  = "#21222c"
BORDER   = "#44475a"
TEXT     = "#f8f8f2"
SUBTEXT  = "#6272a4"
ACCENT   = "#bd93f9"
ACCENT_H = "#a87ff1"   # hover state
SUCCESS  = "#50fa7b"
DANGER   = "#ff5555"

FONT_TITLE  = ("Helvetica", 15, "bold")
FONT_LABEL  = ("Helvetica", 11, "bold")
FONT_METRIC = ("Helvetica", 28, "bold")
FONT_UNIT   = ("Helvetica", 12)
FONT_BTN    = ("Helvetica", 13, "bold")
FONT_STATUS = ("Helvetica", 12, "bold")


def _sep(parent):
    """Thin horizontal rule matching the flat aesthetic."""
    f = ctk.CTkFrame(parent, fg_color=BORDER, height=1, corner_radius=0)
    f.pack(fill="x", padx=20, pady=0)
    return f


class FanControllerUI:
    def __init__(self, root, apply_callback=None, mode_callback=None):
        self.root = root
        self.root.title("chore chill")
        self.root.geometry("740x450")
        self.root.resizable(False, False)
        self.root.configure(fg_color=BG)

        # store the functions passed from main.py
        self.apply_callback = apply_callback
        self.mode_callback  = mode_callback
        self.current_mode   = "auto"

        self._build_header()
        
        # Main two-column container
        self.main_container = ctk.CTkFrame(self.root, fg_color="transparent")
        self.main_container.pack(fill="both", expand=True, padx=20, pady=(10, 0))

        # Left Column (Telemetry + Fan Control)
        self.left_col = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.left_col.pack(side="left", fill="both", expand=True, padx=(0, 10))

        # Right Column (Profiles)
        self.right_col = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.right_col.pack(side="right", fill="both", expand=True, padx=(10, 0))

        self._build_telemetry()
        self._build_fan_control()
        self._build_profiles()
        self._build_status()

    # ==========================================
    # LAYOUT BUILDERS
    # ==========================================

    def _build_header(self):
        bar = ctk.CTkFrame(self.root, fg_color="transparent", height=50)
        bar.pack(fill="x", padx=20, pady=(10, 0))
        bar.pack_propagate(False)

        ctk.CTkLabel(
            bar, text="chore chill",
            font=FONT_TITLE, text_color=TEXT
        ).pack(side="left", pady=12)

        self._status_badge = ctk.CTkLabel(
            bar, text="OFFLINE",
            font=FONT_LABEL, text_color=TEXT,
            fg_color=DANGER,
            corner_radius=4,
            width=80,
            height=24
        )
        self._status_badge.pack(side="right", pady=12)

    def _build_telemetry(self):
        row = ctk.CTkFrame(self.left_col, fg_color="transparent")
        row.pack(fill="x", pady=(0, 12))

        self._temp_var = ctk.StringVar(value="--")
        self._fan_var  = ctk.StringVar(value="--")
        self._rpm_var  = ctk.StringVar(value="--")

        metrics = [
            ("CPU",  self._temp_var, "C"),
            ("FAN",  self._fan_var,  "%"),
            ("RPM",  self._rpm_var,  ""),
        ]

        for i, (label, var, unit) in enumerate(metrics):
            cell = ctk.CTkFrame(row, fg_color=SURFACE, corner_radius=10,
                                border_width=1, border_color=BORDER)
            cell.grid(row=0, column=i, padx=(0, 0 if i == 2 else 8), sticky="ew")
            row.columnconfigure(i, weight=1)

            # value + unit on the same line
            val_row = ctk.CTkFrame(cell, fg_color="transparent")
            val_row.pack(pady=(12, 2))
            ctk.CTkLabel(val_row, textvariable=var,
                         font=FONT_METRIC, text_color=TEXT).pack(side="left")
            if unit:
                ctk.CTkLabel(val_row, text=unit,
                             font=FONT_UNIT, text_color=SUBTEXT).pack(side="left", padx=(2, 0))

            ctk.CTkLabel(cell, text=label,
                         font=FONT_LABEL, text_color=SUBTEXT).pack(pady=(0, 10))

    def _build_fan_control(self):
        section = ctk.CTkFrame(self.left_col, fg_color=SURFACE, corner_radius=10,
                               border_width=1, border_color=BORDER, padding=16)
        section.pack(fill="both", expand=True, pady=(0, 10))

        ctk.CTkLabel(section, text="FAN CONTROL",
                     font=FONT_LABEL, text_color=SUBTEXT).pack(anchor="w", pady=(0, 8))

        # Auto / Manual toggle
        self._seg = ctk.CTkSegmentedButton(
            section,
            values=["Auto", "Manual"],
            command=self._on_mode_change,
            font=FONT_BTN,
            fg_color=BG,
            selected_color=ACCENT,
            selected_hover_color=ACCENT_H,
            selected_text_color=BG,       # dark text on light accent background
            unselected_color=BG,
            unselected_hover_color=BORDER,
            text_color=TEXT,
            corner_radius=8,
            border_width=1,
        )
        self._seg.set("Auto")
        self._seg.pack(fill="x", pady=(0, 10))

        # slider
        self._slider_var = ctk.IntVar(value=50)
        self._slider = ctk.CTkSlider(
            section,
            from_=0, to=100,
            variable=self._slider_var,
            command=self._on_slider_move,
            button_color=ACCENT,
            button_hover_color=ACCENT_H,
            progress_color=ACCENT,
            fg_color=BG,
            state="disabled",
            height=16,
            corner_radius=6,
            button_corner_radius=6,
        )
        self._slider.pack(fill="x")

        self._slider_label = ctk.CTkLabel(
            section, text="Target: 50%",
            font=FONT_LABEL, text_color=SUBTEXT
        )
        self._slider_label.pack(pady=(2, 8))

        # apply button
        self._apply_btn = ctk.CTkButton(
            section,
            text="Apply Speed",
            command=self._on_apply_click,
            fg_color=ACCENT,
            hover_color=ACCENT_H,
            font=FONT_BTN,
            state="disabled",
            corner_radius=8,
            height=36,
            text_color=BG,               # dark text on light accent background
            border_width=0,
        )
        self._apply_btn.pack(fill="x")

    def _build_profiles(self):
        section = ctk.CTkFrame(self.right_col, fg_color=SURFACE, corner_radius=10,
                               border_width=1, border_color=BORDER, padding=16)
        section.pack(fill="both", expand=True, pady=(0, 10))

        ctk.CTkLabel(section, text="PROFILES",
                     font=FONT_LABEL, text_color=SUBTEXT).pack(anchor="w", pady=(0, 8))

        # each profile is a flat button, left-aligned text, consistent height
        profiles = [
            ("MSI Default", self.msi_default_curve),
            ("Silent",      self.silent_fan_curve),
            ("Gaming",      self.gaming_fan_curve),
            ("Custom...",   self.custom_fan_curve),
        ]

        for text, cmd in profiles:
            btn = ctk.CTkButton(
                section,
                text=text,
                anchor="w",             # left-align label
                command=cmd,
                fg_color=BG,
                hover_color=BORDER,
                font=FONT_BTN,
                text_color=TEXT,
                corner_radius=8,
                height=38,
                border_width=1,
                border_color=BORDER,
            )
            btn.pack(fill="x", pady=(0, 8))

    def _build_status(self):
        # Stretched bottom bar for status
        self._status_label = ctk.CTkLabel(
            self.root, text="",
            font=FONT_STATUS, text_color=SUCCESS
        )
        self._status_label.pack(pady=(0, 10))

    # ==========================================
    # EVENT HANDLERS
    # ==========================================

    def _on_mode_change(self, value):
        self.current_mode = "manual" if value == "Manual" else "auto"

        if self.current_mode == "manual":
            self._slider.configure(state="normal")
            self._apply_btn.configure(state="normal")
        else:
            self._slider.configure(state="disabled")
            self._apply_btn.configure(state="disabled")

        # trigger external logic if provided
        if self.mode_callback:
            self.mode_callback(self.current_mode)

    def _on_slider_move(self, value):
        self._slider_label.configure(text=f"Target: {int(value)}%")

    def _on_apply_click(self):
        if self.apply_callback:
            self.apply_callback(int(self._slider_var.get()))

    # ==========================================
    # PUBLIC API — called from main.py
    # ==========================================

    def update_telemetry(self, temp, fan_pct, rpm):
        self._temp_var.set(str(temp))
        self._fan_var.set(str(fan_pct))
        self._rpm_var.set(str(rpm))

        # status dot reflects connection state
        is_live = str(temp) not in ("--", "offline", "timeout")
        if is_live:
            self._status_badge.configure(text="LIVE", fg_color=SUCCESS, text_color=SURFACE)
        else:
            label = str(temp).upper() if str(temp) in ("offline", "timeout") else "OFFLINE"
            self._status_badge.configure(text=label, fg_color=DANGER, text_color=TEXT)

    def show_success(self, message="Done"):
        self._status_label.configure(text=message, text_color=SUCCESS)
        self.root.after(2500, lambda: self._status_label.configure(text=""))

    def show_error(self, message="Error"):
        self._status_label.configure(text=message, text_color=DANGER)
        self.root.after(2500, lambda: self._status_label.configure(text=""))

    # ==========================================
    # PROFILE BUTTON HANDLERS
    # ==========================================

    def msi_default_curve(self):
        print("[UI] Restoring default fan curve...")
        # update the UI state directly without triggering a double network command
        self._seg.set("Auto")
        self._slider.configure(state="disabled")
        self._apply_btn.configure(state="disabled")
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
        print("[UI] Opening custom curve editor...")
        # open the editor as a modal window; on close, apply the (possibly updated) profile
        editor = CustomCurveEditor(self.root)
        self.root.wait_window(editor)
        # once the editor is closed, trigger the apply so the daemon picks up the new values
        if self.mode_callback:
            self.mode_callback("custom_curve")


# ==========================================
# CUSTOM CURVE EDITOR
# ==========================================

CONFIG_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "config", "profiles.json")
)

class CustomCurveEditor(ctk.CTkToplevel):
    """Modal window to edit the 'custom' fan curve profile.

    Exposes 6 temperature entry fields and 7 speed entry fields.
    On save, writes back to profiles.json so the next apply_profile('custom') picks it up.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self.title("Custom Curve")
        self.geometry("520x360")
        self.resizable(False, False)
        self.configure(fg_color=BG)
        self.grab_set()

        defaults = self._load_custom_defaults()

        # header
        hdr = ctk.CTkFrame(self, fg_color="transparent", height=48)
        hdr.pack(fill="x", padx=20)
        hdr.pack_propagate(False)
        ctk.CTkLabel(hdr, text="Custom Curve", font=FONT_TITLE, text_color=TEXT).pack(side="left", pady=14)
        ctk.CTkLabel(hdr, text="6 temps · 7 speeds", font=FONT_LABEL, text_color=SUBTEXT).pack(side="right", pady=14)

        _sep(self)

        body = ctk.CTkFrame(self, fg_color="transparent")
        body.pack(fill="x", padx=20, pady=16)

        # temperature row
        ctk.CTkLabel(body, text="TEMPERATURE THRESHOLDS (C)",
                     font=FONT_LABEL, text_color=SUBTEXT).grid(row=0, column=0, columnspan=6, sticky="w", pady=(0, 6))

        self._temp_vars = []
        for i, val in enumerate(defaults["temperatures_c"]):
            inner = ctk.CTkFrame(body, fg_color=SURFACE, corner_radius=8,
                                 border_width=1, border_color=BORDER)
            inner.grid(row=1, column=i, padx=(0, 6), sticky="ew")
            body.columnconfigure(i, weight=1)
            ctk.CTkLabel(inner, text=f"T{i+1}", font=FONT_LABEL, text_color=SUBTEXT).pack(pady=(6, 0))
            var = ctk.StringVar(value=str(val))
            ctk.CTkEntry(inner, textvariable=var, width=46,
                         font=("Helvetica", 13, "bold"), justify="center",
                         fg_color="transparent", border_width=0,
                         text_color=TEXT).pack(pady=(0, 6))
            self._temp_vars.append(var)

        # speed row
        ctk.CTkLabel(body, text="FAN SPEEDS (%)",
                     font=FONT_LABEL, text_color=SUBTEXT).grid(row=2, column=0, columnspan=7, sticky="w", pady=(14, 6))

        speed_frame = ctk.CTkFrame(body, fg_color="transparent")
        speed_frame.grid(row=3, column=0, columnspan=7, sticky="ew")

        self._speed_vars = []
        for i, val in enumerate(defaults["speeds_percent"]):
            inner = ctk.CTkFrame(speed_frame, fg_color=SURFACE, corner_radius=8,
                                 border_width=1, border_color=BORDER)
            inner.grid(row=0, column=i, padx=(0, 6), sticky="ew")
            speed_frame.columnconfigure(i, weight=1)
            ctk.CTkLabel(inner, text=f"S{i+1}", font=FONT_LABEL, text_color=SUBTEXT).pack(pady=(6, 0))
            var = ctk.StringVar(value=str(val))
            ctk.CTkEntry(inner, textvariable=var, width=40,
                         font=("Helvetica", 13, "bold"), justify="center",
                         fg_color="transparent", border_width=0,
                         text_color=TEXT).pack(pady=(0, 6))
            self._speed_vars.append(var)

        _sep(self)

        # footer: status + buttons
        footer = ctk.CTkFrame(self, fg_color="transparent")
        footer.pack(fill="x", padx=20, pady=12)

        self._status = ctk.CTkLabel(footer, text="", font=FONT_LABEL, text_color=SUBTEXT)
        self._status.pack(side="left")

        ctk.CTkButton(footer, text="Cancel", command=self.destroy,
                      fg_color=SURFACE, hover_color=BORDER,
                      text_color=TEXT, font=FONT_BTN,
                      width=90, height=34, corner_radius=8,
                      border_width=1, border_color=BORDER).pack(side="right", padx=(6, 0))

        ctk.CTkButton(footer, text="Save & Apply", command=self._on_save,
                      fg_color=ACCENT, hover_color=ACCENT_H,
                      text_color=BG, font=FONT_BTN,           # dark text on light accent background
                      width=110, height=34, corner_radius=8).pack(side="right")

    def _load_custom_defaults(self):
        """Read the current 'custom' profile from profiles.json."""
        try:
            with open(CONFIG_PATH, "r") as f:
                data = json.load(f)
            return data["profiles"]["custom"]
        except Exception:
            # fall back to safe default values if the file is unreadable
            return {
                "temperatures_c": [55, 64, 73, 76, 82, 88],
                "speeds_percent":  [30, 40, 50, 60, 70, 80, 90]
            }

    def _on_save(self):
        """Validate inputs and write the new custom profile back to profiles.json."""
        try:
            temps  = [int(v.get()) for v in self._temp_vars]
            speeds = [int(v.get()) for v in self._speed_vars]
        except ValueError:
            self._status.configure(text="Integers only", text_color=DANGER)
            return

        # temperatures must be strictly ascending
        if temps != sorted(temps):
            self._status.configure(text="Temps must be ascending", text_color=DANGER)
            return

        # speeds must be between 0 and 100
        if any(s < 0 or s > 100 for s in speeds):
            self._status.configure(text="Speeds: 0–100 only", text_color=DANGER)
            return

        try:
            with open(CONFIG_PATH, "r") as f:
                data = json.load(f)

            data["profiles"]["custom"]["temperatures_c"] = temps
            data["profiles"]["custom"]["speeds_percent"]  = speeds

            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=2)

            self._status.configure(text="Saved", text_color=SUCCESS)
            # close after a short delay so the user sees the confirmation
            self.after(700, self.destroy)

        except Exception as e:
            self._status.configure(text=f"{e}", text_color=DANGER)
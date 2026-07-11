import customtkinter as ctk

# ==========================================
# THEME SETUP
# ==========================================
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# brand colours used across the whole UI
COLOR_BG        = "#0f1117"
COLOR_SURFACE   = "#1a1d27"
COLOR_CARD      = "#20232e"
COLOR_ACCENT    = "#4f8ef7"
COLOR_ACCENT2   = "#7c5ff7"
COLOR_SUCCESS   = "#22c55e"
COLOR_WARNING   = "#f59e0b"
COLOR_DANGER    = "#ef4444"
COLOR_TEXT      = "#e2e8f0"
COLOR_SUBTEXT   = "#64748b"
COLOR_BORDER    = "#2d3148"


class FanControllerUI:
    def __init__(self, root, apply_callback=None, mode_callback=None):
        self.root = root
        self.root.title("Chore Chill")
        self.root.geometry("440x740")
        self.root.resizable(False, False)
        self.root.configure(fg_color=COLOR_BG)

        # store the functions passed from main.py
        self.apply_callback = apply_callback
        self.mode_callback  = mode_callback

        # track current mode: "auto" or "manual"
        self.current_mode = "auto"

        self._build_header()
        self._build_telemetry()
        self._build_controls()
        self._build_profiles()
        self._build_status()

    # ==========================================
    # LAYOUT BUILDERS
    # ==========================================

    def _build_header(self):
        header = ctk.CTkFrame(self.root, fg_color=COLOR_SURFACE, corner_radius=0, height=56)
        header.pack(fill="x")
        header.pack_propagate(False)

        ctk.CTkLabel(
            header,
            text="❄  Chore Chill",
            font=ctk.CTkFont(family="Helvetica", size=18, weight="bold"),
            text_color=COLOR_ACCENT
        ).pack(side="left", padx=20)

        self.status_dot = ctk.CTkLabel(
            header,
            text="● offline",
            font=ctk.CTkFont(size=11),
            text_color=COLOR_DANGER
        )
        self.status_dot.pack(side="right", padx=20)

    def _build_telemetry(self):
        card = ctk.CTkFrame(self.root, fg_color=COLOR_CARD, corner_radius=12)
        card.pack(fill="x", padx=16, pady=(14, 0))

        ctk.CTkLabel(
            card,
            text="SENSORS",
            font=ctk.CTkFont(size=10, weight="bold"),
            text_color=COLOR_SUBTEXT
        ).pack(anchor="w", padx=14, pady=(10, 4))

        row = ctk.CTkFrame(card, fg_color="transparent")
        row.pack(fill="x", padx=14, pady=(0, 12))

        # temperature block
        self._temp_val  = ctk.StringVar(value="--")
        self._fan_val   = ctk.StringVar(value="--")
        self._rpm_val   = ctk.StringVar(value="--")

        for label, var, unit, col in [
            ("CPU TEMP", self._temp_val, "°C", 0),
            ("FAN",      self._fan_val,  "%",  1),
            ("RPM",      self._rpm_val,  "",   2),
        ]:
            block = ctk.CTkFrame(row, fg_color=COLOR_SURFACE, corner_radius=8)
            block.grid(row=0, column=col, padx=4, sticky="ew")
            row.columnconfigure(col, weight=1)

            ctk.CTkLabel(block, text=label, font=ctk.CTkFont(size=9, weight="bold"), text_color=COLOR_SUBTEXT).pack(pady=(8, 0))
            val_frame = ctk.CTkFrame(block, fg_color="transparent")
            val_frame.pack(pady=(0, 8))
            ctk.CTkLabel(val_frame, textvariable=var, font=ctk.CTkFont(size=22, weight="bold"), text_color=COLOR_TEXT).pack(side="left")
            ctk.CTkLabel(val_frame, text=unit, font=ctk.CTkFont(size=12), text_color=COLOR_SUBTEXT).pack(side="left", padx=(2, 0))

    def _build_controls(self):
        card = ctk.CTkFrame(self.root, fg_color=COLOR_CARD, corner_radius=12)
        card.pack(fill="x", padx=16, pady=(10, 0))

        ctk.CTkLabel(
            card,
            text="FAN CONTROL",
            font=ctk.CTkFont(size=10, weight="bold"),
            text_color=COLOR_SUBTEXT
        ).pack(anchor="w", padx=14, pady=(10, 6))

        # mode toggle: Auto / Manual
        seg = ctk.CTkSegmentedButton(
            card,
            values=["Auto (Curve)", "Manual"],
            command=self._on_mode_change,
            font=ctk.CTkFont(size=12),
            fg_color=COLOR_SURFACE,
            selected_color=COLOR_ACCENT,
            selected_hover_color=COLOR_ACCENT2,
            unselected_color=COLOR_SURFACE,
            text_color=COLOR_TEXT,
        )
        seg.set("Auto (Curve)")
        seg.pack(fill="x", padx=14, pady=(0, 10))
        self._seg = seg

        # slider + label
        self._slider_var = ctk.IntVar(value=50)
        self._slider = ctk.CTkSlider(
            card,
            from_=0, to=100,
            variable=self._slider_var,
            command=self._on_slider_move,
            button_color=COLOR_ACCENT,
            button_hover_color=COLOR_ACCENT2,
            progress_color=COLOR_ACCENT,
            fg_color=COLOR_SURFACE,
            state="disabled",
        )
        self._slider.pack(fill="x", padx=14)

        self._slider_label = ctk.CTkLabel(
            card,
            text="Target: 50%",
            font=ctk.CTkFont(size=12),
            text_color=COLOR_SUBTEXT
        )
        self._slider_label.pack(pady=(2, 0))

        # apply button
        self._apply_btn = ctk.CTkButton(
            card,
            text="Apply Speed",
            command=self._on_apply_click,
            fg_color=COLOR_ACCENT,
            hover_color=COLOR_ACCENT2,
            font=ctk.CTkFont(size=13, weight="bold"),
            state="disabled",
            corner_radius=8,
            height=36,
        )
        self._apply_btn.pack(fill="x", padx=14, pady=(8, 12))

    def _build_profiles(self):
        card = ctk.CTkFrame(self.root, fg_color=COLOR_CARD, corner_radius=12)
        card.pack(fill="x", padx=16, pady=(10, 0))

        ctk.CTkLabel(
            card,
            text="PROFILES",
            font=ctk.CTkFont(size=10, weight="bold"),
            text_color=COLOR_SUBTEXT
        ).pack(anchor="w", padx=14, pady=(10, 6))

        buttons = [
            ("🏭  MSI Default",  self.msi_default_curve,  COLOR_SURFACE,  COLOR_BORDER),
            ("🤫  Silent",        self.silent_fan_curve,    COLOR_SURFACE,  COLOR_BORDER),
            ("🎮  Gaming",        self.gaming_fan_curve,    COLOR_SURFACE,  COLOR_BORDER),
            ("✏️  Custom…",       self.custom_fan_curve,    COLOR_ACCENT,   COLOR_ACCENT2),
        ]

        for text, cmd, fg, hover in buttons:
            ctk.CTkButton(
                card,
                text=text,
                command=cmd,
                fg_color=fg,
                hover_color=hover,
                font=ctk.CTkFont(size=13),
                text_color=COLOR_TEXT,
                corner_radius=8,
                height=36,
                border_width=1,
                border_color=COLOR_BORDER,
            ).pack(fill="x", padx=14, pady=(0, 6))

        # extra padding at the bottom of the card
        ctk.CTkLabel(card, text="", height=4).pack()

    def _build_status(self):
        self._status_label = ctk.CTkLabel(
            self.root,
            text="",
            font=ctk.CTkFont(size=12, weight="bold"),
            text_color=COLOR_SUCCESS
        )
        self._status_label.pack(pady=(10, 0))

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
        self._temp_val.set(str(temp))
        self._fan_val.set(str(fan_pct))
        self._rpm_val.set(str(rpm))

        # update the status dot based on whether we have live data
        if str(temp) in ("--", "offline", "timeout"):
            label = "timeout" if str(temp) == "timeout" else "offline"
            self.status_dot.configure(text=f"● {label}", text_color=COLOR_DANGER)
        else:
            self.status_dot.configure(text="● live", text_color=COLOR_SUCCESS)

    def show_success(self, message="✅ Done!"):
        self._status_label.configure(text=message, text_color=COLOR_SUCCESS)
        self.root.after(2500, lambda: self._status_label.configure(text=""))

    def show_error(self, message="❌ Error"):
        self._status_label.configure(text=message, text_color=COLOR_DANGER)
        self.root.after(2500, lambda: self._status_label.configure(text=""))

    # ==========================================
    # PROFILE BUTTON HANDLERS
    # ==========================================

    def msi_default_curve(self):
        print("[UI] Restoring default fan curve...")
        # update the UI state directly without triggering a double network command
        self._seg.set("Auto (Curve)")
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
# CUSTOM CURVE EDITOR (inline — avoids circular import)
# ==========================================

import json
import os

CONFIG_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "config", "profiles.json")
)

class CustomCurveEditor(ctk.CTkToplevel):
    """Modal window to edit the 'custom' fan curve profile.

    Exposes 6 temperature spinboxes and 7 speed spinboxes.
    On save, writes back to profiles.json so the next apply_profile('custom') picks it up.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self.title("Custom Fan Curve")
        self.geometry("520x420")
        self.resizable(False, False)
        self.configure(fg_color=COLOR_BG)
        self.grab_set()  # make it modal

        defaults = self._load_custom_defaults()

        # --- title ---
        ctk.CTkLabel(
            self,
            text="✏️  Custom Fan Curve",
            font=ctk.CTkFont(size=16, weight="bold"),
            text_color=COLOR_ACCENT
        ).pack(pady=(18, 2))

        ctk.CTkLabel(
            self,
            text="6 temperature thresholds (°C)  +  7 fan speeds (%)",
            font=ctk.CTkFont(size=11),
            text_color=COLOR_SUBTEXT
        ).pack()

        # --- temperature row ---
        t_card = ctk.CTkFrame(self, fg_color=COLOR_CARD, corner_radius=10)
        t_card.pack(fill="x", padx=20, pady=(14, 0))
        ctk.CTkLabel(t_card, text="TEMPERATURE THRESHOLDS (°C)", font=ctk.CTkFont(size=9, weight="bold"), text_color=COLOR_SUBTEXT).pack(anchor="w", padx=12, pady=(8, 4))

        t_row = ctk.CTkFrame(t_card, fg_color="transparent")
        t_row.pack(padx=10, pady=(0, 10))
        self._temp_vars = []
        for i, val in enumerate(defaults["temperatures_c"]):
            col = ctk.CTkFrame(t_row, fg_color=COLOR_SURFACE, corner_radius=6, width=62)
            col.grid(row=0, column=i, padx=3)
            col.grid_propagate(False)
            ctk.CTkLabel(col, text=f"T{i+1}", font=ctk.CTkFont(size=9), text_color=COLOR_SUBTEXT).pack(pady=(4, 0))
            var = ctk.StringVar(value=str(val))
            ctk.CTkEntry(col, textvariable=var, width=50, font=ctk.CTkFont(size=13), justify="center", fg_color=COLOR_BG, border_color=COLOR_BORDER).pack(pady=(0, 4))
            self._temp_vars.append(var)

        # --- speed row ---
        s_card = ctk.CTkFrame(self, fg_color=COLOR_CARD, corner_radius=10)
        s_card.pack(fill="x", padx=20, pady=(10, 0))
        ctk.CTkLabel(s_card, text="FAN SPEEDS (%)", font=ctk.CTkFont(size=9, weight="bold"), text_color=COLOR_SUBTEXT).pack(anchor="w", padx=12, pady=(8, 4))

        s_row = ctk.CTkFrame(s_card, fg_color="transparent")
        s_row.pack(padx=10, pady=(0, 10))
        self._speed_vars = []
        for i, val in enumerate(defaults["speeds_percent"]):
            col = ctk.CTkFrame(s_row, fg_color=COLOR_SURFACE, corner_radius=6, width=62)
            col.grid(row=0, column=i, padx=3)
            col.grid_propagate(False)
            ctk.CTkLabel(col, text=f"S{i+1}", font=ctk.CTkFont(size=9), text_color=COLOR_SUBTEXT).pack(pady=(4, 0))
            var = ctk.StringVar(value=str(val))
            ctk.CTkEntry(col, textvariable=var, width=50, font=ctk.CTkFont(size=13), justify="center", fg_color=COLOR_BG, border_color=COLOR_BORDER).pack(pady=(0, 4))
            self._speed_vars.append(var)

        # --- feedback + buttons ---
        self._status = ctk.CTkLabel(self, text="", font=ctk.CTkFont(size=11, weight="bold"), text_color=COLOR_SUCCESS)
        self._status.pack(pady=(10, 0))

        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack(pady=(6, 16))
        ctk.CTkButton(btn_row, text="Save & Apply", command=self._on_save, fg_color=COLOR_ACCENT, hover_color=COLOR_ACCENT2, width=130, height=34, font=ctk.CTkFont(size=13, weight="bold")).pack(side="left", padx=6)
        ctk.CTkButton(btn_row, text="Cancel", command=self.destroy, fg_color=COLOR_SURFACE, hover_color=COLOR_BORDER, width=100, height=34, text_color=COLOR_TEXT, font=ctk.CTkFont(size=13)).pack(side="left", padx=6)

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
            self._status.configure(text="❌ Only integers allowed.", text_color=COLOR_DANGER)
            return

        # temperatures must be strictly ascending
        if temps != sorted(temps):
            self._status.configure(text="❌ Temperatures must be ascending.", text_color=COLOR_DANGER)
            return

        # speeds must be between 0 and 100
        if any(s < 0 or s > 100 for s in speeds):
            self._status.configure(text="❌ Speeds must be between 0 and 100.", text_color=COLOR_DANGER)
            return

        try:
            with open(CONFIG_PATH, "r") as f:
                data = json.load(f)

            data["profiles"]["custom"]["temperatures_c"] = temps
            data["profiles"]["custom"]["speeds_percent"]  = speeds

            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=2)

            self._status.configure(text="✅ Saved! Closing…", text_color=COLOR_SUCCESS)
            # close after a short delay so the user sees the confirmation
            self.after(800, self.destroy)

        except Exception as e:
            self._status.configure(text=f"❌ Save error: {e}", text_color=COLOR_DANGER)
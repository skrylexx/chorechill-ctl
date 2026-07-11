import customtkinter as ctk
import tkinter as tk
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
        self.root.geometry("640x300")
        self.root.resizable(False, False)
        self.root.configure(fg_color=BG)

        # store the functions passed from main.py
        self.apply_callback = apply_callback
        self.mode_callback  = mode_callback

        self._build_header()
        
        # Main two-column container
        self.main_container = ctk.CTkFrame(self.root, fg_color="transparent")
        self.main_container.pack(fill="both", expand=True, padx=20, pady=(10, 0))

        # Left Column (Telemetry + Configure Custom Mode)
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
                               border_width=1, border_color=BORDER)
        section.pack(fill="both", expand=True, pady=(0, 10))

        ctk.CTkLabel(section, text="FAN CONTROL",
                     font=FONT_LABEL, text_color=SUBTEXT).pack(anchor="w", padx=16, pady=(12, 4))

        # Single button to configure the custom curve
        self._config_btn = ctk.CTkButton(
            section,
            text="Configure Custom Mode",
            command=self.custom_fan_curve,
            fg_color=BG,
            hover_color=BORDER,
            font=FONT_BTN,
            text_color=TEXT,
            corner_radius=8,
            height=38,
            border_width=1,
            border_color=BORDER,
        )
        self._config_btn.pack(fill="x", padx=16, pady=(8, 16))

    def _build_profiles(self):
        section = ctk.CTkFrame(self.right_col, fg_color=SURFACE, corner_radius=10,
                               border_width=1, border_color=BORDER)
        section.pack(fill="both", expand=True, pady=(0, 10))

        ctk.CTkLabel(section, text="PROFILES",
                     font=FONT_LABEL, text_color=SUBTEXT).pack(anchor="w", padx=16, pady=(12, 4))

        # each profile is a flat button, left-aligned text, consistent height
        profiles = [
            ("MSI Default", self.msi_default_curve),
            ("Silent Mode", self.silent_fan_curve),
            ("Gaming Mode", self.gaming_fan_curve),
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
            btn.pack(fill="x", padx=16, pady=(4, 4))

    def _build_status(self):
        # Stretched bottom bar for status
        self._status_label = ctk.CTkLabel(
            self.root, text="",
            font=FONT_STATUS, text_color=SUCCESS
        )
        self._status_label.pack(pady=(0, 10))

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
    """Modal window to edit the 'custom' fan curve profile graphically.

    Displays a canvas with 7 interactive draggable points.
    Horizontal axis represents CPU temperature (25 to 95 C).
    Vertical axis represents fan speed percentage (0 to 100 %).
    On save, writes back to profiles.json so the next apply_profile('custom') picks it up.
    """

    def __init__(self, parent):
        super().__init__(parent)
        self.title("Configure Custom Mode")
        self.geometry("760x460")
        self.resizable(False, False)
        self.configure(fg_color=BG)
        self.grab_set()

        # Canvas boundaries
        self.margin_left = 50
        self.margin_right = 30
        self.margin_top = 30
        self.margin_bottom = 50
        self.plot_w = 760 - self.margin_left - self.margin_right
        self.plot_h = 360 - self.margin_top - self.margin_bottom

        defaults = self._load_custom_defaults()

        # Build self.points (7 points) from defaults:
        # P0 is fixed at temp=25 (start threshold), adjustable in speed (S0).
        # P1..P6 correspond to temperatures_c[0..5] and speeds_percent[1..6].
        # We ensure temperatures are loaded with 25 C minimum.
        self.points = []
        
        # P0 starting speed S[0]
        self.points.append({"temp": 25, "speed": defaults["speeds_percent"][0], "fixed_x": True})
        
        # P1..P6 representing thresholds T[0..5] and speeds S[1..6]
        for i in range(6):
            t_val = max(26 + i, defaults["temperatures_c"][i])  # enforce strictly ascending above 25 C
            self.points.append({
                "temp": t_val,
                "speed": defaults["speeds_percent"][i + 1],
                "fixed_x": False
            })

        self.dragged_point_idx = None

        # Build top bar layout
        hdr_frame = ctk.CTkFrame(self, fg_color="transparent", height=60)
        hdr_frame.pack(fill="x", padx=20, pady=(10, 0))
        hdr_frame.pack_propagate(False)

        ctk.CTkLabel(hdr_frame, text="Custom Curve Editor", font=FONT_TITLE, text_color=TEXT).pack(side="left")

        # Top-right live recap label (displays real temperatures in C, and speeds in %)
        self.recap_label = ctk.CTkLabel(
            hdr_frame, text="",
            font=("Helvetica", 11, "bold"),
            text_color=SUBTEXT,
            justify="right"
        )
        self.recap_label.pack(side="right")

        _sep(self)

        # Build Canvas Area
        self.canvas_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.canvas_frame.pack(fill="both", expand=True, padx=20, pady=10)

        self.canvas = ctk.CTkCanvas(
            self.canvas_frame,
            width=720,
            height=300,
            bg=BG,
            highlightthickness=0
        )
        self.canvas.pack(fill="both", expand=True)

        self.canvas.bind("<Button-1>", self._on_canvas_click)
        self.canvas.bind("<B1-Motion>", self._on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_canvas_release)

        _sep(self)

        # Footer Area
        footer = ctk.CTkFrame(self, fg_color="transparent", height=50)
        footer.pack(fill="x", padx=20, pady=10)
        footer.pack_propagate(False)

        self._status = ctk.CTkLabel(footer, text="", font=FONT_LABEL, text_color=SUBTEXT)
        self._status.pack(side="left", pady=10)

        ctk.CTkButton(footer, text="Cancel", command=self.destroy,
                      fg_color=SURFACE, hover_color=BORDER,
                      text_color=TEXT, font=FONT_BTN,
                      width=90, height=34, corner_radius=8,
                      border_width=1, border_color=BORDER).pack(side="right", padx=(6, 0), pady=8)

        ctk.CTkButton(footer, text="Save & Apply", command=self._on_save,
                      fg_color=ACCENT, hover_color=ACCENT_H,
                      text_color=BG, font=FONT_BTN,
                      width=120, height=34, corner_radius=8).pack(side="right", pady=8)

        self._update_recap()
        self._draw_graph()

    def _temp_to_x(self, temp):
        # Range is 25 C to 95 C (width of 70 degrees)
        return self.margin_left + ((temp - 25) / 70.0) * self.plot_w

    def _speed_to_y(self, speed):
        return self.margin_top + (1.0 - speed / 100.0) * self.plot_h

    def _x_to_temp(self, x):
        val = 25.0 + ((x - self.margin_left) / float(self.plot_w)) * 70.0
        return max(25.0, min(95.0, val))

    def _y_to_speed(self, y):
        val = (1.0 - (y - self.margin_top) / float(self.plot_h)) * 100.0
        return max(0.0, min(100.0, val))

    def _update_recap(self):
        # Format the selected coordinates dynamically using real decimal values (with °C and %)
        temps_str = ", ".join(f"{p['temp']}°C" for p in self.points[1:])
        speeds_str = ", ".join(f"{p['speed']}%" for p in self.points)
        self.recap_label.configure(
            text=f"Temps:  {temps_str}\nSpeeds: {speeds_str}"
        )

    def _draw_graph(self):
        self.canvas.delete("all")

        # Draw grid & axes
        # Y-Axis ticks (Speed)
        for s in [0, 20, 40, 60, 80, 100]:
            y = self._speed_to_y(s)
            self.canvas.create_line(self.margin_left, y, self.margin_left + self.plot_w, y, fill=BORDER, dash=(2, 2))
            self.canvas.create_text(self.margin_left - 15, y, text=f"{s}%", fill=SUBTEXT, font=("Helvetica", 9))

        # X-Axis ticks (Temperature grid lines starting at 25 C)
        for t in [25, 40, 55, 70, 85, 95]:
            x = self._temp_to_x(t)
            self.canvas.create_line(x, self.margin_top, x, self.margin_top + self.plot_h, fill=BORDER, dash=(2, 2))
            self.canvas.create_text(x, self.margin_top + self.plot_h + 15, text=f"{t}°C", fill=SUBTEXT, font=("Helvetica", 9))

        # Draw main axis lines
        self.canvas.create_line(self.margin_left, self.margin_top, self.margin_left, self.margin_top + self.plot_h, fill=BORDER)
        self.canvas.create_line(self.margin_left, self.margin_top + self.plot_h, self.margin_left + self.plot_w, self.margin_top + self.plot_h, fill=BORDER)

        # Plot coordinates
        coords = []
        for p in self.points:
            coords.append((self._temp_to_x(p["temp"]), self._speed_to_y(p["speed"])))

        # 1. Polish: Draw glowing area under the curve (filled polygon using Dracula palette)
        poly_coords = []
        poly_coords.append(self.margin_left)
        poly_coords.append(self.margin_top + self.plot_h)
        for cx, cy in coords:
            poly_coords.append(cx)
            poly_coords.append(cy)
        poly_coords.append(coords[-1][0])
        poly_coords.append(self.margin_top + self.plot_h)
        # Soft fill color blending nicely with the dark background
        self.canvas.create_polygon(poly_coords, fill="#352e4f", outline="")

        # 2. Polish: Draw thin vertical dashed projection lines down to the X-axis for each point
        for idx, p in enumerate(self.points):
            x = self._temp_to_x(p["temp"])
            y = self._speed_to_y(p["speed"])
            
            # Dashed line from point down to X-axis
            self.canvas.create_line(x, y, x, self.margin_top + self.plot_h, fill=BORDER, dash=(2, 2))

            # Display a floating coordinate tooltip above the active dragged point
            if idx == self.dragged_point_idx:
                self.canvas.create_text(
                    x, y - 20,
                    text=f"{p['temp']}°C, {p['speed']}%",
                    fill=SUCCESS,
                    font=("Helvetica", 10, "bold")
                )

        # 3. Polish: Draw the main curve line (thick purple accent line)
        for i in range(1, len(coords)):
            self.canvas.create_line(coords[i - 1][0], coords[i - 1][1], coords[i][0], coords[i][1], fill=ACCENT, width=3)

        # 4. Polish: Draw points with clean circular joints and high-contrast outer rings
        for idx, p in enumerate(self.points):
            x = self._temp_to_x(p["temp"])
            y = self._speed_to_y(p["speed"])
            
            # Determine color and size (active point glows in green)
            color = SUCCESS if idx == self.dragged_point_idx else ACCENT
            r_outer = 7
            r_inner = 4
            
            # Outer ring
            self.canvas.create_oval(x - r_outer, y - r_outer, x + r_outer, y + r_outer, fill=BORDER, outline="")
            # Inner circle
            self.canvas.create_oval(x - r_inner, y - r_inner, x + r_inner, y + r_inner, fill=color, outline="")

    def _on_canvas_click(self, event):
        # Find closest point
        closest_idx = None
        min_dist = 15.0  # threshold radius in pixels
        for idx, p in enumerate(self.points):
            px = self._temp_to_x(p["temp"])
            py = self._speed_to_y(p["speed"])
            dist = ((px - event.x) ** 2 + (py - event.y) ** 2) ** 0.5
            if dist < min_dist:
                min_dist = dist
                closest_idx = idx

        if closest_idx is not None:
            self.dragged_point_idx = closest_idx
            self._draw_graph()

    def _on_canvas_drag(self, event):
        if self.dragged_point_idx is None:
            return

        idx = self.dragged_point_idx
        p = self.points[idx]

        # Drag vertical (Speed) - STRICTLY clamp within [0, 100] to prevent points from going below the graph
        new_speed = int(self._y_to_speed(event.y))
        p["speed"] = max(0, min(100, new_speed))

        # Drag horizontal (Temperature)
        if not p["fixed_x"]:
            # Clamp between previous point and next point to maintain ascending order
            min_temp = self.points[idx - 1]["temp"] + 1
            max_temp = self.points[idx + 1]["temp"] - 1 if idx < 6 else 95
            
            new_temp = int(self._x_to_temp(event.x))
            p["temp"] = max(min_temp, min(max_temp, new_temp))

        self._update_recap()
        self._draw_graph()

    def _on_canvas_release(self, event):
        self.dragged_point_idx = None
        self._draw_graph()

    def _load_custom_defaults(self):
        """Read the current 'custom' profile from profiles.json."""
        try:
            with open(CONFIG_PATH, "r") as f:
                data = json.load(f)
            return data["profiles"]["custom"]
        except Exception:
            return {
                "temperatures_c": [55, 64, 73, 76, 82, 88],
                "speeds_percent":  [30, 40, 50, 60, 70, 80, 90]
            }

    def _on_save(self):
        # Extract temperatures and speeds to match the EC expectations:
        # temperatures_c has 6 values (from P1 to P6)
        # speeds_percent has 7 values (from P0 to P6)
        temps = [p["temp"] for p in self.points[1:]]
        speeds = [p["speed"] for p in self.points]

        try:
            with open(CONFIG_PATH, "r") as f:
                data = json.load(f)

            data["profiles"]["custom"]["temperatures_c"] = temps
            data["profiles"]["custom"]["speeds_percent"]  = speeds

            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=2)

            self._status.configure(text="Saved successfully", text_color=SUCCESS)
            self.after(600, self.destroy)

        except Exception as e:
            self._status.configure(text=f"Save error: {e}", text_color=DANGER)
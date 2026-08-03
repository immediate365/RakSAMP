import tkinter as tk
from tkinter import messagebox
import subprocess
import os
import sys
import random

class BotLauncherGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RakSAMP Multi-Bot Launcher")
        self.root.geometry("420x460")
        self.root.resizable(False, False)
        self.root.configure(bg="#1e1e2e")

        # Title
        title_label = tk.Label(root, text="RakSAMP Launcher", font=("Segoe UI", 16, "bold"), fg="#cdd6f4", bg="#1e1e2e")
        title_label.pack(pady=(15, 10))

        frame = tk.Frame(root, bg="#1e1e2e")
        frame.pack(padx=20, pady=5, fill="both", expand=True)

        # Host
        tk.Label(frame, text="Server Host / IP:", font=("Segoe UI", 10), fg="#a6adc8", bg="#1e1e2e", anchor="w").pack(fill="x")
        self.entry_host = tk.Entry(frame, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.entry_host.insert(0, "samp.ulgaming.net")
        self.entry_host.pack(fill="x", ipady=4, pady=(2, 10))

        # Port
        tk.Label(frame, text="Port:", font=("Segoe UI", 10), fg="#a6adc8", bg="#1e1e2e", anchor="w").pack(fill="x")
        self.entry_port = tk.Entry(frame, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.entry_port.insert(0, "7777")
        self.entry_port.pack(fill="x", ipady=4, pady=(2, 10))

        # Password
        tk.Label(frame, text="Server Password (Optional):", font=("Segoe UI", 10), fg="#a6adc8", bg="#1e1e2e", anchor="w").pack(fill="x")
        self.entry_pass = tk.Entry(frame, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.entry_pass.pack(fill="x", ipady=4, pady=(2, 10))

        # Nicknames
        tk.Label(frame, text="Bot Nicknames (one per line):", font=("Segoe UI", 10), fg="#a6adc8", bg="#1e1e2e", anchor="w").pack(fill="x")
        self.txt_nicks = tk.Text(frame, height=5, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.txt_nicks.insert("1.0", "Bot_1\nBot_2\nBot_3")
        self.txt_nicks.pack(fill="x", pady=(2, 10))

        # Ping Amplification Range
        ampl_frame = tk.Frame(frame, bg="#1e1e2e")
        ampl_frame.pack(fill="x", pady=(0, 15))

        tk.Label(ampl_frame, text="Ping Ampl (ms) Min:", font=("Segoe UI", 9), fg="#a6adc8", bg="#1e1e2e").pack(side="left")
        self.entry_min_ampl = tk.Entry(ampl_frame, width=6, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.entry_min_ampl.insert(0, "50")
        self.entry_min_ampl.pack(side="left", padx=5)

        tk.Label(ampl_frame, text="Max:", font=("Segoe UI", 9), fg="#a6adc8", bg="#1e1e2e").pack(side="left", padx=(10, 0))
        self.entry_max_ampl = tk.Entry(ampl_frame, width=6, font=("Consolas", 10), bg="#313244", fg="#cdd6f4", insertbackground="white", relief="flat")
        self.entry_max_ampl.insert(0, "250")
        self.entry_max_ampl.pack(side="left", padx=5)

        # Launch Button
        btn_launch = tk.Button(root, text="🚀 LAUNCH BOTS", font=("Segoe UI", 11, "bold"), bg="#89b4fa", fg="#11111b",
                               activebackground="#b4befe", relief="flat", cursor="hand2", command=self.launch_bots)
        btn_launch.pack(fill="x", padx=20, pady=(0, 15), ipady=6)

    def launch_bots(self):
        host = self.entry_host.get().strip()
        port = self.entry_port.get().strip()
        password = self.entry_pass.get().strip()
        nicks = [n.strip() for n in self.txt_nicks.get("1.0", tk.END).splitlines() if n.strip()]

        if not host or not port or not nicks:
            messagebox.showerror("Error", "Please fill in Host, Port, and at least one Nickname.")
            return

        try:
            min_ampl = int(self.entry_min_ampl.get().strip())
            max_ampl = int(self.entry_max_ampl.get().strip())
        except ValueError:
            messagebox.showerror("Error", "Min and Max Ping Amplification must be numbers.")
            return

        exe_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "client", "bin", "RakSAMPClient.exe")
        if not os.path.exists(exe_path):
            messagebox.showerror("Error", f"Could not find RakSAMPClient.exe at:\n{exe_path}")
            return

        launched = 0
        for nick in nicks:
            ampl = random.randint(min_ampl, max_ampl)
            cmd = [exe_path, "-n", nick, "-h", host, "-p", port, "-ampl", str(ampl)]
            if password:
                cmd.extend(["-pass", password])

            subprocess.Popen(cmd)
            launched += 1

        messagebox.showinfo("Success", f"Successfully launched {launched} bot instance(s)!")

if __name__ == "__main__":
    root = tk.Tk()
    app = BotLauncherGUI(root)
    root.mainloop()

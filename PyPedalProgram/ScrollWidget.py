from tkinter import *


class ScrollWidget(Frame):
    def __init__(self, parent, width=800):
        Frame.__init__(self, parent)
        self.canvas = Canvas(self, borderwidth=0, width=width)
        self.frame = Frame(self.canvas)
        self.scrollbar = Scrollbar(self, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        self.widgets = []

        self.scrollbar.pack(side="right", fill="y")
        self.canvas.pack(side="top", fill="y", expand=True)
        self.canvas.create_window((4, 4), window=self.frame, anchor="nw", tags="self.frame")
        self.frame.bind("<Configure>", self._on_frame_configure)

        # Bind scroll buttons (for Linux) and mouse wheel (for Windows)
        self.canvas.bind_all("<Button-4>", lambda _: self._on_scrollbutton(-1))
        self.canvas.bind_all("<Button-5>", lambda _: self._on_scrollbutton(1))
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)

    def populate(self, widgets):
        self.widgets = widgets
        for _index, _widget in enumerate(widgets):
            _widget.grid(row=_index, column=0, pady=20, sticky="n")

    def empty(self):
        for widget in self.widgets:
            widget.destroy()
        self.widgets = []

    def _on_frame_configure(self, event):
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))

    def _on_mousewheel(self, event):
        self.canvas.yview_scroll(-1 * (event.delta // 120), "units")

    def _on_scrollbutton(self, direction):
        self.canvas.yview_scroll(direction, "units")

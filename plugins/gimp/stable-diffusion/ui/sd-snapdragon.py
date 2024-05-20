#!/usr/bin/env python2

"""
Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
"""

from gimpfu import *
import time
import socket
import threading
import subprocess
import os
import random
import sys
import gtk
import webbrowser

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)
current_dir = os.path.dirname(__file__)
os.chdir(current_dir)


def gimp_log(text):
    pdb.gimp_message(text)


def initiate_sd_execution():
    cmd = "gimp_sd_wos_plugin.exe --config_file_Path injected.stablediffusion.properties.in --backend QnnHtp.dll"
    subprocess.call(cmd, shell=True)


class Server:
    """TCP IP communication server
    If automatic_port == True, will iterate over port until find a free one
    """

    def __init__(self, ip, port, automatic_port=True):
        self.__size_message_length = 16  # Buffer size for the length
        max_connections_attempts = 5

        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        if automatic_port:
            connected = False
            while (not connected) and (max_connections_attempts > 0):
                if self.s.bind((ip, port)) == -1:  # Check if bind operation fails
                    print(
                        "[Server]: Port",
                        port,
                        "already in use. Binding to port:",
                        port + 1,
                    )
                    port += 1
                    max_connections_attempts -= 1
                else:
                    connected = True
            if not connected:
                print("[Server]: Error binding to address!")
        else:
            self.s.bind((ip, port))

        self.s.listen(True)
        self.conn, addr = self.s.accept()

    def __del__(self):
        self.s.close()

    def send(self, message):
        message_size = str(len(message)).ljust(self.__size_message_length).encode()
        self.conn.sendall(message_size)  # Send length of msg (in known size, 16)
        self.conn.sendall(message.encode())  # Send message

    def receive(self, decode=True):
        length = self.__receive_value(self.conn, self.__size_message_length)
        if length is not None:  # If None received, no new message to read
            message = self.__receive_value(
                self.conn, int(length), decode
            )  # Get message
            return message
        return None

    def __receive_value(self, conn, buf_lentgh, decode=True):
        buf = b""
        while buf_lentgh:
            newbuf = conn.recv(buf_lentgh)
            # if not newbuf: return None  # Comment this to make it non-blocking
            buf += newbuf
            buf_lentgh -= len(newbuf)
        if decode:
            return buf.decode()
        else:
            return buf

    def clear_buffer(self):
        try:
            while self.conn.recv(1024):
                pass
        except:
            pass
        return


##########################################################


def read_radio_button(button):
    if button.get_active():
        return button.get_label()


def clear_placeholder_text(widget, event):
    if widget.get_text() == "Enter prompt":
        widget.set_text("")


def is_valid_int(text):
    try:
        int(text)
        return True
    except ValueError:
        return False


# Function to restrict input to integers only
def on_seed_entry_changed(widget):
    text = seed_entry.get_text()
    if not is_valid_int(text):
        seed_entry.set_text("")  # Clear the entry if it's not a valid integer


def display_image(prompt, step, guidance_scale, seed, number_of_images):
    width, height, image_type = 512, 512, 0
    image_new = gimp.Image(width, height, image_type)
    display = gimp.Display(image_new)

    curr_file_path = os.path.abspath(__file__)
    img_path = os.path.join(current_dir, "test.jpeg")
    result = pdb.file_jpeg_load(img_path, img_path)

    result_layer = pdb.gimp_image_get_active_layer(result)

    copy = pdb.gimp_layer_new_from_drawable(result_layer, image_new)
    pdb.gimp_item_set_name(
        copy,
        "Step:{} Seed:{} Guidance:{} Prompt:{}".format(
            step, seed, guidance_scale, prompt
        ),
    )
    pdb.gimp_layer_set_mode(copy, NORMAL_MODE)
    pdb.gimp_image_insert_layer(image_new, copy, None, -1)

    gimp.displays_flush()

    # Deleting the Image after display
    os.remove(os.path.join("test.jpeg"))


def sd_plugin_run(dropdown_list, draw):
    # Dialog setup
    dialog = gtk.Dialog(
        "Stable Diffusion", None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT
    )
    dialog.set_border_width(10)
    dialog.modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("#333"))
    content_area = dialog.get_content_area()

    # Add the "Powered by" text
    hbox = gtk.HBox()
    powered_by_label = gtk.Label("Powered by")
    powered_by_label.modify_fg(
        gtk.STATE_NORMAL, gtk.gdk.color_parse("white")
    )  # Set text color to white
    hbox.pack_start(powered_by_label, expand=False, fill=False, padding=5)

    # Add Qualcomm Image
    image_path = "./plugin_logo.png"
    image_widget = gtk.Image()
    image_widget.set_from_file(image_path)
    hbox.pack_start(image_widget)
    content_area.pack_start(hbox)

    # Add dropdown list
    hbox3 = gtk.HBox()
    dropdown_label = gtk.Label("Device:")
    dropdown_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    dropdown_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox3.pack_start(dropdown_label)
    model = gtk.ListStore(str)
    model.append(["Qualcomm Hexagon NPU"])
    combo_box = gtk.ComboBox(model)
    cell = gtk.CellRendererText()
    combo_box.pack_start(cell, True)
    combo_box.add_attribute(cell, "text", 0)
    hbox3.pack_start(combo_box)
    combo_box.set_active(0)  # Set "Qualcomm" as the default value
    content_area.pack_start(hbox3)
    content_area.pack_start(gtk.Label())  # Add an empty label for vertical space

    # Prompt
    hbox1 = gtk.HBox()
    prompt_label = gtk.Label("Enter text to get image:")
    prompt_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    prompt_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox1.pack_start(prompt_label)
    prompt_entry = gtk.Entry()
    prompt_entry.modify_text(gtk.STATE_NORMAL, gtk.gdk.color_parse("black"))
    prompt_entry.set_text("Enter prompt")  # Set default value
    prompt_entry.connect("focus-in-event", clear_placeholder_text)
    hbox1.pack_start(prompt_entry)
    content_area.pack_start(hbox1)
    content_area.pack_start(gtk.Label())  # Add an empty label for vertical space

    # Seed
    hbox4 = gtk.HBox()
    seed_label = gtk.Label("Seed:")
    seed_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    seed_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox4.pack_start(seed_label)
    seed_entry = gtk.Entry()
    seed_entry.set_text("5")
    hbox4.pack_start(seed_entry)
    content_area.pack_start(hbox4)
    content_area.pack_start(gtk.Label())
    seed_entry.connect(
        "changed", on_seed_entry_changed
    )  # Connect the function to the "changed" signal of the entry

    # Number of Inference - Dropdown list
    hbox5 = gtk.HBox()
    steps_label = gtk.Label("Number of Inference Steps:")
    steps_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    steps_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox5.pack_start(steps_label)
    model_steps = gtk.ListStore(str)
    model_steps.append(["20"])
    model_steps.append(["50"])
    combo_box_steps = gtk.ComboBox(model_steps)
    cell_steps = gtk.CellRendererText()
    combo_box_steps.pack_start(cell_steps, True)
    combo_box_steps.add_attribute(cell_steps, "text", 0)
    hbox5.pack_start(combo_box_steps)
    combo_box_steps.set_active(0)  # Set "20" as the default value
    content_area.pack_start(hbox5)
    content_area.pack_start(gtk.Label())  # Add an empty label for vertical space

    # Scale
    hbox3 = gtk.HBox()
    scale_label = gtk.Label("Guidance scale:")
    scale_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    scale_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox3.pack_start(scale_label)
    scale_entry = gtk.Entry()
    scale_entry.set_text("7.5")
    hbox3.pack_start(scale_entry)
    content_area.pack_start(hbox3)
    content_area.pack_start(gtk.Label())

    # Number of images
    hbox5 = gtk.HBox()
    num_images_label = gtk.Label("Number of images:")
    num_images_label.modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
    num_images_label.set_alignment(0, 0.5)  # Align label text to the left
    hbox5.pack_start(num_images_label)
    num_images_spin = gtk.SpinButton(gtk.Adjustment(1, 0, 100, 1, 10, 0), 1.0, 0)
    num_images_spin.set_text("1")
    hbox5.pack_start(num_images_spin)
    content_area.pack_start(hbox5)
    content_area.pack_start(gtk.Label())

    # Add Generate and Cancel buttons
    generate_button = gtk.Button("Generate")
    dialog.add_action_widget(generate_button, gtk.RESPONSE_OK)
    cancel_button = gtk.Button(stock=gtk.STOCK_CANCEL)
    dialog.add_action_widget(cancel_button, gtk.RESPONSE_CANCEL)

    # Add Help button
    help_button = gtk.Button("Help")
    help_button.connect(
        "clicked",
        lambda x: webbrowser.open(
            "https://www.qualcomm.com/developer/artificial-intelligence#overview"
        ),
    )
    dialog.add_action_widget(help_button, gtk.RESPONSE_HELP)

    # Start the client and server
    sd_initialise_thread = None
    sd_initialise_thread = threading.Thread(target=initiate_sd_execution, args=())
    sd_initialise_thread.start()
    global server
    server = Server("127.0.0.1", 5001)
    message = server.receive()
    message = message.replace(" ", "")
    # gimp_log(message)

    if message != "initialized":
        gimp_log("Error to connect with cpp file")
        server.send("end")
        taskkill_cmd = "taskkill /im gimp_sd_wos_plugin.exe /f"
        subprocess.call(taskkill_cmd, shell=True)
        dialog.destroy()
        return

    # Display the dialog
    dialog.show_all()
    response = dialog.run()

    # Read The values and log it
    if response == gtk.RESPONSE_OK:
        try:
            prompt = str(prompt_entry.get_text())
            prompt = " " + prompt
            selected_steps = str(combo_box_steps.get_active_text())
            seed = int(seed_entry.get_text())
            scale = float(scale_entry.get_text())
            num_img = int(num_images_spin.get_text())
        except:
            gimp_log("Please enter valid input")
            server.send("end")
            taskkill_cmd = "taskkill /im gimp_sd_wos_plugin.exe /f"
            subprocess.call(taskkill_cmd, shell=True)
            dialog.destroy()
            return

        # Send payload to the server
        message = "{}:{}:{}:{}:{}:".format(scale, seed, selected_steps, prompt, num_img)
        server.send(message)
        # gimp_log(message)

        # # Start Inference
        for i in range(0, num_img):
            # Log the Inference time.
            start_time = time.time()

            server.send("execute_next\0")
            message = server.receive()

            execution_time = time.time() - start_time
            log_message = "Execution time :" + str(execution_time)
            gimp_log(log_message)
            # Display the image in a new layer
            display_image(prompt, selected_steps, scale, seed, num_img)

    # If user closes the window with cancle, help or cross above.
    else:
        pass
        # gimp_log('User closed the window without clicking any action button')

    # End the connection and Kill the process
    server.send("end")
    taskkill_cmd = "taskkill /im gimp_sd_wos_plugin.exe /f"
    subprocess.call(taskkill_cmd, shell=True)
    dialog.destroy()


register(
    "Stable-Diffusion",
    N_("Powered by Qualcomm AI stack"),
    "Given a prompt Image will be generated",
    "quic",
    "Qualcomm AI Plugins",
    "2024",
    N_("_Stable Diffusion 1.5"),
    "*",
    [
        (PF_IMAGE, "image", "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
    ],
    [],
    sd_plugin_run,
    menu="<Image>/Layer/Qualcomm AI Plugins/",
    domain=("gimp20-python", gimp.locale_directory),
)

main()

# !/usr/bin/env python3
"""
Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
"""

import gi

gi.require_version("Gimp", "3.0")
gi.require_version("GimpUi", "3.0")
gi.require_version("Gtk", "3.0")
from gi.repository import Gimp, GimpUi, GObject, GLib, Gio, Gtk
import gettext
import subprocess
import json
import pickle
import os
import sys
from enum import IntEnum
import random
import glob
from pathlib import Path
import gettext
import time
import socket


class Server():
  def __init__(self, ip, port, automatic_port=True):
    self.__size_message_length = 16  # Buffer size for the length
    max_connections_attempts = 5

    # Start and connect to client
    self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if automatic_port:
      connected = False
      while (not connected) and (max_connections_attempts > 0):
        try:
          self.s.bind((ip, port))
          connected = True
        except:
          print("[Server]: Port", port, "already in use. Binding to port:", port+1)
          port += 1
          max_connections_attempts -= 1
      if not connected:
        print("[Server]: Error binding to adress!")
    else:
      self.s.bind((ip, port))

    self.s.listen(True)
    print("[Server]: Waiting for connection...")
    self.conn, addr = self.s.accept()
    print("[Server]: Connected")

  def __del__(self):
    self.s.close()

  def send(self, message):
    message_size = str(len(message)).ljust(self.__size_message_length).encode()
    self.conn.sendall(message_size)  # Send length of msg (in known size, 16)
    self.conn.sendall(message.encode())  # Send message

  def receive(self, decode=True):
    length = self.__receive_value(self.conn, self.__size_message_length)
    if length is not None:  # If None received, no new message to read
      message = self.__receive_value(self.conn, int(length), decode)  # Get message
      return message
    return None

  def __receive_value(self, conn, buf_lentgh, decode=True):
    buf = b''
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
      while self.conn.recv(1024): pass
    except:
      pass
    return

def show_dialog(message, title, icon="logo", image_paths=None):
    use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
    dialog = GimpUi.Dialog(use_header_bar=use_header_bar, title=_(title))
    # Add buttons
    dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
    dialog.add_button("_OK", Gtk.ResponseType.APPLY)
    vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, homogeneous=False, spacing=10)
    dialog.get_content_area().add(vbox)
    vbox.show()

    # Create grid to set all the properties inside.
    grid = Gtk.Grid()
    grid.set_column_homogeneous(False)
    grid.set_border_width(10)
    grid.set_column_spacing(10)
    grid.set_row_spacing(10)
    vbox.add(grid)
    grid.show()

    # Show Logo
    logo = Gtk.Image.new_from_file(image_paths[icon])
    vbox.pack_start(logo, False, False, 1)
    grid.attach(logo, 0, 0, 1, 1)
    logo.show()
    # Show message
    label = Gtk.Label(label=_(message))
    vbox.pack_start(label, False, False, 1)
    grid.attach(label, 1, 0, 1, 1)
    label.show()
    dialog.show()
    dialog.run()
    return


def save_image(image, drawable, file_path):
    interlace, compression = 0, 2
    Gimp.get_pdb().run_procedure(
        "file-png-save",
        [
            GObject.Value(Gimp.RunMode, Gimp.RunMode.NONINTERACTIVE),
            GObject.Value(Gimp.Image, image),
            GObject.Value(GObject.TYPE_INT, 1),
            GObject.Value(
                Gimp.ObjectArray, Gimp.ObjectArray.new(Gimp.Drawable, drawable, 0)
            ),
            GObject.Value(
                Gio.File,
                Gio.File.new_for_path(file_path),
            ),
            GObject.Value(GObject.TYPE_BOOLEAN, interlace),
            GObject.Value(GObject.TYPE_INT, compression),

            GObject.Value(GObject.TYPE_BOOLEAN, True),
            GObject.Value(GObject.TYPE_BOOLEAN, True),
            GObject.Value(GObject.TYPE_BOOLEAN, False),
            GObject.Value(GObject.TYPE_BOOLEAN, True),
        ],
    )


def N_(message):
    return message



sys.path.extend([os.path.join(os.path.dirname(os.path.realpath(__file__)))])

_ = gettext.gettext
image_paths = {
    "logo": os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "plugin_logo.png"
    ),
    "error": os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "error_icon.png"
    ),
}


class StringEnum:
    """
    Helper class for when you want to use strings as keys of an enum. The values would be
    user facing strings that might undergo translation.
    The constructor accepts an even amount of arguments. Each pair of arguments
    is a key/value pair.
    """

    def __init__(self, *args):
        self.keys = []
        self.values = []

        for i in range(len(args) // 2):
            self.keys.append(args[i * 2])
            self.values.append(args[i * 2 + 1])

    def get_tree_model(self):
        """Get a tree model that can be used in GTK widgets."""
        tree_model = Gtk.ListStore(GObject.TYPE_STRING, GObject.TYPE_STRING)
        for i in range(len(self.keys)):
            tree_model.append([self.keys[i], self.values[i]])
        return tree_model




class DeviceEnum:
    def __init__(self, supported_devices):
        self.keys = []
        self.values = []
        for i in supported_devices:
            self.keys.append(i)
            self.values.append(i)


    def get_tree_model(self):
        """Get a tree model that can be used in GTK widgets."""
        tree_model = Gtk.ListStore(GObject.TYPE_STRING, GObject.TYPE_STRING)
        for i in range(len(self.keys)):
            tree_model.append([self.keys[i], self.values[i]])
        return tree_model

    def get_tree_model_no_npu(self):
        """Get a tree model that can be used in GTK widgets."""
        tree_model = Gtk.ListStore(GObject.TYPE_STRING, GObject.TYPE_STRING)
       
        for i in range(len(self.keys)):
            if self.keys[i] != "NPU":
                tree_model.append([self.keys[i], self.values[i]])
        return tree_model

class SDDialogResponse(IntEnum):
    LoadModelComplete = 777
    RunInferenceComplete = 778
    ProgressUpdate = 779
    
class SDRunner:
    def __init__ (self, procedure, image, drawable, prompt, seed, step, guidance_scale, progress_bar):
        self.procedure = procedure
        self.image = image
        self.drawable = drawable
        self.prompt = prompt
        self.seed = seed
        self.step = step
        self.guidance_scale = guidance_scale
        self.progress_bar = progress_bar
        self.result = None

    def run(self, dialog, num_images):
        from datetime import datetime
        start_time = datetime.now()
        file1 = open("execution_time.txt", "w")
        file1.write("Entered run\n")
        procedure = self.procedure
        image = self.image
        drawable = self.drawable
        prompt = self.prompt
        progress_bar = self.progress_bar
        seed = self.seed
        step = self.step
        guidance_scale = self.guidance_scale
        Gimp.context_push()
        image.undo_group_start()
        
        for i in range(0, num_images):
            server.send("execute_next\0")
            message = server.receive()
            print("[SERVER RECEIVED]:" + message)
            if "execution_complete" == message: 
                file1.write("execution complete received for image: {}\n".format(i))

            image_new = Gimp.Image.new(
                512, 512, 0
            )

            display = Gimp.Display.new(image_new)
            result = Gimp.file_load(
                Gimp.RunMode.NONINTERACTIVE,
                Gio.file_new_for_path(os.path.join("test.jpeg")),
            )
            try:
                # 2.99.10
                result_layer = result.get_active_layer()
            except:
                # > 2.99.10
                result_layers = result.list_layers()
                result_layer = result_layers[0]

            copy = Gimp.Layer.new_from_drawable(result_layer, image_new)
            copy.set_name("Step:{} Seed:{} Guidance:{} Prompt:{}".format(step, seed, guidance_scale, prompt))
            copy.set_mode(Gimp.LayerMode.NORMAL_LEGACY)  # DIFFERENCE_LEGACY
            image_new.insert_layer(copy, None, -1)

            Gimp.displays_flush()
            image.undo_group_end()
            Gimp.context_pop()

            

            
        server.send("end")
        os.remove(os.path.join("test.jpeg"))
        execution_time = datetime.now() - start_time 

        file1.write("Execution time : {}".format(execution_time))
        file1.close()
        self.result = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
        return self.result

def async_sd_run_func(runner, dialog, num_images):
    print("Running SD async")
    runner.run(dialog, num_images)
    print("async SD done")
    dialog.response(SDDialogResponse.RunInferenceComplete)

#
# This is what brings up the UI
#
    
def syscmd(cmd, encoding=''):
    """
    Runs a command on the system, waits for the command to finish, and then
    returns the text output of the command. If the command produces no text
    output, the command's return code will be returned instead.
    """
    p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        close_fds=True)
    p.wait()
    output = p.stdout.read()
    if len(output) > 1:
        if encoding: return output.decode(encoding)
        else: return output
    return p.returncode

def initiate_sd_execution():
    from pathlib import Path
    home = Path.home()
    cmd = 'gimp_sd_wos_plugin.exe --config_file_Path injected.stablediffusion.properties.in --backend QnnHtp.dll'
    os.chdir(f"{home}/AppData/Roaming/GIMP/2.99/plug-ins/sd-snapdragon")
    subprocess.call(cmd, shell=True)


def run(procedure, run_mode, image, n_drawables, layer, args, data):
    if run_mode == Gimp.RunMode.INTERACTIVE:

        supported_devices = ["NPU"]

        device_name_enum = DeviceEnum(supported_devices) 
        #config_path_output["supported_devices"])
        config = procedure.create_config()
        #config.begin_run(image, run_mode, args)

        GimpUi.init("sd-htp.py")
        use_header_bar = Gtk.Settings.get_default().get_property(
            "gtk-dialogs-use-header"
        )
        dialog = GimpUi.Dialog(use_header_bar=use_header_bar, title=_("Stable Diffusion"))
        dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("_Help", Gtk.ResponseType.HELP)
        run_button = dialog.add_button("_Generate", Gtk.ResponseType.OK)
        run_button.set_sensitive(True)

        vbox = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL, homogeneous=False, spacing=10
        )
        dialog.get_content_area().add(vbox)
        vbox.show()

        # Create grid to set all the properties inside.
        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        grid.set_border_width(10)
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        # Show Logo
        powered_by_text = _("Powered by")
        powered_by_label = Gtk.Label(label=powered_by_text)
        grid.attach(powered_by_label, 0, 0, 1, 1)
        powered_by_label.show()
        logo = Gtk.Image.new_from_file(image_paths["logo"])
        grid.attach(logo, 1, 0, 1, 1)
        vbox.pack_start(logo, False, False, 1)
        logo.show()

        vbox.add(grid)
        grid.show()
        # Prompt text
            
        
        basic_device_label = Gtk.Label.new_with_mnemonic(_("_Device"))
        grid.attach(basic_device_label, 0,1,1,1)
        basic_device_label.show()

        basic_device_combo = GimpUi.prop_string_combo_box_new(
            config, "device_name", device_name_enum.get_tree_model(), 0, 1
        )
        grid.attach(basic_device_combo, 1,1,1,1)

        prompt_text = Gtk.Entry.new()
        grid.attach(prompt_text, 1, 2, 1, 1)
        prompt_text.set_width_chars(60)
        prompt_text.show()

        prompt_text_label = _("Enter text to generate image")
        prompt_label = Gtk.Label(label=prompt_text_label)
        grid.attach(prompt_label, 0, 2, 1, 1)
        vbox.pack_start(prompt_label, False, False, 1)
        prompt_label.show()


        # num_infer_steps parameter
        label = Gtk.Label.new_with_mnemonic(_("_Number of inference steps"))
        grid.attach(label, 0, 3, 1, 1)
        label.show()
        spin_steps = GimpUi.prop_spin_button_new(
            config, "num_infer_steps", step_increment=30, page_increment=0.1, digits=0
        )
        grid.attach(spin_steps, 1, 3, 1, 1)
        spin_steps.show()

        # guidance_scale parameter
        label = Gtk.Label.new_with_mnemonic(_("_Guidance scale"))
        grid.attach(label, 0, 4, 1, 1)
        label.show()
        spin_guidance = GimpUi.prop_spin_button_new(
            config, "guidance_scale", step_increment=0.1, page_increment=0.1, digits=1
        )
        grid.attach(spin_guidance, 1, 4, 1, 1)
        spin_guidance.show()

        # seed
        seed = Gtk.Entry.new()
        grid.attach(seed, 1, 5, 1, 1)
        seed.set_width_chars(40)
        seed.set_placeholder_text(_("If left blank, random seed between 0 to 50 will be set.."))
        seed.show()

        seed_text = _("Seed")
        seed_label = Gtk.Label(label=seed_text)
        grid.attach(seed_label, 0, 5, 1, 1)
        seed_label.show()

        # num_images parameter
        num_images_label = Gtk.Label.new_with_mnemonic(_("_Number of images"))
        grid.attach(num_images_label, 0, 6, 1, 1)
        num_images_label.show()
        num_images_steps = GimpUi.prop_spin_button_new(
            config, "num_image", step_increment=1, page_increment=0.1, digits=0
        )
        grid.attach(num_images_steps, 1, 6, 1, 1)
        num_images_steps.show()

        # status label
        sd_run_label = Gtk.Label(label="Running Stable Diffusion...") 
        grid.attach(sd_run_label, 1, 12, 1, 1)

        # spinner
        spinner = Gtk.Spinner()
        grid.attach_next_to(spinner, sd_run_label, Gtk.PositionType.RIGHT, 1, 1)

        

        # Show License

        progress_bar = Gtk.ProgressBar()
        vbox.add(progress_bar)
        progress_bar.show()
        progress_bar = None

        
        # Wait for user to click
        dialog.show()
        
        import threading
        run_inference_thread = None
        sd_initialise_thread = None
        
        global server

        
        
        sd_initialise_thread = threading.Thread(target=initiate_sd_execution, args=())
        sd_initialise_thread.start()
        run_button.set_sensitive(False)
        
        server = Server("127.0.0.1", 5001)
        message = server.receive()
        print("[SERVER RECEIVED]:" + message)   
        if "initialized" == message.lower():
            run_button.set_sensitive(True)

        runner = None
        
        while True:
            response = dialog.run()    

                                   
                
            if response == Gtk.ResponseType.OK:
                
                run_button.set_sensitive(False)
                basic_device_combo.set_sensitive(False)

                prompt = prompt_text.get_text()

                if("#" != prompt[0]):
                    prompt = "#" + prompt
                    
                if len(seed.get_text()) != 0:
                    seed = int(seed.get_text())
                else:
                    seed = random.randint(0,50)

                step = int(spin_steps.get_text())
                guidance_scale = float(spin_guidance.get_text())
                num_images = int(num_images_steps.get_text())
                
                server.send("{}:{}:{}:{}:{}:".format(guidance_scale, seed, step, prompt, num_images))

                runner = SDRunner(procedure, image, layer, prompt, seed, step, guidance_scale, progress_bar)

                sd_run_label.set_label("Running Stable Diffusion...")
                sd_run_label.show()
                spinner.start()
                spinner.show()
                

                run_inference_thread = threading.Thread(target=async_sd_run_func, args=(runner, dialog, num_images))
                run_inference_thread.start()
                run_button.set_sensitive(False)
                

            elif response == Gtk.ResponseType.HELP:

                url = "https://www.qualcomm.com/developer/artificial-intelligence#overview"
                Gio.app_info_launch_default_for_uri(url, None)
                continue

            elif response == SDDialogResponse.RunInferenceComplete:
                print("run inference complete.")
                if run_inference_thread:
                    run_inference_thread.join()
                    result = runner.result
                    if result.index(0) == Gimp.PDBStatusType.SUCCESS and config is not None:
                        config.end_run(Gimp.PDBStatusType.SUCCESS)
                    return result
            elif response == SDDialogResponse.ProgressUpdate:
                progress_string=""
                if runner.current_step == runner.num_infer_steps:
                    progress_string = "Running Stable Diffusion... Finalizing Generated Image"
                else:
                    progress_string = "Running Stable Diffusion... (Inference Step " + str(runner.current_step + 1) +  " / " + str(runner.num_infer_steps) + ")"

                sd_run_label.set_label(progress_string)
                if runner.num_infer_steps > 0:
                    perc_complete = runner.current_step / runner.num_infer_steps
                    progress_bar.set_fraction(perc_complete)

            else:
                dialog.destroy()
                taskkill_cmd = "taskkill /im gimp_sd_wos_plugin.exe /f"
                subprocess.call(taskkill_cmd, shell=True)
                return procedure.new_return_values(
                    Gimp.PDBStatusType.CANCEL, GLib.Error()
                )


class StableDiffusion(Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "num_infer_steps": (
        int, _("_Number of inference steps (Default:32)"), "Number of inference steps (Default:20)", 20, 50, 20,
        GObject.ParamFlags.READWRITE,),
        "num_image": (
        int, _("_Number of images to be generated (Default:1)"), "Number of images to be generated (Default:1)", 1, 10, 1,
        GObject.ParamFlags.READWRITE,),
        "guidance_scale": (float, _("Guidance Scale (Default:7.5)"), "_Guidance Scale (Default:7.5)", 5, 20.0, 7.5,
                           GObject.ParamFlags.READWRITE,),
        "guidance_scale": (float, _("Guidance Scale (Default:7.5)"), "_Guidance Scale (Default:7.5)", 5, 20.0, 7.5,
                           GObject.ParamFlags.READWRITE,),
        "device_name": (
            str,
            _("Device Name"),
            "Device Name: 'NPU'",
            "NPU",
            GObject.ParamFlags.READWRITE,
        ),
    }

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):

        try:
            self.set_translation_domain(
                "gimp30-python", Gio.file_new_for_path(Gimp.locale_directory())
            )
        except:
            print("Error in set_translation_domain. This is expected if running GIMP 2.99.11 or later")

        return ["stable-diffusion-htp"]

    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_create_procedure(self, name):
        procedure = None
        if name == "stable-diffusion-htp":
            procedure = Gimp.ImageProcedure.new(
                self, name, Gimp.PDBProcType.PLUGIN, run, None
            )
            procedure.set_image_types("*")
            procedure.set_documentation(
                N_("Stable Diffusion 1.5 on the current layer."),
                globals()[
                    "__doc__"
                ],  # This includes the docstring, on the top of the file
                name,
            )
            procedure.set_menu_label(N_("Stable Diffusion 1.5"))
            procedure.set_attribution("QUIC", "Qualcomm AI Plugins", "2024")
            procedure.add_menu_path("<Image>/Layer/Qualcomm AI Plugins/")

            procedure.add_argument_from_property(self, "num_infer_steps")
            procedure.add_argument_from_property(self, "num_image")
            procedure.add_argument_from_property(self, "guidance_scale")
            procedure.add_argument_from_property(self, "device_name")

        return procedure


Gimp.main(StableDiffusion.__gtype__, sys.argv)
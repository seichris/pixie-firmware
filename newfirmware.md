The device we are currently working on is the Firefly Pixie, based on the ESP32-C3 (32-bit, RISC-V; 400kb RAM), 16MB flash, 240x240 16-bit color IPS display and 4 buttons.

the readme.md file has all the info needed to know more about the hardware and the picture of the device is also attached.

this code base is the firmware for such device, however following the docker build guide fails with this error: 

/opt/esp/idf/components/bt/host/nimble/nimble/nimble/host/services/hid/src/ble_svc_hid.c: In function 'fill_boot_mouse_inp':
/opt/esp/idf/components/bt/host/nimble/nimble/nimble/host/services/hid/src/ble_svc_hid.c:264:19: error: expected ';' before numeric constant
  264 |                   0;
      |                   ^
[884/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/src/ble_hs_shutdown.c.obj
[885/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/services/cts/src/ble_svc_cts.c.obj
[886/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/services/sps/src/ble_svc_sps.c.obj
[887/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/services/cte/src/ble_svc_cte.c.obj
[888/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/src/ble_store_util.c.obj
[889/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/src/ble_hs_conn.c.obj
[890/1136] Building C object esp-idf/bt/CMakeFiles/__idf_bt.dir/host/nimble/nimble/nimble/host/src/ble_sm.c.obj
ninja: build stopped: subcommand failed.
ninja failed with exit code 1, output of the command is in the /project/build/log/idf_py_stderr_output_319 and /project/build/log/idf_py_stdout_output_319

=======================================================

What I want you to do is to understand the basics of the hardware, and write a new firmware including a few games. this new firmwire should include easier method and guideline on how to boot the firmware on the device.


the idea is either to use the current firmware code if needed, if it seems too complex we can start from scratch or just copy paste some codes needed, do as you think is the best


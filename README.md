# prukaSlicer
Simple openframeworks application to convert regular .gcode for 3D-printing into Kuka KRL.

prukaSlicer is used to create KRL .src files from .gcode files. It has some tools for adjusting Z-offset of the print, a crude preview and settings for adjusting flowrate. This is all custom work for our own setup which uses an analog output on the robot to make an extruder spin between 0-150 RPM. This may or may not be useful to your setup.

Use this program with caution; it has the ability to crash you our your robot completely atonomously ;) Always start the robot slow.

This source has been build with openFrameworks 11.2 on Windows, the gcode generation is done by PrusaSlicer 2.7.1, a preset for slicing has been provided in config.ini

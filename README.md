# Direct3D 12 DXR demo :sparkler:
## A fully raytraced dynamic scene with skeletal animated models 
##### Made by Alexander Wester, Fredrik Junede and Henrik Johansson for a project in the course DV2551 at BTH 2019.
#### [Associated report](Report.pdf)
[Pre-built binaries can be found here](https://github.com/Piratkopia13/DV2551_Project_DXR/releases)
### Requirements
 - Windows version 1809 or later
 - Visual studio 2017 or later
 - A Graphics card with DXR support (currently only Nvidia RTX or GTX 1060 6GB or above)

### Please note
-  Animation skinning is done on the CPU (reason explained in report). This is very slow! Turn down animation speed to 0 in the GUI to fully disable skinning to show pure raytracing performance.
- Run in release configuration for all features. Running in debug will only show dummy models.
- Resolution can be changed in the beginning of src/Game.cpp. Default resolution is 1700x900.
- Temporal Accumulation is used to reduce noise in static objects. Disable this when looking at the animated models (checkbox called " TA" in Raytracer gui window).
### Media
![Demo image](demo.png)
#### [Demo video](https://www.youtube.com/watch?v=mwR-VezOWOk)

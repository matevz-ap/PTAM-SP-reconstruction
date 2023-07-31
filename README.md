# Reconstruction CLI

Expanding original dekstop [app](https://github.com/KristianZarn/Reconstruction) with command line interface in order to use it on with [server](https://github.com/matevz-ap/PTAM-SP-reconstruction-server). 

## Building
Original build instruction are available [here](https://github.com/KristianZarn/Reconstruction#building).

### Dependencies
- [Colmap](https://github.com/matevz-ap/colmap)
- [TheiaSfM](https://github.com/sweeneychris/TheiaSfM) 
- [openMVS](https://github.com/cdcseacave/openMVS/tree/v2.0.1)

Other smaller dependencies are located in the [packages.txt](https://github.com/matevz-ap/PTAM-SP-reconstruction/blob/master/packages.txt) file.

Sadly Colmap and Theia need custom fixes so I forked them.
OpenMVS is locked to this version otherwise it breaks some functionalities. 

Dokcer file should serve as instructions for building. Avoid Cuda 11.6 as it is not compatible with dependancies.

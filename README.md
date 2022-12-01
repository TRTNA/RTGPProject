
# Rendering of objects in homogeneous participating media

The project is a real-time application developed in C++ and OpenGL 4.1 for the Real-Time Graphics Programming master's degree course exam.
The scene is rendered considering the presence of a homogeneous participating media which interacts with the light.
Those interactions are calculated through a ray-marching from the camera to every fragment.

The application displays a GUI to change the absorption and scattering coefficients of the media, the phase function used, the position of the point light in the space and the technique used to render the skybox (participating media or volumetric fog).

The theory for the participating media effect was taken from *Real-Time Rendering, Fourth Edition* by *Eric Haines* [ch. 14]

## Screenshots

**Render with no participating media**
![alt text](https://github.com/TRTNA/RTGPProject/blob/main/Screenshots/no_participatingmedia_objects.png?raw=true)

**Render with medium participating media 2**
![alt text](https://github.com/TRTNA/RTGPProject/blob/main/Screenshots/mediumscattering_objects.png?raw=true)

**Render with medium participating media 2**
![alt text](https://github.com/TRTNA/RTGPProject/blob/main/Screenshots/medium_scattering_objects1.png?raw=true)

**Volumetric light effect 1**
![alt text](https://github.com/TRTNA/RTGPProject/blob/main/Screenshots/volumetric_light.png?raw=true)

**Volumetric light effect 2**
![alt text](https://github.com/TRTNA/RTGPProject/blob/main/Screenshots/volumetric_light1.png?raw=true)


## Authors

- [@TRTNA](https://www.github.com/TRTNA)

The project has been developed using some RTGP course lab lectures' code (developed by the professor).
The main of the application, all the shaders and some of the classes have been developed by me.

## License
This work is licensed under a Creative Commons Attribution 4.0 International License: https://creativecommons.org/licenses/by-nc-sa/4.0/
![alt text](https://mirrors.creativecommons.org/presskit/buttons/88x31/svg/by-nc-sa.eu.svg)


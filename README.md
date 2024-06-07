# Bilinear Interpolation 2024

## Description

This project is about image processing. The goal is to convert color images to grayscale and then scale them using bilinear interpolation.

The aim was to convert coloured input images into greyscale images and scale them as required. The gaps created by the scaling are filled using bilinear interpolation. Vectorisation was used to optimise the V0 algorithm in order to improve the runtime.
The outlook can go in several directions. On the one hand, the division and multiplication that have been implemented for vectorisation can be made even more efficient. The greyscale calculation that runs through once can also be vectorised. On the other hand, there is certainly the possibility of optimising the mathematical formulas. Another idea that could be interesting would be the use of multithreading. This topic still offers many possibilities for optimisation and improvement, although it does not have to be limited to bilinear interpolation.

## Project Structure

InterpolationApp/
├── app/
│ ├── input_data/ 
│ ├── grayscale.c
│ ├── grayscale.h 
│ ├── interpolate.c
│ ├── interpolate.h
│ ├── main.c
│ └── Makefile


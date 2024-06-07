# Bilinear Interpolation 2024

## Description

This project is about image processing. The goal is to convert color images to grayscale and then scale them using bilinear interpolation.

The aim was to convert coloured input images into greyscale images and scale them as required. The gaps created by the scaling are filled using bilinear interpolation. Vectorisation was used to optimise the V0 algorithm in order to improve the runtime.
The outlook can go in several directions. On the one hand, the division and multiplication that have been implemented for vectorisation can be made even more efficient. The greyscale calculation that runs through once can also be vectorised. On the other hand, there is certainly the possibility of optimising the mathematical formulas. Another idea that could be interesting would be the use of multithreading. This topic still offers many possibilities for optimisation and improvement, although it does not have to be limited to bilinear interpolation.

## Project Structure
```
InterpolationApp/
├── app/
│ ├── input_data/
│ ├── .gitignore
│ ├── grayscale.c
│ ├── grayscale.h
│ ├── interpolate.c
│ ├── interpolate.h
│ ├── main.c
│ └── Makefile
```
## Getting Started

### Prerequisites

- A C compiler (e.g., `gcc`)
- Make utility

### Building the Project

To build the project, navigate to the `app` directory and run:

```sh
make
```

This will compile the source files and generate the executable.

### Running the Application

The application supports several command-line options:

- `-V<Number>`: Specify the implementation to be used. Use `-V 0` for your main implementation. If this option is not set, the main implementation will be executed.
- `-B<Number>`: If set, the runtime of the specified implementation will be measured and output. The optional argument specifies the number of repetitions of the function call.
- `<Filename>`: Positional argument for the input file.
- `-o<Filename>`: Output file.
- `--coeffs<FP Number>,<FP Number>,<FP Number>`: Coefficients for grayscale conversion (a, b, and c). If this option is not set, the default values will be used.
- `-f<Number>`: Scaling factor.
- `-h|--help`: Displays a description of all program options and usage examples, then exits.

### Example Usage

To run the application with the main implementation and an input file:

```sh
./interpolationapp -V 0 input_file.txt -o output_file.txt
```

To measure the runtime of the implementation with 10 repetitions:

```sh
./interpolationapp -B 10 -V 0 input_file.txt
```

To set custom coefficients for grayscale conversion:

```sh
./interpolationapp --coeffs 0.3,0.59,0.11 input_file.txt -o output_file.txt
```

To apply a scaling factor:

```sh
./interpolationapp -f 2 input_file.txt -o output_file.txt
```

For help:

```sh
./interpolationapp -h
```



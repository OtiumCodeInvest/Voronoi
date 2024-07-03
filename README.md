# Voronoi

Dual sweep pass for generating an image that contains a vector for each pixel that points to the nearest non zero pixel for all pixels in an image. This is similar to the function distanceTransform in opencv.

## Build command on Linux
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Build command on Windows
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..
$ Open voronoi.sln project in Visual Studio and build
```

Alternative open as folder


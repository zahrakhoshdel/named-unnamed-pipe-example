# named-unnamed-pipe-example
Calculate histogram and Guassian filter to a bmp image

Targets : 
* Takes BMP images as input 
* Calculate the histogram of an image
* Applying a Gaussian filter
* Communication between processes as pipe unnamed / named

First we create a parent process and three subprocesses named A, B and C with the fork () command.
- child A: histogram_calculator
- child B: filtering (Gaussian)
- child C: image address and histogram

flowchart of Communication between processes
![flowchart](https://user-images.githubusercontent.com/91828519/141436657-38fc2c92-0e88-41b3-86bb-e41c0eed7609.png)

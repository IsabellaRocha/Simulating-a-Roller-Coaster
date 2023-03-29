# Assignment 2: Simulating a Roller Coaster
### Background of system used
This assignment was written on a Windows device using Visual Studio 2019. To change the command line input, the debugger under properties was used and then to run the Local Windows Debugger was used.

### Basic features
- The curve of the rollercoaster is rendered from a spline from a .sp file, specified in track.txt
- A rectangle is rendered for the track based off the points normal and binormal
- The camera follows along the spline to simulate first person view
- A 512x512 ground is rendered along z = -300
- The ground uses texture mapping from a texture image, specified as input to the initTexture() function
- Click 'x': Takes a screenshot
- Click 'm': Stops and starts motion along the rollercoaster
- Click 's': Take a screenshot every time the camera moves 20 units, significantly slows down movement along the rollercoaster

### Extra features implemented
- The curve was extended to be fully closed (C1 continuous)
  - However, with goodRide.sp, the camera ends up on the bottom towards the end of the spline, so while the loop is closed and continuous, the camera jumps back up to the top once finished

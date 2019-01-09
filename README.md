# EzBMP
A simple header file that easily imports or exports BMP files with the ability to change certain properties associated in the BMP file format. 

BMP files are often used in programs to represent heightmaps which can be easily done with this header file.

---

### How to read and write an 8 bit BMP file.
Code:
```
// Create a BMP object with the type of info header we desire: V4InfoHeader
bmp::BMPFileUserDefined<bmp::InfoHeaderFormats::V4InfoHeader> bmp1;

// Read in the image (found in the examples folder for reference)
bmp1.read_bmp("8bit_1024_example.bmp");

// Create a copy of the 8 bit image was imported
int error = bmp1.create_bmp("copy_example1.bmp", false);
```
Example 8 bit image used with no pixel padding: 
![Alt text](../master/Example BMPs/8bit_1024_example.bmp?raw=true?raw=true "")

---

### How to import and export a 24 bit BMP file
Code:
```
// Create a BMP object with the type of info header we desire: V4InfoHeader
bmp::BMPFileUserDefined<bmp::InfoHeaderFormats::V4InfoHeader> bmp2;

// Read in the image (found in the examples folder for reference)
bmp2.read_bmp("24bit_373_517_example.bmp");

// Create a copy of the 24 bit image that was imported
int error = bmp2.create_bmp("copy_example2.bmp", true);
```
Example 24 bit image used with pixel padding:
![Alt text](../master/Example BMPs/24bit_373_517_example.bmp?raw=true?raw=true "")



### To-Do
- [ ] Add in an easy way to determine pixel color format
- [ ] Import more versions of the BMP info header
- [ ] More error checking

### License
This is licensed under the MIT license as seen in the LICENSE file. 

# image2wx

A CMake/C++ package for including/accessing images files as baked data within a WX app.

## The Problem

WX apps can open up image files to use with their UI. But if you want to distribute a single app file, you need to somehow include the image files inside your app. Windows and the mac have "resources", but on linux you basically need to turn the image into C data within your app.

Various tools exist to do this (bin2c/bin2h/xxd/[etc](https://stackoverflow.com/a/73340865)), but none are cross platform.

## Other Solutions

In CMake projects, we can use the scripting language to turn image files into C data without any external apps. several generic methods are available on github:

- [Jonathan Hamberg's FileEmbed.cmake uses an intervening library](https://gitlab.com/jhamberg/cmake-examples).
- [Sivachandran's code is a bit simpler](https://github.com/sivachandran/cmake-bin2h)

## This Solution

Image2wx provides the image packaging mechanism within CMake, as well as a few functions for using the images within C++ code.

## How to Use

First, include the image2wx tree in your project somewhere. In a top(ish) level `CMakeLists.txt` file, ensure the image2wx directory is in CMake's "prefix path".

```
# ensure find_package(image2wx) can find our version
list(APPEND CMAKE_PREFIX_PATH path/to/image2wx)
```

Then, in your app's `CMakeLists.txt` file:
- List your image files
- Turn them into generated C files
- Give those to your executable
```
set(PNGS image1.png image2.bmp)

find_package(image2wx REQUIRED)
include(${image2wx_USE_FILE})
include_directories(${image2wx_SOURCE_DIR} ${image2wx_INCLUDE_DIRS})
generate_image2wx_sources(PNG_SRCS "${PNGS}")
...
add_executable(main main.cpp "${PNG_SRCS}")
...
```

Now, in your C++ code, you can call the function `image2wx::BitmapFromName("image1.png")` and you'll be given a `wxBitmap` to do with what you wish.Like this:

```
...
	wxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	wxPanel * panel = new wxPanel(this);
	wxStaticBitmap * bitmap = new wxStaticBitmap(panel, wxID_ANY, image2wx::BitmapFromName("image1.png"));
	sizer->Add(bitmap, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
...
```

Image2wx caches the bitmaps on your behalf so you can request them multiple times without using more memory. Call `image2wx::FreeBitmaps()` to free the cached objects when desired (e.g. at `wxApp::OnExit()` time).

## Caveats

Calling `generate_image2wx_sources` should work, but hasn't been tested extensively.

This system could be slow at runtime with large numbers of assets. It could be made faster by `image2wx::BitmapFromName` maintaining a mapping of names to `node_t` instances. This is left as an exercise to the reader.

## Internals

In its C++ code, image2wx uses a small amount of trickery to maintain a global list of available images. The `image2wx::node_t` struct has a static member (basically a global) that is the head of a linked list of `node_t` instances. When `image2wx::BitmapFromName` wants to look up an image, it just walks the list.

## Motivation

I create image2wx as part of an effort to move [MotionCal](https://github.com/PaulStoffregen/MotionCal) to use CMake as its build system. It used a perl script to generate the C files from images.

_bruce oberg_  

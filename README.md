# MSFS glTF Viewer

This is an experimental model viewer for the upcoming Microsoft Flight Simulator. The new platform uses slight modifications of the standardized glTF 2.0 format for visual models. This code is forked and modified from the official Microsoft DirectX GLTF Viewer Sample.

This is still under development and is not useful as a standalone viewer yet. Many areas of the DirectX GLTF Viewer Sample needed modification to allow these files to be read - mostly areas not respecting more complicated parts of the glTF 2.0 spec. Additional schema support is added for several Asobo extensions. 

You must already have access to the MSFS Alpha and developer program to find this useful. As this is still under NDA, no discussion of the Alpha or any MSFS internals may take place in this repo. Please do not message me about this - if you don't have access there is nothing I can do.

Original readme from the parent project below:

# DirectX GLTF Viewer Sample
This project was motivated by a lack of sample code demonstrating the graphics API agnostic nature of the [glTF specification](https://www.google.com). The sample code is written using modern C++, DirectX 11 and the Universal Windows Platform (UWP) for the client application. The client application could have been written using any application development platform that supports DirectX 11 rendering. This sample is a port of the [Khronos PBR WebGL Sample](https://github.com/KhronosGroup/glTF-WebGL-PBR) and supports the same feature set.

![Main Sample App Screenshot](https://github.com/Microsoft/glTF-DXViewer/blob/master/img/screenshot2.PNG)

The screenshot is showing the DamagedHelmet sample file  being rendered in the scene window, some controls to the right to adjust transforms and a Tree View control with the scene hierarchy.

# Features

* Physically Based Rendering (PBR)
* Buffer Management
* Specification Support
* Loader
* Environment Map
* Selective PBR Rendering

The selective PBR rendering allow you to turn on and off different parts of the PBR shader to provide a better understanding of the visual effect of each.

![Selective PBR Rendering](https://github.com/Microsoft/glTF-DXViewer/blob/master/img/selective-rendering.png)

# Dependencies
* [Microsoft.glTF.cpp](https://www.nuget.org/packages/Microsoft.glTF.CPP/)

[Nuget](https://www.nuget.org/) was used for package management for installing the binary dependencies.

# Building
The original version of this project was built using Visual Studio 2017 Version 15.6.7 on Windows 10 Fall Creators Update (16299.0). However, the TreeView control was offered in the SDK from version 17134.0 so the project has been updated to require this removing the extra code dependency in the process. The project has also been testd with version 15.7.1 but there was a need to add the compiler flag '/d2CoroOptsWorkaround' as in coroutines some variables may get optimised away incorrectly causing an access violation under certain circumstances. The project has subsequently been tested in Visual Studio version 15.7.2.

# Further Information
Please see this [article series](http://peted.azurewebsites.net/gltf-directx/) for full details around features and coding for this sample.

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
